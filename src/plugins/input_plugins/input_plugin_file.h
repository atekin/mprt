#ifndef input_plugin_file_h__
#define input_plugin_file_h__

#include <functional>
#include <thread>
#include <iosfwd>
#include <list>
#include <utility>
#include <memory>
#include <iosfwd>
#include <unordered_set>

#include <boost/circular_buffer_fwd.hpp>
#include <boost/optional.hpp>

#include "common/input_plugin_api.h"

namespace mprt {
	class output_plugin_api;

	struct file_details {
		std::string _filename;
		size_type _url_id;
		std::shared_ptr<std::ifstream> _file;
		size_type _file_size;
		size_type _current_read_so_far;
		cache_buffer_shared _cache_buf;

		file_details(
			std::string &filename,
			size_type url_id,
			std::shared_ptr<std::ifstream> file = std::make_shared<std::ifstream>(),
			size_type file_size = 0,
			cache_buffer_shared cache_buf = nullptr)
			: _filename(filename)
			, _url_id(url_id)
			, _file(file)
			, _file_size(file_size)
			, _current_read_so_far(0)
			, _cache_buf(cache_buf)
		{}
	};

	class input_plugin_file : public input_plugin_api
	{
	private:
		using file_list_t = std::list<std::shared_ptr<file_details>>;
		file_list_t _files;
		file_list_t _finished_files;
		std::stack<bool> _can_cont_decode;
		size_type _max_file_chunk_size;
		decoder_finish_callback_register_func_t _decoder_finish_callback;
		set_input_cache_buf_callback_register_func_t _set_input_cache_buf_callback;
		seek_callback_register_func_t _seek_callback;
		size_type _max_finish_files;

		bool add_file(std::string filename, url_id_t url_id);
		void remove_file(url_id_t url_id);
		bool open_file(file_list_t::iterator& file_iter);
		void close_file(file_list_t::iterator& file_iter);
		void try_read();
		void input_close(url_id_t url_id);
		
		bool is_no_job() override;
		virtual void stop_internal() override;
		virtual void pause_internal() override;
		virtual void cont_internal() override;
		virtual void quit_internal() override;

		bool remove_file_helper(file_list_t & cnt, url_id_t url_id);
		
		file_list_t::iterator seek_helper(file_list_t & file_cont, url_id_t url_id, size_type seek_point);
		void seek_callback_job(url_id_t url_id, size_type seek_point);
		void seek_callback_internal(url_id_t url_id, size_type seek_point);

	public:
		input_plugin_file();
		virtual ~input_plugin_file();

		// ref plugin functions
		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void * arguments = nullptr) override;

		// job functions
		virtual url_id_t add_input_item(std::string filename) override;
		virtual url_id_t add_input_item(std::string filename, url_id_t url_id) override;
		virtual void remove_input_item(url_id_t url_id) override;
		void set_cache_buffer(url_id_t url_id, cache_buffer_shared cache_buf) override;
		
		// decoder thread functions
		virtual void decoder_finish(std::string plugin_name, url_id_t url_id) override;

	};
}

#endif // input_plugin_file_h__
