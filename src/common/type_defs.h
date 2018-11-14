#ifndef type_defs_h__
#define type_defs_h__

#include <cstdint>
#include <memory>
#include <list>

#include <boost/circular_buffer.hpp>

#include "common_defs.h"
#include "producerconsumerqueue.h"
#include "cache_buffer.h"

namespace mprt
{
	struct command_line_args
	{
		int argc;
		char **argv;
	};

	constexpr static size_type _INVALID_URL_ID_ = -1;
	constexpr static size_type _INVALID_PLAYLIST_ID_ = -1;
	constexpr static size_type _INVALID_PLAYLIST_ITEM_ID_ = -1;

	struct current_decoder_details;
	struct sound_details;

	// input callbacks
	using set_input_cache_buf_callback_register_func_t = std::function<void (url_id_t, cache_buffer_shared)>;
	using decoder_finish_callback_register_func_t = std::function<void (std::string plugin_name, url_id_t)>;
	using seek_callback_register_func_t = std::function<void (url_id_t, size_type seek_point)>;
	using input_opened_register_func_t = std::function<void (std::shared_ptr<current_decoder_details>)>;

	// decoder callbacks
	using decoder_opened_callback_register_func_t = std::function<void(sound_details)>;
	using set_output_buffer_callback_register_func_t = std::function<void (url_id_t, cache_buffer_shared)>;
	using play_finished_callback_register_func_t = std::function<void (url_id_t)>;
	using decoder_seek_finished_callback_register_func_t = std::function<void ()>;

	// output callbacks
	using progress_callback_register_func_t = std::function<void (url_id_t, size_type current_position_ms)>;
	using output_opened_callback_register_func_t = std::function<void(bool output_opened)>;

	struct sound_details {
		bool _ok;
		size_type _total_samples;
		size_type _current_samples_written_to_sound_buffer;
		size_type _channels;
		size_type _sample_rate;
		bool _is_float;
		size_type _bps;
		size_type _orig_bps;
		url_id_t _url_id;
		cache_buffer_shared _current_cache_buffer;
		set_output_buffer_callback_register_func_t _decoder_set_output_buffer_callback;
		play_finished_callback_register_func_t _decoder_play_finished_callback;
		bool _stop;

		sound_details()
			: _ok(false)
			, _total_samples(-1)
			, _current_samples_written_to_sound_buffer(-1)
			, _channels(-1)
			, _sample_rate(-1)
			, _is_float(false)
			, _bps(-1)
			, _orig_bps(-1)
			, _url_id(_INVALID_URL_ID_)
			, _stop(false)
		{
		}
	};

	class decoder_plugin_api;
	struct current_decoder_details
	{
		std::string _url;
		std::string _url_ext;
		size_type _stream_length;
		size_type _current_stream_pos;
		size_type _current_samples_written;
		bool _last_read_empty;
		bool _seek_supported;
		bool _length_supported;
		bool _tell_supported;
		std::list<cache_buffer_shared> _output_buf_list;
		cache_buffer_shared _current_cache_buf;
		decoder_finish_callback_register_func_t _decoder_finish_callback;
		set_input_cache_buf_callback_register_func_t _set_input_cache_buf_callback;
		seek_callback_register_func_t _seek_callback;
		std::shared_ptr<decoder_plugin_api> _current_decoder_plugin;
		sound_details _sound_details;

		current_decoder_details()
			: _url("")
			, _url_ext("")
			, _stream_length(-1)
			, _current_stream_pos(-1)
			, _current_samples_written(-1)
			, _last_read_empty(true)
			, _seek_supported(false)
			, _length_supported(false)
			, _tell_supported(false)
			, _current_cache_buf(nullptr)
			, _sound_details()
		{}
	};


}


#endif // type_defs_h__
