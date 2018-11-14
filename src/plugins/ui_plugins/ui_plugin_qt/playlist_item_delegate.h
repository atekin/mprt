#ifndef playlist_item_delegate_h__
#define playlist_item_delegate_h__

#include <QStyledItemDelegate>

namespace mprt_ui_plugin_qt
{
	class playlist_view;

	class playlist_item_delegate : public QStyledItemDelegate {
		Q_OBJECT

	public:
		playlist_view * _playlist_view;

		explicit playlist_item_delegate(QObject *parent = 0);

		QString anchorAt(QString html, const QPoint &point) const;

	protected:
		void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

	};
}


#endif // playlist_item_delegate_h__
