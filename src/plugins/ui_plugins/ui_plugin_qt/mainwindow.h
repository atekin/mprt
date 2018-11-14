#ifndef mainwindow_h__
#define mainwindow_h__

#include <QMainWindow>

class QToolBar;
class QToolButton;
class QAction;

namespace mprt_ui_plugin_qt
{

	class central_widget;
	class playlist_dock_widget;
	class song_progress_widget;
	class ui_plugin_qt;

	class main_window : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit main_window(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
		~main_window();

		void set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt);

		central_widget * _central_widget;
		QToolBar *_menu_tb;
		QToolBar *_player_controls_tb;
		QToolBar *_song_progress_tb;
		playlist_dock_widget * _playlist_dock_widget;
		song_progress_widget * _song_progress_widget;

	private:
		void init_menu_items();
		void init_player_controls();
		void init_song_progress();

		QToolButton * init_player_control_button(
			QString const& icon_file,
			QString const& tooltip,
			const char * slot_func);

		QAction * init_menu_action(
			QMenu * menu_to_add,
			QString const& icon_file,
			QString const& action_text,
			QString const& tooltip,
			QObject * receiver,
			const char * slot_func);

	private slots:
		void slot_play();
		void slot_pause();
		void slot_stop();
		void slot_previous();
		void slot_next();
		void slot_random();

		void slot_add_files();
		void slot_add_directory();
		void slot_create_playlist();
		void slot_load_playlist();
		void slot_save_playlist();
		void slot_preferences();
		void slot_exit();
		void slot_about();

	};
}

#endif // mainwindow_h__
