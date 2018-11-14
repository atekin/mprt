#ifndef centralwidget_h__
#define centralwidget_h__

#include <QWidget>

class QGridLayout;

namespace mprt_ui_plugin_qt
{

	class central_widget : public QWidget
	{
		Q_OBJECT

	public:
		explicit central_widget(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
		~central_widget();

		void add_playlist(QString const & playlist);

	private:
	};
}


#endif // centralwidget_h__
