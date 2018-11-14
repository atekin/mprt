#include <memory>

#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDirIterator>
#include <QMenu>

#include "common/utils.h"

#include "plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"
#include "ui_plugin_qt.h"
#include "mainwindow.h"
#include "playlist_dock_widget.h"
#include "playlist_tabwidget.h"
#include "playlist_view.h"
#include "playlist_view_item_model.h"
#include "song_progress_widget.h"
#include "tab_bar.h"

namespace mprt_ui_plugin_qt
{
	playlist_tabwidget::playlist_tabwidget(QWidget *parent /*= Q_NULLPTR*/)
		: QTabWidget(parent)
		, _ui_plugin_qt(nullptr)
		, _playlist_dock_widget(qobject_cast<playlist_dock_widget*>(parent))
		, _playlist_context_menu(new QMenu(this))
		, _tab_bar(new tab_bar)
		, _last_current_index(-1)
	{
		qRegisterMetaType<size_type>("size_type");
		qRegisterMetaType<mprt::playlist_id_t>("mprt::playlist_id_t");
		qRegisterMetaType<mprt::playlist_item_id_t>("mprt::playlist_item_id_t");
		qRegisterMetaType<mprt::playlist_item_shr_list_t>("mprt::playlist_item_shr_list_t");
		qRegisterMetaType<std::shared_ptr<mprt::playlist_item_shr_list_t>>("std::shared_ptr<mprt::playlist_item_shr_list_t>");

		_layout = new QHBoxLayout(this);

		setTabBar(_tab_bar);
		_tab_bar->setAcceptDrops(true);
		_tab_bar->setChangeCurrentOnDrag(true);
		_tab_bar->setMovable(true);
		_tab_bar->setContextMenuPolicy(Qt::CustomContextMenu);

		setTabsClosable(true);

		connect(_tab_bar, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(slot_tab_context_menu(const QPoint &)));
		connect(_tab_bar, SIGNAL(sig_tabtext_change_request(int, QString const&)), SLOT(slot_tabtext_change_request(int, QString const&)));
		connect(this, SIGNAL(tabCloseRequested(int)), SLOT(slot_tab_context_remove(int)));
		connect(this, SIGNAL(currentChanged(int)), SLOT(slot_current_changed(int)));

		init_playlist_context_menu();

		setTabShape(QTabWidget::Rounded);
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
			_ui_plugin_qt->_playlist_management_plugin->unregister_rename_playlist_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_remove_playlist_callback(_ui_plugin_qt->plugin_name());
			_ui_plugin_qt->_playlist_management_plugin->unregister_seek_finished_callback(_ui_plugin_qt->plugin_name());
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

		auto url_list = std::make_shared<std::vector<std::string>>();
		for (auto & url : urls)
		{
			url_list->push_back(url.toStdString());
		}

		add_files(url_list);
	}

	void playlist_tabwidget::add_files(std::shared_ptr<std::vector<std::string>> const& urls)
	{
		if (auto cur_widget = current_play_widget())
		{
			_ui_plugin_qt->_playlist_management_plugin->add_playlist_items(
				cur_widget->playlist_id(), cur_widget->get_next_selected_playlist_item_id(), urls);
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

		auto files = std::make_shared<std::vector<std::string>>();
		for (; iter != iter_end; ++iter)
		{
			auto fname = iter->toString(QUrl::PreferLocalFile);
			files->push_back(fname.toStdString());
		}

		add_files(files);
	}

	void playlist_tabwidget::save_playlist(QString const& playlist_name)
	{
		auto w = qobject_cast<playlist_view*>(currentWidget());

		if (w)
		{
			_ui_plugin_qt->_playlist_management_plugin->save_playlist(w->playlist_id(), playlist_name.toStdString());
		}
	}

	void playlist_tabwidget::play()
	{
		_plugin_play_state = plugin_states::play;

		play_item_offset(0);
	}

	void playlist_tabwidget::stop()
	{
		_plugin_play_state = plugin_states::stop;

		_ui_plugin_qt->_playlist_management_plugin->stop_play();
	}

	void playlist_tabwidget::pause()
	{
		if (_plugin_play_state == plugin_states::play)
		{
			_plugin_play_state = plugin_states::pause;

			_ui_plugin_qt->_playlist_management_plugin->pause();
		}

	}

	void playlist_tabwidget::unpause()
	{
		if (_plugin_play_state == plugin_states::pause)
		{
			_plugin_play_state = plugin_states::play;
			_ui_plugin_qt->_playlist_management_plugin->cont();
		}
		else if (_plugin_play_state == plugin_states::stop)
		{
			play();
		}
	}

	void playlist_tabwidget::toggle()
	{
		switch (_plugin_play_state)
		{
		case plugin_states::stop:
			play();
			break;
		case plugin_states::pause:
			unpause();
			break;
		case plugin_states::play:
			pause();
			break;
		case plugin_states::quit:
			break;
		default:
			break;
		}
	}

	void playlist_tabwidget::previous_item()
	{
		play_item_offset(-1);
	}

	void playlist_tabwidget::next_item()
	{
		play_item_offset(+1);
	}

	std::pair<mprt::playlist_id_t, mprt::playlist_item_id_t> playlist_tabwidget::get_current_play_ids()
	{
		if (auto cur_widget = current_play_widget())
		{
			return std::make_pair(cur_widget->playlist_id(), _playlist_dock_widget->_main_window->_song_progress_widget->_current_playlist_item_id);
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

	void playlist_tabwidget::slot_tab_context_menu(const QPoint & point)
	{
		if (point.isNull())
			return;

		_context_menu_tab_index = _tab_bar->tabAt(point);
		_playlist_context_menu->exec(_tab_bar->mapToGlobal(point));
	}

	void playlist_tabwidget::slot_tab_context_rename()
	{
		_tab_bar->slot_start_rename(_context_menu_tab_index);
	}

	void playlist_tabwidget::slot_tab_context_remove()
	{
		slot_tab_context_remove(_context_menu_tab_index);
	}

	void playlist_tabwidget::slot_tab_context_remove(int tab_index)
	{
		_ui_plugin_qt->_playlist_management_plugin->remove_playlist(play_widget(tab_index)->playlist_id());
	}

	void playlist_tabwidget::slot_tabtext_change_request(int tab_index, QString const& text)
	{
		auto p_w = play_widget(tab_index);
		if (p_w)
		{
			_ui_plugin_qt->_playlist_management_plugin->rename_playlist(p_w->playlist_id(), text.toStdString());
		}
	}

	void playlist_tabwidget::slot_current_changed(int tab_index)
	{
		set_close_visible(_last_current_index, false);
		set_close_visible(tab_index, true);
		_last_current_index = tab_index;
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

		_ui_plugin_qt->_playlist_management_plugin->register_rename_playlist_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::rename_playlist_cb, this, std::placeholders::_1, std::placeholders::_2));

		_ui_plugin_qt->_playlist_management_plugin->register_remove_playlist_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::remove_playlist_cb, this, std::placeholders::_1));

		_ui_plugin_qt->_playlist_management_plugin->register_seek_finished_callback(
			ui_plug_qt->plugin_name(),
			std::bind(&playlist_tabwidget::seek_finished_cb, this));
	}

	QHBoxLayout * playlist_tabwidget::layout()
	{
		return _layout;
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
		auto tab_index = addTab(playlist_view_inst, playlist_name);
		playlist_view_inst->add_playlist(playlist_id, items);
		setCurrentIndex(tab_index);
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
		_ui_plugin_qt->_current_state = plugin_states::play;
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

	void playlist_tabwidget::rename_playlist_cb(mprt::playlist_id_t playlist_id, std::string playlist_name)
	{
		QMetaObject::invokeMethod(
			this,
			"rename_playlist_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id),
			Q_ARG(QString, QString::fromStdString(playlist_name)));
	}

	Q_INVOKABLE void playlist_tabwidget::rename_playlist_qt_cb(mprt::playlist_id_t playlist_id, QString playlist_name)
	{
		BOOST_LOG_TRIVIAL(debug) << "rename called: " << playlist_id << " name: " << playlist_name.toStdString();
		_tab_bar->setTabText(tab_index_from_playlist_id(playlist_id), playlist_name);
	}

	void playlist_tabwidget::remove_playlist_cb(mprt::playlist_id_t playlist_id)
	{
		QMetaObject::invokeMethod(
			this,
			"remove_playlist_qt_cb",
			Qt::QueuedConnection,
			Q_ARG(mprt::playlist_id_t, playlist_id));
	}

	Q_INVOKABLE void playlist_tabwidget::remove_playlist_qt_cb(mprt::playlist_id_t playlist_id)
	{
		auto iter = _playlist_views.find(playlist_id);
		if (iter != _playlist_views.end())
		{
			removeTab(tab_index_from_playlist_id(playlist_id));
			delete_ptr(iter->second);
			_playlist_views.erase(playlist_id);
		}
	}

	void playlist_tabwidget::seek_finished_cb()
	{
		QMetaObject::invokeMethod(
			this,
			"seek_finished_qt_cb",
			Qt::QueuedConnection);
	}

	Q_INVOKABLE void playlist_tabwidget::seek_finished_qt_cb()
	{
		_playlist_dock_widget->_main_window->_song_progress_widget->seek_finished();
	}

	void playlist_tabwidget::play_item_offset(size_type offset)
	{
		if (auto curr_widget = current_play_widget())
		{
			_ui_plugin_qt->_playlist_management_plugin->stop_play();
			auto sel_row = curr_widget->selected_row();
			sel_row = bound_val(0, sel_row + static_cast<int>(offset), curr_widget->_playlist_view_item_model->rowCount() - 1);
			curr_widget->selectRow(sel_row);
			auto item_id = curr_widget->get_item_playlist_id(sel_row);
			_ui_plugin_qt->_playlist_management_plugin->start_play(curr_widget->playlist_id(), item_id);
			curr_widget->_playlist_view_item_model->set_current_playing_item_id(item_id);
		}
	}

	playlist_view * playlist_tabwidget::current_play_widget()
	{
		return qobject_cast<playlist_view*>(currentWidget());
	}

	mprt_ui_plugin_qt::playlist_view * playlist_tabwidget::play_widget(int tab_index)
	{
		return qobject_cast<playlist_view*>(widget(tab_index));
	}

	int playlist_tabwidget::tab_index_from_playlist_id(mprt::playlist_id_t playlist_id)
	{
		for (int i = 0; i != _tab_bar->count(); ++i)
		{
			auto p = play_widget(i);
			if (p)
			{
				if (p->playlist_id() == playlist_id)
				{
					return i;
				}
			}
		}

		return -1;
	}

	void playlist_tabwidget::init_playlist_context_menu()
	{
		init_context_menu_action(
			":/player_control_icons/icons/rename_icon.svg",
			"&Rename", "Rename Playlist", this, SLOT(slot_tab_context_rename()));

		init_context_menu_action(
			":/player_control_icons/icons/delete_icon.svg",
			"&Delete", "Delete Playlist", this, SLOT(slot_tab_context_remove()));
	}

	QAction * playlist_tabwidget::init_context_menu_action(
		QString const& icon_file, QString const& action_text,
		QString const& tooltip,
		QObject * receiver,
		const char * slot_func)
	{
		auto action = new QAction(action_text);
		if (!icon_file.isEmpty())
		{
			action->setIcon(QIcon(icon_file));
		}
		action->setToolTip(tooltip);
		action->setStatusTip(tooltip);
		connect(action, SIGNAL(triggered()), receiver, slot_func);
		_playlist_context_menu->addAction(action);
		return action;
	}

	void playlist_tabwidget::set_close_visible(int tab_index, bool visible)
	{
		auto w = _tab_bar->tabButton(tab_index, QTabBar::RightSide);
		if (w)
		{
			w->setVisible(visible);
		}
	}

}

