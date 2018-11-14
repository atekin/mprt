#ifndef decoder_plugin_api_h__
#define decoder_plugin_api_h__

#include <memory>
#include <queue>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <regex>

#include "type_defs.h"
#include "cache_buffer.h"
#include "input_plugin_api.h"
#include "utils.h"
#include "sound_plugin_api.h"
#include "output_plugin_api.h"

#include "core/decoder_plugins_manager.h"

namespace mprt {

	union float_int32_bytes {
		float fval;
		int32_t ival;
		unsigned char bytes[sizeof(float)];
	};

	class decoder_plugin_api : public refcounting_plugin_api, public sound_plugin_api {
	protected:
		using output_plugin_list = std::unordered_set<std::shared_ptr<output_plugin_api>>;
		
		play_finished_callback_register_func_t _play_finished_callback;
		output_plugin_list _output_plugin_list;

		decoder_plugins_manager *_decoder_plugins_manager;
		size_type _priority;
		std::vector<std::regex> _supported_extensions;

		size_type get_bit_count(size_type decoder_bits)
		{
			return 8 * (decoder_bits / 8 + (decoder_bits % 8 != 0));
		}

		virtual void seek_time_internal(url_id_t url_id, size_type byte_to_seek) {}
		virtual void init_decode_internal_single(url_id_t url_id) = 0;

		void push_func_call(shared_chunk_buffer_t & buf, float_int32_bytes sample, std::size_t sample_size)
		{
  			//for (std::size_t i = 0; i != sample_size; ++i)
  			//{
  			//	buf->second.push_back(sample.bytes[i]);
  			//}

			/*int index = 0;
			switch (sample_size)
			{
			case 4:
				index = 2;
				buf->second.push_back(sample.bytes[0]);
				buf->second.push_back(sample.bytes[1]);
			case 2:
				buf->second.push_back(sample.bytes[index + 0]);
				buf->second.push_back(sample.bytes[index + 1]);
				break;

			case 1:
				buf->second.push_back(sample.bytes[index + 0]);
			}*/

			buf->second.insert(buf->second.end(), sample.bytes, sample.bytes + sample_size);
		}

		void push_func_call_24(shared_chunk_buffer_t & buf, float_int32_bytes sample, std::size_t /*sample_size*/)
		{
			buf->second.push_back(0);
			buf->second.push_back(sample.ival);
			buf->second.push_back(sample.ival >> 8);
			buf->second.push_back(sample.ival >> 16);
		}


// 		void push_func_call_8(shared_chunk_buffer_type & buf, float_int32_bytes sample)
// 		{
// 			uninitialized_char c;
// 			c.m = sample.ival;
// 			buf->push_back(c);
// 		}
// 
// 		void push_func_call_16(shared_chunk_buffer_type & buf, float_int32_bytes sample)
// 		{
// 			uninitialized_char c;
// 			c.m = sample.ival;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 8;
// 			buf->push_back(c);
// 		}
// 
// 		void push_func_call_24(shared_chunk_buffer_type & buf, float_int32_bytes sample)
// 		{
// 			uninitialized_char c;
// 			c.m = 0;
// 			buf->push_back(c);
// 			c.m = sample.ival;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 8;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 16;
// 			buf->push_back(c);
// 		}
// 
// 		void push_func_call_32(shared_chunk_buffer_type & buf, float_int32_bytes sample)
// 		{
// 			uninitialized_char c;
// 			c.m = sample.ival;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 8;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 16;
// 			buf->push_back(c);
// 			c.m = sample.ival >> 24;
// 			buf->push_back(c);
// 		}
// 
// 		
// 		void push_func_call_float(shared_chunk_buffer_type & buf, float_int32_bytes sample)
// 		{
// 			uninitialized_char c;
// 			for (int i = 0; i != sizeof(float); ++i)
// 			{
// 				c.m =sample.bytes[i];
// 				buf->push_back(c);
// 			}
// 		}

		template <typename Decoders, typename Func>
		void play_finished_callback(Decoders & decoders, Func f, url_id_t url_id)
		{
			_decoder_plugins_manager->add_job([this, &decoders, f, url_id]
			{
				//BOOST_LOG_TRIVIAL(debug) << "sound called back " << plugin_name() << " clearing url_id: " << url_id;
				auto finished_dec = _decoder_plugins_manager->get_finished_decoder_details(url_id);

				if (!finished_dec)
				{
					_decoder_plugins_manager->finish_decode_internal_single(url_id);
					finished_dec = _decoder_plugins_manager->get_finished_decoder_details(url_id);
				}

				if (finished_dec && finished_dec->_sound_details._url_id == url_id)
				{
					finished_dec->_decoder_finish_callback(plugin_name(), url_id);

					give_cache_buf_back(url_id);

					decoders.put_cache_back(f, url_id);

					_decoder_plugins_manager->clear_finish_decoder(url_id);
				}
				else
				{
					BOOST_LOG_TRIVIAL(debug) << "There is an error here url_id: " << url_id;
				}
			});
		}

		template <typename T>
		bool init_decode_internal_single_internal(T & decoders, url_id_t url_id)
		{
			//_STATE_CHECK_(plugin_states::play);

			if (!decoders.is_in_cache(url_id))
			{
				init_api();
			}

			return decoders.is_in_cache(url_id);
		}

		
	public:
		decoder_plugin_api()
		{}

		virtual ~decoder_plugin_api()
		{}

		void add_output_plugin(std::shared_ptr<output_plugin_api> output_plugin)
		{
			_output_plugin_list.insert(output_plugin);

			// @TODO: later add the current playing item ... or someone else 
		}

		void remove_output_plugin(std::shared_ptr<output_plugin_api> output_plugin)
		{
			_output_plugin_list.erase(output_plugin);

			// @TODO: later remove the current playing item ... or someone else
		}

		virtual void decode() = 0;
		virtual void init_api() = 0;
		virtual void seek_duration(size_type duration_ms) = 0;

		void set_decoder_plugin_manager(decoder_plugins_manager * decoder_plug_man)
		{
			_decoder_plugins_manager = decoder_plug_man;
		}

		size_type priority()
		{
			return _priority;
		}

		bool match_extension(std::string const& url_ext)
		{
			for (auto & ext_pattern : _supported_extensions)
			{
				if (std::regex_match(url_ext, ext_pattern))
				{
					return true;
				}
			}

			return false;
		}


		friend class decoder_plugins_manager;
	};

}

#endif // decoder_plugin_api_h__
