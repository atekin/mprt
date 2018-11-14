#include "dock_widget.h"

dock_widget::dock_widget(const QString &title, QWidget *parent /*= Q_NULLPTR*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	: QDockWidget(title, parent, flags)
{

}

dock_widget::dock_widget(QWidget *parent /*= Q_NULLPTR*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	: QDockWidget(parent, flags)
{

}

dock_widget::~dock_widget()
{

}

void dock_widget::resizeEvent(QResizeEvent* event)
{
	//setMask(QRegion(rect()));

	QDockWidget::resizeEvent(event);
}

