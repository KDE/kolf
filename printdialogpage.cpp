/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "printdialogpage.h"

#include <QCheckBox>
#include <QVBoxLayout>

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>

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
