#ifndef playlist_dock_widget_h__
#define playlist_dock_widget_h__

#include <QDockWidget>

namespace mprt_ui_plugin_qt
{
	class ui_plugin_qt;

	class playlist_tabwidget;
	class main_window;

	class playlist_dock_widget : public QDockWidget
	{
		Q_OBJECT

	public:
		main_window * _main_window;
		playlist_tabwidget *_playlist_tabwidget;

		explicit playlist_dock_widget(const QString &title, QWidget *parent = Q_NULLPTR,
			Qt::WindowFlags flags = Qt::WindowFlags());
		explicit playlist_dock_widget(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
		~playlist_dock_widget();

		void set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt);

	private:

		void init();
	};

}

#endif // playlist_dock_widget_h__
