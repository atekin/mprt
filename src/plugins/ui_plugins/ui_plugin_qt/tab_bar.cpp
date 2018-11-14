#include <boost/log/trivial.hpp>

#include <QLineEdit>

#include "tab_bar.h"

namespace mprt_ui_plugin_qt
{
	tab_bar::tab_bar(QWidget *parent /*= Q_NULLPTR*/)
		: QTabBar(parent)
		, _line_edit(new QLineEdit(this))
	{
		_line_edit->hide();

		connect(_line_edit, SIGNAL(editingFinished()), SLOT(slot_finish_rename()));
		connect(this, SIGNAL(tabBarDoubleClicked(int)), SLOT(slot_start_rename(int)));
		//QString style_sheet("QTabBar::tab {	border-top-left-radius: 4px; border-top-right-radius: 4px;	}");
		//setStyleSheet(style_sheet);
	}

	tab_bar::~tab_bar()
	{

	}

	void tab_bar::slot_start_rename(int tab_index)
	{
		_edited_tab = tab_index;

		//int top_margin = 3;
		//int left_margin = 6;

		auto margings = contentsMargins();
		auto top_margin = margings.top() + 3;
		auto left_margin = margings.left() + 6;

		auto rect = tabRect(tab_index);

		_line_edit->show();
		_line_edit->move(rect.left() + left_margin, rect.top() + top_margin);
		_line_edit->resize(rect.width() - 2 * left_margin, rect.height() - 2 * top_margin);
		_line_edit->setText(tabText(tab_index));
		_line_edit->selectAll();
		_line_edit->setFocus();
	}

	void tab_bar::slot_finish_rename()
	{
		// @INFO: QLineEdit Enter key / hide double signal workaround
		_line_edit->blockSignals(true);
		_line_edit->hide();
		_line_edit->blockSignals(false);
		
		emit sig_tabtext_change_request(_edited_tab, _line_edit->text());
		BOOST_LOG_TRIVIAL(debug) << "finish rename";
	}

}


