#ifndef plugins_loader_h__
#define plugins_loader_h__

#include <vector>

namespace mprt
{
	class decoder_plugin_api;
	class output_plugin_api;
	class input_plugin_api;
	class server_plugin_api;
	class ui_plugin_api;
	class playlist_management_plugin;

	class plugins_loader
	{
		std::vector<std::shared_ptr<server_plugin_api>> _server_plugins;
		std::vector<std::shared_ptr<ui_plugin_api>> _ui_plugins;
		std::shared_ptr<decoder_plugins_manager> _decoder_plugins_manager;
		std::shared_ptr<playlist_management_plugin> _playlist_management_plugin;

		void search_for_symbols(void *args);
		bool is_shared_library(const boost::filesystem::path& p);

	public:
		plugins_loader();
		~plugins_loader();

		void init(void *args);
	};

}

#endif // plugins_loader_h__
