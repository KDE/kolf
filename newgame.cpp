#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kseparator.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kfiledialog.h>

#include <qcheckbox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <klistbox.h>
#include <qstyle.h>
#include <qmap.h>
#include <qpainter.h>
#include <qwidget.h>
#include <qscrollview.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qvbox.h>

#include "newgame.h"
#include "game.h"

void ColorButton::drawButtonLabel(QPainter *painter)
{
	QColor fillCol = isEnabled()? color() : backgroundColor();

	painter->setBrush(fillCol);
	const int w = 16;
	painter->drawEllipse(width() / 2 - w / 2, height() / 2 - w / 2, w, w);
}

/////////////////////////

NewGameDialog::NewGameDialog(QWidget *parent, const char *_name)
	: KDialogBase(KDialogBase::TreeList, i18n("New Game"), Ok | Cancel, Ok, parent, _name)
{
	editors.setAutoDelete(true);
	KConfig *config = kapp->config();

	// lots o' colors :)
	startColors << yellow << blue << red << lightGray << cyan << darkBlue << magenta << darkGray << darkMagenta << darkYellow;

	playerPage = addPage(i18n("Players"));
	QVBoxLayout *bigLayout = new QVBoxLayout(playerPage, marginHint(), spacingHint());

	QHBoxLayout *hlayout = new QHBoxLayout(bigLayout, spacingHint());

	addButton = new KPushButton(i18n("&New Player"), playerPage);
	delButton = new KPushButton(i18n("&Remove Last Player"), playerPage);

	hlayout->addWidget(addButton);
	hlayout->addStretch();
	hlayout->addWidget(delButton);

	connect(addButton, SIGNAL(clicked()), this, SLOT(addPlayer()));
	connect(delButton, SIGNAL(clicked()), this, SLOT(delPlayer()));

	scroller = new QScrollView(playerPage);
	bigLayout->addWidget(scroller);
	layout = new QVBox(scroller->viewport());
	scroller->addChild(layout);

	QMap<QString, QString> entries = config->entryMap("New Game Dialog");
	unsigned int i = 0;
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

	coursePage = addPage(i18n("Course"), i18n("Choose Course to Play"));
	hlayout = new QHBoxLayout(coursePage, marginHint(), spacingHint());

	// following use this group
	config->setGroup("New Game Dialog Mode");

	// find other courses
	externCourses = config->readListEntry("extra");

	/// course loading
	QStringList items = externCourses + KGlobal::dirs()->findAllResources("appdata", "courses/*.kolf");
	QStringList nameList;
	const QString lastCourse(config->readEntry("course", ""));
	int curItem = -1;
	i = 0;
	for (QStringList::Iterator it = items.begin(); it != items.end(); ++it, ++i)
	{
		QString file = *it;
		CourseInfo curinfo;
		KolfGame::courseInfo(curinfo, file);
		info[file] = curinfo;
		names.append(file);
		nameList.append(curinfo.name);

		if (lastCourse == file)
			curItem = i;
	}

	const QString newName(i18n("Create New"));
	info[QString::null] = CourseInfo(newName, i18n("You"), 0, 0);
	names.append(QString::null);
	nameList.append(newName);
	if (curItem < 0)
		curItem = names.count() - 1;

	courseList = new KListBox(coursePage);
	hlayout->addWidget(courseList);
	courseList->insertStringList(nameList);
	courseList->setCurrentItem(curItem);
	connect(courseList, SIGNAL(highlighted(int)), this, SLOT(courseSelected(int)));
	connect(courseList, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

	QVBoxLayout *detailLayout = new QVBoxLayout(hlayout, spacingHint());
	name = new QLabel(coursePage);
	detailLayout->addWidget(name);
	author = new QLabel(coursePage);
	detailLayout->addWidget(author);

	QHBoxLayout *minorLayout = new QHBoxLayout(detailLayout, spacingHint());
	par = new QLabel(coursePage);
	minorLayout->addWidget(par);
	holes = new QLabel(coursePage);
	minorLayout->addWidget(holes);

	detailLayout->addStretch();
	detailLayout->addWidget(new KSeparator(coursePage));

	minorLayout = new QHBoxLayout(detailLayout, spacingHint());

	QPushButton *addCourseButton = new QPushButton(i18n("Add..."), coursePage);
	minorLayout->addWidget(addCourseButton);
	connect(addCourseButton, SIGNAL(clicked()), this, SLOT(addCourse()));

	remove = new QPushButton(i18n("Remove"), coursePage);
	minorLayout->addWidget(remove);
	connect(remove, SIGNAL(clicked()), this, SLOT(removeCourse()));

	courseSelected(curItem);
	selectionChanged();
	
	// options page
	optionsPage = addPage(i18n("Options"), i18n("Game Options"));
	QVBoxLayout *vlayout = new QVBoxLayout(optionsPage, marginHint(), spacingHint());

	mode = new QCheckBox(i18n("&Strict Mode"), optionsPage);
	vlayout->addWidget(mode);
	mode->setChecked(config->readBoolEntry("competition", false));

	QLabel *desc = new QLabel(i18n("In strict mode, neither editing nor switching holes is allowed. This is generally for competition."), optionsPage);
	desc->setTextFormat(RichText);
	vlayout->addWidget(desc);
}

void NewGameDialog::slotOk()
{
	KConfig *config = kapp->config();

	config->setGroup("New Game Dialog Mode");
	config->writeEntry("competition", mode->isChecked());
	config->writeEntry("course", currentCourse);
	config->writeEntry("extra", externCourses);

	config->deleteGroup("New Game Dialog");
	config->setGroup("New Game Dialog");

	PlayerEditor *curEditor = 0;
	int i = 0;
	for (curEditor = editors.first(); curEditor; curEditor = editors.next(), ++i)
		config->writeEntry(QString::number(i) + curEditor->name(), curEditor->color().name());

	config->sync();

	KDialogBase::slotOk();
}

void NewGameDialog::courseSelected(int index)
{
	currentCourse = *names.at(index);

	CourseInfo &curinfo = info[currentCourse];

	name->setText(QString("<strong>%1</strong>").arg(curinfo.name));
	author->setText(i18n("By %1").arg(curinfo.author));
	par->setText(i18n("Par %1").arg(curinfo.par));
	holes->setText(i18n("%1 Holes").arg(curinfo.holes));
}

void NewGameDialog::removeCourse()
{
	int curItem = courseList->currentItem();
	if (curItem < 0)
		return;

	QString file = *names.at(curItem);
	if (externCourses.contains(file) < 1)
		return;

	names.remove(names.at(curItem));
	externCourses.remove(names.at(curItem));
	courseList->removeItem(curItem);
}

void NewGameDialog::selectionChanged()
{
	bool enabled = true;
	int curItem = courseList->currentItem();
	if (curItem < 0)
		enabled = false;
	if (externCourses.contains(*names.at(curItem)) < 1)
		enabled = false;
	remove->setEnabled(enabled);
}

void NewGameDialog::addCourse()
{
	QString file = KFileDialog::getOpenFileName(QString::null, QString::fromLatin1("*.kolf"), this, i18n("Pick Kolf Course"));
	if (file.isNull())
		return;

	if (names.contains(file) > 0)
	{
		KMessageBox::information(this, i18n("Chosen course is already on course list."));
		return;
	}

	CourseInfo curinfo;
	KolfGame::courseInfo(curinfo, file);
	info[file] = curinfo;
	names.prepend(file);
	externCourses.prepend(file);

	courseList->insertItem(curinfo.name, 0);
	courseList->setCurrentItem(0);
	courseSelected(0);
	selectionChanged();
}

void NewGameDialog::addPlayer()
{
	if (editors.count() >= startColors.count())
		return;

	editors.append(new PlayerEditor(i18n("Player %1").arg(editors.count() + 1), *startColors.at(editors.count()), layout));
	editors.last()->show();

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
	layout->addWidget(colorButton = new ColorButton(startColor, this));
}

#include "newgame.moc"
