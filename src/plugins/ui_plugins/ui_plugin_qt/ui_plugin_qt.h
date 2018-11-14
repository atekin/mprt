#ifndef ui_plugin_qt_h__
#define ui_plugin_qt_h__

#include <unordered_map>
#include <memory>
#include <string>

#include <boost/filesystem/path.hpp>

#include "common/refcounting_plugin_api.h"
#include "common/ui_plugin_api.h"
#include "common/async_task.h"

Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(std::unordered_map)
Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

namespace mprt_ui_plugin_qt
{
	class ui_plugin_qt : public mprt::ui_plugin_api, public mprt::async_task
	{
		virtual void stop_internal() override;
		virtual void pause_internal() override;
		virtual void cont_internal() override;
		virtual void quit_internal() override;

		void init_qt(void * arguments);

	public:
		ui_plugin_qt();
		~ui_plugin_qt();

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void * arguments = nullptr) override;
	};

}

#endif // ui_plugin_qt_h__
