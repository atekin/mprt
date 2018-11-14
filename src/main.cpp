#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <thread>
#include <iostream>
#include <string>

#include <boost/log/trivial.hpp>

#include "core/config.h"
#include "common/job_type_enums.h"
#include "common/refcounting_plugin_api.h"
#include "common/decoder_plugin_api.h"
#include "common/input_plugin_api.h"
#include "common/output_plugin_api.h"
#include "common/ui_plugin_api.h"

#include "core/plugins_loader.h"

#include "core/decoder_plugins_manager.h"
#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"
#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin_imp.h"

using namespace mprt;

void init();
void init_logging();
void init_conf();
void init_plugins(int argc, char **argv);

void destroy_log();

int main(int argc, char **argv)
{
	try
	{
		init();

		init_conf();
		init_plugins(argc, argv);

		destroy_log();
	}
	catch (std::exception const& e)
	{
		BOOST_LOG_TRIVIAL(debug) << e.what();

	}


	return 0;
}

void init()
{
	// conf_init
	init_conf();

	// log init
	init_logging();
}

void init_conf()
{
	
}

void init_logging()
{

}

void destroy_log()
{
	// logging::core::get()->remove_all_sinks();
}

void init_plugins(int argc, char **argv)
{

	command_line_args cla;
	cla.argc = argc;
	cla.argv = argv;

	plugins_loader p;

	p.init(&cla);

	BOOST_LOG_TRIVIAL(debug) << "waiting ...";

	std::this_thread::sleep_for(std::chrono::hours(10 * 24));

	//std::this_thread::sleep_for(std::chrono::seconds(5));
}
