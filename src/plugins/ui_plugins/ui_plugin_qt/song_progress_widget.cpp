#include <iostream>
#include <iomanip>
#include <sstream>

#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"

#include <QStyle>
#include <QPainter>
#include <QSvgRenderer>
#include <QStyleOptionSlider>
#include <QApplication>
#include <QScreen>
#include <QToolTip>

#include "common/utils.h"

#include "mainwindow.h"
#include "playlist_dock_widget.h"
#include "playlist_tabwidget.h"
#include "song_progress_widget.h"
#include "ui_plugin_qt.h"

namespace mprt_ui_plugin_qt
{
#define _BLOCK_VAL 5
	song_progress_widget::song_progress_widget(QWidget *parent /*= Q_NULLPTR*/)
		: QSlider(parent)
		, _main_window(qobject_cast<main_window*>(parent))
	{
		init();
	}

	song_progress_widget::song_progress_widget(Qt::Orientation orientation, QWidget *parent /*= Q_NULLPTR*/)
		: QSlider(orientation, parent)
		, _main_window(qobject_cast<main_window*>(parent))
	{
		init();
	}

	song_progress_widget::~song_progress_widget()
	{

	}


	bool song_progress_widget::progress(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms)
	{
		//BOOST_LOG_TRIVIAL(debug) << "progress called with: " << position_ms;

		bool changed_item(false);
		if (playlist_id != _current_playlist_id || playlist_item_id != _current_playlist_item_id)
		{
			_current_playlist_id = playlist_id;
			_current_playlist_item_id = playlist_item_id;

			setMaximum(_main_window->_playlist_dock_widget->_playlist_tabwidget->get_duration_ms(playlist_id, playlist_item_id));
			changed_item = true;
		}

		if (!_is_seek_block && !_mouse_pressed)
		{
			auto pos = static_cast<int>(position_ms);

			if (pos > maximum())
				setMaximum(pos);

			setSliderPosition(pos);
		}

		return changed_item;
	}

	void song_progress_widget::seek_finished()
	{
		_is_seek_block = false;
		setSliderPosition(_seek_position);
	}

	void song_progress_widget::mousePressEvent(QMouseEvent *event)
	{
		_mouse_pressed = true;

		auto pos = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width());
		setSliderPosition(pos);
		QToolTip::showText(mapToGlobal(event->pos()), get_formatted_time(pos), this);

		QSlider::mousePressEvent(event);
	}

	void song_progress_widget::mouseMoveEvent(QMouseEvent *event)
	{
		auto pos = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width());
		setSliderPosition(pos);
		
		QToolTip::showText(mapToGlobal(QPoint(event->x(), this->pos().y())), get_formatted_time(pos), this);

		QSlider::mouseMoveEvent(event);
	}

	void song_progress_widget::mouseReleaseEvent(QMouseEvent *event)
	{
		_mouse_pressed = false;

		_seek_position = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width());
		setSliderPosition(_seek_position);
		
		QSlider::mouseReleaseEvent(event);
		
		seek_to();
	}

	void song_progress_widget::paintEventX(QPaintEvent *event)
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
		QStyleOptionSlider o;
		initStyleOption(&o);
		QRect knobRect = style()->subControlRect(QStyle::CC_Slider, &o, QStyle::SC_SliderHandle, this);
		QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &o, QStyle::SC_SliderGroove, this);

		o.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
		if (tickPosition() != NoTicks) {
			o.subControls |= QStyle::SC_SliderTickmarks;
		}

		//QSlider::paintEvent(event);

		//if (grooveSVG != Q_NULLPTR)
		//	grooveSVG->render(&painter, grooveRect);
		//if (knobSVG != Q_NULLPTR)
		//	knobSVG->render(&painter, knobRect);
		//QSvgRenderer knob_renderer(QString(":/player_control_icons/icons/song_progress_play_icon.svg"));
		//knob_renderer.render(&painter, knobRect);


		//QSvgRenderer groove_renderer(QString(":/player_control_icons/icons/right-angled-triangle.svg"));
		//groove_renderer.render(&painter, grooveRect);
		
		QPolygonF tri;
		tri.push_back(grooveRect.bottomLeft());
		tri.push_back(grooveRect.bottomRight());
		tri.push_back(grooveRect.topRight());
		painter.drawConvexPolygon(tri);
	
		//QSlider::paintEvent(event);
	}

	void song_progress_widget::init()
	{
		_is_seek_block = false;
		_mouse_pressed = false;

		_current_playlist_id = mprt::_INVALID_PLAYLIST_ID_;
		_current_playlist_item_id = mprt::_INVALID_PLAYLIST_ITEM_ID_;

		//QString style = NULL;
		//style.append("QSlider::handle:horizontal { image: url(:/player_control_icons/icons/song_progress_play_icon.svg); width: 32px; border-right: 10px solid transparent; border-left: 12px solid transparent; margin-right: -10px; margin-left: -48px; background-clip: content;}");
		//style.append("QSlider::handle:horizontal { image: url(:/player_control_icons/icons/song_progress_play_icon.svg); width: 32px; border-right: 10px solid transparent; border-left: 12px solid transparent; margin-right: -10px; margin-left: -48px; background-clip: content;}");
		//style.append("QSlider::handle:horizontal { image: url(:/player_control_icons/icons/song_progress_play_icon.svg); }");
		//style.append("QSlider::groove:horizontal { border-image: url(:/player_control_icons/icons/right-angled-triangle.svg) 0 0 0 0; border-left: 0px; border-right: 0px;}");
		//style.append("QSlider::groove:horizontal { border-image: url(:/player_control_icons/icons/right-angled-triangle.svg) 0 3 0 0 stretch stretch ; }");
		//setStyleSheet(style);
	}

	void song_progress_widget::seek_to()
	{
		_main_window->_playlist_dock_widget->_playlist_tabwidget->seek_duration(_seek_position);

		_is_seek_block = true;
	}

	QPixmap song_progress_widget::svg_to_pixmap(const QSize &image_size, const QString &svg_file)
	{
		const qreal pixel_ratio = qApp->primaryScreen()->devicePixelRatio(); 
		QSvgRenderer svg_renderer(svg_file);
		QPixmap image(image_size);
		QPainter painter;

		image.fill(Qt::transparent);

		painter.begin(&image);
		svg_renderer.render(&painter);
		painter.end();

		image.setDevicePixelRatio(pixel_ratio);

		return image;
	}

	QString song_progress_widget::get_formatted_time(int msecs)
	{
// 		int hours = msecs / (1000 * 60 * 60);
// 		int minutes = (msecs - (hours * 1000 * 60 * 60)) / (1000 * 60);
// 		int seconds = (msecs - (minutes * 1000 * 60) - (hours * 1000 * 60 * 60)) / 1000;
// 		int milliseconds = msecs - (seconds * 1000) - (minutes * 1000 * 60) - (hours * 1000 * 60 * 60);
// 
// 		QString formattedTime;
// 		if (hours > 0)
// 		{
// 			formattedTime.append(
// 				QString("%1").arg(hours, 2, 10, QLatin1Char('0')) + ":");
// 		}
// 
// 		formattedTime.append(
// 			QString("%1").arg(minutes, 2, 10, QLatin1Char('0')) + ":" +
// 			QString("%1").arg(seconds, 2, 10, QLatin1Char('0')) + "." +
// 			QString("%1").arg(milliseconds, 3, 10, QLatin1Char('0')));
// 
// 		return formattedTime;

		return QString::fromStdString(create_time_format_string<std::ostringstream>(std::chrono::milliseconds(msecs)));
	}

}