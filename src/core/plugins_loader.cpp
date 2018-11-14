#include <memory>

#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/log/trivial.hpp>

#include "common/refcounting_plugin_api.h"

#include "common/decoder_plugin_api.h"
#include "common/output_plugin_api.h"
#include "common/input_plugin_api.h"
#include "common/ui_plugin_api.h"
#include "core/decoder_plugins_manager.h"
#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"

#include "plugins_loader.h"

namespace mprt
{
	plugins_loader::plugins_loader()
		: _decoder_plugins_manager(std::make_shared<decoder_plugins_manager>())
	{

	}

	plugins_loader::~plugins_loader()
	{

	}

	void plugins_loader::init(void *args)
	{
		search_for_symbols(args);
	}

	bool plugins_loader::is_shared_library(const boost::filesystem::path& p)
	{
		const std::string s = p.string();
		return (s.find(".dll") != std::string::npos || s.find(".so") != std::string::npos || s.find(".dylib") != std::string::npos)
			&& s.find(".lib") == std::string::npos
			&& s.find(".exp") == std::string::npos
			&& s.find(".pdb") == std::string::npos
			&& s.find(".manifest") == std::string::npos
			&& s.find(".rsp") == std::string::npos
			&& s.find(".obj") == std::string::npos
			&& s.find(".a") == std::string::npos;
	}

	void plugins_loader::search_for_symbols(void * args)
	{
		auto curr_path = boost::dll::program_location().parent_path();

#ifndef _NDEBUG
		//curr_path /= "Debug";
#else
		//curr_path /= "Release";
#endif

		auto decoder_plugins{ std::make_shared < std::vector<std::shared_ptr<decoder_plugin_api>>>() };
		auto output_plugins{ std::make_shared < std::vector< std::shared_ptr<output_plugin_api>>>() };
		auto input_plugins{ std::make_shared < std::vector<std::shared_ptr<input_plugin_api>>>() };

		for (auto & entry : boost::make_iterator_range(boost::filesystem::directory_iterator(curr_path), {}))
		{
			auto filename = entry.path().filename();
			if (is_shared_library(entry.path()))
			{
				try
				{
					BOOST_LOG_TRIVIAL(debug) << "trying: " << entry;
					auto plugin = get_plugin(entry.path(), "create_refc_plugin");

					switch (plugin->plugin_type())
					{
					case plugin_types::decoder_plugin:
					{
						auto dec_plug_api{ std::dynamic_pointer_cast<decoder_plugin_api>(plugin) };
						dec_plug_api->init(args);
						decoder_plugins->push_back(dec_plug_api);
					}
					break;

					case plugin_types::output_plugin:
					{
						auto out_plug_api{ std::dynamic_pointer_cast<output_plugin_api>(plugin) };
						out_plug_api->init();
						output_plugins->push_back(out_plug_api);
					}
					break;

					case plugin_types::input_plugin:
					{
						auto in_plug_api{ std::dynamic_pointer_cast<input_plugin_api>(plugin) };
						in_plug_api->init(args);
						input_plugins->push_back(in_plug_api);
					}
					break;

					case plugin_types::server_plugin:
						break;

					case plugin_types::ui_plugin:
					{
						auto ui_plug_api{ std::dynamic_pointer_cast<ui_plugin_api>(plugin) };
						_ui_plugins.push_back(ui_plug_api);
					}
					break;

					case plugin_types::playlist_management_plugin:
						_playlist_management_plugin = std::dynamic_pointer_cast<playlist_management_plugin>(plugin);
						_playlist_management_plugin->init(args);
						break;

					default:
						break;
					}
				}
				catch (std::exception const& e)
				{
					BOOST_LOG_TRIVIAL(debug) << "plugin load err: " << e.what();
				}
				catch (...)
				{
					BOOST_LOG_TRIVIAL(debug) << "plugin unknown load err";

				}
			}
		}



		_decoder_plugins_manager->add_decoder_plugins(decoder_plugins);
		_decoder_plugins_manager->add_output_plugins(output_plugins);

		_playlist_management_plugin->set_input_plugins(input_plugins);
		_playlist_management_plugin->set_decoder_plugins_manager(_decoder_plugins_manager);
		_playlist_management_plugin->set_output_plugins(output_plugins);

		for (auto ui_plu : _ui_plugins)
		{
			ui_plu->_playlist_management_plugin = _playlist_management_plugin;
			ui_plu->init(args);
			while (!ui_plu->is_init_ok())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		_playlist_management_plugin->add_playlist("../deneme.m3u8");
		_playlist_management_plugin->start_play(0, 0);
	}

}
