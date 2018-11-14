
#include <fstream>
#include <limits>
#include <algorithm>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio.hpp>

#include "core/config.h"
#include "common/refcounting_plugin_api.h"
#include "common/job_type_enums.h"
#include "common/utils.h"
#include "common/scope_exit.h"

#include "input_plugin_file.h"

#ifdef _WIN32
std::wstring ToUtf16(std::string str)
{
	std::wstring ret;
	int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
	if (len > 0)
	{
		ret.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &ret[0], len);
	}
	return ret;
}
#endif


namespace mprt {


	input_plugin_file::input_plugin_file()
		: _set_input_cache_buf_callback(std::bind(&input_plugin_file::set_cache_buffer, this, std::placeholders::_1, std::placeholders::_2))
		, _decoder_finish_callback(std::bind(&input_plugin_file::decoder_finish, this, std::placeholders::_1, std::placeholders::_2))
		, _seek_callback(std::bind(&input_plugin_file::seek_callback_job, this, std::placeholders::_1, std::placeholders::_2))
	{

	}

	input_plugin_file::~input_plugin_file()
	{
		BOOST_LOG_TRIVIAL(trace) << "input_plugin_file::~input_plugin_file() called";
	}

	boost::filesystem::path input_plugin_file::location() const
	{
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string input_plugin_file::plugin_name() const
	{
		return "input_file_plugin";
	}

	plugin_types input_plugin_file::plugin_type() const
	{
		return plugin_types::input_plugin;
	}

	void input_plugin_file::init(void *)
	{

		config::instance().init("../config/config_input_plugin_file.xml");
		auto pt = config::instance().get_ptree_node("mprt.input_plugin_file");
		_max_file_chunk_size = pt.get<size_type>("max_file_chunk_size", 128) * 1024;
		_max_finish_files = pt.get<size_type>("max_finish_files", 3);

		_async_task = std::make_shared<async_tasker>(pt.get<std::size_t>("max_free_timer_count", 10));
	}

	bool input_plugin_file::add_file(
		std::string filename, url_id_t url_id)
	{
		if (!boost::filesystem::exists(filename)) {
			BOOST_LOG_TRIVIAL(error) << plugin_name() << " file does not exist: " << filename;
			return false;
		}

		auto file_dets_ptr =
			std::make_shared<file_details>(
				filename,
				url_id);

		_files.push_back(file_dets_ptr);

		BOOST_LOG_TRIVIAL(trace) << "file added: " << filename << " id: " << url_id;

		if (plugin_states::play == _current_state)
		{
			cont();
		}

		return true;
	}
	
	bool input_plugin_file::open_file(file_list_t::iterator& file_iter)
	{
		_STATE_CHECK_(plugin_states::play, false);

		auto & filename = (*file_iter)->_filename;
		auto & file_iter_ref = *file_iter;

		bool is_opened = false;

		if (!boost::filesystem::exists(filename)) {
			BOOST_LOG_TRIVIAL(error) << "file does not exist: " << filename;
			_files.erase(file_iter);
		}
		else
		{
			file_iter_ref->_file_size =
				static_cast<size_type>(boost::filesystem::file_size(boost::filesystem::path(filename)));
			file_iter_ref->_file->open(
#ifdef _WIN32
				ToUtf16(filename).c_str()
#else
				filename.c_str()
#endif
				, std::ios_base::binary);

			if (!(file_iter_ref->_file->is_open())) {
				BOOST_LOG_TRIVIAL(error) << "file open error: " << filename << " removing from queue";
				_files.erase(file_iter);
			}
			else
			{
				// Stop eating new lines in binary mode!!!
				file_iter_ref->_file->unsetf(std::ios::skipws);
				is_opened = true;
			}
			
		}

		auto cur_decoder_detail = std::make_shared<current_decoder_details>();
		cur_decoder_detail->_sound_details._ok = is_opened;
		cur_decoder_detail->_url = filename;
		cur_decoder_detail->_url_ext = get_ext(filename);
		cur_decoder_detail->_stream_length = file_iter_ref->_file_size;
		cur_decoder_detail->_sound_details._total_samples = 0;
		cur_decoder_detail->_current_samples_written = 0;
		cur_decoder_detail->_current_stream_pos = 0;
		cur_decoder_detail->_last_read_empty = true;
		cur_decoder_detail->_length_supported = true;
		cur_decoder_detail->_seek_supported = true;
		cur_decoder_detail->_tell_supported = true;
		cur_decoder_detail->_sound_details._url_id = file_iter_ref->_url_id;
		cur_decoder_detail->_set_input_cache_buf_callback = _set_input_cache_buf_callback;
		cur_decoder_detail->_decoder_finish_callback = _decoder_finish_callback;
		cur_decoder_detail->_seek_callback = _seek_callback;

		//_decoder_plugins_manager->add_input_url_job(cur_decoder_detail);
		_input_opened_register_func(cur_decoder_detail);

		BOOST_LOG_TRIVIAL(trace) << "filename opened: " << filename;

		return is_opened;
	}

	void input_plugin_file::close_file(file_list_t::iterator& file_iter)
	{
		_finished_files.push_back(*file_iter);
		_files.pop_front();
	}

	void input_plugin_file::remove_file(url_id_t url_id)
	{
		if (!remove_file_helper(_files, url_id))
		{
			remove_file_helper(_finished_files, url_id);
		}
	}

	void input_plugin_file::try_read()
	{
		_STATE_CHECK_(plugin_states::play);

		auto file_iter = _files.begin();
		if (file_iter == _files.end())
		{
			BOOST_LOG_TRIVIAL(debug) << "normally input_plugin_file::try_read() should not be here";
			return;
		}

		std::chrono::microseconds next_dur = std::chrono::microseconds(1000);

		if (_file_read_timer && is_active_timer(_file_read_timer))
		{
			return;
		}
		
		SCOPE_EXIT_REF(
			if (!is_no_job()) {
				// we have more data to read ...
				//BOOST_LOG_TRIVIAL(debug) << "file duration: " << next_dur.count();
				if (!_file_read_timer || (_file_read_timer && is_timer_expired(_file_read_timer)))
				{
					_file_read_timer = add_job_thread_internal([this]() {
						try_read();
					}, next_dur);
				}
			}
			else {
				BOOST_LOG_TRIVIAL(trace) << "stopping file read";
			}
		);

		if (_finished_files.size() >= _max_finish_files)
		{
			next_dur = std::chrono::milliseconds(1000);
			return;
		}


		auto & file_iter_ref = *file_iter;
		if (!file_iter_ref->_file->is_open() && !open_file(file_iter))
		{
			return;
		}

		
		if (
			file_iter_ref->_cache_buf
			&&_current_state == plugin_states::play)
		{

			// file read part
			// here is complicated may be a better algorithm can be found ...
			// get the cache buffer
			auto file_chunk_contents = file_iter_ref->_cache_buf->get_cache_ptr();
			bool is_file_finished = false;
			// check the buffer and file id
// 			if (file_chunk_contents_tagged->second != (*file_iter)->_url_id) {
// 				BOOST_LOG_TRIVIAL(debug)
// 					<< "file : " << file_chunk_contents_tagged->second
// 					<< " file : " << (*file_iter)->_url_id;
// 			}
			if (file_chunk_contents)
			{
				// read the file now
				try
				{
					auto & file_chunk_contents_ref = *file_chunk_contents;
					auto free_size = std::min<size_type>(_max_file_chunk_size, file_chunk_contents_ref->second.reserve());
					auto chunk_size = file_chunk_contents_ref->second.size();
					if (0 == chunk_size) file_chunk_contents_ref->first = file_iter_ref->_file->tellg();
					file_chunk_contents_ref->second.resize(chunk_size + static_cast<std::size_t>(free_size));
					auto begin_point = file_chunk_contents_ref->second.linearize();
					file_iter_ref->_file->read(reinterpret_cast<char*>(begin_point + chunk_size), free_size);
					auto read_count = static_cast<size_type>(file_iter_ref->_file->gcount());

					if (free_size > read_count) {
						file_chunk_contents_ref->second.erase_end(static_cast<size_t>(free_size - read_count));
					}
					file_iter_ref->_current_read_so_far += read_count;
					is_file_finished =
						file_iter_ref->_current_read_so_far == file_iter_ref->_file_size ||
						file_iter_ref->_file->eof();

					file_iter_ref->_cache_buf->put_cache_ptr(is_file_finished);

					/*BOOST_LOG_TRIVIAL(debug)
					<< "data size:"
					<< (*file_iter)->_cache_buf->data_size_guess();*/

					if (file_iter_ref->_cache_buf->buffer_size_count() - file_iter_ref->_cache_buf->data_size_guess() <= 2)
					{
						//BOOST_LOG_TRIVIAL(debug) << "data near full";
						next_dur = std::chrono::seconds(1);
					}
					else if (file_iter_ref->_cache_buf->data_size_guess() > 2)
					{
						//BOOST_LOG_TRIVIAL(debug) << "data ok";
						next_dur = std::chrono::milliseconds(100);
					}
					else
					{
						//BOOST_LOG_TRIVIAL(debug) << "data near empty";
						next_dur = std::chrono::microseconds(0);
					}

					if (is_file_finished) {
						close_file(file_iter);
					}

				}
				catch (std::exception const& e)
				{
					BOOST_LOG_TRIVIAL(debug) << "exception occurred: " << e.what();
				}
			}
			else {
				//BOOST_LOG_TRIVIAL(debug)<< "no cache means full data";
				next_dur = std::chrono::seconds(1);
			}
		}
	}

	// job functions start here ...
	url_id_t input_plugin_file::add_input_item(std::string filename)
	{
		BOOST_LOG_TRIVIAL(trace) << "input_plugin_file::add_input_item called with " + filename;

		auto url_id = ++_url_id;
		add_job([=]() {
			add_file(filename, url_id);
		});

		return url_id;
	}


	url_id_t input_plugin_file::add_input_item(std::string filename, url_id_t url_id)
	{
		add_job([=]() {
			add_file(filename, url_id);
		});

		return url_id;
	}

	void input_plugin_file::remove_input_item(url_id_t url_id)
	{
		BOOST_LOG_TRIVIAL(trace) << "input_plugin_file::remove() item called with " << url_id;

		add_job([=]() { remove_file(url_id); });
	}
	
	void input_plugin_file::set_cache_buffer(url_id_t url_id, cache_buffer_shared cache_buf)
	{
		add_job([=]
		{
			for (auto & file : _files)
			{
				if (url_id == file->_url_id) {
					file->_cache_buf = cache_buf;
					break;
				}
			}
		});
	}

	bool input_plugin_file::is_no_job()
	{
		return _files.empty();
	}

	void input_plugin_file::stop_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "input_plugin_file::stop_internal() called";

		/*
		auto
			iter_file = _files.begin(),
			iter_file_end = _files.end();

		while (iter_file != iter_file_end) {
			if (!(*iter_file)->_file->is_open())
			{
				_files.erase(iter_file++);
			}
			else
			{
				++iter_file;
			}
		}
		*/
	}

	void input_plugin_file::pause_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "input_plugin_file::pause_internal() called";
		
	}

	void input_plugin_file::cont_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "input_plugin_file::cont_internal() called";

		try_read();
	}

	void input_plugin_file::quit_internal()
	{

	}
	
	bool input_plugin_file::remove_file_helper(file_list_t & cnt, url_id_t url_id)
	{
		auto _file_iter = cnt.begin(), _file_iter_end = cnt.end();
		while (_file_iter != _file_iter_end)
		{
			if ((*_file_iter)->_url_id == url_id)
			{
				cnt.erase(_file_iter);
				return true;
			}
		}

		return false;
	}

	input_plugin_file::file_list_t::iterator input_plugin_file::seek_helper(file_list_t & file_cont, url_id_t url_id, size_type seek_point)
	{
		auto iter = file_cont.begin(), iter_end = file_cont.end();
		for (; iter != iter_end; ++iter)
		{
			if ((*iter)->_url_id == url_id)
			{
				(*iter)->_cache_buf->clear_cache();

				BOOST_LOG_TRIVIAL(debug)
					<< "FILE address: " << (*iter)->_cache_buf
					<< " FILE cache size: " << (*iter)->_cache_buf->cache_size_guess()
					<< " data size: " << (*iter)->_cache_buf->data_size_guess()
					<< " total data: " << (*iter)->_cache_buf->total_bytes_in_buffer_guess();

				(*iter)->_file->clear();
				(*iter)->_file->seekg(seek_point);
				(*iter)->_current_read_so_far = seek_point;
				return iter;
			}
		}

		return iter_end;
	}


	void input_plugin_file::seek_callback_internal(url_id_t url_id, size_type seek_point)
	{
		BOOST_LOG_TRIVIAL(debug) << "FILE seek callback point: " << seek_point;
		auto was_no_job = is_no_job();

		auto file_detail_iter = seek_helper(_files, url_id, seek_point);
		bool found(true);

		if (file_detail_iter == _files.end())
		{
			found = false;
			file_detail_iter = seek_helper(_finished_files, url_id, seek_point);

			if (file_detail_iter != _finished_files.end())
			{
				found = true;
				_files.push_front(*file_detail_iter);
				_finished_files.erase(file_detail_iter);
			}
		}

		if (was_no_job /*&& _current_state == to_underlying(plugin_states::play)*/)
		{
			_current_state = plugin_states::play;
			cont_internal();
		}
	}

	void input_plugin_file::seek_callback_job(url_id_t url_id, size_type seek_point)
	{
		add_job([this, url_id, seek_point]
		{
			seek_callback_internal(url_id, seek_point);
		});
	}

	void input_plugin_file::input_close(url_id_t url_id)
	{
		BOOST_LOG_TRIVIAL(trace) <<
			"input_plugin_file::input_close() called for id: " << url_id;

		auto
			iter = _finished_files.begin(),
			iter_end = _finished_files.end();

		while (iter != iter_end)
		{
			if ((*iter)->_url_id == url_id)
			{
				_finished_files.erase(iter++);
				return;
			}
			else
			{
				++iter;
			}
		}

		auto
			iter_list = _files.begin(),
			iter_list_end = _files.end();

		while (iter_list != iter_list_end)
		{
			if ((*iter_list)->_url_id == url_id)
			{
				_files.erase(iter_list++);
				return;
			}
			else
			{
				++iter_list;
			}
		}

		BOOST_LOG_TRIVIAL(debug) << "cannot remove: " << url_id;
	}

	void input_plugin_file::decoder_finish(std::string plugin_name, url_id_t url_id)
	{
		add_job([this, url_id]() {
			input_close(url_id);
		});
	}
}

// Factory method. Returns *simple pointer*!
std::unique_ptr<refcounting_plugin_api> create() {
	return std::make_unique<mprt::input_plugin_file>();
}


BOOST_DLL_ALIAS(create, create_refc_plugin)
