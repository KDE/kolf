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

#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <QWidget>

#include "game.h"

class QListWidget;
class QHBoxLayout;
class QListWidgetItem;
class Config;

namespace Kolf
{
	class ItemFactory;
}

class Editor : public QWidget
{
	Q_OBJECT
public:
	explicit Editor(const Kolf::ItemFactory& factory, QWidget * = nullptr);
Q_SIGNALS:
	void changed(bool mod);
	void addNewItem(const QString&);
public Q_SLOTS:
	void setItem(CanvasItem *);
private Q_SLOTS:
	void listboxExecuted(QListWidgetItem*);
private:
	const Kolf::ItemFactory& m_factory;
	QHBoxLayout *hlayout;
	QListWidget* m_typeList;
	Config *config;
};

#endif
