#include <kdialog.h>
#include <klistbox.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>
#include <q3frame.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "editor.h"
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
	listbox = new KListBox(this, "Listbox");
	vlayout->addWidget(listbox);
	hlayout->setStretchFactor(vlayout, 2);

	QStringList items;
	Object *obj = 0;
	for (obj = list->first(); obj; obj = list->next())
		items.append(obj->name());

	listbox->insertStringList(items);

	connect(listbox, SIGNAL(executed(Q3ListBoxItem *)), SLOT(listboxExecuted(Q3ListBoxItem *)));
}

void Editor::listboxExecuted(Q3ListBoxItem * /*item*/)
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
	config->setFrameStyle(Q3Frame::Box | Q3Frame::Raised);
	config->setLineWidth(1);
	config->show();
	connect(config, SIGNAL(modified()), this, SIGNAL(changed()));
}

#include "editor.moc"
