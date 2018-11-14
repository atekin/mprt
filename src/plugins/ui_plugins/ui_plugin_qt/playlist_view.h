#ifndef playlist_view_h__
#define playlist_view_h__

#include <unordered_map>

#include <QTableView>

#include "plugins/core_plugins/playlist_management_plugin/playlist_item.h"

namespace std {
	template<> struct hash<QString> {
		std::size_t operator()(const QString& s) const {
			return qHash(s);
		}
		/*
		std::size_t operator()(const QString& s) const noexcept
		{
			const QChar* str = s.data();
			std::size_t hash = 5381;

			for (int i = 0; i < s.size(); ++i)
				hash = ((hash << 5) + hash) + ((str->row() << 8) | (str++)->cell());

			return hash;
		}
		*/
	};


}

class QStandardItem;

namespace mprt
{
	class playlist;
}

namespace mprt_ui_plugin_qt
{
	const static int _INVALID_ROW_ = -1;
	class playlist_tabwidget;
	class playlist_view_item_model;
	class playlist_item_delegate;

	class playlist_view : public QTableView {
		Q_OBJECT

	public:
		playlist_view_item_model * _playlist_view_item_model;
		explicit playlist_view(QWidget *parent = 0);
		~playlist_view();

		void set_tab_widget(playlist_tabwidget * pl_tab_widget);

		mprt::playlist_id_t playlist_id();
		mprt::playlist_item_id_t get_selected_playlist_item_id();
		mprt::playlist_item_id_t get_next_selected_playlist_item_id();
		void set_playlist_id(mprt::playlist_id_t pllist_id);

		void add_playlist(mprt::playlist_id_t pllist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		void save_playlist();
		void add_items(mprt::playlist_item_id_t from_playlist_item, std::shared_ptr<mprt::playlist_item_shr_list_t> items);
		void move_playlist_item(mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id);
		void remove_playlist_item(mprt::playlist_item_id_t playlist_item_id);

		void start_playlist_item(mprt::playlist_item_id_t playlist_item_id);

		bool is_drop();
		int selected_row();
		mprt::playlist_item_id_t get_item_playlist_id(int row);
		int get_item_row(mprt::playlist_item_id_t playlist_item_id);
		size_type get_duration_ms(mprt::playlist_item_id_t playlist_item_id);

	signals:
		void linkActivated(QString link);
		void linkHovered(QString link);
		void linkUnhovered();

	protected:
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		void setModel(QAbstractItemModel *model) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dragMoveEvent(QDragMoveEvent *event) override;
		void dragLeaveEvent(QDragLeaveEvent *event) override;
		void dropEvent(QDropEvent *event) override; void keyPressEvent(QKeyEvent *event) override;
		void setItemDelegate(QAbstractItemDelegate *delegate);

	private:
		QString anchorAt(const QPoint &pos) const;
		QStandardItem *create_standard_item_index(mprt::playlist_item_id_t item_id, int row);
		QStandardItem *get_standard_item_index(mprt::playlist_item_id_t item_id);
		void remove_standard_item_index(mprt::playlist_item_id_t item_id);
		void remove_row(int row);
		void enlarge_item_list(QList<QStandardItem*> &row_items, int col_size);
		void set_item_list(QList<QStandardItem*> & row_items, int col_to_use, QString const& tag_val);
		void delete_selected_items();

	private:
		playlist_tabwidget * _playlist_tabwidget;
		mprt::playlist_id_t _playlist_id;
		QVector<QStandardItem*> _selected_rows;
		bool _drop_event;

		QString _mousePressAnchor;
		QString _lastHoveredAnchor;

		playlist_item_delegate * _playlist_item_delegate;

		std::unordered_map<QString, int> _table_headers;
		std::unordered_map<mprt::playlist_item_id_t, QStandardItem*> _playlist_item_index;
		//QList<mprt::playlist_item_shr_t> _item_list;
		
	};
}

#endif // playlist_view_h__
