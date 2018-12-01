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

#include "editor.h"
#include "game.h"
#include "itemfactory.h"

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <KLocalizedString>
Editor::Editor(const Kolf::ItemFactory& factory, QWidget *parent)
	: QWidget(parent)
	, m_factory(factory)
{
	config = 0;

	hlayout = new QHBoxLayout(this);

	QVBoxLayout *vlayout = new QVBoxLayout;
	hlayout->addLayout( vlayout );
	vlayout->addWidget(new QLabel(i18n("Add object:"), this));
	m_typeList = new QListWidget(this);
	vlayout->addWidget(m_typeList);
	hlayout->setStretchFactor(vlayout, 2);

	//populate type list
	foreach (const Kolf::ItemMetadata& metadata, factory.knownTypes())
	{
		QListWidgetItem* item = new QListWidgetItem(metadata.name);
		item->setData(Qt::UserRole, metadata.identifier);
		m_typeList->addItem(item);
	}
	connect(m_typeList, &QListWidget::itemClicked, this, &Editor::listboxExecuted);
}

void Editor::listboxExecuted(QListWidgetItem* item)
{
	emit addNewItem(item->data(Qt::UserRole).toString()); //data(UserRole) contains the type identifier
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
	connect(config, &Config::modified, this, &Editor::changed);
}


