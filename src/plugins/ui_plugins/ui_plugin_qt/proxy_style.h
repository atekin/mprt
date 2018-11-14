#ifndef proxy_style_h__
#define proxy_style_h__

#include <QProxyStyle>

class proxy_style : public QProxyStyle
{
	Q_OBJECT
public:
	proxy_style(QStyle *style = Q_NULLPTR);
	proxy_style(const QString &key);
	~proxy_style();

public:

	void drawPrimitive(PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0) const override;

};

#endif // proxy_style_h__
