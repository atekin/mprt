#include "mainwindow.h"

#include "playlist_tabwidget.h"
#include "playlist_dock_widget.h"


namespace mprt_ui_plugin_qt
{
	playlist_dock_widget::playlist_dock_widget(const QString &title, QWidget *parent /*= Q_NULLPTR*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
		: QDockWidget(title, parent, flags)
		, _main_window(qobject_cast<main_window *>(parent))
	{
		init();
	}


	playlist_dock_widget::playlist_dock_widget(QWidget *parent /*= Q_NULLPTR*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
		: QDockWidget(parent, flags)
		, _main_window(qobject_cast<main_window *>(parent))
	{
		init();
	}

	playlist_dock_widget::~playlist_dock_widget()
	{

	}

	void playlist_dock_widget::init()
	{
		_playlist_tabwidget = new playlist_tabwidget(this);
		//_playlist_tabwidget->set_playlist_dock_widget(this);
		_playlist_tabwidget->setParent(this);
		setWidget(_playlist_tabwidget);
	}

	void playlist_dock_widget::set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt)
	{
		_playlist_tabwidget->set_ui_plugin_qt(ui_plug_qt);
	}

}
