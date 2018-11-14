
#include <unordered_map>
#include <memory>
#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include "common/refcounting_plugin_api.h"

#include <QMainWindow>
#include <QApplication>

#include "ui_plugin_qt.h"

#include "mainwindow.h"

namespace mprt_ui_plugin_qt
{

	ui_plugin_qt::ui_plugin_qt()
	{
		_async_task = std::make_shared<mprt::async_tasker>(1);
	}

	ui_plugin_qt::~ui_plugin_qt()
	{

	}

	// Must be instantiated in plugin
	boost::filesystem::path ui_plugin_qt::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string ui_plugin_qt::plugin_name() const {
		return "ui_plugin_qt";
	}

	plugin_types ui_plugin_qt::plugin_type() const {
		return plugin_types::ui_plugin;
	}

	void ui_plugin_qt::init(void *arguments)
	{
		while (!_async_task->is_ready())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		add_job([this, arguments]()
		{
			init_qt(arguments);
		});
	}

	void ui_plugin_qt::stop_internal()
	{

	}

	void ui_plugin_qt::pause_internal()
	{

	}

	void ui_plugin_qt::cont_internal()
	{

	}

	void ui_plugin_qt::quit_internal()
	{

	}

	void ui_plugin_qt::init_qt(void *arguments)
	{
		int argc = 1;
		char **argv = nullptr; 
		if (arguments)
		{
			auto args = reinterpret_cast<mprt::command_line_args*>(arguments);
			argc = args->argc;
			argv = args->argv;
		}

		QApplication app(argc, argv);

		main_window m;
		m.set_ui_plugin_qt(this);
		m.show();

		_is_init_ok = true;
		app.exec();
	}

}

// Factory method. Returns *simple pointer
std::unique_ptr<refcounting_plugin_api> create()
{
	return std::make_unique<mprt_ui_plugin_qt::ui_plugin_qt>();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)
