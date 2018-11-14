#include <boost/dll/runtime_symbol_info.hpp>

#include "playlist_management_plugin.h"

#include "playlist.h"

namespace mprt
{

	void playlist_management_plugin::stop_internal()
	{

	}

	void playlist_management_plugin::pause_internal()
	{

	}

	void playlist_management_plugin::cont_internal()
	{

	}

	void playlist_management_plugin::quit_internal()
	{

	}

	playlist_management_plugin::playlist_management_plugin()
		: _playlist_id_counter(_INVALID_PLAYLIST_ID_)
	{
		_async_task = std::make_shared<mprt::async_tasker>(10);

		while (!_async_task->is_ready())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	playlist_management_plugin::~playlist_management_plugin()
	{

	}

	// Must be instantiated in plugin
	boost::filesystem::path playlist_management_plugin::location() const {
		return boost::dll::this_line_location(); // location of this plugin
	}

	std::string playlist_management_plugin::plugin_name() const {
		return "playlist_management_plugin";
	}

	plugin_types playlist_management_plugin::plugin_type() const {
		return plugin_types::playlist_management_plugin;
	}

	void playlist_management_plugin::init(void * arguments /*= nullptr*/)
	{

	}

	playlist_id_t playlist_management_plugin::add_playlist(std::string filename)
	{
		auto playlist_id = ++_playlist_id_counter;

		add_job([this, playlist_id, filename]
		{
			auto playlist(std::make_unique<playlist>());

			auto added_items = playlist->load_playlist(filename, false);

			call_callback_funcs(_add_playlist_cb_list, playlist->playlist_name(), playlist_id, added_items);
			_playlist_list[playlist_id] = std::move(playlist);
		});
		return playlist_id;
	}

	void playlist_management_plugin::register_add_playlist_callback(std::string plug_name, add_playlist_callback_t add_playlist_callback)
	{
		add_job([this, plug_name, add_playlist_callback]() {
			_add_playlist_cb_list[plug_name] = add_playlist_callback;
		});
	}

	void playlist_management_plugin::unregister_add_playlist_callback(std::string plug_name)
	{
		add_job([this, plug_name] {
			_add_playlist_cb_list.erase(plug_name);
		});
	}

	void playlist_management_plugin::move_playlist_item(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id)
	{
		add_job([this, playlist_id, from_playlist_item_id, to_playlist_item_id]
		{
			auto iter = _playlist_list.find(playlist_id);

			if ((iter != _playlist_list.end()) && (iter->second->move_item(from_playlist_item_id, to_playlist_item_id)))
			{
				call_callback_funcs(_move_playlist_item_cb_list, playlist_id, from_playlist_item_id, to_playlist_item_id);
			}
		});
	}


	void playlist_management_plugin::register_move_item_callback(std::string plug_name, move_playlist_item_callback_t move_playlist_callback)
	{
		add_job([this, plug_name, move_playlist_callback]
		{
			_move_playlist_item_cb_list[plug_name] = move_playlist_callback;
		});
	}

	void playlist_management_plugin::unregister_move_item_callback(std::string plug_name)
	{
		add_job([this, plug_name]
		{
			_move_playlist_item_cb_list.erase(plug_name);
		});
	}

}

// Factory method. Returns *simple pointer*!
refcounting_plugin_api * create() {
	return new mprt::playlist_management_plugin();
}

BOOST_DLL_ALIAS(create, create_refc_plugin)
