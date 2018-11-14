#ifndef playlist_tabwidget_h__
#define playlist_tabwidget_h__

#include <unordered_map>

#include <QTabWidget>
#include <QList>

#include "common/job_type_enums.h"
#include "plugins/core_plugins/playlist_management_plugin/playlist_item.h"

class QHBoxLayout;
class QMenu;
class QAction;

Q_DECLARE_METATYPE(mprt::playlist_id_t);
//DECLARE_METATYPE(mprt::playlist_item_id_t);
Q_DECLARE_METATYPE(mprt::playlist_item_shr_list_t);
Q_DECLARE_METATYPE(std::shared_ptr<mprt::playlist_item_shr_list_t>);

namespace mprt_ui_plugin_qt
{
	class playlist_view;
	class ui_plugin_qt;
	class playlist_dock_widget;
	class tab_bar;

	class playlist_tabwidget : public QTabWidget
	{
		Q_OBJECT

	public:
		ui_plugin_qt * _ui_plugin_qt;
		playlist_dock_widget *_playlist_dock_widget;
		plugin_states _plugin_play_state;

		explicit playlist_tabwidget(QWidget *parent = Q_NULLPTR);
		~playlist_tabwidget();

		void set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt);
	
		void add_playlist(QString const& playlist_name);
		void add_files(QStringList const& urls);
		void add_files(std::shared_ptr<std::vector<std::string>> const& urls);
		void add_urls(QList<QUrl> const& urls);

		void save_playlist(QString const& playlist_name);

		void play();
		void stop();
		void pause();
		void unpause();
		void toggle();
		void previous_item();
		void next_item();

		std::pair<mprt::playlist_id_t, mprt::playlist_item_id_t> get_current_play_ids();
		void seek_duration(size_type duration_ms);

		size_type get_duration_ms(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id);

	protected slots:
		void slot_tab_context_menu(const QPoint & point);
		void slot_tab_context_rename();
		void slot_tab_context_remove();
		void slot_tab_context_remove(int);
		void slot_tabtext_change_request(int tab_index, QString const& text);
		void slot_current_changed(int tab_index);

	protected:
		QHBoxLayout * layout();
		
		void add_playlist_cb(std::string playlist_name, mprt::playlist_id_t playlist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		Q_INVOKABLE void add_playlist_qt_cb(QString playlist_name, mprt::playlist_id_t playlist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		
		void add_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		Q_INVOKABLE void add_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		
		void move_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id);
		Q_INVOKABLE void move_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id);

		void remove_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id);
		Q_INVOKABLE void remove_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id);

		void start_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type duration_ms);
		Q_INVOKABLE void start_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type duration_ms);

		void progress_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms);
		Q_INVOKABLE void progress_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms);

		void rename_playlist_cb(mprt::playlist_id_t playlist_id, std::string playlist_name);
		Q_INVOKABLE void rename_playlist_qt_cb(mprt::playlist_id_t playlist_id, QString playlist_name);

		void remove_playlist_cb(mprt::playlist_id_t playlist_id);
		Q_INVOKABLE void remove_playlist_qt_cb(mprt::playlist_id_t playlist_id);

		void seek_finished_cb();
		Q_INVOKABLE void seek_finished_qt_cb();

		void play_item_offset(size_type offset);

	private:
		QHBoxLayout * _layout;
		std::unordered_map<mprt::playlist_id_t, playlist_view*> _playlist_views;
		QMenu *_playlist_context_menu;
		int _context_menu_tab_index;
		tab_bar *_tab_bar;
		int _last_current_index;
		
		playlist_view *current_play_widget();
		playlist_view *play_widget(int tab_index);
		int tab_index_from_playlist_id(mprt::playlist_id_t playlist_id);
		void init_playlist_context_menu();
		QAction *init_context_menu_action(
			QString const& icon_file, QString const& action_text,
			QString const& tooltip,
			QObject * receiver,
			const char * slot_func);
		void set_close_visible(int tab_index, bool visible);
	};
}

#endif // playlist_tabwidget_h__
