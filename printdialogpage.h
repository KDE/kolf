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

#ifndef PRINTDIALOGPAGE_H
#define PRINTDIALOGPAGE_H

#include <kdeprint/kprintdialogpage.h>
#include <QMap>

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
