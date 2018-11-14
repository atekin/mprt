#include <codecvt>

#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

#include "core/config.h"
#include "common/decoder_plugin_api.h"
#include "common/output_plugin_api.h"
#include "common/input_plugin_api.h"
#include "common/ui_plugin_api.h"
#include "core/decoder_plugins_manager.h"
#include "tag_lib_parser.h"

#include "playlist_management_plugin_imp.h"

#include "playlist.h"
#include "album_art.h"

namespace mprt
{

	void playlist_management_plugin_imp::stop_internal()
	{
		for_each(_input_plugins->begin(), _input_plugins->end(), [](std::shared_ptr<input_plugin_api> & inp_plug)
		{
			inp_plug->stop();

		});

		_decoder_plugins_manager->stop();

		for_each(_output_plugins->begin(), _output_plugins->end(), [](std::shared_ptr<output_plugin_api> & out_plug)
		{
			out_plug->stop();
		});
	}

	void playlist_management_plugin_imp::pause_internal()
	{
		for_each(_input_plugins->begin(), _input_plugins->end(), [](std::shared_ptr<input_plugin_api> & inp_plug)
		{
			inp_plug->pause();

		});

		_decoder_plugins_manager->pause();

		for_each(_output_plugins->begin(), _output_plugins->end(), [](std::shared_ptr<output_plugin_api> & out_plug)
		{
			out_plug->pause();
		});
	}

	void playlist_management_plugin_imp::cont_internal()
	{
		for_each(_input_plugins->begin(), _input_plugins->end(), [](std::shared_ptr<input_plugin_api> & inp_plug)
		{
			inp_plug->cont();
		});
	}

	void playlist_management_plugin_imp::quit_internal()
	{
		for_each(_input_plugins->begin(), _input_plugins->end(), [](std::shared_ptr<input_plugin_api> & inp_plug)
		{
			inp_plug->quit();

		});

		_decoder_plugins_manager->quit();

		for_each(_output_plugins->begin(), _output_plugins->end(), [](std::shared_ptr<output_plugin_api> & out_plug)
		{
			out_plug->quit();
		});
	}

	void playlist_management_plugin_imp::set_input_plugins(std::shared_ptr<std::vector<std::shared_ptr<input_plugin_api>>> input_plugins)
	{
		_input_plugins = input_plugins;

		for (auto & ip : *_input_plugins)
		{
			ip->set_input_opened_cb(std::bind(&playlist_management_plugin_imp::input_opened_cb, this, std::placeholders::_1));
		}
	}

	void playlist_management_plugin_imp::set_decoder_plugins_manager(std::shared_ptr<decoder_plugins_manager > decoder_plugins_manager)
	{
		_decoder_plugins_manager = decoder_plugins_manager;

		_decoder_plugins_manager->set_decoder_opened_cb(std::bind(&playlist_management_plugin_imp::decoder_opened_cb, this, std::placeholders::_1));
		_decoder_plugins_manager->set_decoder_seek_finish_cb(std::bind(&playlist_management_plugin_imp::decoder_seek_finished_cb, this));
	}

	playlist_management_plugin_imp::playlist_management_plugin_imp()
		: _playlist_id_counter(_INVALID_PLAYLIST_ID_)
	{
		_tag_parser = std::make_shared<tag_lib_parser>();

		_progress_callback = std::bind(&playlist_management_plugin_imp::progress_callback, this, std::placeholders::_1, std::placeholders::_2);

		config::instance().init("../config/config_playlist_management_plugin.xml");
		auto pt = config::instance().get_ptree_node("mprt.playlist_management_plugin");

		_async_task = std::make_shared<async_tasker>(pt.get<std::size_t>("max_free_timer_count", 10));
		_min_wait_next_song = std::chrono::milliseconds(pt.get<std::size_t>("min_wait_next_song_msecs", 10000));

		_added_next_song = false;

		while (!_async_task->is_ready())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	playlist_management_plugin_imp::~playlist_management_plugin_imp()
	{

	}

	// Must be instantiated in plugin
	boost::filesystem::path playlist_management_plugin_imp::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string playlist_management_plugin_imp::plugin_name() const {
		return "playlist_management_plugin_imp";
	}

	plugin_types playlist_management_plugin_imp::plugin_type() const {
		return plugin_types::playlist_management_plugin;
	}

	void playlist_management_plugin_imp::init(void *)
	{
		_album_art = std::make_shared<album_art>();
	}

	void playlist_management_plugin_imp::start_play_internal(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id)
	{
		_current_state = plugin_states::play;

		auto iter = _playlist_list.find(playlist_id);

		if ((iter != _playlist_list.end()))
		{
			_current_playing_list_id = playlist_id;

			cont_play_internal(playlist_item_id);
			
			cont_internal();
		}
	}

	void playlist_management_plugin_imp::stop_play_internal()
	{
		for (auto & inp_plugi : *_input_plugins)
		{
			inp_plugi->stop();
		}

		_decoder_plugins_manager->stop();

		for (auto & out_plugi : *_output_plugins)
		{
			out_plugi->stop();
		}
	}

	void playlist_management_plugin_imp::cont_play_internal(playlist_item_id_t playlist_item_id)
	{
		auto iter = _playlist_list.find(_current_playing_list_id);

		if ((iter != _playlist_list.end()))
		{
			auto pl_item_shr = iter->second->get_playlist_item_with_item_id(playlist_item_id);

			if (pl_item_shr)
			{
				// @TODO: this _input_plugins part will be updated ...
				auto inp_plug = _input_plugins->at(0);
				//auto url_id = inp_plug->add_input_item(pl_item_shr->_url, get_pair_cantor(_current_playing_list_id, playlist_item_id));
				auto url_id = inp_plug->add_input_item(iter->second->base_path() + pl_item_shr->_url);
				iter->second->update_url_id(playlist_item_id, url_id);

				auto dur_iter = pl_item_shr->_tags->find(std::string(_AUDIO_PROP_DURATION_MS_));
				auto duration = 0LL;
				if (dur_iter != pl_item_shr->_tags->end())
				{
					duration = std::stoll(dur_iter->second);
				}

				call_callback_funcs(_start_playlist_item_cb_list, _current_playing_list_id, playlist_item_id, duration);
			}
		}
	}

	void playlist_management_plugin_imp::seek_duration_internal(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id, size_type duration_ms)
	{
		auto iter = _playlist_list.find(playlist_id);
		if (iter != _playlist_list.end())
		{
			auto pl_item_shr = iter->second->get_playlist_item_with_item_id(playlist_item_id);

			_decoder_plugins_manager->seek_duration(pl_item_shr->_url_id, duration_ms);
		}
	}

	void playlist_management_plugin_imp::progress_callback(url_id_t url_id, size_type current_position_ms)
	{
		//BOOST_LOG_TRIVIAL(debug) << "song url_id: " << url_id << " position(ms): " << current_position_ms;

		add_job([this, url_id, current_position_ms]
		{
			auto iter = _playlist_list.find(_current_playing_list_id);

			if ((iter != _playlist_list.end()))
			{
				auto pl_item_shr = iter->second->get_playlist_item_with_url_id(url_id);
				if (pl_item_shr)
				{
					call_callback_funcs(_progress_playlist_item_cb_list, _current_playing_list_id, pl_item_shr->_playlist_item_id, current_position_ms);
				
					if (pl_item_shr->_playlist_item_id != _current_playing_item_id)
					{
						_added_next_song = false;
						_current_playing_item_id = pl_item_shr->_playlist_item_id;
					}
					
					if (
						pl_item_shr->_duration - std::chrono::milliseconds(current_position_ms) <= _min_wait_next_song
						&&
						!_added_next_song)
					{
						BOOST_LOG_TRIVIAL(debug) << "adding a new song to the queue";
						cont_play_internal(get_next_playlist_item_id());

						_added_next_song = true;
					}
				}
			}
		});
	}

	void playlist_management_plugin_imp::add_playlist_items_internal(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::shared_ptr<std::vector<std::string>> urls)
	{
		if (!urls)
		{
			return;
		}

		auto iter = _playlist_list.find(playlist_id);
		if (iter != _playlist_list.end())
		{
			auto playlist_item_shr_list(std::make_shared<playlist_item_shr_list_t>());

			
			std::vector<std::string> local_urls;

			for (auto & url : *urls)
			{
				boost::filesystem::path p(url);
				if (boost::filesystem::is_directory(p))
				{
					boost::filesystem::path canon_path =
						boost::filesystem::canonical(p, boost::filesystem::path("/"));
					canon_path.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>()));
					

					auto sub_files = add_playlist_directory_builder(canon_path.string());
					local_urls.insert(local_urls.end(), sub_files->begin(), sub_files->end());
				}
				else
				{
					local_urls.push_back(url);
				}
			}

			auto
				iter_url = local_urls.begin(),
				iter_url_end = local_urls.end();


			//auto from_id = (from_playlist_item_id == _INVALID_PLAYLIST_ITEM_ID_) ? iter->second->get_last_item_id() : from_playlist_item_id;
			for (; iter_url != iter_url_end; ++iter_url)
			{
				auto added_item =
					(from_playlist_item_id == _INVALID_PLAYLIST_ITEM_ID_) ? iter->second->add_item(*iter_url) : iter->second->add_item(from_playlist_item_id, *iter_url);
				if (added_item)
				{
					playlist_item_shr_list->push_back(added_item);
				}
			}

			if (playlist_item_shr_list->size() > 0)
			{
				auto from_id = (from_playlist_item_id == _INVALID_PLAYLIST_ITEM_ID_) ? iter->second->get_last_item_id() : from_playlist_item_id;
				call_callback_funcs(_add_playlist_item_cb_list, playlist_id, from_id, playlist_item_shr_list);
			}
		}
	}

	void playlist_management_plugin_imp::add_playlist_directory_internal(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::string dirname)
	{
		auto file_list = add_playlist_directory_builder(dirname);

		add_playlist_items_internal(playlist_id, from_playlist_item_id, file_list);
	}

	std::shared_ptr<std::vector<std::string>> playlist_management_plugin_imp::add_playlist_directory_builder(std::string dirname)
	{
		boost::filesystem::recursive_directory_iterator dir(dirname), end;

		auto file_list = std::make_shared<std::vector<std::string>>();

		while (dir != end)
		{
			try
			{
				if (boost::filesystem::is_directory(dir->path()) && (dir->path().filename() == "." || dir->path().filename() == ".."))
				{
					dir.no_push();
				}
				else if (boost::filesystem::is_regular_file(dir->path()))
				{
					boost::filesystem::path canon_path =
						boost::filesystem::canonical(dir->path(), boost::filesystem::path("/"));
					canon_path.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>()));
					file_list->push_back(canon_path.string());
				}
			}
			catch (std::exception const & e)
			{
				BOOST_LOG_TRIVIAL(debug) << "there is an error here: " << e.what() << " path: " << dir->path();
			}
			catch (...)
			{
				BOOST_LOG_TRIVIAL(debug) << "general error path: " << dir->path();
			}

			try
			{
				++dir;
			}
			catch (std::exception const & e)
			{
				BOOST_LOG_TRIVIAL(debug) << "there is an error here: " << e.what() << " path: " << dir->path();
			}
			catch (...)
			{
				BOOST_LOG_TRIVIAL(debug) << "general error path: " << dir->path();
			}

			//BOOST_LOG_TRIVIAL(debug) << "path: " << dir->path();
		}

		return file_list;
	}

	void playlist_management_plugin_imp::input_opened_cb(std::shared_ptr<current_decoder_details> cur_det)
	{
		add_job([this, cur_det]
		{
			if (cur_det->_sound_details._ok)
			{
				_decoder_plugins_manager->add_input_url_job(cur_det);

				_decoder_plugins_manager->cont();
			}
			else
			{
				start_play_internal(get_next_playlist_item_id(), _current_playing_list_id);
			}
		});
	}

	void playlist_management_plugin_imp::decoder_opened_cb(sound_details sound_det)
	{
		add_job([this, sound_det]
		{
			if (sound_det._ok)
			{
				for_each(_output_plugins->begin(), _output_plugins->end(), [sound_det](std::shared_ptr<output_plugin_api> & out_plug)
				{
					out_plug->add_sound_details(sound_det);
					out_plug->cont();
				});
			}
			else
			{
				start_play_internal(get_next_playlist_item_id(), _current_playing_list_id);
			}
		});
	}

	void playlist_management_plugin_imp::decoder_seek_finished_cb()
	{
		add_job([this]
		{
			call_callback_funcs(_seek_finished_cb_list);
		});
	}

	void playlist_management_plugin_imp::sound_opened_cb(bool sound_opened)
	{
		add_job([this, sound_opened]
		{
			if (!sound_opened)
			{
				start_play_internal(get_next_playlist_item_id(), _current_playing_list_id);
			}
		});
	}

	void playlist_management_plugin_imp::set_output_plugins(std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> output_plugins)
	{
		playlist_management_plugin::set_output_plugins(output_plugins);

		std::for_each(_output_plugins->begin(), _output_plugins->end(), [this](std::shared_ptr<output_plugin_api> & out_plug) {
			out_plug->add_progress_callback(_progress_callback, plugin_name());
		});
	}

	mprt::playlist_item_id_t playlist_management_plugin_imp::get_next_playlist_item_id()
	{
		auto iter = _playlist_list.find(_current_playing_list_id);
		if (iter != _playlist_list.end())
		{
			return iter->second->get_next_item_id(_current_playing_item_id);
		}

		return _INVALID_PLAYLIST_ITEM_ID_;
	}

	void playlist_management_plugin_imp::start_play(playlist_id_t playlist_id, playlist_item_id_t playlist_item)
	{
		add_job([this, playlist_id, playlist_item]()
		{
			start_play_internal(playlist_id, playlist_item);
		});
	}

	void playlist_management_plugin_imp::stop_play()
	{
		stop();
	}

	void playlist_management_plugin_imp::seek_duration(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id, size_type duration_ms)
	{
		if (_INVALID_PLAYLIST_ID_ != playlist_id && _INVALID_PLAYLIST_ITEM_ID_ != playlist_item_id)
		{
			add_job([this, playlist_id, playlist_item_id, duration_ms]
			{
				seek_duration_internal(playlist_id, playlist_item_id, duration_ms);
			});
		}
	}

	void playlist_management_plugin_imp::add_playlist(std::string filename)
	{
		add_playlist(filename, std::shared_ptr<std::vector<std::string>>());
	}

	void playlist_management_plugin_imp::add_playlist(std::string filename, std::shared_ptr<std::vector<std::string>> url_names)
	{
		add_job([this, filename, url_names]
		{
			auto playlist_id = ++_playlist_id_counter;
			auto playlist(std::make_unique<playlist>(_tag_parser));
			playlist->set_album_art(_album_art);

			std::shared_ptr<playlist_item_shr_list_t> added_items;
			if (!filename.empty())
			{
				added_items = playlist->load_playlist(filename, false);
			}
			else
			{
				playlist->set_playlist_name("unnamed_" + std::to_string(playlist_id));
			}

			call_callback_funcs(_add_playlist_cb_list, playlist->playlist_name(), playlist_id, added_items);

			add_playlist_items(playlist_id, _INVALID_PLAYLIST_ITEM_ID_, url_names);

			_playlist_list[playlist_id] = std::move(playlist);
		});
	}

	void playlist_management_plugin_imp::add_playlist(std::shared_ptr<std::vector<std::string>> url_names)
	{
		add_playlist("", url_names);
	}

	void playlist_management_plugin_imp::add_playlist_directory(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::string dirname)
	{
		add_job([this, playlist_id, from_playlist_item_id, dirname]() {
			add_playlist_directory_internal(playlist_id, from_playlist_item_id, dirname);
		});
	}

	void playlist_management_plugin_imp::register_add_playlist_callback(std::string plug_name, add_playlist_callback_t add_playlist_callback)
	{
		add_job([this, plug_name, add_playlist_callback]() {
			_add_playlist_cb_list[plug_name] = add_playlist_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_add_playlist_callback(std::string plug_name)
	{
		add_job([this, plug_name] {
			_add_playlist_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::add_playlist_items(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::shared_ptr<std::vector<std::string>> urls)
	{
		add_job([this, playlist_id, from_playlist_item_id, urls]
		{
			add_playlist_items_internal(playlist_id, from_playlist_item_id, urls);
		});
	}

	void playlist_management_plugin_imp::register_add_playlist_item_callback(std::string plug_name, add_playlist_item_callback_t add_playlist_item_callback)
	{
		add_job([this, plug_name, add_playlist_item_callback]() {
			_add_playlist_item_cb_list[plug_name] = add_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_add_playlist_item_callback(std::string plug_name)
	{
		add_job([this, plug_name] {
			_add_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::remove_playlist_item(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id)
	{
		add_job([this, playlist_id, playlist_item_id]
		{
			auto iter = _playlist_list.find(playlist_id);

			if ((iter != _playlist_list.end()) && (iter->second->remove_item(playlist_item_id)))
			{
				call_callback_funcs(_remove_playlist_item_cb_list, playlist_id, playlist_item_id);
			}
		});
	}

	void playlist_management_plugin_imp::register_remove_item_callback(std::string plug_name, remove_playlist_item_callback_t remove_playlist_item_callback)
	{
		add_job([this, plug_name, remove_playlist_item_callback]
		{
			_remove_playlist_item_cb_list[plug_name] = remove_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_remove_item_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_remove_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::move_playlist_item(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id)
	{
		add_job([this, playlist_id, from_playlist_item_id, to_playlist_item_id]
		{
			auto iter = _playlist_list.find(playlist_id);

			if ((iter != _playlist_list.end()) && (iter->second->move_item(from_playlist_item_id, to_playlist_item_id)))
			{
				call_callback_funcs(_move_playlist_item_cb_list, playlist_id, from_playlist_item_id, to_playlist_item_id);
			}
		});
	}


	void playlist_management_plugin_imp::register_move_item_callback(std::string plug_name, move_playlist_item_callback_t move_playlist_item_callback)
	{
		add_job([this, plug_name, move_playlist_item_callback]
		{
			_move_playlist_item_cb_list[plug_name] = move_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_move_item_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_move_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::register_start_playlist_item_callback(std::string plug_name, start_playlist_item_callback_t start_playlist_item_callback)
	{
		add_job([this, plug_name, start_playlist_item_callback]
		{
			_start_playlist_item_cb_list[plug_name] = start_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_start_playlist_item_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_start_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::register_progress_playlist_item_callback(std::string plug_name, progress_playlist_item_callback_t progress_playlist_item_callback)
	{
		add_job([this, plug_name, progress_playlist_item_callback]
		{
			_progress_playlist_item_cb_list[plug_name] = progress_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_progress_playlist_item_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_progress_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::save_playlist(playlist_id_t playlist_id, std::string playlist_name)
	{
		add_job([this, playlist_id, playlist_name]
		{

			auto iter = _playlist_list.find(playlist_id);

			if ((iter != _playlist_list.end()))
			{
				iter->second->save_playlist(playlist_name);
			}
		});
	}

	void playlist_management_plugin_imp::rename_playlist(playlist_id_t playlist_id, std::string playlist_name)
	{
		add_job([this, playlist_id, playlist_name]
		{
			auto iter = _playlist_list.find(playlist_id);

			if ((iter != _playlist_list.end()))
			{
				iter->second->set_playlist_name(playlist_name);

				call_callback_funcs(_rename_playlist_item_cb_list, playlist_id, playlist_name);
			}
		});
	}

	void playlist_management_plugin_imp::register_rename_playlist_callback(std::string plug_name, rename_playlist_callback_t rename_playlist_item_callback)
	{
		add_job([this, plug_name, rename_playlist_item_callback]
		{
			_rename_playlist_item_cb_list[plug_name] = rename_playlist_item_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_rename_playlist_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_rename_playlist_item_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::remove_playlist(playlist_id_t playlist_id)
	{
		add_job([this, playlist_id]
		{
			_playlist_list.erase(playlist_id);

			call_callback_funcs(_remove_playlist_cb_list, playlist_id);
		});
	}

	void playlist_management_plugin_imp::register_remove_playlist_callback(std::string plug_name, remove_playlist_callback_t remove_playlist_callback)
	{
		add_job([this, plug_name, remove_playlist_callback]
		{
			_remove_playlist_cb_list[plug_name] = remove_playlist_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_remove_playlist_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_remove_playlist_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin_imp::register_seek_finished_callback(std::string plug_name, seek_finished_callback_t seek_finished_callback)
	{
		add_job([this, plug_name, seek_finished_callback]
		{
			_seek_finished_cb_list[plug_name] = seek_finished_callback;
		});
	}

	void playlist_management_plugin_imp::unregister_seek_finished_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_seek_finished_cb_list.erase(plug_name);
		});
	}

	album_art_list const& playlist_management_plugin_imp::get_album_art(album_name_t const& album_name)
	{
		return _album_art->get_album_art(album_name);
	}

}

// Factory method. Returns *simple pointer
std::unique_ptr<refcounting_plugin_api> create()
{
	return std::make_unique<mprt::playlist_management_plugin_imp>();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)
