#ifndef tab_bar_h__
#define tab_bar_h__

#include <QTabBar>

class QLineEdit;

namespace mprt_ui_plugin_qt
{

	class tab_bar : public QTabBar
	{
		Q_OBJECT

	public:
		explicit tab_bar(QWidget *parent = Q_NULLPTR);
		~tab_bar();

	signals:
		void sig_tabtext_change_request(int tab_index, QString const& text);

	public slots:
		void slot_start_rename(int tab_index);

	protected slots:
		void slot_finish_rename();

	private:
		QLineEdit * _line_edit;
		int _edited_tab;
	};
}

#endif // tab_bar_h__
