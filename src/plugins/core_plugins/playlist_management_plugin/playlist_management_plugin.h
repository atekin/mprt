#ifndef playlist_management_plugin_h__
#define playlist_management_plugin_h__

#include <vector>
#include <memory>

#include "common/refcounting_plugin_api.h"
#include "common/async_task.h"

#include "playlist_item.h"

namespace mprt
{
	class input_plugin_api;
	class output_plugin_api;
	class decoder_plugins_manager;
	class album_art;

	// general callbacks ...
	using add_playlist_callback_t = std::function<void(std::string, playlist_id_t, std::shared_ptr<playlist_item_shr_list_t> items)>;
	using add_playlist_item_callback_t = std::function<void(playlist_id_t, playlist_item_id_t from_playlist_item_id, std::shared_ptr<playlist_item_shr_list_t> items)>;
	using remove_playlist_callback_t = std::function<void(playlist_id_t)>;
	using move_playlist_item_callback_t = std::function<void(playlist_id_t, playlist_item_id_t from_where, playlist_item_id_t to_where)>;
	using move_playlist_item_list_callback_t = std::function<void(playlist_id_t, std::vector<playlist_item_id_t> const& items, playlist_item_id_t to_where)>;
	using remove_playlist_item_callback_t = std::function<void(playlist_id_t, playlist_item_id_t)>;
	using start_playlist_item_callback_t = std::function<void(playlist_id_t, playlist_item_id_t playlist_item, size_type duration_ms)>;
	using progress_playlist_item_callback_t = std::function<void(playlist_id_t, playlist_item_id_t playlist_item, size_type position_ms)>;
	using rename_playlist_callback_t = std::function<void(playlist_id_t, std::string playlist_name)>;
	using seek_finished_callback_t = std::function<void()>;

	class playlist_management_plugin : public refcounting_plugin_api, public async_task
	{
	protected:
		std::shared_ptr<std::vector<std::shared_ptr<input_plugin_api>>> _input_plugins;
		std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> _output_plugins;
		std::shared_ptr<decoder_plugins_manager> _decoder_plugins_manager;
		std::shared_ptr<album_art> _album_art;

		progress_callback_register_func_t _progress_callback;

		std::shared_ptr<tag_parser_abstract> _tag_parser;

		std::chrono::milliseconds _min_wait_next_song;

		virtual mprt::playlist_item_id_t get_next_playlist_item_id() = 0;

	public:
		//playlist_management_plugin() {}
		virtual ~playlist_management_plugin() {}

		virtual void start_play(playlist_id_t playlist_id, playlist_item_id_t playlist_item) = 0;
		virtual void stop_play() = 0;
		virtual void seek_duration(playlist_id_t playlist_id, playlist_item_id_t playlist_item, size_type duration_ms) = 0;

		virtual void set_input_plugins(std::shared_ptr<std::vector<std::shared_ptr<input_plugin_api>>> input_plugins) = 0;
		virtual void set_output_plugins(std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> output_plugins) { _output_plugins = output_plugins; }
		virtual void set_decoder_plugins_manager(std::shared_ptr<decoder_plugins_manager > decoder_plugins_manager) = 0;

		virtual void add_playlist(std::string filename) = 0;
		virtual void add_playlist(std::string filename, std::shared_ptr<std::vector<std::string>> url_names) = 0;
		virtual void add_playlist(std::shared_ptr<std::vector<std::string>> url_names) = 0;
		virtual void register_add_playlist_callback(std::string plug_name, add_playlist_callback_t add_playlist_callback) = 0;
		virtual void unregister_add_playlist_callback(std::string plug_name) = 0;
		
		virtual void add_playlist_items(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::shared_ptr<std::vector<std::string>> urls) = 0;
		virtual void register_add_playlist_item_callback(std::string plug_name, add_playlist_item_callback_t add_playlist_item_callback) = 0;
		virtual void unregister_add_playlist_item_callback(std::string plug_name) = 0;
		
		virtual void save_playlist(playlist_id_t playlist_id, std::string playlist_name) = 0;		
		
		virtual void rename_playlist(playlist_id_t playlist_id, std::string playlist_name) = 0;
		virtual void register_rename_playlist_callback(std::string plug_name, rename_playlist_callback_t rename_playlist_item_callback) = 0;
		virtual void unregister_rename_playlist_callback(std::string plug_name) = 0;

		virtual void remove_playlist_item(size_type playlist_id, size_type playlist_item_id) = 0;
		virtual void register_remove_item_callback(std::string plug_name, remove_playlist_item_callback_t remove_playlist_item_callback) = 0;
		virtual void unregister_remove_item_callback(std::string plug_name) = 0;

		virtual void move_playlist_item(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id) = 0;
		virtual void register_move_item_callback(std::string plug_name, move_playlist_item_callback_t move_playlist_item_callback) = 0;
		virtual void unregister_move_item_callback(std::string plug_name) = 0;

		virtual void register_start_playlist_item_callback(std::string plug_name, start_playlist_item_callback_t start_playlist_item_callback) = 0;
		virtual void unregister_start_playlist_item_callback(std::string plug_name) = 0;

		virtual void register_progress_playlist_item_callback(std::string plug_name, progress_playlist_item_callback_t progress_playlist_item_callback) = 0;
		virtual void unregister_progress_playlist_item_callback(std::string plug_name) = 0;

		virtual void remove_playlist(playlist_id_t playlist_id) = 0;
		virtual void register_remove_playlist_callback(std::string plug_name, remove_playlist_callback_t remove_playlist_callback) = 0;
		virtual void unregister_remove_playlist_callback(std::string plug_name) = 0;

		virtual void register_seek_finished_callback(std::string plug_name, seek_finished_callback_t seek_finished_callback) = 0;
		virtual void unregister_seek_finished_callback(std::string plug_name) = 0;

		virtual album_art_list const& get_album_art(album_name_t const& album_name) = 0;
	};

}

#endif // playlist_management_plugin_h__
