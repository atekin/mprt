#include "common/type_defs.h"

#include "playlist_view_item_model.h"
#include "playlist_view.h"

namespace mprt_ui_plugin_qt
{
	playlist_view_item_model::playlist_view_item_model(QObject *parent /*= Q_NULLPTR*/)
		: QStandardItemModel(parent)
		, _playing_item_id(mprt::_INVALID_PLAYLIST_ITEM_ID_)
	{

	}


	playlist_view_item_model::playlist_view_item_model(int rows, int columns, QObject *parent /*= Q_NULLPTR*/)
		: QStandardItemModel(rows, columns, parent)
		, _playing_item_id(mprt::_INVALID_PLAYLIST_ITEM_ID_)
	{

	}

	playlist_view_item_model::~playlist_view_item_model()
	{

	}

	Qt::ItemFlags playlist_view_item_model::flags(const QModelIndex& index) const
	{
		//if (index.isValid())
		//	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled /*| Qt::ItemIsEditable*/;
	}

	QVariant playlist_view_item_model::data(const QModelIndex &index, int role) const
	{
		if (index.isValid() && index.row() == _playlist_view->get_item_row(_playing_item_id) && role == Qt::BackgroundRole)
		{
			return QColor(Qt::yellow);
		}

		return QStandardItemModel::data(index, role);
	}

	void playlist_view_item_model::set_current_playing_item_id(mprt::playlist_item_id_t playlist_item_id)
	{
		_playing_item_id = playlist_item_id;
		emit layoutChanged();
	}

}
