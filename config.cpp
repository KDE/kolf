/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

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

#include "config.h"

#include <QLabel>
#include <QVBoxLayout>
#include <KDialog>
#include <KLocalizedString>

Config::Config(QWidget *parent)
	: QFrame(parent)
{
	startedUp = false;
}

void Config::ctorDone()
{
	startedUp = true;
}

int Config::spacingHint()
{
	return KDialog::spacingHint() / 2;
}

int Config::marginHint()
{
	return KDialog::marginHint();
}

void Config::changed()
{
	if (startedUp)
		emit modified();
}

MessageConfig::MessageConfig(const QString &text, QWidget *parent)
	: Config(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );
	layout->addWidget(new QLabel(text, this));
}

DefaultConfig::DefaultConfig(QWidget *parent)
	: MessageConfig(i18n("No configuration options"), parent)
{
}


