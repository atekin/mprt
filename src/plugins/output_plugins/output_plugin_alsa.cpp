#include <alsa/asoundlib.h>

#include <boost/dll/runtime_symbol_info.hpp>

#include "common/decoder_plugin_api.h"
#include "common/scope_exit.h"
#include "core/config.h"

#include "output_plugin_alsa.h"

#include "common/type_defs.h"

extern "C" snd_pcm_hw_params_t * m_snd(size_t /*size*/)
{
	snd_pcm_hw_params_t *tmp;
	snd_pcm_hw_params_malloc(&tmp);
	return tmp;
}

namespace mprt
{

	// please note that what alsa called a frame we call sample, 
	// alsa says a sample for a channel, their nomenclature is more correct
	
	// Must be instantiated in plugin
	boost::filesystem::path output_plugin_alsa::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string output_plugin_alsa::plugin_name() const {
		return "output_plugin_alsa";
	}

	plugin_types output_plugin_alsa::plugin_type() const {
		return plugin_types::output_plugin;
	}

	void output_plugin_alsa::init(void *)
	{
		try
		{	
			config::instance().init("../config/config_output_plugin_alsa.xml");
			auto pt = config::instance().get_ptree_node("mprt.output_plugin_alsa");

			_async_task = std::make_shared<async_tasker>(pt.get<std::size_t>("max_free_timer_count", 10));
			_preffered_device_name = pt.get<std::string>("preffered_device_name", "default");
			_mixer_device = pt.get<std::string>("mixer_device", "default");
			_mixer_name = pt.get<std::string>("mixer_name", "Master");
			_mixer_index = pt.get<int>("mixer_index", 0);
			_use_db_vol = pt.get<std::string>("use_db_vol", "false") == "true";
			_hw_resampling = (pt.get<std::string>("hardware_resampling", "false") == "true");
			_max_period = pt.get<size_type>("max_period", 100);
			_max_buffer = pt.get<size_type>("max_buffer", 500);
			_use_buffer_duration_size = (pt.get<std::string>("use_buffer_size_or_duration_msec", "duration") == "size");
			_next_diff_item_drain_time_msecs = pt.get<size_type>("next_diff_item_drain_time_msecs", 300);
			_use_poll = (pt.get<std::string>("use_poll", "true") == "true");
			_use_drain = (pt.get<std::string>("use_drain", "true") == "true");
			_use_duration = pt.get<std::string>("use_memory_size_or_durationms", "duration") == "duration";

			_max_chunk_read_size = pt.get<size_type>("max_chunk_read_size", 128) * 1024;
			if (!_use_duration)
			{
				_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file_duration", 4096) * 1024;
			}
			else
			{
				_max_memory_size_per_file = pt.get<size_type>("max_memory_size_per_file_duration", 20000);

			}
		}
		catch (std::exception const& e) {
			BOOST_LOG_TRIVIAL(error) << "error: " << e.what();
		}
	
		while (!_async_task->is_ready()) {
			std::this_thread::yield();
		}

		add_job([this]() { 
			int op_mode = SND_PCM_NONBLOCK; // | SND_PCM_NO_AUTO_RESAMPLE | SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_CHANNELS /*| SND_PCM_NONBLOCK*/;
							 //op_mode |= SND_PCM_NO_AUTO_RESAMPLE;
			if ((_pcm_handle = snd_pcm_open(
				&_playback_handle,
				_preffered_device_name.c_str(),
				SND_PCM_STREAM_PLAYBACK,
				op_mode)) < 0) {
				BOOST_LOG_TRIVIAL(error)
					<< "cannot open alsa driver: " << _preffered_device_name
					<< " error: " << snd_strerror(_pcm_handle);
				return;
			}

			_init_open = true;
			_init_poll = _use_poll ? init_poll_params() : false;

			init_mixer();
		});

	
	}

	void output_plugin_alsa::stop_internal()
	{
		for (auto & sound_det : _sound_details_queue)
		{
			sound_det._decoder_play_finished_callback(sound_det._url_id);

			give_cache_buf_back(sound_det._url_id);
		}

		_init_api = false;

		output_plugin_api::stop_internal();
		sound_plugin_api::reset_buffers();

	}

	void output_plugin_alsa::pause_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "alsa pause called";

		pause_alsa();
	}

	void output_plugin_alsa::quit_internal()
	{
	}

	void output_plugin_alsa::set_volume(size_type volume)
	{
		add_job([this, volume]() { set_volume_internal(volume); });
	}


	void output_plugin_alsa::reset_buffers()
	{
		_init_api = false;
	}

	snd_pcm_format_t output_plugin_alsa::get_pcm_format()
	{
		auto const& sound_dets = sound_details_top();

		if (sound_dets._is_float)
		{
			return is_big_endian() ? SND_PCM_FORMAT_FLOAT_BE : SND_PCM_FORMAT_FLOAT_LE;
		}
		else
		{
			switch (sound_dets._bps)
			{
			case 8:
				return SND_PCM_FORMAT_S8;
			case 16:
				return SND_PCM_FORMAT_S16_LE;
			case 32:
				return SND_PCM_FORMAT_S32_LE;
			}
		}
		
		return SND_PCM_FORMAT_S16_LE;
	}

	int output_plugin_alsa::xrun_recovery(int err)
	{
		BOOST_LOG_TRIVIAL(debug) << "stream recovery";

		if (err == -EPIPE) {
			BOOST_LOG_TRIVIAL(debug) << "alsa underrun recovery";
			err = snd_pcm_prepare(_playback_handle);
			if (err < 0)
				BOOST_LOG_TRIVIAL(debug)
					<< "Can't recovery from underrun, prepare failed: " << snd_strerror(err);
			return 0;
		} else if (err == -ESTRPIPE) {
			while ((err = snd_pcm_resume(_playback_handle)) == -EAGAIN)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (err < 0) {
				err = snd_pcm_prepare(_playback_handle);
				if (err < 0)
					BOOST_LOG_TRIVIAL(debug)
					<< "Can't recovery from suspend, prepare failed: " << snd_strerror(err);
			}
			return 0;
		}
		return err;
	}

	void output_plugin_alsa::init_mixer()
	{
		if (snd_mixer_open(&_mixer_handle, 0))
			return;

		if (snd_mixer_attach(_mixer_handle, _mixer_device.c_str()))
			return;

		if (snd_mixer_selem_register(_mixer_handle, NULL, NULL))
			return;

		if (snd_mixer_load(_mixer_handle))
			return;

		snd_mixer_selem_id_alloca(&_sid);
		if (!_sid)
			return;

		snd_mixer_selem_id_set_index(_sid, _mixer_index);
		snd_mixer_selem_id_set_name(_sid, _mixer_name.c_str());

		_mixer_elem = snd_mixer_find_selem(_mixer_handle, _sid);
		if (!_mixer_elem)
		{
			return;
		}

		_use_db_vol ? 
			snd_mixer_selem_get_playback_dB_range(_mixer_elem, &_mixer_min, &_mixer_max) : 
			snd_mixer_selem_get_playback_volume_range(_mixer_elem, &_mixer_min, &_mixer_max);
	}

	void output_plugin_alsa::pause_alsa()
	{
		if (_paused)
			return;

		if (_alsa_can_pause) {
			snd_pcm_state_t state = snd_pcm_state(_playback_handle);
			if (state == SND_PCM_STATE_PREPARED) {
				BOOST_LOG_TRIVIAL(debug) << "pause PREPARED";

			}
			else if (state == SND_PCM_STATE_RUNNING) {
				BOOST_LOG_TRIVIAL(debug) << "pause OK";

				snd_pcm_wait(_playback_handle, -1);
				snd_pcm_pause(_playback_handle, 1);
			}
			else {
				BOOST_LOG_TRIVIAL(debug) << "pause error: state is not RUNNING or PREPARED";
			}
		}
		else {
			snd_pcm_drop(_playback_handle);
		}

		_paused = true;
	}

	void output_plugin_alsa::unpause_alsa()
	{
		if (!_paused)
			return;

		if (_alsa_can_pause) {
			snd_pcm_state_t state = snd_pcm_state(_playback_handle);
			if (state == SND_PCM_STATE_PREPARED) {
				// state is PREPARED -> no need to unpause
			}
			else if (state == SND_PCM_STATE_PAUSED) {
				snd_pcm_wait(_playback_handle, -1);
				snd_pcm_pause(_playback_handle, 0);
			}
			else {
				BOOST_LOG_TRIVIAL(debug) << "unpause error: state is not RUNNING or PREPARED";
			}
		}
		else {
			snd_pcm_prepare(_playback_handle);
		}

		_paused = false;
	}

	void output_plugin_alsa::set_alsa_volume(long volume)
	{
		if (_is_mixer_open)
		{
			_use_db_vol ? set_alsa_volume_db_internal(volume) : set_alsa_volume_internal(volume);
		}
	}

	void output_plugin_alsa::set_alsa_volume_internal(long volume)
	{
		snd_mixer_selem_set_playback_volume_all(_mixer_elem, volume * _mixer_max / 100);
	}

	void output_plugin_alsa::set_alsa_volume_db_internal(long volume)
	{
		snd_mixer_selem_set_playback_dB_all(_mixer_elem, volume * _mixer_max / 100, 1);
	}

	size_type output_plugin_alsa::fill_drain(size_type bytes, sound_details const& sound_dets)
	{
		auto drain_sample_size = bytes_to_samples(bytes, sound_dets);
		std::vector<uint8_t> drain_buf(bytes, 0);

		auto written_samples =
			_use_poll ?
			poll_write(drain_buf.data(), drain_sample_size, sound_dets._bps / 8, sound_dets._channels)
			:
			direct_write(drain_buf.data(), drain_sample_size, sound_dets._bps / 8, sound_dets._channels);

		return samples_to_bytes(written_samples, sound_dets);
	}

	void output_plugin_alsa::fill_drain_internal()
	{
		fill_drain(_alsa_buffer_size_bytes, _prev_sound_details);
	}

	void output_plugin_alsa::init_api()
	{
		init_alsa();
	}

	bool output_plugin_alsa::init_alsa()
	{
		_STATE_CHECK_(plugin_states::play, false);

		_paused = false;
		
		if (!_init_open) {
			return false;
		}

		if (_sound_details_queue.empty()) {
			//wait until some sound detail has arrived ...
			return false;
		}

		bool is_same_before = is_sound_details_same_as_before();
		auto & new_sound_dets = sound_details_top_ref();

		_prev_sound_details = new_sound_dets;
		BOOST_LOG_TRIVIAL(debug) << "alsa changing current cache";

		// check for previous ...
		if (is_same_before) {
			BOOST_LOG_TRIVIAL(debug) << "alsa same as before";

			return (_init_api = true);
		}

		_prev_sound_details = new_sound_dets;
		auto init_hw = init_hw_params();
		auto init_sw = init_sw_params();
		return (_init_api = _use_poll ? (init_hw && init_sw && _init_poll) : (init_hw && init_sw));
	}

	bool output_plugin_alsa::init_hw_params()
	{
		_STATE_CHECK_(plugin_states::play, false);

		//BOOST_LOG_TRIVIAL(debug) << "hw params";

		int err, dir = SND_PCM_STREAM_PLAYBACK;

		snd_pcm_drop(_playback_handle);

		auto const& sound_dets = sound_details_top();

		if (!_hw_params)
		{
			if ((err = snd_pcm_hw_params_malloc(&_hw_params)) < 0) {
				/*BOOST_LOG_TRIVIAL(error) 
					<< "cannot allocate hardware parameter structure: " << snd_strerror(err);*/
				return false;
			}
		}
		
		/* choose all parameters */
		err = snd_pcm_hw_params_any(_playback_handle, _hw_params);
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug)
				<< "Broken configuration for playback: no configurations available: " << snd_strerror(err);*/
			return err;
		}
		/* set hardware resampling */
		err = snd_pcm_hw_params_set_rate_resample(_playback_handle, _hw_params, static_cast<int>(_hw_resampling));
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug)
				<< "Resampling setup failed for playback: " << snd_strerror(err);*/
			// return false;
		}
		/* set the interleaved read/write format */
		err = snd_pcm_hw_params_set_access(_playback_handle, _hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug) <<
				"Access type not available for playback: " << snd_strerror(err);*/
			return false;
		}
		/* set the sample format */
		err = snd_pcm_hw_params_set_format(_playback_handle, _hw_params, get_pcm_format());
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug)
				<<"Sample format not available for playback: " << snd_strerror(err);*/
			return false;
		}
		/* set the count of channels */
		err = snd_pcm_hw_params_set_channels(_playback_handle, _hw_params, sound_dets._channels);
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug)
				<< "Channels count: " << sound_dets._channels << " is not available for playbacks: " << snd_strerror(err);*/
			return false;
		}
		/* set the stream rate */
		unsigned int rrate = sound_dets._sample_rate;
		err = snd_pcm_hw_params_set_rate_near(_playback_handle, _hw_params, &rrate, 0);
		if (err < 0) {
			//BOOST_LOG_TRIVIAL(debug) <<"Rate: " << rrate << "Hz not available for playback: " << snd_strerror(err);
			return false;
		}
		if (rrate != sound_dets._sample_rate) {
			BOOST_LOG_TRIVIAL(debug) <<"Rate doesn't match (requested: " << sound_dets._sample_rate << "Hz, get: " << rrate << "Hz)";
			//return false;
		}

		if (_use_buffer_duration_size)
		{
			/* set the buffer time */
			snd_pcm_uframes_t buf_size = static_cast<unsigned int>(_max_buffer);
			err = snd_pcm_hw_params_set_buffer_size(_playback_handle, _hw_params, buf_size);
			if (err < 0) {
				/*BOOST_LOG_TRIVIAL(debug)
					<< "Unable to set buffer time: " << buf_size << " for playback: " << snd_strerror(err);*/
				return false;
			}

			/* set the period time */
			snd_pcm_uframes_t period_size = static_cast<unsigned int>(_max_period);
			err = snd_pcm_hw_params_set_period_size(_playback_handle, _hw_params, period_size, 0);
			if (err < 0) {
				//BOOST_LOG_TRIVIAL(debug) << "Unable to set period size: " << period_size << " for playback: " << snd_strerror(err);
				//return false;
			}
		}
		else
		{
			/* set the buffer time */
			unsigned int buffer_time = static_cast<unsigned int>(_max_buffer * 1000);
			err = snd_pcm_hw_params_set_buffer_time_near(_playback_handle, _hw_params, &buffer_time, &dir);
			if (err < 0) {
				/*BOOST_LOG_TRIVIAL(debug)
					<< "Unable to set buffer time: " << buffer_time << " for playback: " << snd_strerror(err);*/
						return false;
			}

			/* set the period time */
			unsigned int period_time = static_cast<unsigned int>(_max_period * 1000);
			err = snd_pcm_hw_params_set_period_time_near(_playback_handle, _hw_params, &period_time, &dir);
			if (err < 0) {
			 	//BOOST_LOG_TRIVIAL(debug) <<"Unable to set period time: " << period_time << " for playback: %s\n" << snd_strerror(err);
			 	return false;
			}
		}

		snd_pcm_uframes_t size;
		err = snd_pcm_hw_params_get_buffer_size(_hw_params, &size);
		if (err < 0) {
			/*BOOST_LOG_TRIVIAL(debug) 
				<< "Unable to get buffer size for playback: " << snd_strerror(err);*/
			return false;
		}
		_buffer_size = size;
		_alsa_buffer_size_bytes = samples_to_bytes(_buffer_size, sound_dets);
		BOOST_LOG_TRIVIAL(debug) << "buffer size: " << _buffer_size << " _alsa_buffer_size_bytes: " << _alsa_buffer_size_bytes;

		err = snd_pcm_hw_params_get_period_size(_hw_params, &size, &dir);
		if (err < 0) {
			//BOOST_LOG_TRIVIAL(debug) <<"Unable to get period size for playback: " << snd_strerror(err);
			return false;
		}
		_period_size = size;
		BOOST_LOG_TRIVIAL(debug) << "period size: " << _period_size;
		
		if (2 * _period_size > _buffer_size)
		{
			BOOST_LOG_TRIVIAL(debug) << "ALSA does not allow max_buffer_duration_msec >= 2*max_period_duration_msec, change them";
			return false;
		}

		_alsa_can_pause = snd_pcm_hw_params_can_pause(_hw_params);

		/* write the parameters to device */
		err = snd_pcm_hw_params(_playback_handle, _hw_params);
		if (err < 0) {
			BOOST_LOG_TRIVIAL(debug) <<"Unable to set hw params for playback: " << snd_strerror(err);
			return false;
		}

		return true;
	}

	bool output_plugin_alsa::init_sw_params()
	{
		_STATE_CHECK_(plugin_states::play, false);

		int err;

		//return true;

		BOOST_LOG_TRIVIAL(debug) << "sw params";

		if (!_sw_params)
		{
			if ((err = snd_pcm_sw_params_malloc(&_sw_params)) < 0) {
				BOOST_LOG_TRIVIAL(error) 
					<< "cannot allocate software parameters structure: " << snd_strerror(err);
				return false;
			}
		}

		/* get the current swparams */
		err = snd_pcm_sw_params_current(_playback_handle, _sw_params);
		if (err < 0) {
			BOOST_LOG_TRIVIAL(debug)
				<< "Unable to determine current swparams for playback: "<< snd_strerror(err);
			return false;
		}
		/* start the transfer when the buffer is almost full: */
		/* (buffer_size / avail_min) * avail_min */
		err = snd_pcm_sw_params_set_start_threshold(_playback_handle, _sw_params, (_buffer_size / _period_size) * _period_size);
		if (err < 0) {
			BOOST_LOG_TRIVIAL(debug)
				<<"Unable to set start threshold mode for playback: "<< snd_strerror(err);
			return false;
		}
		/* allow the transfer when at least period_size samples can be processed */
		err = snd_pcm_sw_params_set_avail_min(_playback_handle, _sw_params, _period_size);
		if (err < 0) {
			BOOST_LOG_TRIVIAL(debug)
				<<"Unable to set avail min for playback: "<< snd_strerror(err);
			return false;
		}
		
		/* write the parameters to the playback device */
		err = snd_pcm_sw_params(_playback_handle, _sw_params);
		if (err < 0) {
			BOOST_LOG_TRIVIAL(debug)
				<<"Unable to set sw params for playback: "<< snd_strerror(err);
			return false;
		}

		return true;
	}


	bool output_plugin_alsa::init_poll_params()
	{
		//BOOST_LOG_TRIVIAL(debug) << "ALSA init poll params";
		_poll_count = snd_pcm_poll_descriptors_count(_playback_handle);
		if (_poll_count <= 0) {
			//BOOST_LOG_TRIVIAL(error) << "Invalid poll descriptors count";
			return false;
		}

		_poll_ufds = (struct pollfd *)malloc(sizeof(struct pollfd) * _poll_count);
		if (_poll_ufds == nullptr) {
			//BOOST_LOG_TRIVIAL(error) << "Not enough memory";
			return false;
		}

		int err;
		if ((err = snd_pcm_poll_descriptors(_playback_handle, _poll_ufds, _poll_count)) < 0) {
			//BOOST_LOG_TRIVIAL(error) << "Unable to obtain poll descriptors for playback: " << snd_strerror(err);;
			return false;
		}

		_poll_init = true;
		return true;
	}

	void output_plugin_alsa::set_volume_internal(size_type volume)
	{
		_volume = volume;

		set_alsa_volume(volume);
	}

	snd_pcm_sframes_t output_plugin_alsa::direct_write(const void *buffer, snd_pcm_uframes_t size, int sample_width, int channels)
	{
		snd_pcm_sframes_t written_samples = 0;
		int err;
		unsigned char * buf = (unsigned char *)buffer;
		while (size > 0) {

			err = snd_pcm_writei(_playback_handle, buf, size);
			if (err < 0)
			{
				/*BOOST_LOG_TRIVIAL(debug)
					<< "alsa write failed: " << snd_strerror(written_samples)
					<< " error was: " << written_samples
					<< " need was: " << size;*/
				//<< " need bytes: " << avail_bytes_to_write
				//<< " alsa max: " << _alsa_buffer_size_bytes;
				if (-EAGAIN == err) {
					//BOOST_LOG_TRIVIAL(error) << "alsa write error trying one more";
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				else if (xrun_recovery(err) < 0)
				{
					//BOOST_LOG_TRIVIAL(error) << "there is a recovery error";
					return -1;
				}

				continue;
			}
			
			buf += err * channels * sample_width;
			written_samples += err;
			size -= err;
		}

		return written_samples;
	}

	int output_plugin_alsa::wait_for_poll()
	{
		unsigned short revents;

		while (1) {
			poll(_poll_ufds, _poll_count, -1);
			snd_pcm_poll_descriptors_revents(_playback_handle, _poll_ufds, _poll_count, &revents);
			if (revents & POLLERR)
				return -EIO;
			if (revents & POLLOUT)
				return 0;
		}
	}

	snd_pcm_sframes_t output_plugin_alsa::poll_write(const void *buffer, snd_pcm_uframes_t size, int sample_width, int channels)
	{
		int err;
		unsigned char *buf = (unsigned char *)buffer;
		if (!_poll_init)
		{
			err = wait_for_poll();
			if (err < 0) {
				if (snd_pcm_state(_playback_handle) == SND_PCM_STATE_XRUN ||
					snd_pcm_state(_playback_handle) == SND_PCM_STATE_SUSPENDED) {
					err = snd_pcm_state(_playback_handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
					if (xrun_recovery(err) < 0) {
						//BOOST_LOG_TRIVIAL(error) << "wait_for_poll Write error: " << snd_strerror(err);
						return -1;
					}
					_poll_init = true;
				}
				else {
					//BOOST_LOG_TRIVIAL(error) << "Wait for poll failed";
					return -1;
				}
			}
		}

		snd_pcm_sframes_t written_samples = 0;
		while (size > 0)
		{
			err = snd_pcm_writei(_playback_handle, buf, size);
			if (err < 0) {
				if (xrun_recovery(err) < 0) {
					BOOST_LOG_TRIVIAL(error) << "Write error: " << snd_strerror(err);
					return -1;
				}
				_poll_init = true;
				break;	/* skip one period */
			}

			if (snd_pcm_state(_playback_handle) == SND_PCM_STATE_RUNNING)
			{
				_poll_init = false;
			}
			
			written_samples += err;
			buf += err * channels * sample_width;
			size -= err;

			/* it is possible, that the initial buffer cannot store */
			/* all data from the last period, so wait awhile */
			if (size)
			{
				err = wait_for_poll();
				if (err < 0) {
					if (snd_pcm_state(_playback_handle) == SND_PCM_STATE_XRUN ||
						snd_pcm_state(_playback_handle) == SND_PCM_STATE_SUSPENDED) {
						err = snd_pcm_state(_playback_handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
						if (xrun_recovery(err) < 0) {
							//BOOST_LOG_TRIVIAL(error) << "wait_for_poll 2 Write error: " << snd_strerror(err);
							return -1;
						}
						_poll_init = true;
					}
					else {
						//BOOST_LOG_TRIVIAL(error) << "Wait for poll 2 failed";
						return err;
					}
				}
			}

		}

		return written_samples;
	}

	void output_plugin_alsa::play()
	{
		//BOOST_LOG_TRIVIAL(debug) << "alsa play state" << _current_state;

		_STATE_CHECK_(plugin_states::play);

		if (_play_timer && is_active_timer(_play_timer))
		{
			return;
		}

		std::chrono::microseconds next_duration(1000);

		size_type avail_bytes_to_write = _alsa_buffer_size_bytes;

		SCOPE_EXIT_REF(
			if (!is_no_job() && _current_state == plugin_states::play) {
				next_duration = 
				bytes_to_time_duration(_alsa_buffer_size_bytes - avail_bytes_to_write,
					sound_details_top()) / 4;

				_play_timer = add_job_thread_internal(
					[this, next_duration]() {
						play(); 
					}, next_duration);
				/*BOOST_LOG_TRIVIAL(debug)
					<< "play called with: " << next_duration.count() << " usecs"
					<< " alsa_buf: " << _alsa_buffer_size_bytes
					<< " write avail: " << avail_bytes_to_write;*/
			}
			//else {
			//	BOOST_LOG_TRIVIAL(debug) << "stopping alsa scope exit job: " << is_no_job();

				//_current_state = to_underlying(plugin_states::stop);
				//_init_api = false;
			//}
			);

		if (_current_state != plugin_states::play) {
			//BOOST_LOG_TRIVIAL(debug) << "alsa state not play";

			return;
		}

		if (_sound_details_queue.empty())
			return;

		auto & current_sound_dets = sound_details_top_ref();

		if (!_init_api) {
			BOOST_LOG_TRIVIAL(debug) << "starting new alsa sound parameters";

			auto is_init_ok = init_alsa();

			if (!is_init_ok)
			{
				BOOST_LOG_TRIVIAL(debug) <<
					"cannot initialize the direct sound with the current sound parameters";
				current_sound_dets._current_cache_buffer->clear_data();
				current_sound_dets._decoder_play_finished_callback(current_sound_dets._url_id);

				sound_details_pop();

				return;
			}
		}

		if (!current_sound_dets._current_cache_buffer)
		{
			sound_details_pop();
			return;
		}

		int wait_count = 0;
		while (current_sound_dets._current_cache_buffer->is_data_empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "sound waiting for the decoder";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++wait_count;
			if (wait_count > 20)
			{
				sound_details_pop();
				return;
			}
		}

		auto & decoded_data_buf = *(current_sound_dets._current_cache_buffer->get_data_ptr());
		auto begin_point = decoded_data_buf->second.linearize();
		auto buf_size = decoded_data_buf->second.size();
		avail_bytes_to_write =
			std::min<size_type>(
				alsa_available_bytes_to_write(),
				buf_size);

		//BOOST_LOG_TRIVIAL(debug) << "avail_bytes_to_write: " << avail_bytes_to_write;
		if (avail_bytes_to_write % samples_to_bytes(1, current_sound_dets)) {
			// we have full buffer or byte cannot be converted to samples
			return;
		}

		auto need_to_written = bytes_to_samples(avail_bytes_to_write, current_sound_dets);
		auto written_samples = 
			_use_poll ?
			poll_write(begin_point, need_to_written, current_sound_dets._bps / 8, current_sound_dets._channels)
			:
			direct_write(begin_point, need_to_written, current_sound_dets._bps / 8, current_sound_dets._channels);
		if (written_samples < 0)
		{
			return;
		}
		current_sound_dets._current_samples_written_to_sound_buffer += written_samples;
		auto written_bytes = samples_to_bytes(written_samples, current_sound_dets);
		/*BOOST_LOG_TRIVIAL(debug)
			<< " _alsa_buffer_size_bytes" << _alsa_buffer_size_bytes
			<< " avail_bytes_write: " << avail_bytes_to_write
			<< " written bytes: " << written_bytes;*/
		avail_bytes_to_write -= written_bytes;
		/*BOOST_LOG_TRIVIAL(debug)
			<< " avail_bytes_write: " << avail_bytes_to_write
			<< " written bytes: " << written_bytes;*/
		decoded_data_buf->second.erase_begin(written_bytes);
		decoded_data_buf->first -= written_bytes;
		auto is_play_finished = (
			current_sound_dets._current_samples_written_to_sound_buffer == current_sound_dets._total_samples);
		current_sound_dets._current_cache_buffer->put_data_ptr(is_play_finished);

		//BOOST_LOG_TRIVIAL(debug) << "data total size: " <<
		//	current_sound_dets._current_cache_buffer->total_bytes_in_buffer_guess();
		
		next_duration = bytes_to_time_duration(_alsa_buffer_size_bytes - avail_bytes_to_write, current_sound_dets) / 2;
		//BOOST_LOG_TRIVIAL(debug)
		//	<< "next dur: " << next_duration.count()
		//	//;
		//	<< " avail_bytes_write: " << avail_bytes_to_write;

		call_callback_funcs(
			_progress_func_call_list,
			 current_sound_dets._url_id,
			  samples_to_time_duration(current_sound_dets._current_samples_written_to_sound_buffer, current_sound_dets).count() / 1000);

		if (is_play_finished)
		{
			BOOST_LOG_TRIVIAL(debug) << "finishing playing alsa for id: " << current_sound_dets._url_id;

			current_sound_dets._decoder_play_finished_callback(current_sound_dets._url_id);
			
			sound_details_pop();

			if (!is_sound_details_same_as_before())
			{
				
				auto avail_play = alsa_available_bytes_to_play();
				auto silence_need_bytes =
					time_duration_to_bytes(
							std::chrono::microseconds(_next_diff_item_drain_time_msecs * 1000),
							_prev_sound_details);
				if (silence_need_bytes)
				{
					fill_drain(silence_need_bytes, _prev_sound_details);
				}

				BOOST_LOG_TRIVIAL(debug)
					<< "entering draining id: " << _prev_sound_details._url_id
					<< " drain bytes: " << avail_play
					<< " silence bytes: " << silence_need_bytes;
				if (_use_drain)
				{
					snd_pcm_nonblock(_playback_handle, 0);
					snd_pcm_drain(_playback_handle);
					snd_pcm_nonblock(_playback_handle, 1);
				}
				else
				{
					std::this_thread::sleep_for(bytes_to_time_duration(avail_play, _prev_sound_details));
					snd_pcm_drop(_playback_handle);
				}

				BOOST_LOG_TRIVIAL(debug) << "drain finished with id: " <<
					_prev_sound_details._url_id << " silence bytes: " << silence_need_bytes;

			}

			give_cache_buf_back(_prev_sound_details._url_id);
			_init_api = false;
		}

		if (is_no_job()) {
			BOOST_LOG_TRIVIAL(debug) << "alsa sound play finished";

			_current_state = plugin_states::stop;
		}

		return;
	}

	void output_plugin_alsa::pause_play_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "alsa pause called";

		pause_alsa();
	}

	void output_plugin_alsa::resume_play_internal()
	{
		BOOST_LOG_TRIVIAL(debug) << "alsa unpause called";

		unpause_alsa();

		play();
	}

	void output_plugin_alsa::resume_clear_play_internal()
	{
		unpause_alsa();

		play();
	}

	size_type output_plugin_alsa::alsa_available_bytes_to_play()
	{
		return _alsa_buffer_size_bytes - alsa_available_bytes_to_write();
	}

	size_type output_plugin_alsa::alsa_available_bytes_to_write()
	{
		size_type frames_to_deliver = 
			_use_poll ?
			snd_pcm_avail_update(_playback_handle)
			:
			snd_pcm_avail(_playback_handle);

		//BOOST_LOG_TRIVIAL(debug) << "frames_to_deliver" << frames_to_deliver;

		if (frames_to_deliver < 0) {
			if (frames_to_deliver == -EPIPE) {
				xrun_recovery(frames_to_deliver);
				return frames_to_deliver;
			}
			else {
				/*BOOST_LOG_TRIVIAL(debug)
					<< "unknown ALSA avail update return value: ", snd_strerror(frames_to_deliver);*/
				return frames_to_deliver;
			}
		}
	
		return samples_to_bytes(frames_to_deliver, sound_details_top());
	}

	std::chrono::microseconds output_plugin_alsa::get_next_duration()
	{
		return 
			bytes_to_time_duration(
				alsa_available_bytes_to_play(), sound_details_top());
	}

}

// Factory method. Returns *simple pointer*!
std::unique_ptr<refcounting_plugin_api> create() {
	return std::make_unique<mprt::output_plugin_alsa>();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)
