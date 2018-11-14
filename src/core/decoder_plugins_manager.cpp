
#include "config.h"

#include "config.h"
#include "../common/refcounting_plugin_api.h"
#include "../common/decoder_plugin_api.h"
#include "../common/output_plugin_api.h"

#include "decoder_plugins_manager.h"


namespace mprt
{

	decoder_plugins_manager::decoder_plugins_manager()
		: _set_output_buf_callback(std::bind(&decoder_plugins_manager::set_cache_buffer, this, std::placeholders::_1, std::placeholders::_2))
	{
		config::instance().init("../config/config.xml");
		_async_task = std::make_shared<async_tasker>(config::instance().get_ptree_node("mprt.plugin_configs.decoder_plugins").get<size_t>("max_free_timer_count", 5));
	}

	decoder_plugins_manager::~decoder_plugins_manager()
	{

	}

	void decoder_plugins_manager::add_output_plugin(std::shared_ptr<output_plugin_api> & output_plugin)
	{
		for (auto & dec_plugin : _decoder_plugin_list)
		{
			dec_plugin->add_output_plugin(output_plugin);
		}
	}

	void decoder_plugins_manager::add_output_plugins(std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> & output_plugins)
	{
		std::for_each(output_plugins->begin(), output_plugins->end(), [this](std::shared_ptr<output_plugin_api> & output_plugin)
		{
			add_output_plugin(output_plugin);
		});
	}

	void decoder_plugins_manager::remove_output_plugin(std::shared_ptr<output_plugin_api> output_plugin)
	{
		for (auto & dec_plugin : _decoder_plugin_list)
		{
			dec_plugin->remove_output_plugin(output_plugin);
		}
	}

	void decoder_plugins_manager::decode_cont()
	{
		_STATE_CHECK_(plugin_states::play);

		if (_decoder_detail_list.empty())
			return;

		if (_decode_timer && is_active_timer(_decode_timer))
		{
			return;
		}

		auto &cur_det = get_current_decoder_details_ref();
		if (cur_det->_output_buf_list.empty()) {
			BOOST_LOG_TRIVIAL(debug) << "there is no output buf yet";
			if (!_decode_timer || (_decode_timer && is_timer_expired(_decode_timer)))
			{
				_decode_timer = add_job_thread_internal([this]() {
					decode_cont();
				}, std::chrono::milliseconds(100));
			}
			return;
		}

		/*BOOST_LOG_TRIVIAL(debug)
		<< "seek in prog: " << _seek_in_progress
		<< " first buf: " << cur_det->_current_cache_buf->total_bytes_in_buffer_guess()
		<< " cache size guess: " << cur_det->_current_cache_buf->cache_size_guess()
		<< " data size guess: " << cur_det->_current_cache_buf->data_size_guess();*/


		bool any_cache_empty = false;
		for (auto & out_buf : cur_det->_output_buf_list)
		{
			//BOOST_LOG_TRIVIAL(debug) << "output cache size guess: " << out_buf->cache_size_guess();
			any_cache_empty = any_cache_empty || (out_buf->cache_size_guess() == 0);
		}

		if (any_cache_empty) {
			if (!_decode_timer || (_decode_timer && is_timer_expired(_decode_timer)))
			{
				_decode_timer = add_job_thread_internal([this]() {
					decode_cont();
				}, std::chrono::milliseconds(1000));
			}
			return;
		}

		cur_det->_current_decoder_plugin->decode();

		if (!is_no_job()) {

			//BOOST_LOG_TRIVIAL(debug) << "_output_buffer_full: " << _output_buffer_full;

			add_job([this] { decode_cont(); });
		}

		//BOOST_LOG_TRIVIAL(debug) << "flac_decode_plugin::decode_cont exited";
	}

	

	void decoder_plugins_manager::add_input_url_job_internal(std::shared_ptr<current_decoder_details> const& decoder_dets)
	{
		auto was_no_job = is_no_job();

		push_back_current_decoder_details(decoder_dets);

		decoder_dets->_current_decoder_plugin = get_decoder_plugin(decoder_dets->_url_ext);
		auto cache_buf = decoder_dets->_current_decoder_plugin->get_cache_put_buf(decoder_dets->_sound_details._url_id);

		decoder_dets->_set_input_cache_buf_callback(decoder_dets->_sound_details._url_id, cache_buf);

		if (plugin_states::play == _current_state && was_no_job)
		{
			cont();
		}
	}

	bool decoder_plugins_manager::update_dec_detail_seek(std::shared_ptr<current_decoder_details> dec_det, size_type seek_point)
	{
		auto wait_count = 0;

		if (seek_point < 0 || seek_point > dec_det->_stream_length)
			return false;
		
		auto need_pos = seek_point - dec_det->_current_stream_pos;
		if (need_pos < 0)
		{
			BOOST_LOG_TRIVIAL(debug) << "need pos: " << need_pos << " buf size: " << _last_read_buffer.size();
			need_pos = -need_pos;
			if (need_pos < static_cast<int>(_last_read_buffer.size()))
			{
				decltype(dec_det->_current_cache_buf->get_data_ptr()) buf;
				while ((buf = dec_det->_current_cache_buf->get_data_ptr()) == nullptr)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					++wait_count;
					if (wait_count > 20)
					{
						return false;
					}
				}
				auto need_capacity = (*buf)->second.size() + static_cast<std::size_t>(need_pos);
				if ((*buf)->second.capacity() < need_capacity)
					(*buf)->second.set_capacity(need_capacity);
				(*buf)->second.insert((*buf)->second.begin(), _last_read_buffer.end() - static_cast<int>(need_pos), _last_read_buffer.end());
				dec_det->_current_stream_pos -= need_pos;
				(*buf)->first = dec_det->_current_stream_pos;
				dec_det->_current_cache_buf->put_data_ptr(false);
			}

			need_pos = seek_point - dec_det->_current_stream_pos;
		}

		if (
			need_pos >= 0 &&
			(need_pos - dec_det->_current_cache_buf->total_bytes_in_buffer_guess() <= 0)
			)
		{
			BOOST_LOG_TRIVIAL(debug) << "we already have the seek data very nice ...";

			dec_det->_current_cache_buf->discard_data_upto(need_pos);
			dec_det->_current_stream_pos += need_pos;
		}
		else
		{
			dec_det->_seek_callback(dec_det->_sound_details._url_id, seek_point);

			// wait till data arrives ...
			bool wait_input = true;
			while (wait_input)
			{
				wait_count = 0;
				decltype(dec_det->_current_cache_buf->get_data_ptr()) buf;
				while ((buf = dec_det->_current_cache_buf->get_data_ptr()) == nullptr)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					++wait_count;
					if (wait_count > 20)
					{
						//return false;
					}
				}

				if ((wait_input = (*buf)->first != seek_point)) {
					(*buf)->second.clear();
					dec_det->_current_cache_buf->put_data_ptr(true);
				}
			}
		}

		dec_det->_current_stream_pos = seek_point;

		/*BOOST_LOG_TRIVIAL(debug)
		<< "FFMPEG seek_point: " << seek_point
		<< " FFMPEG cache size: " << dec_det->_current_cache_buf->cache_size_guess()
		<< " data size: " << dec_det->_current_cache_buf->data_size_guess()
		<< " total data: " << dec_det->_current_cache_buf->total_bytes_in_buffer_guess();*/

		return true;
	}

	bool decoder_plugins_manager::seek_byte_internal(url_id_t url_id, size_type seek_point)
	{
		BOOST_LOG_TRIVIAL(debug) << "seek comes here";

		for (auto & cur_det : _decoder_detail_list)
		{
			if (url_id == cur_det->_sound_details._url_id)
			{
				return update_dec_detail_seek(cur_det, seek_point);
			}
		}

		/*
		auto iter = _finished_decoder_detail_list.find(url_id);

		if (iter != _finished_decoder_detail_list.end())
		{
			return update_dec_detail_seek(iter->second, seek_point);
		}
		*/

		return false;
	}


	void decoder_plugins_manager::seek_duration(url_id_t url_id, size_type seek_duration_ms)
	{
		add_job([this, url_id, seek_duration_ms]
		{
			auto iter = _finished_decoder_detail_list.find(url_id);
			if (iter != _finished_decoder_detail_list.end())
			{
				if (_decoder_detail_list.empty() || _decoder_detail_list.front()->_sound_details._url_id != url_id)
					push_front_current_decoder_details(iter->second);
			}
			else
			{
				auto
					iter_list_beg = _decoder_detail_list.begin(),
					iter_list = iter_list_beg,
					iter_list_end = _decoder_detail_list.end();

				for (; iter_list != iter_list_end; ++iter_list)
				{
					if ((*iter_list)->_sound_details._url_id == url_id) {
						break;
					}
				}

				if (iter_list != iter_list_beg)
				{
					_decoder_detail_list.splice(iter_list_beg, _decoder_detail_list, iter_list, iter_list == iter_list_end ? iter_list_end : std::next(iter_list));
				}
			}

			if (!is_current_decoder_details_empty())
			{
				//_seek_duration_ms = point_ms;

				auto &cur_det = get_current_decoder_details_ref();
				cur_det->_current_decoder_plugin->seek_duration(seek_duration_ms);
				cur_det->_current_samples_written = sound_plugin_api::time_duration_to_samples(std::chrono::microseconds(seek_duration_ms * 1000), cur_det->_sound_details);

				for (auto & output_buf : cur_det->_output_buf_list)
				{
					output_buf->clear_cache();
				}

				for (auto output_plugin : cur_det->_current_decoder_plugin->_output_plugin_list)
				{
					output_plugin->pause_play();

					output_plugin->clear_play_data(cur_det->_sound_details._url_id);
					output_plugin->fill_drain();
					output_plugin->set_seek_duration_ms(cur_det->_sound_details._url_id, seek_duration_ms);

					output_plugin->resume_clear_play();
				}


				decode_cont();
			}

			_decoder_seek_finished_cb();

		});
		
	}

	std::pair<size_type, url_id_t> decoder_plugins_manager::decoder_read_buffer(buffer_elem_t *buffer, size_type buf_size)
	{
		//_STATE_CHECK_(plugin_states::play, std::make_pair(0, -1));

		auto & decoder_dets = get_current_decoder_details_ref();

		decoder_dets->_last_read_empty = true;
		_last_read_buffer.clear();

		auto max_buf_size = std::min(buf_size, decoder_dets->_stream_length - decoder_dets->_current_stream_pos);
		if (decoder_dets->_current_stream_pos < decoder_dets->_stream_length)
		{
			auto total_buf_data = decoder_dets->_current_cache_buf->total_bytes_in_buffer_guess();
			auto count_same = 0;
			while (
				total_buf_data < max_buf_size ||
				decoder_dets->_current_cache_buf->is_data_empty())
			{
				BOOST_LOG_TRIVIAL(debug) << decoder_dets->_current_decoder_plugin->plugin_name()
					<< " waiting for input data, max_buf_size: "
					<< max_buf_size << " data size: "
					<< decoder_dets->_current_cache_buf->total_bytes_in_buffer_guess();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				if (total_buf_data == decoder_dets->_current_cache_buf->total_bytes_in_buffer_guess())
				{
					++count_same;
					if (count_same > 20)
					{
						BOOST_LOG_TRIVIAL(debug) << "breaking decoder wait loop";
						return std::make_pair(-1, decoder_dets->_sound_details._url_id);
					}
				}
				else
				{
					count_same = 0;
					total_buf_data = decoder_dets->_current_cache_buf->total_bytes_in_buffer_guess();
				}
			}

		}

		auto written_bytes = decoder_dets->_current_cache_buf->write_into_raw_buffer(buffer, max_buf_size);

		decoder_dets->_current_stream_pos += written_bytes;
		decoder_dets->_last_read_empty = false;

		if (_last_read_buffer.capacity() < written_bytes)
		{
			_last_read_buffer.set_capacity(static_cast<std::size_t>(written_bytes));
		}
		//_last_read_buffer.resize(written_bytes);
		//std::memcpy(_last_read_buffer.linearize(), buffer, static_cast<std::size_t>(written_bytes));
		_last_read_buffer.insert(_last_read_buffer.begin(), buffer, buffer + written_bytes);

		return std::make_pair(written_bytes, decoder_dets->_sound_details._url_id);
	}


	void decoder_plugins_manager::cont_internal()
	{
		BOOST_LOG_TRIVIAL(trace) << "decoder_plugins_manager::cont_internal() called";

		if (is_current_decoder_details_empty())
		{
			add_job([this]()
			{
				BOOST_LOG_TRIVIAL(trace) << "decoder waiting for input to be ready";

				cont_internal();
			}, std::chrono::milliseconds(100));

			return;
		}

		auto cur_det = get_current_decoder_details();
		cur_det->_current_decoder_plugin->init_decode_internal_single(cur_det->_sound_details._url_id);

		decode_cont();
	}

	std::shared_ptr<mprt::decoder_plugin_api> decoder_plugins_manager::get_decoder_plugin(std::string const& url_ext)
	{
		for (auto & dec_api : _decoder_plugin_list)
		{
			if (dec_api->match_extension(url_ext))
			{
				return dec_api;
			}
		}

		return std::shared_ptr<mprt::decoder_plugin_api>();
	}

	void decoder_plugins_manager::stop_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "decoder_plugins_manager::stop_internal() called";
	}

	void decoder_plugins_manager::pause_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "decoder_plugins_manager::pause_internal() called";

	}

	void decoder_plugins_manager::quit_internal()
	{

	}

	void decoder_plugins_manager::finish_decode_internal_single(url_id_t url_id)
	{
		auto & decoder_dets = get_current_decoder_details_ref();

		BOOST_LOG_TRIVIAL(debug) <<
			"finishing decode process for " << decoder_dets->_sound_details._url_id << " with name: " << decoder_dets->_url;

		if (decoder_dets->_current_samples_written != decoder_dets->_sound_details._total_samples) {
			decoder_dets->_sound_details._total_samples = decoder_dets->_current_samples_written;
			for (auto & output_plugin : decoder_dets->_current_decoder_plugin->_output_plugin_list)
			{
				output_plugin->update_total_samples(decoder_dets->_sound_details._url_id, decoder_dets->_current_samples_written);
			}
		}

		_finished_decoder_detail_list[url_id] = decoder_dets;

		for (auto decoder_output_buffer : decoder_dets->_output_buf_list) {
			decoder_output_buffer->sync_cache();
		}

		get_current_decoder_details_pop();

		if(!is_no_job())
		{
			get_current_decoder_details()->_current_decoder_plugin->init_decode_internal_single(get_current_decoder_details()->_sound_details._url_id);
		}
	}

	mprt::set_output_buffer_callback_register_func_t decoder_plugins_manager::set_output_buf_callback()
	{
		return _set_output_buf_callback;
	}

	void decoder_plugins_manager::add_decoder_plugin(std::shared_ptr<decoder_plugin_api> & dec_plugin)
	{
		_decoder_plugin_list.insert(dec_plugin);

		dec_plugin->set_decoder_plugin_manager(this);
	}

	void decoder_plugins_manager::add_decoder_plugins(std::shared_ptr<std::vector<std::shared_ptr<decoder_plugin_api>>> & dec_plugins)
	{
		std::for_each(dec_plugins->begin(), dec_plugins->end(), [this](std::shared_ptr<decoder_plugin_api> & dec_plugin)
		{
			add_decoder_plugin(dec_plugin);
		});
	}

	void decoder_plugins_manager::clear_finish_decoder(url_id_t url_id)
	{
		_finished_decoder_detail_list.erase(url_id);
	}

	bool comp_decode_plugin_api::operator()(std::shared_ptr<decoder_plugin_api> const & lhs, std::shared_ptr<decoder_plugin_api> const & rhs) const
	{
		return lhs->priority() > rhs->priority();
	}

}