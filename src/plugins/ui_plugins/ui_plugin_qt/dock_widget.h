#ifndef dock_widget_h__
#define dock_widget_h__

#include <QDockWidget>

class dock_widget : public QDockWidget
{
	Q_OBJECT

public:
	explicit dock_widget(const QString &title, QWidget *parent = Q_NULLPTR,
		Qt::WindowFlags flags = Qt::WindowFlags());
	explicit dock_widget(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
	~dock_widget();

protected:
	void resizeEvent(QResizeEvent* event);
};

#endif // dock_widget_h__
