#ifndef playlist_management_plugin_imp_h__
#define playlist_management_plugin_imp_h__

#include <unordered_map>

#include "common/refcounting_plugin_api.h"
#include "common/async_task.h"

#include "playlist_management_plugin.h"
#include "playlist_item.h"

namespace mprt
{

	class input_plugin_api;
	class decoder_plugin_api;
	class decoder_plugins_manager;
	class output_plugin_api;
	class ui_plugin_api;

	class playlist;

	class playlist_management_plugin_imp : public playlist_management_plugin
	{
	private:
		std::unordered_map<playlist_id_t, std::unique_ptr<playlist>> _playlist_list;
		size_type _playlist_id_counter;
		
		playlist_id_t _current_playing_list_id;
		playlist_item_id_t _current_playing_item_id;
		bool _added_next_song;

		std::unordered_map<std::string, add_playlist_callback_t> _add_playlist_cb_list;
		std::unordered_map<std::string, add_playlist_item_callback_t> _add_playlist_item_cb_list;
		std::unordered_map<std::string, remove_playlist_callback_t> _remove_playlist_cb_list;
		std::unordered_map<std::string, move_playlist_item_callback_t> _move_playlist_item_cb_list;
		std::unordered_map<std::string, move_playlist_item_list_callback_t> _move_playlist_item_list_cb_list;
		std::unordered_map<std::string, remove_playlist_item_callback_t> _remove_playlist_item_cb_list;
		std::unordered_map<std::string, start_playlist_item_callback_t> _start_playlist_item_cb_list;
		std::unordered_map<std::string, progress_playlist_item_callback_t> _progress_playlist_item_cb_list;
		std::unordered_map<std::string, rename_playlist_callback_t> _rename_playlist_item_cb_list;
		std::unordered_map<std::string, seek_finished_callback_t> _seek_finished_cb_list;

		virtual void stop_internal() override;
		virtual void pause_internal() override;
		virtual void cont_internal() override;
		virtual void quit_internal() override;

		virtual void set_input_plugins(std::shared_ptr<std::vector<std::shared_ptr<input_plugin_api>>> input_plugins) override;
		virtual void set_decoder_plugins_manager(std::shared_ptr<decoder_plugins_manager > decoder_plugins_manager) override;

		void start_play_internal(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id);
		void stop_play_internal();
		void cont_play_internal(playlist_item_id_t playlist_item_id);
		void seek_duration_internal(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id, size_type duration_ms);

		void progress_callback(url_id_t url_id, size_type current_position_ms);
		virtual void add_playlist_items_internal(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::shared_ptr<std::vector<std::string>> urls);
		virtual void add_playlist_directory_internal(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::string dirname);
		virtual std::shared_ptr<std::vector<std::string>> add_playlist_directory_builder(std::string dirname);

		void input_opened_cb(std::shared_ptr<current_decoder_details> cur_det);
		void decoder_opened_cb(sound_details sound_det);
		void decoder_seek_finished_cb();
		void sound_opened_cb(bool sound_opened);

	protected:
		virtual void set_output_plugins(std::shared_ptr<std::vector<std::shared_ptr<output_plugin_api>>> output_plugins) override;
		virtual mprt::playlist_item_id_t get_next_playlist_item_id() override;

	public:
		playlist_management_plugin_imp();
		virtual ~playlist_management_plugin_imp();

		virtual boost::filesystem::path location() const override;
		virtual std::string plugin_name() const override;
		virtual plugin_types plugin_type() const override;
		virtual void init(void *) override;

		virtual void start_play(playlist_id_t playlist_id, playlist_item_id_t playlist_item) override;
		virtual void stop_play() override;
		virtual void seek_duration(playlist_id_t playlist_id, playlist_item_id_t playlist_item, size_type duration_ms) override;

		virtual void add_playlist(std::string filename) override;
		virtual void add_playlist(std::string filename, std::shared_ptr<std::vector<std::string>> url_names) override;
		virtual void add_playlist(std::shared_ptr<std::vector<std::string>> url_names) override;
		virtual void register_add_playlist_callback(std::string plug_name, add_playlist_callback_t add_playlist_callback) override;
		virtual void unregister_add_playlist_callback(std::string plug_name) override;
		
		virtual void add_playlist_items(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::shared_ptr<std::vector<std::string>> urls) override;
		virtual void add_playlist_directory(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, std::string dirname);
		virtual void register_add_playlist_item_callback(std::string plug_name, add_playlist_item_callback_t add_playlist_item_callback) override;
		virtual void unregister_add_playlist_item_callback(std::string plug_name) override;
		
		virtual void remove_playlist_item(playlist_id_t playlist_id, playlist_item_id_t playlist_item_id) override;
		virtual void register_remove_item_callback(std::string plug_name, remove_playlist_item_callback_t remove_playlist_item_callback) override;
		virtual void unregister_remove_item_callback(std::string plug_name) override;

		virtual void move_playlist_item(playlist_id_t playlist_id, playlist_item_id_t from_playlist_item_id, playlist_item_id_t to_playlist_item_id) override;
		virtual void register_move_item_callback(std::string plug_name, move_playlist_item_callback_t move_playlist_item_callback) override;
		virtual void unregister_move_item_callback(std::string plug_name) override;

		virtual void register_start_playlist_item_callback(std::string plug_name, start_playlist_item_callback_t start_playlist_item_callback) override;
		virtual void unregister_start_playlist_item_callback(std::string plug_name) override;

		virtual void register_progress_playlist_item_callback(std::string plug_name, progress_playlist_item_callback_t progress_playlist_item_callback) override;
		virtual void unregister_progress_playlist_item_callback(std::string plug_name) override;

		virtual void save_playlist(playlist_id_t playlist_id, std::string playlist_name) override;
		
		virtual void rename_playlist(playlist_id_t playlist_id, std::string playlist_name) override;
		virtual void register_rename_playlist_callback(std::string plug_name, rename_playlist_callback_t rename_playlist_item_callback) override;
		virtual void unregister_rename_playlist_callback(std::string plug_name) override;

		virtual void remove_playlist(playlist_id_t playlist_id) override;
		virtual void register_remove_playlist_callback(std::string plug_name, remove_playlist_callback_t remove_playlist_callback) override;
		virtual void unregister_remove_playlist_callback(std::string plug_name) override;

		virtual void register_seek_finished_callback(std::string plug_name, seek_finished_callback_t seek_finished_callback) override;
		virtual void unregister_seek_finished_callback(std::string plug_name) override;

		virtual album_art_list const& get_album_art(album_name_t const& album_name) override;

	};
}

#endif // playlist_management_plugin_imp_h__
