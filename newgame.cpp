#include <kapplication.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

#include <qcheckbox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qmap.h>
#include <qwidget.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qstring.h>

#include "newgame.h"

NewGameDialog::NewGameDialog(QWidget *parent, const char *name)
	: KDialogBase(KDialogBase::Plain, i18n("Create Players"), Ok | Cancel, Ok, parent, name)
{
	editors.setAutoDelete(true);
	KConfig *config = kapp->config();

	// lots o' colors :)
	startColors << yellow << blue << red << lightGray << cyan;

	dummy = plainPage();
	dummy->setMinimumSize(250, 300);

	layout = new QVBoxLayout(dummy, 3);

	mode = new QCheckBox(i18n("&Disallow Editing/Hole Movement"), dummy);
	layout->addWidget(mode);

	config->setGroup("New Game Dialog Mode");
	mode->setChecked(config->readBoolEntry("competition", false));

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());

	addButton = new KPushButton(i18n("&New Player"), dummy);
	delButton = new KPushButton(i18n("&Remove Last Player"), dummy);

	hlayout->addWidget(addButton);
	layout->addStretch();
	hlayout->addWidget(delButton);

	connect(addButton, SIGNAL(clicked()), this, SLOT(addPlayer()));
	connect(delButton, SIGNAL(clicked()), this, SLOT(delPlayer()));

	layout->addStretch();

	QMap<QString, QString> entries = config->entryMap("New Game Dialog");
	int i = 0;
	for (QMap<QString, QString>::Iterator it = entries.begin(); it != entries.end(); ++it)
	{
		if (i > startColors.count())
			return;

		addPlayer();
		editors.last()->setName(it.key().right(it.key().length() - 1));
		editors.last()->setColor(QColor(it.data()));
		++i;
	}

	if (editors.isEmpty())
	{
		//kdDebug() << "editors.count() is " << editors.count() << endl;
		addPlayer();
		addPlayer();
	}

	enableButtons();
}

void NewGameDialog::slotOk()
{
	KConfig *config = kapp->config();
	config->setGroup("New Game Dialog Mode");
	config->writeEntry("competition", mode->isChecked());
	config->deleteGroup("New Game Dialog");
	config->setGroup("New Game Dialog");

	PlayerEditor *curEditor = 0;
	int i = 0;
	for (curEditor = editors.first(); curEditor; curEditor = editors.next())
	{
		config->writeEntry(QString::number(i) + curEditor->name(), curEditor->color().name());
		++i;
	}

	config->sync();

	KDialogBase::slotOk();
}

void NewGameDialog::addPlayer()
{
	if (editors.count() >= startColors.count())
		return;

	editors.append(new PlayerEditor(i18n("Player %1").arg(editors.count() + 1), *startColors.at(editors.count()), dummy));
	editors.last()->show();
	layout->addWidget(editors.last());

	enableButtons();
}

void NewGameDialog::delPlayer()
{
	if (editors.count() < 2)
		return;

	editors.removeLast();

	enableButtons();
}

void NewGameDialog::enableButtons()
{
	addButton->setEnabled(!(editors.count() >= startColors.count()));
	delButton->setEnabled(!(editors.count() < 2));
}

/////////////////////////

PlayerEditor::PlayerEditor(QString startName, QColor startColor, QWidget *parent, const char *_name)
	: QWidget(parent, _name)
{
	QHBoxLayout *layout = new QHBoxLayout(this, KDialogBase::spacingHint());

	layout->addWidget(editor = new KLineEdit(this));
	editor->setText(startName);
	layout->addStretch();
	layout->addWidget(colorButton = new KColorButton(startColor, this));
}

#include "newgame.moc"
