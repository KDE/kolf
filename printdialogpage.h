#ifndef PRINTDIALOGPAGE_H
#define PRINTDIALOGPAGE_H

#include <kdeprint/kprintdialogpage.h>
#include <QMap>
#include <QString>

class QCheckBox;
class QWidget;

class PrintDialogPage : public KPrintDialogPage
{
	Q_OBJECT

	public:
		PrintDialogPage(QWidget *parent = 0);

		//reimplement virtual functions
		void getOptions(QMap<QString, QString> &opts, bool incldef = false);
		void setOptions(const QMap<QString, QString> &opts);

	private:
		QCheckBox *bgCheck;
		QCheckBox *titleCheck;
};

#endif
