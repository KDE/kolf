#ifndef PRINTDIALOGPAGE_H
#define PRINTDIALOGPAGE_H

#include <kdeprint/kprintdialogpage.h>
#include <qmap.h>
#include <qstring.h>

class QCheckBox;
class QWidget;

class PrintDialogPage : public KPrintDialogPage
{
	Q_OBJECT

	public:
		PrintDialogPage(QWidget *parent = 0, const char *name = 0);

		//reimplement virtual functions
		void getOptions(QMap<QString, QString> &opts, bool incldef = false);
		void setOptions(const QMap<QString, QString> &opts);

	private:
		QCheckBox *bgCheck;
};

#endif
