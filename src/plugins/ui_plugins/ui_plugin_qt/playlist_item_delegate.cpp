#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include "playlist_view.h"

#include "playlist_item_delegate.h"


namespace mprt_ui_plugin_qt
{

	playlist_item_delegate::playlist_item_delegate(QObject *parent) :
		QStyledItemDelegate(parent)
	{}

	QString playlist_item_delegate::anchorAt(QString html, const QPoint &point) const {
		QTextDocument doc;
		doc.setHtml(html);

		auto textLayout = doc.documentLayout();
		Q_ASSERT(textLayout != 0);
		return textLayout->anchorAt(point);
	}

	void playlist_item_delegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
	{
		QStyledItemDelegate::initStyleOption(option, index);
		option->state &= ~QStyle::State_HasFocus;
	}

	void playlist_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyleOptionViewItem options = option;
		initStyleOption(&options, index);

		QStyle *style;
		if (options.widget)
			style = options.widget->style();
		else
			style = qApp->style();

		QTextDocument doc;
		doc.setHtml(options.text);

		options.text = "";
		style->drawControl(QStyle::CE_ItemViewItem, &options, painter);

		QAbstractTextDocumentLayout::PaintContext ctx;
		// Adjust color palette if the cell is selected
		QSize iconSize = options.icon.actualSize(options.rect.size());
		QRect clip(0, 0, options.rect.width() + iconSize.width(), options.rect.height());
		if (option.state & QStyle::State_Selected)
			ctx.palette.setColor(QPalette::Text, option.palette.color(QPalette::Active, QPalette::HighlightedText));
		
		ctx.clip = clip;

		auto item_rect = _playlist_view->visualRect(index);

		painter->save();
		if (_playlist_view->is_drop() && index.row() == _playlist_view->selected_row())	painter->drawLine(options.rect.topLeft(), options.rect.topRight());
		// right shift the icon
		painter->translate(options.rect.left() + iconSize.width(), options.rect.top());
		painter->setClipRect(clip);
		// Vertical Center alignment instead of the default top alignment
		painter->translate(0, 0.5*(options.rect.height() - doc.size().height()));
		doc.documentLayout()->draw(painter, ctx);
		painter->restore();
	}

	QSize playlist_item_delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
		QStyleOptionViewItem options = option;
		initStyleOption(&options, index);

		QTextDocument doc;
		doc.setHtml(options.text);
		doc.setTextWidth(options.rect.width());
		return QSize(doc.idealWidth(), doc.size().height());
	}
}
