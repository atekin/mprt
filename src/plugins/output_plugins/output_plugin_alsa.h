#ifndef output_plugin_alsa_h__
#define output_plugin_alsa_h__

#include <string>

extern "C"
{
#include <alsa/asoundlib.h>
}

#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include "common/output_plugin_api.h"

namespace mprt
{
	
	class output_plugin_alsa : public output_plugin_api
	{
	private:
		size_type _max_buffer;
		size_type _max_period;
		bool _use_buffer_duration_size;
		unsigned int _period_size;
		unsigned int _buffer_size;
		size_type _next_diff_item_drain_time_msecs;
		size_type _volume;
		size_type _alsa_buffer_size_bytes;
		std::string _preffered_device_name;
		std::string _mixer_device;
		std::string _mixer_name;
		bool _use_db_vol;
		snd_pcm_t *_playback_handle;
		snd_pcm_hw_params_t *_hw_params;
		snd_pcm_sw_params_t *_sw_params;
		snd_mixer_t *_mixer_handle;
		snd_mixer_selem_id_t *_sid;
		snd_mixer_elem_t* _mixer_elem;
		long _mixer_min, _mixer_max;
		int _mixer_index;
		bool _is_mixer_open;
		int _pcm_handle;
		bool _hw_resampling;
		bool _use_poll;
		bool _init_poll;
		struct pollfd *_poll_ufds;
		int _poll_count;
		bool _poll_init;
		bool _init_open;
		bool _alsa_can_pause;
		bool _use_drain;
		bool _paused;

		void reset_buffers() override;
		bool init_alsa();
		bool init_hw_params();
		bool init_sw_params();
		bool init_poll_params();
		void set_volume_internal(size_type volume);
		size_type alsa_available_bytes_to_write();
		size_type alsa_available_bytes_to_play();
		std::chrono::microseconds get_next_duration();

		snd_pcm_sframes_t direct_write(const void *buffer, snd_pcm_uframes_t size, int sample_width, int channels);

		int wait_for_poll();
		snd_pcm_sframes_t poll_write(const void *buffer, snd_pcm_uframes_t size, int sample_width, int channels);

		virtual void play() override;
		virtual void pause_play_internal() override;
		virtual void resume_play_internal() override;
		virtual void resume_clear_play_internal() override;
		virtual void stop_internal() override;
		virtual void pause_internal() override;
		virtual void quit_internal() override;

		snd_pcm_format_t get_pcm_format();
		int xrun_recovery(int err);

		void init_mixer();
		void pause_alsa();
		void unpause_alsa();
		void set_alsa_volume(long volume);
		void set_alsa_volume_internal(long volume);
		void set_alsa_volume_db_internal(long volume);

		size_type fill_drain(size_type bytes, sound_details const& sound_dets);
		virtual void fill_drain_internal() override;
		virtual void init_api() override;
	public:
		output_plugin_alsa()
			: _hw_params(nullptr)
			, _sw_params(nullptr)
			, _poll_ufds(nullptr)
			, _init_open(false)
			, _is_mixer_open(false)
			, _sid(nullptr)
			, _mixer_min(0)
			, _mixer_max(100)
		{}
		
		virtual ~output_plugin_alsa() {
			BOOST_LOG_TRIVIAL(debug) << "output_plugin_alsa::~output_plugin_alsa() called";

			delete_ptr(_hw_params, snd_pcm_hw_params_free);

			delete_ptr(_sw_params, snd_pcm_sw_params_free);
			
			delete_ptr(_poll_ufds);

			if (_init_open)
			{
				snd_pcm_close(_playback_handle);
				_init_open = false;
			}
			
			if (_is_mixer_open)
			{
				snd_mixer_close(_mixer_handle);
			}

			delete_ptr(_sid, snd_mixer_selem_id_free);
		}

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void *) override;

		// job thread functions
		virtual void set_volume(size_type volume) override; // 0 to 100
		virtual size_type get_volume()  override { return _volume; }

	};

}

#endif // output_plugin_alsa_h__
