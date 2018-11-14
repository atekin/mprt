#ifndef sound_plugin_api_h__
#define sound_plugin_api_h__

#include <boost/log/trivial.hpp>

#include "cache_manage.h"

namespace mprt
{
	class sound_plugin_api
	{
	private:
		cache_manage<cache_buffer_shared> _cache_buf_mgr;

		cache_buffer_shared get_new_cache_buffer_shared()
		{
			return std::make_shared<cache_buffer_shared::element_type>(
				_max_memory_size_per_file, _max_chunk_read_size);
		}

		cache_buffer_shared get_new_cache_buffer_shared_size(size_type buffer_size)
		{
			return std::make_shared<cache_buffer_shared::element_type>(
				buffer_size, _max_chunk_read_size);
		}

		void clear_cache_buf(cache_buffer_shared cache_buf)
		{
			cache_buf->clear_data();
		}

	protected:
		size_type _max_memory_size_per_file;
		size_type _max_chunk_read_size;

		cache_buffer_shared get_cache_put_buf(url_id_t url_id)
		{
			return _cache_buf_mgr.get_from_cache(std::bind(&sound_plugin_api::get_new_cache_buffer_shared, this), url_id);
		}

		cache_buffer_shared get_cache_put_buf(url_id_t url_id, size_type buffer_size)
		{
			return _cache_buf_mgr.get_from_cache(std::bind(&sound_plugin_api::get_new_cache_buffer_shared_size, this, buffer_size), url_id);
		}

		void give_cache_buf_back(url_id_t url_id)
		{
			return _cache_buf_mgr.put_cache_back(std::bind(&sound_plugin_api::clear_cache_buf, this, std::placeholders::_1), url_id);
		}



	public:
		sound_plugin_api()
			: _cache_buf_mgr(0)
		{}

		virtual ~sound_plugin_api()
		{
			BOOST_LOG_TRIVIAL(debug) << "sound_plugin_api::~sound_plugin_api() called";
		}

		virtual void reset_buffers()
		{
			_cache_buf_mgr.reset();
		}

		static size_type samples_to_bytes(size_type samples, sound_details const& sound_dets)
		{

			return (samples <= 0) ? 0 : (samples * (sound_dets._channels * sound_dets._bps / 8));
		}

		static size_type bytes_to_samples(size_type bytes, sound_details const& sound_dets)
		{
			return (bytes <= 0) ? 0 : (bytes / (sound_dets._channels * sound_dets._bps / 8));
		}

		static std::chrono::microseconds bytes_to_time_duration(size_type bytes, sound_details const& sound_dets)
		{
			return std::chrono::microseconds(std::llround(
				1e6 * static_cast<double>(bytes) /
				static_cast<double>(sound_dets._sample_rate * sound_dets._channels * sound_dets._bps / 8)));
		}

		static size_type time_duration_to_bytes(std::chrono::microseconds dur, sound_details const& sound_dets)
		{
			return std::llround(static_cast<double>(dur.count() * sound_dets._sample_rate * sound_dets._channels * sound_dets._bps / 8) / 1e6);
		}

		static std::chrono::microseconds samples_to_time_duration(size_type samples, sound_details const& sound_dets)
		{
			return std::chrono::microseconds(std::llround(
				1e6 * static_cast<double>(samples) /
				static_cast<double>(sound_dets._sample_rate)));
		}

		static size_type time_duration_to_samples(std::chrono::microseconds dur, sound_details const& sound_dets)
		{
			return std::llround(static_cast<double>(dur.count() * sound_dets._sample_rate) / 1e6);
		}
	};

}

#endif // sound_plugin_api_h__
