#ifndef ui_plugin_api_h__
#define ui_plugin_api_h__

#include <memory>
#include <atomic>

#include "common/refcounting_plugin_api.h"

namespace mprt
{
	class playlist_management_plugin;

	class ui_plugin_api : public refcounting_plugin_api
	{
	protected:
		std::atomic_bool _is_init_ok;

	public:
		ui_plugin_api()
			: _is_init_ok(false)
		{}

		virtual ~ui_plugin_api() {}

		std::shared_ptr<playlist_management_plugin> _playlist_management_plugin;

		bool is_init_ok() { return _is_init_ok; }
	};
}

#endif // ui_plugin_api_h__
