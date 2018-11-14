#include <QToolBar>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QSpacerItem>
#include <QLabel>
#include <QMainWindow>
#include <QDockWidget>
#include <QSlider>
#include <QApplication>
#include <QFileDialog>
#include <QListView>
#include <QTreeView>

#include <boost/log/trivial.hpp>

#include "mainwindow.h"
#include "centralwidget.h"
#include "playlist_dock_widget.h"
#include "song_progress_widget.h"
#include "playlist_tabwidget.h"

namespace mprt_ui_plugin_qt
{

	main_window::main_window(QWidget *parent, Qt::WindowFlags flags)
		: QMainWindow(parent, flags)
	{
		//_central_widget = new central_widget();
		//setCentralWidget(_central_widget);
		//_central_widget->hide();
		setCentralWidget(nullptr);

		setDockNestingEnabled(true);

		init_menu_items();
		init_player_controls();
		init_song_progress();

		_playlist_dock_widget = new playlist_dock_widget(tr("Playlist"), this);
		addDockWidget(Qt::BottomDockWidgetArea, _playlist_dock_widget);
		_playlist_dock_widget->show();

		setAcceptDrops(true);

	}

	main_window::~main_window()
	{

	}

	void main_window::set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt)
	{
		_playlist_dock_widget->set_ui_plugin_qt(ui_plug_qt);
	}

	void main_window::init_menu_items()
	{
		_menu_tb = addToolBar("Menu");
		auto menubar = new QMenuBar();

		auto file_menu = new QMenu(tr("&File"));
		init_menu_action(file_menu, ":/player_control_icons/icons/addfile_icon.svg", "&Add File(s)", "Add files to current playlist", this, SLOT(slot_add_files()));
		init_menu_action(file_menu, ":/player_control_icons/icons/adddirectory_icon.svg", "Add &Folder(s)", "Add folders recursively to the current playlist", this, SLOT(slot_add_directory()));
		file_menu->addSeparator();
		init_menu_action(file_menu, ":/player_control_icons/icons/create_icon.svg", "&New Playlist", "Create new playlist", this, SLOT(slot_create_playlist()));
		init_menu_action(file_menu, ":/player_control_icons/icons/load_playlist_icon.svg", "&Load Playlist", "Load playlist", this, SLOT(slot_load_playlist()));
		init_menu_action(file_menu, ":/player_control_icons/icons/save_icon.svg", "&Save Playlist", "Save playlist", this, SLOT(slot_save_playlist()));
		file_menu->addSeparator();
		init_menu_action(file_menu, ":/player_control_icons/icons/preferences_icon.svg", "&Preferences", "Preferences", this, SLOT(slot_preferences()));
		file_menu->addSeparator();
		init_menu_action(file_menu, ":/player_control_icons/icons/exit_icon.svg", "&Exit", "Exit", this, SLOT(slot_exit()));

		auto edit_menu = new QMenu(tr("&Edit"));

		auto help_menu = new QMenu(tr("&Help"));
		init_menu_action(help_menu, "", "&About", "About", this, SLOT(slot_about()));
		init_menu_action(help_menu, "", "About &Qt", "About Qt", qApp, SLOT(aboutQt()));

		menubar->addMenu(file_menu);
		menubar->addMenu(edit_menu);
		menubar->addMenu(help_menu);

		_menu_tb->addWidget(menubar);
	}

	void main_window::init_player_controls()
	{
		_player_controls_tb = addToolBar("Player Controls");

		init_player_control_button(
			":/player_control_icons/icons/stop_icon.svg", "Stop", SLOT(slot_stop()));

		init_player_control_button(
			":/player_control_icons/icons/play_icon.svg", "Play", SLOT(slot_play()));

		init_player_control_button(
			":/player_control_icons/icons/pause_icon.svg", "Pause", SLOT(slot_pause()));

		init_player_control_button(
			":/player_control_icons/icons/previous_icon.svg", "Previous Item", SLOT(slot_previous()));

		init_player_control_button(
			":/player_control_icons/icons/next_icon.svg", "Next Item", SLOT(slot_next()));

		init_player_control_button(
			":/player_control_icons/icons/random_icon.svg", "Random", SLOT(slot_random()));
	}

	void main_window::init_song_progress()
	{
		_song_progress_tb = addToolBar("Song Progress");

		_song_progress_widget = new song_progress_widget(Qt::Horizontal, this);

		_song_progress_tb->addWidget(_song_progress_widget);
	}

	QToolButton * main_window::init_player_control_button(
		QString const& icon_file,
		QString const& tooltip,
		const char * slot_func)
	{
		auto
			tb = new QToolButton(_player_controls_tb);
		QPixmap pixmap(icon_file);
		pixmap = pixmap.scaled(12, 12, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QIcon icon(pixmap);
		tb->setIcon(icon);
		tb->setToolTip(tooltip);
		_player_controls_tb->addWidget(tb);
		connect(tb, SIGNAL(clicked()), slot_func);

		return tb;
	}

	QAction * main_window::init_menu_action(
		QMenu * menu_to_add,
		QString const& icon_file,
		QString const& action_text,
		QString const& tooltip,
		QObject * receiver,
		const char * slot_func)
	{

		auto action = new QAction(action_text, _menu_tb);
		if (!icon_file.isEmpty())
		{
			action->setIcon(QIcon(icon_file));
		}
		action->setToolTip(tooltip);
		action->setStatusTip(tooltip);
		connect(action, SIGNAL(triggered()), receiver, slot_func);
		menu_to_add->addAction(action);
		return action;
	}

	void main_window::slot_play()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_play() called";

		_playlist_dock_widget->_playlist_tabwidget->play();
	}

	void main_window::slot_pause()
	{
		_playlist_dock_widget->_playlist_tabwidget->toggle();
	}

	void main_window::slot_stop()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_stop() called";

		_playlist_dock_widget->_playlist_tabwidget->stop();

	}

	void main_window::slot_previous()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_previous() called";

		_playlist_dock_widget->_playlist_tabwidget->previous_item();
	}

	void main_window::slot_next()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_next() called";
		_playlist_dock_widget->_playlist_tabwidget->next_item();
	}

	void main_window::slot_random()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_random() called";

	}

	void main_window::slot_add_files()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_add_files() called";

		auto fileNames = QFileDialog::getOpenFileNames(this,
			tr("Add Music Files"), QString(), tr("Audio Files (*.*)"), nullptr, QFileDialog::DontUseNativeDialog);

		if (!(fileNames.isEmpty()))
		{
			_playlist_dock_widget->_playlist_tabwidget->add_files(fileNames);
		}

	}

	void main_window::slot_add_directory()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_add_directory() called";

		QFileDialog w;
		w.setFileMode(QFileDialog::Directory);
		//w.setOption(QFileDialog::DontUseNativeDialog, true);
		//w.setOption(QFileDialog::ShowDirsOnly, true);
		w.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
		QListView *l = w.findChild<QListView*>("listView");
		if (l) {
			l->setSelectionMode(QAbstractItemView::MultiSelection);
		}
		QTreeView *t = w.findChild<QTreeView*>();
		if (t) {
			t->setSelectionMode(QAbstractItemView::MultiSelection);
		}

		w.exec();

		_playlist_dock_widget->_playlist_tabwidget->add_urls(w.selectedUrls());
	}

	void main_window::slot_create_playlist()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_create_playlist() called";

	}

	void main_window::slot_load_playlist()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_load_playlist() called";

		auto fileName = QFileDialog::getOpenFileName(this,
			tr("Open Playlist"), QString(), tr("Playlist Files (*.m3u *.m3u8)"), nullptr, QFileDialog::DontUseNativeDialog);

		if (!fileName.isNull() && !(fileName.isEmpty()))
		{
			_playlist_dock_widget->_playlist_tabwidget->add_playlist(fileName);
		}

	}

	void main_window::slot_save_playlist()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_save_playlist() called";

	
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_load_playlist() called";

		auto fileName = QFileDialog::getSaveFileName(this,
			tr("Save Playlist"), QString(), tr("Playlist Files (*.m3u *.m3u8)"), nullptr, QFileDialog::DontUseNativeDialog);

		if (!fileName.isNull() && !(fileName.isEmpty()))
		{
			_playlist_dock_widget->_playlist_tabwidget->save_playlist(fileName);
		}
	}

	void main_window::slot_preferences()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_preferences() called";

	}

	void main_window::slot_exit()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_exit() called";

	}

	void main_window::slot_about()
	{
		BOOST_LOG_TRIVIAL(debug) << "main_window::slot_about() called";

	}

}
