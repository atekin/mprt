#include <QStyleOption>
#include <QPen>
#include <QPainter>
#include <QDebug>

#include "proxy_style.h"

proxy_style::proxy_style(QStyle *style /*= Q_NULLPTR*/)
	: QProxyStyle(style)
{

}

proxy_style::proxy_style(const QString &key)
	: QProxyStyle(key)
{

}

proxy_style::~proxy_style()
{

}

void proxy_style::drawPrimitive(PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget /*= 0*/)  const
{
	//qDebug() << "draw pir: " << element;
	
	
	if (element != QStyle::PE_IndicatorItemViewItemDrop) {
		QProxyStyle::drawPrimitive(element, option, painter, widget);
		return;
	}


	QStyleOption opt(*option);
	opt.rect.setLeft(0);
	if (widget) opt.rect.setRight(widget->width());
	QProxyStyle::drawPrimitive(element, &opt, painter, widget);

	//custom paint here, you can do nothing as well
	/*QColor c(Qt::yellow);
	QPen pen(c);
	pen.setWidth(1);

	painter->setPen(pen);
	if (!option->rect.isNull())
		painter->drawLine(option->rect.topLeft(), option->rect.topRight());*/
}
