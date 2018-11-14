#ifndef playlist_view_item_model_h__
#define playlist_view_item_model_h__

#include "plugins/core_plugins/playlist_management_plugin/playlist_item.h"


#include <QStandardItemModel>

namespace mprt_ui_plugin_qt
{
	class playlist_view;
	class playlist_view_item_model : public QStandardItemModel
	{
		Q_OBJECT

	public:
		mprt::playlist_item_id_t _playing_item_id;
		playlist_view *_playlist_view;
		
		explicit playlist_view_item_model(QObject *parent = Q_NULLPTR);
		playlist_view_item_model(int rows, int columns, QObject *parent = Q_NULLPTR);
		~playlist_view_item_model();

		Qt::ItemFlags flags(const QModelIndex& index) const override;

		QVariant data(const QModelIndex &index, int role) const override;
		void set_current_playing_item_id(mprt::playlist_item_id_t playlist_item_id);
	};
}

#endif // playlist_view_item_model_h__
