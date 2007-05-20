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

#include "editor.h"

#include <kdialog.h>
#include <k3listbox.h>

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "game.h"

Editor::Editor(ObjectList *list, QWidget *parent)
	: QWidget(parent)
{
	this->list = list;
	config = 0;

	hlayout = new QHBoxLayout(this);
        hlayout->setMargin( KDialog::marginHint() );
        hlayout->setSpacing( KDialog::spacingHint() );

	QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setSpacing( KDialog::spacingHint() );
        hlayout->addLayout( vlayout );
	vlayout->addWidget(new QLabel(i18n("Add object:"), this));
	listbox = new K3ListBox(this, "Listbox");
	vlayout->addWidget(listbox);
	hlayout->setStretchFactor(vlayout, 2);

	QStringList items;
	QList<Object *>::const_iterator obj;
	for (obj = list->constBegin(); obj != list->constEnd(); ++obj)
		items.append((*obj)->name());

	listbox->insertStringList(items);

	connect(listbox, SIGNAL(executed(Q3ListBoxItem *)), SLOT(listboxExecuted(Q3ListBoxItem *))); //Q3ListBoxItem used here because that is what KListBox uses
}

void Editor::listboxExecuted(Q3ListBoxItem * /*item*/) //again, Q3ListBoxItem used here because that is what KListBox uses
{
	int curItem = listbox->currentItem();
	if (curItem < 0)
		return;

	emit addNewItem(list->at(curItem));
}

void Editor::setItem(CanvasItem *item)
{
	delete config;
	config = item->config(this);
	if (!config)
		return;
	config->ctorDone();
	hlayout->addWidget(config);
	hlayout->setStretchFactor(config, 2);
	config->setFrameStyle(QFrame::Box | QFrame::Raised);
	config->setLineWidth(1);
	config->show();
	connect(config, SIGNAL(modified()), this, SIGNAL(changed()));
}

#include "editor.moc"
