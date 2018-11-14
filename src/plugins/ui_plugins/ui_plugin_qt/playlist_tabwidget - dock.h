#ifndef playlist_tabwidget_h__
#define playlist_tabwidget_h__

#include <unordered_map>

#include <QMainWindow>
#include <QList>

#include "plugins/core_plugins/playlist_management_plugin/playlist_item.h"

class QHBoxLayout;

Q_DECLARE_METATYPE(mprt::playlist_id_t);
//DECLARE_METATYPE(mprt::playlist_item_id_t);
Q_DECLARE_METATYPE(mprt::playlist_item_shr_list_t);
Q_DECLARE_METATYPE(std::shared_ptr<mprt::playlist_item_shr_list_t>);

namespace mprt_ui_plugin_qt
{
	class playlist_view;
	class ui_plugin_qt;
	class playlist_dock_widget;

	class playlist_tabwidget : public QMainWindow
	{
		Q_OBJECT

	public:
		ui_plugin_qt * _ui_plugin_qt;
		playlist_dock_widget *_playlist_dock_widget;

		explicit playlist_tabwidget(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
		~playlist_tabwidget();


		void set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt);
	
		void add_playlist(QString const& playlist_name);
		void add_files(QStringList const& urls);
		void add_files(std::vector<std::string> const& urls);
		void add_urls(QList<QUrl> const& urls);

		void play();
		void stop();
		void pause();
		void unpause();
		void previous_item();
		void next_item();
		void set_playlist_dock_widget(playlist_dock_widget *pl_dock_widget);

		std::pair<mprt::playlist_id_t, mprt::playlist_item_id_t> get_current_play_ids();
		void seek_duration(size_type duration_ms);

		size_type get_duration_ms(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id);

	protected slots:
		void slot_dock_vis_changed(bool visible);

	protected:
		
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

		void play_item_offset(size_type offset);

	private:
		QHBoxLayout * _layout;
		std::unordered_map<mprt::playlist_id_t, playlist_view*> _playlist_views;
		playlist_view *_current_play_widget;

		std::vector<std::string> get_recursive_filenames(QString const & dirname);
	};
}

#endif // playlist_tabwidget_h__
