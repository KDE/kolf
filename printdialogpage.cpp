#include <qcheckbox.h>
#include <qlayout.h>
#include <qwidget.h>

#include <klocale.h>
#include <kdialog.h>

#include "printdialogpage.h"

PrintDialogPage::PrintDialogPage(QWidget *parent, const char *name)
	: KPrintDialogPage( parent, name )
{
	setTitle(i18n("Kolf Options"));

	QVBoxLayout *layout = new QVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());

	bgCheck = new QCheckBox(i18n("Draw background grass"), this);
	layout->addWidget(bgCheck);
}

void PrintDialogPage::getOptions(QMap<QString, QString> &opts, bool incldef)
{
	if (incldef || bgCheck->isChecked())
		opts["kde-kolf-background"] = bgCheck->isChecked()? "true" : "false";
}

void PrintDialogPage::setOptions(const QMap<QString, QString> &opts)
{
	bgCheck->setChecked(opts["kde-kolf-background"] == "true");
}

#include "printdialogpage.moc"
