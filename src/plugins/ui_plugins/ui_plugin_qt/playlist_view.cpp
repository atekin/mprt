
#include <codecvt>
#include <set>

#include <QApplication>
#include <QCursor>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QByteArray>
#include <QBuffer>
#include <QPixmap>
#include <QHeaderView>
#include <QWidget>
#include <QMimeData>

#include <boost/log/trivial.hpp>

#include "common/utils.h"
#include "common/type_defs.h"

#include "ui_plugin_qt.h"
#include "playlist_tabwidget.h"
#include "playlist_item_delegate.h"
#include "playlist_view_item_model.h"
#include "playlist_tabwidget.h"
#include "playlist_view.h"

#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/id3v2tag.h>


namespace mprt_ui_plugin_qt
{
#define _ALBUM_ART_ITEM_COL_ 0
#define _PLAYLIST_ITEM_ID_COL_ 1

	QString toCamelCase(QString const& s)
	{
		QStringList parts = s.split(' ', QString::SkipEmptyParts);
		for (auto i = 0; i != parts.size(); ++i)
			parts[i].replace(0, 1, parts[i][0].toTitleCase());

		return parts.join(" ");
	}

	playlist_view::playlist_view(QWidget *parent)
		: QTableView(parent)
		, _playlist_tabwidget(qobject_cast<playlist_tabwidget*>(parent))
		, _drop_event(false)
	{
		// needed for the hover functionality
		setMouseTracking(true);

		setModel(new playlist_view_item_model(this));
		setItemDelegate(new playlist_item_delegate(this));

		setSelectionMode(QAbstractItemView::ExtendedSelection);
		setSelectionBehavior(QAbstractItemView::SelectRows);
		viewport()->setAcceptDrops(true);
		setDragDropMode(QAbstractItemView::DragDrop);
		setShowGrid(false);
		horizontalHeader()->setSectionsMovable(true);

		setDragEnabled(true);
		setAcceptDrops(true);
		setDragDropOverwriteMode(false);
		setDropIndicatorShown(true);

		QString playlist_item_id = tr("playlist_item_id");
		_table_headers[playlist_item_id] = _PLAYLIST_ITEM_ID_COL_;
		_playlist_view_item_model->setHorizontalHeaderItem(_PLAYLIST_ITEM_ID_COL_, new QStandardItem(playlist_item_id));

		QString album_art = tr("Album Art");
		_table_headers[album_art] = _ALBUM_ART_ITEM_COL_;
		_playlist_view_item_model->setHorizontalHeaderItem(_ALBUM_ART_ITEM_COL_, new QStandardItem(album_art));

		/*
		QString styleSheet = "::section {" // "QHeaderView::section {"
			"spacing: 10px;"
			"background-color: lightblue;"
			"color: white;"
			"border: 1px solid red;"
			"margin: 1px;"
			"text-align: right;"
			"font-family: arial;"
			"font-size: 12px; }";
		horizontalHeader()->setStyleSheet(styleSheet);
		verticalHeader()->setStyleSheet(styleSheet);

		//QString styleSheet1 = "::section {" // "QHeaderView::section {"
		//	"spacing: 10px;"
		//	"background-color: blue;"
		//	"color: white;"
		//	"border: 1px solid red;"
		//	"margin: 1px;"
		//	"text-align: right;"
		//	"font-family: arial;"
		//	"font-size: 12px; }";

		QString styleSheet1 = "QTableView {"
			"selection-background-color: qlineargradient(x1 : 0, y1 : 0, x2 : 0.5, y2 : 0.5, stop: 0 #FF92BB, stop: 1 white);"
			"selection-color: yellow;"
			"background-color: gray;"
			"}"
			"QTableView QTableCornerButton::section{"
			"background: red;"
			"border: 2px outset red;"
			"}";

		*/

		//QString styleSheet2 = "QTableView::item:hover { "
		//	"	background - color: #D3F1FC;"
		//	"}";
		//setStyleSheet(styleSheet2);

		//setStyle(new proxy_style());
	}

	playlist_view::~playlist_view()
	{

	}

	void playlist_view::add_playlist(mprt::playlist_id_t pllist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		set_playlist_id(pllist_id);

		add_items(mprt::_INVALID_PLAYLIST_ITEM_ID_, items);
	}

	void playlist_view::save_playlist()
	{

	}

	void playlist_view::add_items(mprt::playlist_item_id_t from_playlist_item, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		if (!items || items->empty())
		{
			return;
		}

		auto item_iter = items->begin(), item_iter_end = items->end();

		int from_row = get_item_row(from_playlist_item);
		from_row = from_row == _INVALID_ROW_ ? _playlist_view_item_model->rowCount() : from_row;

		//BOOST_LOG_TRIVIAL(debug) << "working dir: " << QDir::currentPath().toStdString();

		int last_used_col = _table_headers.size(), col_to_use;
		for (int row = from_row; item_iter != item_iter_end; ++item_iter)
		{
			QList<QStandardItem*> row_items;
			if (*item_iter && (*item_iter)->_tags && ((*item_iter)->_tags->size() > 0))
			{
				for (auto tags : (*(*item_iter)->_tags)) {
					auto tag_name = QString::fromStdString(tags.first);
					if (tag_name.isEmpty())
						continue;
					tag_name = tag_name.toLower();
					tag_name = toCamelCase(tag_name);
					auto iter_tb_header = _table_headers.find(tag_name);
					if (iter_tb_header == _table_headers.end())
					{
						_playlist_view_item_model->setHorizontalHeaderItem(last_used_col, new QStandardItem(tag_name));
						col_to_use = last_used_col;
						_table_headers[tag_name] = last_used_col++;
					}
					else
					{
						col_to_use = iter_tb_header->second;
					}

					set_item_list(row_items, col_to_use, QString::fromStdString(tags.second));

					//new QStandardItem(QString("<style type=\"text/css\"> p.swimmers { color: red } </style> <p class=\"swimmers\">") + tag_val + QString("</p>")));
					//new QStandardItem(QString("<style type=\"text/css\"> p.swimmers { color: red; background-color:black } </style> <p class=\"swimmers\">") + tag_val + QString("</p>")));
				}

				row_items[_PLAYLIST_ITEM_ID_COL_] = create_standard_item_index((*item_iter)->_playlist_item_id, row);

				if ((*item_iter)->_albumart.size() > 0)
				{
					auto img_name = (*item_iter)->_albumart.begin()->first;
					auto img_data = (*item_iter)->_albumart.begin()->second;

					img_name = string_trim(img_name);

					auto img_extension = get_ext(img_name);

					QPixmap pixmap;
					if (pixmap.loadFromData(reinterpret_cast<const uchar *>(img_data->data()), img_data->size(), img_extension.c_str(), Qt::AutoColor))
					{
						auto scaled_image = pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

						QByteArray scaled_bytes;
						QBuffer scaled_buf(&scaled_bytes);
						scaled_buf.open(QIODevice::WriteOnly);
						scaled_image.save(&scaled_buf, img_extension.c_str());

						QString url = QString("<img src=\"data:image/") + QString::fromStdString(img_extension) + ";base64," + scaled_bytes.toBase64() + "\"/>";

						row_items[_ALBUM_ART_ITEM_COL_] = new QStandardItem(url);
					}
				}

				_playlist_view_item_model->insertRow(row, row_items);

				++row;
			}
		}
	}

	void playlist_view::move_playlist_item(mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id)
	{
		auto take_row = get_item_row(from_playlist_item_id);
		auto to_row = get_item_row(to_playlist_item_id);
		if (_INVALID_ROW_ != take_row && _INVALID_ROW_ != to_row)
		{
			auto taken_row = _playlist_view_item_model->takeRow(take_row);
			if (taken_row.size() > 0)
			{
				_playlist_view_item_model->insertRow(to_row, taken_row);
			}
		}

		selectionModel()->select(_playlist_view_item_model->index(to_row, _PLAYLIST_ITEM_ID_COL_), QItemSelectionModel::Select | QItemSelectionModel::Rows);

		_drop_event = false;
		//verticalHeader()->moveSection(verticalHeader()->visualIndex(take_row), verticalHeader()->visualIndex(to_row));
	}

	mprt::playlist_id_t playlist_view::playlist_id()
	{
		return _playlist_id;
	}

	mprt::playlist_item_id_t playlist_view::get_selected_playlist_item_id()
	{
		if (_playlist_view_item_model->rowCount() < 1)
		{
			return mprt::_INVALID_PLAYLIST_ITEM_ID_;
		}

		return get_item_playlist_id(selected_row());
	}

	mprt::playlist_item_id_t playlist_view::get_next_selected_playlist_item_id()
	{
		auto row_to_add = _playlist_view_item_model->rowCount();
		if (row_to_add < 1)
		{
			return mprt::_INVALID_PLAYLIST_ITEM_ID_;
		}

		auto sel_row = selected_row();
		if (sel_row != _INVALID_ROW_)
		{
			row_to_add = sel_row + 1;
		}
		return get_item_playlist_id(row_to_add);
	}

	void playlist_view::set_playlist_id(mprt::playlist_id_t pllist_id)
	{
		_playlist_id = pllist_id;
	}

	void playlist_view::mousePressEvent(QMouseEvent *event) {
		QTableView::mousePressEvent(event);

		auto anchor = anchorAt(event->pos());
		_mousePressAnchor = anchor;
	}

	void playlist_view::mouseMoveEvent(QMouseEvent *event) {
		auto anchor = anchorAt(event->pos());

		if (_mousePressAnchor != anchor) {
			_mousePressAnchor.clear();
		}

		if (_lastHoveredAnchor != anchor) {
			_lastHoveredAnchor = anchor;
			if (!_lastHoveredAnchor.isEmpty()) {
				QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
				emit linkHovered(_lastHoveredAnchor);
			}
			else {
				QApplication::restoreOverrideCursor();
				emit linkUnhovered();
			}
		}

		QTableView::mouseMoveEvent(event);
	}

	void playlist_view::mouseReleaseEvent(QMouseEvent *event) {
		if (!_mousePressAnchor.isEmpty()) {
			auto anchor = anchorAt(event->pos());

			if (anchor == _mousePressAnchor) {
				emit linkActivated(_mousePressAnchor);
			}

			_mousePressAnchor.clear();
		}

		_drop_event = false;

		QTableView::mouseReleaseEvent(event);
	}

	void playlist_view::setModel(QAbstractItemModel *model)
	{
		_playlist_view_item_model = qobject_cast<playlist_view_item_model*>(model);
		_playlist_view_item_model->_playlist_view = this;

		QTableView::setModel(model);
	}

	void playlist_view::dragEnterEvent(QDragEnterEvent *event)
	{
		//QTableView::dragEnterEvent(event);

		auto sel_row_idxs = selectionModel()->selectedRows();
		_selected_rows.clear();
		_selected_rows.reserve(sel_row_idxs.size());
		for (auto & sel_row : sel_row_idxs)
		{
			_selected_rows.push_back( _playlist_view_item_model->itemFromIndex(sel_row));
		}
		
		event->acceptProposedAction();

		emit _playlist_view_item_model->layoutChanged();
		_drop_event = true;
	}

	void playlist_view::dragMoveEvent(QDragMoveEvent *event)
	{
		//QTableView::dragMoveEvent(event);

		event->acceptProposedAction();

		auto index = indexAt(event->pos());
		auto new_row = index.isValid() ? index.row() : (_playlist_view_item_model->rowCount() - 1);
		QModelIndex idx = _playlist_view_item_model->index(new_row, _PLAYLIST_ITEM_ID_COL_);
		//selectionModel()->select(idx, QItemSelectionModel::Clear);
		selectionModel()->select(idx, QItemSelectionModel::Clear | QItemSelectionModel::Rows | QItemSelectionModel::Select);
		//selectionModel()->select(idx, QItemSelectionModel::SelectCurrent);

		_drop_event = true;
		emit _playlist_view_item_model->layoutChanged();
	}

	void playlist_view::dragLeaveEvent(QDragLeaveEvent *event)
	{
		//QTableView::dragLeaveEvent(event);

		event->accept();

		_drop_event = false;

		emit _playlist_view_item_model->layoutChanged();
	}

	void playlist_view::setItemDelegate(QAbstractItemDelegate *delegate)
	{
		_playlist_item_delegate = qobject_cast<playlist_item_delegate*>(delegate);
		_playlist_item_delegate->_playlist_view = this;

		QTableView::setItemDelegate(delegate);
	}

	void playlist_view::dropEvent(QDropEvent *event)
	{
		if (event->source() == this)
		{
			int newRow = indexAt(event->pos()).row();

			if (_INVALID_ROW_ == newRow)
			{
				newRow = _playlist_view_item_model->rowCount() - 1;
			}

			auto
				iter_sel_item = _selected_rows.rbegin(),
				iter_sel_end = _selected_rows.rend();
			for (; iter_sel_item != iter_sel_end; ++iter_sel_item)
			{
				auto from_playlist_item_id = get_item_playlist_id((*iter_sel_item)->index().row());
				auto to_playlist_item_id = get_item_playlist_id(newRow);
				if ((mprt::_INVALID_PLAYLIST_ITEM_ID_ != from_playlist_item_id) && (mprt::_INVALID_PLAYLIST_ITEM_ID_ != to_playlist_item_id))
				{
					_playlist_tabwidget->_ui_plugin_qt->_playlist_management_plugin->move_playlist_item(playlist_id(), from_playlist_item_id, to_playlist_item_id);
				}				
			}

			clearSelection();
			event->accept();
		}
		else
		{
			const QMimeData* mimeData = event->mimeData();

			// check for our needed mime type, here a file or a list of files
			if (mimeData->hasUrls())
			{
				_playlist_tabwidget->add_urls(mimeData->urls());

				event->accept();
			}
			else
			{
				event->ignore();
			}
		}
	}

	void playlist_view::keyPressEvent(QKeyEvent *event)
	{
		if (event->type() == QEvent::KeyPress)
		{
			switch (event->key())
			{
			case Qt::Key_Delete:
				delete_selected_items();
				break;
			
			default:
				break;
			}
		}

		QTableView::keyPressEvent(event);
	}

	QString playlist_view::anchorAt(const QPoint &pos) const {
		auto index = indexAt(pos);
		if (index.isValid()) {
			auto delegate = itemDelegate(index);
			auto wordDelegate = qobject_cast<playlist_item_delegate *>(delegate);
			if (nullptr != wordDelegate) {
				auto itemRect = visualRect(index);
				auto relativeClickPosition = pos - itemRect.topLeft();

				auto html = model()->data(index, Qt::DisplayRole).toString();

				return wordDelegate->anchorAt(html, relativeClickPosition);
			}
		}

		return QString();
	}

	QStandardItem * playlist_view::create_standard_item_index(mprt::playlist_item_id_t item_id, int row)
	{
		auto std_item = new QStandardItem(QString::number(item_id));
		_playlist_item_index[item_id] = std_item;
		return std_item;
	}

	QStandardItem * playlist_view::get_standard_item_index(mprt::playlist_item_id_t item_id)
	{
		auto iter = _playlist_item_index.find(item_id);
		if (iter != _playlist_item_index.end())
		{
			return iter->second;
		}

		return nullptr;
	}

	void playlist_view::remove_standard_item_index(mprt::playlist_item_id_t item_id)
	{
		_playlist_item_index.erase(item_id);
	}

	int playlist_view::get_item_row(mprt::playlist_item_id_t playlist_item_id)
	{
		auto iter = _playlist_item_index.find(playlist_item_id);
		if ((iter != _playlist_item_index.end()) && (iter->second) && (iter->second->index().isValid()))
		{
			return iter->second->index().row();
		}

		return -1;
	}

	size_type playlist_view::get_duration_ms(mprt::playlist_item_id_t playlist_item_id)
	{
		auto item_row = get_item_row(playlist_item_id);

		if (item_row == _INVALID_ROW_)
		{
			return -1;
		}
		
		auto table_header_iter = _table_headers.find(QString::fromStdString(std::string(mprt::_AUDIO_PROP_DURATION_MS_)));
		if (table_header_iter == _table_headers.end())
		{
			return -1;
		}

		auto idx = _playlist_view_item_model->index(item_row, table_header_iter->second);
		if (!idx.isValid())
		{
			return -1;
		}
		
		bool ok;
		auto dur = idx.data().toLongLong(&ok);
		return ok ? dur : -1;
	}

	mprt::playlist_item_id_t playlist_view::get_item_playlist_id(int row)
	{
		auto model_index = _playlist_view_item_model->index(row, _PLAYLIST_ITEM_ID_COL_);
		bool ok;
		auto from_playlist_item_id = model_index.data().toLongLong(&ok);

		return ok ? from_playlist_item_id : mprt::_INVALID_PLAYLIST_ITEM_ID_;
	}

	void playlist_view::remove_row(int row)
	{
		auto delete_playlist_item_id = get_item_playlist_id(row);
		if (mprt::_INVALID_PLAYLIST_ITEM_ID_ != delete_playlist_item_id)
		{
			_playlist_tabwidget->_ui_plugin_qt->_playlist_management_plugin->remove_playlist_item(playlist_id(), delete_playlist_item_id);
		}
	}

	void playlist_view::enlarge_item_list(QList<QStandardItem*> &row_items, int col_size)
	{
		if (row_items.size() < col_size)
		{
			row_items.reserve(col_size);
			for (int i = row_items.size(); i != col_size; ++i)
			{
				row_items.append(nullptr);
			}
		}
	}

	void playlist_view::set_item_list(QList<QStandardItem*> & row_items, int col_to_use, QString const& tag_val)
	{
		enlarge_item_list(row_items, col_to_use + 1);

		delete_ptr(row_items[col_to_use]);

		row_items[col_to_use] = (new QStandardItem(tag_val));
	}

	void playlist_view::delete_selected_items()
	{
		auto selected_item_list = selectionModel()->selectedRows();

		std::set<int, std::greater<int> > deleted_rows;
		for (auto sel_item : selected_item_list)
		{
			deleted_rows.insert(sel_item.row());
		}

		for (auto row : deleted_rows)
		{
			auto item_id = get_item_playlist_id(row);
			_playlist_tabwidget->_ui_plugin_qt->_playlist_management_plugin->remove_playlist_item(_playlist_id, item_id);
		}
	}

	void playlist_view::remove_playlist_item(mprt::playlist_item_id_t playlist_item_id)
	{
		auto row = get_item_row(playlist_item_id);
		_playlist_view_item_model->removeRow(row);
		remove_standard_item_index(playlist_item_id);
	}

	void playlist_view::start_playlist_item(mprt::playlist_item_id_t playlist_item_id)
	{

	}

	bool playlist_view::is_drop()
	{
		return _drop_event;
	}

	int playlist_view::selected_row()
	{
		auto sel_items = selectedIndexes();

		return sel_items.size() > 0 ? sel_items.at(0).row() : _INVALID_ROW_;
	}

	void playlist_view::set_tab_widget(playlist_tabwidget * pl_tab_widget)
	{
		_playlist_tabwidget = pl_tab_widget;
	}

}


