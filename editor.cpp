#include <kdialog.h>
#include <klistbox.h>

#include <qlayout.h>
#include <qwidget.h>
#include <qframe.h>

#include "editor.h"
#include "game.h"

Editor::Editor(ObjectList *list, QWidget *parent, const char *name)
	: QWidget(parent, name)
{
	this->list = list;
	config = 0;

	hlayout = new QHBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());

	QVBoxLayout *vlayout = new QVBoxLayout(hlayout, KDialog::spacingHint());
	vlayout->addWidget(new QLabel(i18n("Create Element"), this));
	listbox = new KListBox(this, "Listbox");
	vlayout->addWidget(listbox);
	hlayout->setStretchFactor(vlayout, 2);

	QStringList items;
	Object *obj = 0;
	for (obj = list->first(); obj; obj = list->next())
		items.append(obj->name());

	listbox->insertStringList(items);

	connect(listbox, SIGNAL(executed(QListBoxItem *)), SLOT(listboxExecuted(QListBoxItem *)));
}

void Editor::listboxExecuted(QListBoxItem * /*item*/)
{
	int curItem = listbox->currentItem();
	if (curItem < 0)
		return;

	emit addNewItem(list->at(curItem));

	/*
	listbox->setSelected(curItem, false);
	listbox->setSelected(0, true);
	listbox->setSelected(0, false);
	listbox->setSelected(-1, true);
	*/
	listbox->setFocus();
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
