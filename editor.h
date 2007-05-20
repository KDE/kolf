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

#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <QWidget>

#include "game.h"

class K3ListBox;
class QHBoxLayout;
class Q3ListBoxItem;
class Config;

class Editor : public QWidget
{
	Q_OBJECT

public:
	explicit Editor(ObjectList *list, QWidget * = 0);

signals:
	void changed();
	void addNewItem(Object *);

public slots:
	void setItem(CanvasItem *);

private slots:
	void listboxExecuted(Q3ListBoxItem *); //Q3ListBoxItem used here because that is what KListBox uses


private:
	ObjectList *list;
	QHBoxLayout *hlayout;
	K3ListBox *listbox; //note: this uses Q3 listbox
	Config *config;
};

#endif
