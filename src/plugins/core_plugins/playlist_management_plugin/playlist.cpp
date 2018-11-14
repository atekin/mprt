//#define __STDC_WANT_LIB_EXT1__ 1

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <codecvt>
#include <algorithm>
#include <fstream>


#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include "common/input_plugin_api.h"
#include "common/utils.h"

#include "playlist.h"
#include "tag_lib_parser.h"

namespace mprt
{
	playlist::playlist(std::shared_ptr<tag_parser_abstract> tag_parser)
		: _playlist_name("unnamed")
		, _playlist_id_counter(0)
		, _tag_parser(tag_parser)
	{
	}

	playlist::playlist(std::shared_ptr<tag_parser_abstract> tag_parser, std::string filename, bool reverse_insert)
		: _playlist_id_counter(0)
		, _tag_parser(tag_parser)
	{
		load_playlist(filename, reverse_insert);
	}

	playlist::~playlist()
	{
		remove_items(_playlist.begin(), _playlist.end());
	}

	playlist_item_shr_t playlist::construct_playlist_item(std::string url)
	{
		auto pl_item = std::make_shared<playlist_item_t>();
		_tag_parser->clear();
		_tag_parser->set_filename(_base_path + url);
		if (_tag_parser->read_metadata())
		{
			pl_item->_playlist_item_id = _playlist_id_counter++;
			pl_item->_url = url;
			pl_item->_tags = _tag_parser->get_tags();
			pl_item->_albumart = _tag_parser->get_album_arts();
			auto iter = pl_item->_tags->find(std::string(_AUDIO_PROP_DURATION_MS_));
			pl_item->_duration =
				(iter != pl_item->_tags->end()) ?
				std::chrono::milliseconds(std::stoll(iter->second)) :
				std::chrono::microseconds(0);
		}
		return pl_item;

	}

	/*playlist_const_iter_t playlist::get_list_iter(playlist_item_id_t playlist_item_id)
	{
		auto unordered_iter = get_hash_iter(playlist_item_id);
		if (unordered_iter == _playlist.get<by_playlist_item_id>().end())
		{
			return _playlist.get<by_playlist>().end();
		}

		return _playlist.project<by_playlist>(unordered_iter);
	}

	playlist_itemlist_const_iter_t playlist::get_hash_iter(playlist_item_id_t playlist_item_id)
	{
		return _playlist.get<by_playlist_item_id>().find(playlist_item_id);
	}*/

	std::string playlist::playlist_name()
	{
		return _playlist_name;
	}

	void playlist::set_playlist_name(std::string pl_name)
	{
		_playlist_name = pl_name;
	}

	playlist_const_iter_t playlist::get_seq_cbegin()
	{
		return _playlist.cbegin();
	}

	playlist_const_iter_t playlist::get_seq_cend()
	{
		return _playlist.cend();
	}

	playlist_iter_t playlist::get_seq_begin()
	{
		return _playlist.begin();
	}

	playlist_iter_t playlist::get_seq_end()
	{
		return _playlist.end();
	}

	playlist_item_shr_t playlist::get_playlist_item_with_item_id(playlist_item_id_t playlist_item)
	{
		auto iter = get_list_iter_temp<by_playlist_item_id>(playlist_item);
		if (iter == _playlist.end())
		{
			return playlist_item_shr_t();
		}

		return *iter;
	}

	playlist_item_shr_t playlist::get_playlist_item_with_url_id(url_id_t url_id)
	{
		auto iter = get_list_iter_temp<by_playlist_url_id>(url_id);
		if (iter == _playlist.end())
		{
			return playlist_item_shr_t();
		}

		return *iter;
	}

	playlist_item_id_t playlist::get_last_item_id()
	{
		auto playlist_item_id = _INVALID_PLAYLIST_ITEM_ID_;
		if (_playlist.size() > 0)
		{
			playlist_item_id = (*(--_playlist.cend()))->_playlist_item_id;
		}

		return playlist_item_id;
	}

	mprt::playlist_item_id_t playlist::get_next_item_id(playlist_item_id_t playlist_item_id)
	{
		auto list_iter = get_list_iter_temp<by_playlist_item_id>(playlist_item_id);
		if (list_iter != _playlist.end())
		{
			++list_iter;
			return (list_iter != _playlist.end()) ? (*list_iter)->_playlist_item_id : _INVALID_PLAYLIST_ITEM_ID_;
		}
			
		return _INVALID_PLAYLIST_ITEM_ID_;
	}

	playlist_item_shr_t playlist::add_item(std::string const& url)
	{
		return add_item(_playlist.cend(), url);
	}

	playlist_item_shr_t playlist::add_item(playlist_const_iter_t const& iter, std::string const& url)
	{
		auto added_item = construct_playlist_item(url);
		if (added_item && added_item->_playlist_item_id != _INVALID_PLAYLIST_ITEM_ID_)
		{
			auto inserted = _playlist.insert(iter, added_item);
			return inserted.second ? added_item : playlist_item_shr_t();
		}

		return playlist_item_shr_t();
	}

	playlist_item_shr_t playlist::add_item(playlist_item_id_t playlist_item_id, std::string const& url)
	{
		return add_item(get_list_iter_temp<by_playlist_item_id>(playlist_item_id), url);
	}

	bool playlist::move_item(playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id)
	{
		auto from_iter = get_list_iter_temp<by_playlist_item_id>(from_playlist_item_id);
		if (from_iter == _playlist.end())
		{
			return false;
		}

		auto to_iter = get_list_iter_temp<by_playlist_item_id>(to_playlist_item_id);
		return move_item(from_iter, to_iter);
	}

	bool playlist::move_item(
		playlist_const_iter_t const& from_playlist_item_iter,
		playlist_const_iter_t const& to_playlist_item_iter)
	{
		_playlist.splice(to_playlist_item_iter, _playlist, from_playlist_item_iter);

		return true;
	}

	void playlist::move_items(playlist_const_iter_t const& from_playlist_item_id, playlist_const_iter_t const& to_playlist_item_iter, size_type count_items)
	{
		auto stop_iter = from_playlist_item_id;
		std::advance(stop_iter, count_items);
		move_items(from_playlist_item_id, stop_iter, to_playlist_item_iter);
	}

	void playlist::move_items(playlist_const_iter_t const& from_playlist_item_id, playlist_const_iter_t const& stop_playlist_item_id, playlist_const_iter_t const& to_playlist_item_iter)
	{
		_playlist.splice(to_playlist_item_iter, _playlist, from_playlist_item_id, stop_playlist_item_id);

	}

	void playlist::move_items(std::vector<playlist_const_iter_t> const& items, playlist_const_iter_t const& to_playlist_item_iter)
	{
		auto riter = items.rbegin(), rend = items.rend();

		for (; riter != rend; ++riter)
		{
			move_item(*riter, to_playlist_item_iter);
		}
	}

	void playlist::move_items(std::vector<playlist_item_id_t> const& item_ids, playlist_item_id_t to_playlist_item_id)
	{
		auto iter = item_ids.rbegin(), iter_end = item_ids.rend();
		for (; iter != iter_end; ++iter)
		{
			move_item(*iter, to_playlist_item_id);
		}
	}

	bool playlist::remove_item(playlist_const_iter_t const& item_iter)
	{
		auto remove_valid = item_iter != _playlist.get<by_playlist>().end();

		if (remove_valid)
		{
			_playlist.erase(item_iter);
		}

		return remove_valid;
	}

	bool playlist::remove_item(playlist_item_id_t playlist_item_id)
	{
		return remove_item(get_list_iter_temp<by_playlist_item_id>(playlist_item_id));
	}

	void playlist::remove_items(playlist_const_iter_t const& from_playlist_item_iter, playlist_const_iter_t const& to_playlist_item_iter)
	{
		_playlist.erase(from_playlist_item_iter, to_playlist_item_iter);
	}

	void playlist::remove_items(playlist_const_iter_t const& from_playlist_item_iter, size_type count_items)
	{
		auto to_iter = from_playlist_item_iter;
		std::advance(to_iter, count_items);
		remove_items(from_playlist_item_iter, to_iter);
	}

	void playlist::clear()
	{
		_playlist.clear();
	}

	bool playlist::update_url_id(playlist_item_id_t playlist_item, url_id_t url_id)
	{
		return update_url_id_bylist(get_list_iter_temp<by_playlist_item_id>(playlist_item), url_id);
	}

	std::shared_ptr<playlist_item_shr_list_t> playlist::load_playlist(std::string filename, bool reverse_insert = false)
	{
		auto playlist_item_shr_list(std::make_shared<playlist_item_shr_list_t>());

		filename = string_trim(filename);

		if (!boost::filesystem::exists(filename)) {
			BOOST_LOG_TRIVIAL(error) << "playlist file does not exist: " << filename;
			return playlist_item_shr_list;
		}

		boost::filesystem::path filepath(filename);
		filepath.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>()));
		_playlist_name = filepath.stem().string();

		std::fstream playlist_file;
		playlist_file.open(filepath.string());
		if (!playlist_file.is_open())
		{
			constexpr int errmsglen = 100;
			char errmsg[errmsglen];
#ifdef _WIN32
			BOOST_LOG_TRIVIAL(debug) << "cannot open file: " << filepath.string() << " error: " << strerror_s(errmsg, errmsglen, errno);
#else
			BOOST_LOG_TRIVIAL(debug) << "cannot open file: " << filepath.string() << " error: " << strerror_r(errno, errmsg, errmsglen);
#endif

			return playlist_item_shr_list;
		}

		_base_path = filepath.parent_path().string() + "/";
		for (std::string line; std::getline(playlist_file, line); )
		{
			line = string_trim(line);
			if (line.empty() || line.at(0) == '#')
			{
				continue;
			}

			auto playlist_item_shr = reverse_insert ? add_item(_playlist.cbegin(), line) : add_item(line);
			if (playlist_item_shr)
			{
				playlist_item_shr_list->push_back(playlist_item_shr);
			}
		}

		return playlist_item_shr_list;
	}

	bool playlist::save_playlist(std::string const& filename)
	{
		std::ofstream ofile;
		ofile.open(filename, std::fstream::trunc);
		auto is_open = ofile.is_open();

		if (is_open)
		{
			for (auto & pl_item : _playlist)
			{
				ofile << pl_item->_url << "\n";
			}
		}
		
		return is_open;
	}

	std::string playlist::base_path()
	{
		return _base_path;
	}

	void playlist::set_album_art(std::shared_ptr<album_art> alb_art)
	{
		_album_art = alb_art;

		_tag_parser->set_album_art(_album_art);
	}

}
