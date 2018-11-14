#ifndef song_progress_widget_h__
#define song_progress_widget_h__

#include "common/common_defs.h"
#include "plugins/core_plugins/playlist_management_plugin/playlist_item.h"

#include <QSlider>
#include <QPixmap>

namespace mprt_ui_plugin_qt
{
	class main_window;

	class song_progress_widget : public QSlider
	{
		Q_OBJECT

	public:
		main_window * _main_window;
		mprt::playlist_id_t _current_playlist_id;
		mprt::playlist_item_id_t _current_playlist_item_id;


		explicit song_progress_widget(QWidget *parent = Q_NULLPTR);
		explicit song_progress_widget(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR);

		~song_progress_widget();

		bool progress(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms);
		void seek_finished();

	protected:
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		void paintEventX(QPaintEvent *event);

	private slots:

	private:
		bool _is_seek_block;
		int _last_pos;
		int _seek_position;
		bool _mouse_pressed;

		void init();
		void seek_to();
		QPixmap svg_to_pixmap(const QSize &ImageSize, const QString &SvgFile);
		QString get_formatted_time(int msecs);
		
	};
}

#endif // song_progress_widget_h__
