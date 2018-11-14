#include <memory>

#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDirIterator>
#include <QDialog>
#include <QPushButton>

#include "dock_widget.h"

#include "common/utils.h"

#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"
#include "ui_plugin_qt.h"
#include "mainwindow.h"
#include "playlist_dock_widget.h"
#include "playlist_tabwidget.h"
#include "playlist_view.h"
#include "playlist_view_item_model.h"
#include "song_progress_widget.h"

namespace mprt_ui_plugin_qt
{
	playlist_tabwidget::playlist_tabwidget(QWidget *parent /*= Q_NULLPTR*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
		: QMainWindow(parent, flags)
		, _ui_plugin_qt(nullptr)
		, _playlist_dock_widget(qobject_cast<playlist_dock_widget*>(parent))
		, _current_play_widget(nullptr)
	{
		qRegisterMetaType<size_type>("size_type");
		qRegisterMetaType<mprt::playlist_id_t>("mprt::playlist_id_t");
		qRegisterMetaType<mprt::playlist_item_id_t>("mprt::playlist_item_id_t");
		qRegisterMetaType<mprt::playlist_item_shr_list_t>("mprt::playlist_item_shr_list_t");
		qRegisterMetaType<std::shared_ptr<mprt::playlist_item_shr_list_t>>("std::shared_ptr<mprt::playlist_item_shr_list_t>");

		setCentralWidget(nullptr);
		setDockOptions(dockOptions() | QMainWindow::GroupedDragging | QMainWindow::ForceTabbedDocks | QMainWindow::AnimatedDocks);
		setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
	}

	playlist_tabwidget::~playlist_tabwidget()
	{
		if (_ui_plugin_qt)
		{
			_ui_plugin_qt->_playlist_management_plugin->unregister_add_playlist_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_add_playlist_item_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_move_item_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_remove_item_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_start_playlist_item_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_progress_playlist_item_callback(_ui_plugin_qt->plugin_name());
		}
	}


	void playlist_tabwidget::add_playlist(QString const& playlist_name)
	{
		QFile _playlist_file(playlist_name);
		if (!_playlist_file.exists())
			return;

		_ui_plugin_qt->_playlist_management_plugin->add_playlist(playlist_name.toStdString());
	}

	void playlist_tabwidget::add_files(QStringList const& urls)
	{
		if (urls.empty())
		{
			return;
		}

		std::vector<std::string> url_list;
		for (auto & url : urls)
		{
			url_list.push_back(url.toStdString());
		}

		add_files(url_list);
	}

	void playlist_tabwidget::add_files(std::vector<std::string> const& urls)
	{
		if (_current_play_widget)
		{
			_ui_plugin_qt->_playlist_management_plugin->add_playlist_items(
				_current_play_widget->playlist_id(), _current_play_widget->get_next_selected_playlist_item_id(), urls);
		}
		else
		{
			_ui_plugin_qt->_playlist_management_plugin->add_playlist(urls);
		}
	}

	void playlist_tabwidget::add_urls(QList<QUrl> const& urls)
	{
		if (urls.empty())
		{
			return;
		}

		auto
			iter = urls.begin(),
			iter_end = urls.end();

		std::vector<std::string> files;
		for (; iter != iter_end; ++iter)
		{
			auto fname = iter->toString(QUrl::PreferLocalFile);
			files.push_back(fname.toStdString());
		}

		add_files(files);
	}

	void playlist_tabwidget::play()
	{
		play_item_offset(0);
	}

	void playlist_tabwidget::stop()
	{
		if (_current_play_widget)
		{
			_ui_plugin_qt->_playlist_management_plugin->stop_play();
		}
	}

	void playlist_tabwidget::pause()
	{
		_ui_plugin_qt->_playlist_management_plugin->pause();
	}

	void playlist_tabwidget::unpause()
	{
		_ui_plugin_qt->_playlist_management_plugin->cont();
	}

	void playlist_tabwidget::previous_item()
	{
		play_item_offset(-1);
	}

	void playlist_tabwidget::next_item()
	{
		play_item_offset(+1);
	}

	void playlist_tabwidget::set_playlist_dock_widget(playlist_dock_widget *pl_dock_widget)
	{
		_playlist_dock_widget = pl_dock_widget;
	}

	std::pair<mprt::playlist_id_t, mprt::playlist_item_id_t> playlist_tabwidget::get_current_play_ids()
	{
		if (_current_play_widget)
		{
			return std::make_pair(_current_play_widget->playlist_id(), _playlist_dock_widget->_main_window->_song_progress_widget->_current_playlist_item_id);
		}

		return std::make_pair(mprt::_INVALID_PLAYLIST_ID_, mprt::_INVALID_PLAYLIST_ITEM_ID_);
	}

	void playlist_tabwidget::seek_duration(size_type duration_ms)
	{
		auto curret_play_ids = get_current_play_ids();
		_ui_plugin_qt->_playlist_management_plugin->seek_duration(curret_play_ids.first, curret_play_ids.second, duration_ms);
	}

	size_type playlist_tabwidget::get_duration_ms(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id)
	{
		auto iter = _playlist_views.find(playlist_id);

		if (iter != _playlist_views.end())
		{
			return iter->second->get_duration_ms(playlist_item_id);
		}

		return -1;
	}

	void playlist_tabwidget::slot_dock_vis_changed(bool visible)
	{
		if (visible)
		{
			auto sender_dock = qobject_cast<QDockWidget*>(sender());
			_current_play_widget = qobject_cast<playlist_view*>(sender_dock->widget());
		}
	}

	void playlist_tabwidget::set_ui_plugin_qt(ui_plugin_qt * ui_plug_qt)
	{
		_ui_plugin_qt = ui_plug_qt;

		_ui_plugin_qt->_playlist_management_plugin->register_add_playlist_callback(
			_ui_plugin_qt->plugin_name(),
			std::bind(&playlist_tabwidget::add_playlist_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		_ui_plugin_qt->_playlist_management_plugin->register_add_playlist_item_callback(
			_ui_plugin_qt->plugin_name(),
			std::bind(&playlist_tabwidget::add_playlist_item_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		_ui_plugin_qt->_playlist_management_plugin->register_move_item_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::move_playlist_item_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		_ui_plugin_qt->_playlist_management_plugin->register_remove_item_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::remove_playlist_item_cb, this, std::placeholders::_1, std::placeholders::_2));

		_ui_plugin_qt->_playlist_management_plugin->register_start_playlist_item_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::start_playlist_item_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		_ui_plugin_qt->_playlist_management_plugin->register_progress_playlist_item_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::progress_playlist_item_cb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}

	void playlist_tabwidget::add_playlist_cb(std::string playlist_name, mprt::playlist_id_t playlist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		QMetaObject::invokeMethod(
			this,
			"add_playlist_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(QString, QString::fromStdString(playlist_name)),
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(std::shared_ptr<mprt::playlist_item_shr_list_t>, items));
	}

	Q_INVOKABLE void playlist_tabwidget::add_playlist_qt_cb(QString playlist_name, mprt::playlist_id_t playlist_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		
		auto playlist_view_inst = new playlist_view();
		playlist_view_inst->set_tab_widget(this);
		_playlist_views[playlist_id] = playlist_view_inst;
		auto dock_w = new dock_widget(this);
		//dock_w->setWindowFlags(
		//	static_cast<Qt::WindowFlags>(/*Qt::Window | Qt::WindowStaysOnTopHint |
		//		Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint |*/ Qt::WindowTitleHint)
		//	/*Qt::Window |  Qt::Window | Qt::FramelessWindowHint | Qt::CustomizeWindowHint | *//*~Qt::WindowTitleHint*/);
		//auto w = new QDockWidget();
		//w->setWindowTitle(playlist_name);
		//dock_w->setTitleBarWidget(w);
		//QWidget *title_bar = new QWidget();
		//QHBoxLayout* layout = new QHBoxLayout();
		//title_bar->setLayout(layout);
		//auto w = new QWidget();
		//w->setWindowTitle(playlist_name);
		//layout->addWidget(w);
		//title_bar->resize(0, 0);
		//dock_w->setTitleBarWidget(title_bar);
		//dock_w->setTitleBarWidget(nullptr);
		connect(dock_w, SIGNAL(visibilityChanged(bool)), SLOT(slot_dock_vis_changed(bool)));
		dock_w->setWindowTitle(playlist_name);
		_current_play_widget = playlist_view_inst;
		addDockWidget(Qt::TopDockWidgetArea, dock_w);
		dock_w->setWidget(playlist_view_inst);
		playlist_view_inst->add_playlist(playlist_id, items);
	}

	void playlist_tabwidget::add_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id_t, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		if (items->size() > 0)
		{
			QMetaObject::invokeMethod(
				this,
				"add_playlist_item_qt_cb",
				Qt::QueuedConnection,
				Q_ARG(mprt::playlist_id_t, playlist_id),
				Q_ARG(mprt::playlist_item_id_t, from_playlist_item_id_t),
				Q_ARG(std::shared_ptr<mprt::playlist_item_shr_list_t>, items));
		}
	}

	Q_INVOKABLE void playlist_tabwidget::add_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, std::shared_ptr<mprt::playlist_item_shr_list_t> items)
	{
		auto iter = _playlist_views.find(playlist_id);

		if (iter != _playlist_views.end())
		{
			iter->second->add_items(from_playlist_item_id, items);
		}
	}

	void playlist_tabwidget::move_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id)
	{
		QMetaObject::invokeMethod(
			this,
			"move_playlist_item_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(mprt::playlist_item_id_t, from_playlist_item_id),
			Q_ARG(mprt::playlist_item_id_t, to_playlist_item_id));
	}

	Q_INVOKABLE void playlist_tabwidget::move_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t from_playlist_item_id, mprt::playlist_item_id_t to_playlist_item_id)
	{
		auto iter = _playlist_views.find(playlist_id);

		if (iter != _playlist_views.end())
		{
			iter->second->move_playlist_item(from_playlist_item_id, to_playlist_item_id);
		}
	}


	void playlist_tabwidget::remove_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id)
	{
		QMetaObject::invokeMethod(
			this,
			"remove_playlist_item_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(mprt::playlist_item_id_t, playlist_item_id));
	}

	Q_INVOKABLE void playlist_tabwidget::remove_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id)
	{
		auto iter = _playlist_views.find(playlist_id);

		if (iter != _playlist_views.end())
		{
			iter->second->remove_playlist_item(playlist_item_id);
		}
	}

	void playlist_tabwidget::start_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type duration_ms)
	{
		QMetaObject::invokeMethod(
			this,
			"start_playlist_item_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(mprt::playlist_item_id_t, playlist_item_id),
			Q_ARG(size_type, duration_ms));
	}

	Q_INVOKABLE void playlist_tabwidget::start_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type duration_ms)
	{
		_ui_plugin_qt->_current_state = to_underlying(plugin_states::play);
	}

	void playlist_tabwidget::progress_playlist_item_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms)
	{
		QMetaObject::invokeMethod(
			this,
			"progress_playlist_item_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(mprt::playlist_item_id_t, playlist_item_id),
			Q_ARG(size_type, position_ms));
	}

	Q_INVOKABLE void playlist_tabwidget::progress_playlist_item_qt_cb(mprt::playlist_id_t playlist_id, mprt::playlist_item_id_t playlist_item_id, size_type position_ms)
	{
		if (_playlist_dock_widget->_main_window->_song_progress_widget->progress(playlist_id, playlist_item_id, position_ms))
		{
			auto play_iter = _playlist_views.find(playlist_id);
			if (play_iter != _playlist_views.end())
			{
				play_iter->second->_playlist_view_item_model->set_current_playing_item_id(playlist_item_id);

			}

		}
	}

	void playlist_tabwidget::play_item_offset(size_type offset)
	{
		if (_current_play_widget)
		{
			_ui_plugin_qt->_playlist_management_plugin->stop_play();
			auto sel_row = _current_play_widget->selected_row();
			sel_row = bound_val(0, sel_row + static_cast<int>(offset), _current_play_widget->_playlist_view_item_model->rowCount() - 1);
			_current_play_widget->selectRow(sel_row);
			auto item_id = _current_play_widget->get_item_playlist_id(sel_row);
			_ui_plugin_qt->_playlist_management_plugin->start_play(_current_play_widget->playlist_id(), item_id);
			_current_play_widget->_playlist_view_item_model->set_current_playing_item_id(item_id);
		}
	}

	std::vector<std::string> playlist_tabwidget::get_recursive_filenames(QString const & dirname)
	{
		std::vector<std::string> files;
		QDirIterator it(dirname, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			QFileInfo f_info(it.next());
			if (f_info.isFile())
			{
				files.push_back(f_info.absoluteFilePath().toStdString());
			}
		}

		return files;
	}

}

