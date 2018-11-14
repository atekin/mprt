#ifndef output_plugin_h__
#define output_plugin_h__

#include <utility>
#include <memory>
#include <unordered_map>
#include <chrono>

#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>

#include "type_defs.h"
#include "cache_buffer.h"
#include "utils.h"
#include "sound_plugin_api.h"
#include "async_task.h"

namespace mprt {

	class decoder_plugin_api;

	class output_plugin_api : public refcounting_plugin_api, public sound_plugin_api, public async_task
	{
	protected:
		std::list<sound_details> _sound_details_queue;
		sound_details _prev_sound_details;
		bool _init_api;
		std::unordered_map<std::string, progress_callback_register_func_t> _progress_func_call_list;
		bool _use_duration;
		async_tasker::timer_type_shared _play_timer;

		virtual void play() = 0;
		virtual void pause_play_internal() = 0;
		virtual void resume_play_internal() = 0;
		virtual void resume_clear_play_internal() = 0;
		virtual void fill_drain_internal() = 0;
		virtual void init_api() = 0;

		virtual bool is_no_job()
		{
			if (_sound_details_queue.empty())
				return true;

			auto const& current_sound_dets = sound_details_top();
			return 
				(_sound_details_queue.size() == 1) && 
				(current_sound_dets._total_samples == current_sound_dets._current_samples_written_to_sound_buffer);
		}

		void add_sound_details_internal(sound_details sound_dets)
		{
			BOOST_LOG_TRIVIAL(debug) << "setting cache for: " << sound_dets._url_id << " name: " << plugin_name();
			auto cache_buf = _use_duration ?
				get_cache_put_buf(sound_dets._url_id, time_duration_to_bytes(std::chrono::milliseconds(_max_memory_size_per_file), sound_dets)) :
				get_cache_put_buf(sound_dets._url_id, _max_memory_size_per_file);
			sound_dets._current_cache_buffer = cache_buf;
			_sound_details_queue.push_back(sound_dets);

			sound_dets._decoder_set_output_buffer_callback(sound_dets._url_id, cache_buf);
		}

		void remove_sound_details_internal(sound_details const& sound_dets)
		{
			// @TODO: to be implemented
		}

		void update_total_samples_internal(url_id_t url_id, size_type total_samples)
		{
			for (auto & sound_dets : _sound_details_queue) {
				if (sound_dets._url_id == url_id) {
					sound_dets._total_samples = total_samples;
					break;
				}
			}
		}

		double get_soft_vol_gain(int vol_pos)
		{
			// vol pos should be 0 and 200, we accept that 0 is -100db, 200 is 0db, we allow 0.5db x_rements
			return std::pow(10.0, (-100. + bound_val(0, vol_pos, 200) * 0.5) / 20.);
		}

		uint32_t get_soft_vol_val (int vol_pos, uint32_t sample)
		{
			// vol pos should be 0 and 200, we accept that 0 is -100db, 200 is 0db, we allow 0.5db x_rements
			return static_cast<uint32_t>(std::llround(sample * get_soft_vol_gain(vol_pos)));
		}

		virtual void cont_internal() override
		{
			BOOST_LOG_TRIVIAL(trace) << "output_plugin_api::cont_internal() called";

			if (_sound_details_queue.empty()) {

				add_job([this]()
				{
					BOOST_LOG_TRIVIAL(trace) << "sound waiting for decoder to be ready";

					cont_internal();
				}, std::chrono::milliseconds(100));

				return;
			}

			if (!_init_api)
			{
				init_api();
			}

			resume_play();
		}

		void stop_internal() override
		{
			if (_prev_sound_details._current_cache_buffer)
			{
				_prev_sound_details._current_cache_buffer->clear_data();
			}
			_prev_sound_details = sound_details();
			clear_queue(_sound_details_queue);
		}
		
		void clear_play_data_internal(url_id_t url_id)
		{
			for (auto sound_det : _sound_details_queue)
			{
				if (url_id == sound_det._url_id && sound_det._current_cache_buffer)
				{
					sound_det._current_cache_buffer->clear_data();
					return;
				}
			}
		}

		void set_seek_duration_internal(url_id_t url_id, size_type duration_ms)
		{
			for (auto & sound_det : _sound_details_queue)
			{
				if (url_id == sound_det._url_id)
				{
					sound_det._current_samples_written_to_sound_buffer =
						time_duration_to_samples(std::chrono::microseconds(duration_ms * 1000), sound_det);
					return;
				}
			}
		}

	public:
		output_plugin_api()
			: _init_api(false)
		{}

		virtual ~output_plugin_api() {
			BOOST_LOG_TRIVIAL(debug) << "output_plugin_api::~output_plugin_api() called";
		}

		sound_details const& sound_details_top()
		{
			return _sound_details_queue.front();
		}

		sound_details & sound_details_top_ref()
		{
			return _sound_details_queue.front();
		}

		void sound_details_pop()
		{
			_sound_details_queue.pop_front();
		}

		bool is_sound_details_same_as_before()
		{
			if (_sound_details_queue.empty()) {
				return false;
			}

			auto const& sound_dets = sound_details_top();
			return
				_prev_sound_details._bps == sound_dets._bps &&
				_prev_sound_details._channels == sound_dets._channels &&
				_prev_sound_details._sample_rate == sound_dets._sample_rate;
		}

		// job thread functions
		virtual void set_volume(size_type volume) = 0; // 0 to 100
		virtual size_type get_volume() = 0; // 0 to 100

		virtual void reset_buffers() override
		{
			add_job([this] {
				sound_plugin_api::reset_buffers();

				clear_queue(_sound_details_queue);
			});
		}

		void update_total_samples(url_id_t url_id, size_type total_samples)
		{
			add_job([=] {
				update_total_samples_internal(url_id, total_samples);
			});
		}

		void add_sound_details(sound_details sound_dets)
		{
			add_job([=] {
				add_sound_details_internal(sound_dets);
			});
		}

		void remove_sound_details(sound_details const& sound_dets)
		{
			add_job([=] {
				remove_sound_details(sound_dets);
			});
		}

		void clear_play_data(url_id_t url_id)
		{
			add_job([=] {
				clear_play_data_internal(url_id);
			});
		}

		void add_progress_callback(progress_callback_register_func_t progress_cb_func, std::string plug_name)
		{
			add_job([this, plug_name, progress_cb_func]()
			{
				_progress_func_call_list[plug_name] = progress_cb_func;
			});
		}

		void remove_progress_callback(std::string plug_name)
		{
			add_job([this, plug_name]()
			{
				_progress_func_call_list.erase(plug_name);
			});
		}

		void set_seek_duration_ms(url_id_t url_id, size_type duration_ms)
		{
			add_job([this, url_id, duration_ms]
			{
				set_seek_duration_internal(url_id, duration_ms);
			});
		}

		void pause_play()
		{
			add_job([this]
			{
				pause_play_internal();
			});
		}

		void resume_play()
		{
			add_job([this]
			{
				resume_play_internal();
			});
		}

		void resume_clear_play()
		{
			add_job([this]
			{
				resume_clear_play_internal();
			});
		}

		void fill_drain()
		{
			add_job([this]()
			{
				fill_drain_internal();
			});
		}

	};

}

#endif // output_plugin_h__
