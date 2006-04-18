#include <qcheckbox.h>
#include <qlayout.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QVBoxLayout>

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>

#include "printdialogpage.h"

PrintDialogPage::PrintDialogPage(QWidget *parent)
	: KPrintDialogPage( parent )
{
	setTitle(i18n("Kolf Options"));

	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( KDialog::marginHint() );
        layout->setSpacing( KDialog::spacingHint() );

	titleCheck = new QCheckBox(i18n("Draw title text"), this);
	titleCheck->setChecked(true);
	layout->addWidget(titleCheck);
}

void PrintDialogPage::getOptions(QMap<QString, QString> &opts, bool /*incldef*/)
{
	opts["kde-kolf-title"] = titleCheck->isChecked()? "true" : "false";
}

void PrintDialogPage::setOptions(const QMap<QString, QString> &opts)
{
	QString setting = opts["kde-kolf-title"];
	if (!setting.isEmpty())
		titleCheck->setChecked(setting == "true");
}

#include "printdialogpage.moc"
