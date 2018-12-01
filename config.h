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

#ifndef KOLF_CONFIG_H
#define KOLF_CONFIG_H

#include <QFrame>

class Config : public QFrame
{
	Q_OBJECT

public:
	explicit Config(QWidget *parent);
	void ctorDone();

signals:
	void modified(bool mod);

protected:
	bool startedUp;
	void changed();
};

// this is easy to use to show a message
class MessageConfig : public Config
{
	Q_OBJECT

public:
	MessageConfig(const QString &text, QWidget *parent);
};

// internal
class DefaultConfig : public MessageConfig
{
	Q_OBJECT

public:
	explicit DefaultConfig(QWidget *parent);
};

#endif
