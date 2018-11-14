#ifndef decoder_plugins_manager_h__
#define decoder_plugins_manager_h__

#include <vector>
#include <memory>
#include <list>
#include <set>

#include "../common/sound_plugin_api.h"
#include "../common/async_task.h"
#include "../common/type_defs.h"

namespace mprt
{
	class decoder_plugin_api;
	class output_plugin_api;

	struct comp_decode_plugin_api
	{
		bool operator()(std::shared_ptr<decoder_plugin_api> const & lhs, std::shared_ptr<decoder_plugin_api> const & rhs) const;
	};

	class decoder_plugins_manager : public async_task
	{
	private:
		using decoder_plugin_list = std::multiset<std::shared_ptr<decoder_plugin_api>, comp_decode_plugin_api>;
		decoder_plugin_list _decoder_plugin_list;

		using decoder_det_list_t = std::list<std::shared_ptr<current_decoder_details>>;
		using finished_decoder_list_t = std::unordered_map<url_id_t, std::shared_ptr<current_decoder_details>>;

		set_output_buffer_callback_register_func_t _set_output_buf_callback;
		decoder_opened_callback_register_func_t _decoder_opened_cb_func;
		decoder_det_list_t _decoder_detail_list;
		finished_decoder_list_t _finished_decoder_detail_list;
		decoder_seek_finished_callback_register_func_t _decoder_seek_finished_cb;

		chunk_buffer_type::second_type _last_read_buffer;

		async_tasker::timer_type_shared _decode_timer;

		bool is_current_decoder_details_empty()
		{
			return is_gen_decoder_details_empty(_decoder_detail_list);
		}

		void set_cache_buffer(url_id_t url_id, cache_buffer_shared cache_buf)
		{
			for (auto dec_detail : _decoder_detail_list)
			{
				if (url_id == dec_detail->_sound_details._url_id) {
					dec_detail->_output_buf_list.push_back(cache_buf);
					break;
				}
			}
		}

		std::shared_ptr<decoder_plugin_api> get_decoder_plugin(std::string const& url_ext);

		virtual void stop_internal() override;
		virtual void pause_internal() override;
		virtual void cont_internal() override;
		virtual void quit_internal() override;

	public:
		decoder_plugins_manager();
		~decoder_plugins_manager();

		std::shared_ptr<current_decoder_details> & get_gen_decoder_details_ref(decoder_det_list_t & contain)
		{
			return contain.front();
		}

		std::shared_ptr<current_decoder_details> const& get_gen_decoder_details(decoder_det_list_t & contain)
		{
			return contain.front();
		}

		void get_gen_decoder_details_pop(decoder_det_list_t & contain)
		{
			contain.pop_front();
		}

		void push_back_gen_decoder_details(decoder_det_list_t & contain, std::shared_ptr<current_decoder_details> const& dec_dets)
		{
			contain.push_back(dec_dets);
		}

		void push_front_gen_decoder_details(decoder_det_list_t & contain, std::shared_ptr<current_decoder_details> const& dec_dets)
		{
			contain.push_front(dec_dets);
		}

		bool is_gen_decoder_details_empty(decoder_det_list_t & contain)
		{
			return contain.empty();
		}

		std::shared_ptr<current_decoder_details> & get_current_decoder_details_ref()
		{
			return get_gen_decoder_details_ref(_decoder_detail_list);
		}

		std::shared_ptr<current_decoder_details> const& get_current_decoder_details()
		{
			return get_gen_decoder_details(_decoder_detail_list);
		}

		void get_current_decoder_details_pop()
		{
			get_gen_decoder_details_pop(_decoder_detail_list);
		}

		void push_back_current_decoder_details(std::shared_ptr<current_decoder_details> const& dec_dets)
		{
			push_back_gen_decoder_details(_decoder_detail_list, dec_dets);
		}

		void push_front_current_decoder_details(std::shared_ptr<current_decoder_details> const& dec_dets)
		{
			push_front_gen_decoder_details(_decoder_detail_list, dec_dets);
		}

		void add_output_plugin(std::shared_ptr<output_plugin_api> & output_plugin);
		void add_output_plugins(std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> & output_plugins);
		void remove_output_plugin(std::shared_ptr<output_plugin_api> output_plugin);

		bool is_no_job()
		{
			return
				is_current_decoder_details_empty();
		}

		std::shared_ptr<current_decoder_details> get_finished_decoder_details(url_id_t url_id)
		{
			auto iter = _finished_decoder_detail_list.find(url_id);
			if (iter != _finished_decoder_detail_list.end())
			{
				return iter->second;
			}

			return std::shared_ptr<current_decoder_details>();
		}

		std::shared_ptr<current_decoder_details> get_current_dec_details(url_id_t url_id)
		{
			
			auto
				iter = _decoder_detail_list.begin(),
				iter_end = _decoder_detail_list.end();

			for (; iter != iter_end; ++iter)
			{
				if ((*iter)->_sound_details._url_id == url_id)
				{
					return *iter;
				}
			}

			return std::shared_ptr<current_decoder_details>();
		}

		void decode_cont();

		void add_input_url_job(std::shared_ptr<current_decoder_details> const& decoder_dets)
		{
			add_job([this, decoder_dets]()
			{
				add_input_url_job_internal(decoder_dets);
			});
		}

		void add_input_url_job_internal(std::shared_ptr<current_decoder_details> const& decoder_dets);

		bool update_dec_detail_seek(std::shared_ptr<current_decoder_details> dec_det, size_type seek_point);
		bool seek_byte_internal(url_id_t url_id, size_type seek_point);
		void seek_duration(url_id_t url_id, size_type seek_duration_ms);
		std::pair<size_type, url_id_t> decoder_read_buffer(buffer_elem_t *buffer, size_type buf_size);

		void finish_decode_internal_single(url_id_t url_id);

		size_type decoder_tell()
		{
			auto const& cur_decoder_det = get_current_decoder_details();

			return cur_decoder_det->_tell_supported ? cur_decoder_det->_current_stream_pos : -1;
		}

		size_type decoder_length()
		{
			auto const& cur_decoder_det = get_current_decoder_details();

			return cur_decoder_det->_length_supported ? cur_decoder_det->_stream_length : -1;
		}

		bool decoder_eof()
		{
			auto const& cur_decoder_det = get_current_decoder_details();

			return cur_decoder_det->_current_stream_pos == cur_decoder_det->_stream_length;
		}

		bool decoder_is_exhausted()
		{
			auto const& cur_decoder_det = get_current_decoder_details();

			return cur_decoder_det->_current_stream_pos == cur_decoder_det->_stream_length;
		}

		bool decoder_is_last_read_empty()
		{
			auto const& cur_decoder_det = get_current_decoder_details();

			return cur_decoder_det->_last_read_empty;
		}

		set_output_buffer_callback_register_func_t set_output_buf_callback();

		void add_decoder_plugin(std::shared_ptr<decoder_plugin_api> & dec_plugin);
		void add_decoder_plugins(std::shared_ptr<std::vector<std::shared_ptr<decoder_plugin_api>>> & dec_plugins);
		void clear_finish_decoder(url_id_t url_id);

		void set_decoder_opened_cb(decoder_opened_callback_register_func_t func)
		{
			_decoder_opened_cb_func = func;
		}

		void decoder_opened(sound_details sound_det)
		{
			_decoder_opened_cb_func(sound_det);
		}

		void set_decoder_seek_finish_cb(decoder_seek_finished_callback_register_func_t func)
		{
			_decoder_seek_finished_cb = func;
		}
	};
}


#endif // decoder_plugins_manager_h__
