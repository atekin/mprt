#ifndef input_plugin_api_h__
#define input_plugin_api_h__

#include <cstdint>
#include <utility>
#include <memory>
#include <chrono>
#include <unordered_map>

#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>

#include "type_defs.h"
#include "cache_buffer.h"
#include "refcounting_plugin_api.h"

#include "async_task.h"

namespace mprt {
	class decoder_plugin_api;

	class input_plugin_api : public refcounting_plugin_api, public async_task
	{
	protected:
		std::atomic<size_type> _url_id;
		async_tasker::timer_type_shared _file_read_timer;
		input_opened_register_func_t _input_opened_register_func;

		virtual bool is_no_job() = 0;

	public:

		input_plugin_api()
			: _url_id(0)
		{
		}

		virtual ~input_plugin_api()
		{
			BOOST_LOG_TRIVIAL(debug) << "input_plugin_api::~input_plugin_api() called";
		}

		void set_input_opened_cb(input_opened_register_func_t func) { _input_opened_register_func = func; }

		// job functions
		virtual url_id_t add_input_item(std::string filename) = 0;
		virtual url_id_t add_input_item(std::string filename, url_id_t url_id) = 0;
		virtual void remove_input_item(url_id_t url_id) = 0;

		virtual void decoder_finish(std::string plugin_name, url_id_t) = 0;
		virtual void set_cache_buffer(url_id_t url_id, cache_buffer_shared cache_buf) = 0;

	};
}

#endif // input_plugin_api_h__
