#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kscoredialog.h>
#include <kstandarddirs.h>
#include <kseparator.h>
#include <klineedit.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurllabel.h>

#include <qcheckbox.h>
#include <qevent.h>
#include <qpen.h>
#include <qlayout.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <klistbox.h>
#include <qstyle.h>
#include <qrect.h>
#include <qmap.h>
#include <qpainter.h>
#include <qpixmapcache.h>
#include <qwidget.h>
#include <qscrollarea.h>
#include <qstringlist.h>
#include <qstring.h>

#include "newgame.h"
#include "game.h"

NewGameDialog::NewGameDialog(bool enableCourses, QWidget *parent, const char *_name)
	: KDialogBase(KDialogBase::TreeList, i18n("New Game"), Ok | Cancel, Ok, parent, _name)
{
	this->enableCourses = enableCourses;

	KConfig *config = kapp->config();

	// lots o' colors :)
	startColors << Qt::yellow << Qt::blue << Qt::red << Qt::lightGray << Qt::cyan << Qt::darkBlue << Qt::magenta << Qt::darkGray << Qt::darkMagenta << Qt::darkYellow;

	playerPage = addPage(i18n("Players"));
	QVBoxLayout *bigLayout = new QVBoxLayout(playerPage, marginHint(), spacingHint());

	addButton = new KPushButton(i18n("&New Player"), playerPage);
	bigLayout->addWidget(addButton);

	connect(addButton, SIGNAL(clicked()), this, SLOT(addPlayer()));

	scroller = new QScrollArea(playerPage);
	bigLayout->addWidget(scroller);
	playersWidget = new QWidget(playerPage);
	scroller->setWidget(playersWidget);
	new QVBoxLayout(playersWidget);
	if (!QPixmapCache::find("grass", grass))
	{
		grass.load(locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", grass);
	}
	scroller->setBackgroundPixmap(grass);

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
		addPlayer();
		addPlayer();
	}

	enableButtons();

	if (enableCourses)
	{
		coursePage = addPage(i18n("Course"), i18n("Choose Course to Play"));
		QVBoxLayout *coursePageLayout = new QVBoxLayout(coursePage, marginHint(), spacingHint());

		KURLLabel *coursesLink = new KURLLabel("http://katzbrown.com/kolf/Courses/User Uploaded/", "katzbrown.com/kolf/Courses/User Uploaded/", coursePage);
		connect(coursesLink, SIGNAL(leftClickedURL(const QString &)), kapp, SLOT(invokeBrowser(const QString &)));
		coursePageLayout->addWidget(coursesLink);

		QHBoxLayout *hlayout = new QHBoxLayout(coursePageLayout, spacingHint());

		// following use this group
		config->setGroup("New Game Dialog Mode");

		// find other courses
		externCourses = config->readListEntry("extra");

		/// course loading
		QStringList items = externCourses + KGlobal::dirs()->findAllResources("appdata", "courses/*");
		QStringList nameList;
		const QString lastCourse(config->readEntry("course", ""));
		int curItem = 0;
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
		info[QString::null] = CourseInfo(newName, newName, i18n("You"), 0, 0);
		names.append(QString::null);
		nameList.append(newName);

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
		KPushButton *scores = new KPushButton(i18n("Highscores"), coursePage);
		connect(scores, SIGNAL(clicked()), this, SLOT(showHighscores()));
		detailLayout->addWidget(scores);

		detailLayout->addStretch();
		detailLayout->addWidget(new KSeparator(coursePage));

		minorLayout = new QHBoxLayout(detailLayout, spacingHint());

		KPushButton *addCourseButton = new KPushButton(i18n("Add..."), coursePage);
		minorLayout->addWidget(addCourseButton);
		connect(addCourseButton, SIGNAL(clicked()), this, SLOT(addCourse()));

		remove = new KPushButton(i18n("Remove"), coursePage);
		minorLayout->addWidget(remove);
		connect(remove, SIGNAL(clicked()), this, SLOT(removeCourse()));

		courseSelected(curItem);
		selectionChanged();
	}
	
	// options page
	optionsPage = addPage(i18n("Options"), i18n("Game Options"));
	QVBoxLayout *vlayout = new QVBoxLayout(optionsPage, marginHint(), spacingHint());

	mode = new QCheckBox(i18n("&Strict mode"), optionsPage);
	vlayout->addWidget(mode);
	mode->setChecked(config->readBoolEntry("competition", false));

	QLabel *desc = new QLabel(i18n("In strict mode, undo, editing, and switching holes is not allowed. This is generally for competition. Only in strict mode are highscores kept."), optionsPage);
	desc->setTextFormat(Qt::RichText);
	vlayout->addWidget(desc);
}

NewGameDialog::~NewGameDialog()
{
	qDeleteAll(editors);
}

void NewGameDialog::slotOk()
{
	KConfig *config = kapp->config();

	config->setGroup("New Game Dialog Mode");
	config->writeEntry("competition", mode->isChecked());
	if (enableCourses)
	{
		config->writeEntry("course", currentCourse);
		config->writeEntry("extra", externCourses);
	}

	config->deleteGroup("New Game Dialog");
	config->setGroup("New Game Dialog");

	PlayerEditor *curEditor = 0;
	int i = 0;
	for (curEditor = editors.at(i); i < editors.count(); ++i)
		config->writeEntry(QString::number(i) + curEditor->name(), curEditor->color().name());

	config->sync();

	KDialogBase::slotOk();
}

void NewGameDialog::courseSelected(int index)
{
	currentCourse = names.at(index);

	CourseInfo &curinfo = info[currentCourse];

	name->setText(QString("<strong>%1</strong>").arg(curinfo.name));

	author->setText(i18n("By %1").arg(curinfo.author));
	par->setText(i18n("Par %1").arg(curinfo.par));
	holes->setText(i18n("%1 Holes").arg(curinfo.holes));
}

void NewGameDialog::showHighscores()
{
	KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
	scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), "Par");
	scoreDialog->setConfigGroup(info[currentCourse].untranslatedName + QString(" Highscores"));
	scoreDialog->setComment(i18n("High Scores for %1").arg(info[currentCourse].name));
	scoreDialog->show();
}

void NewGameDialog::removeCourse()
{
	int curItem = courseList->currentItem();
	if (curItem < 0)
		return;

	QString file = names.at(curItem);
	if (!externCourses.contains(file))
		return;

	names.remove(file);
	externCourses.remove(file);
	courseList->removeItem(curItem);

	selectionChanged();
}

void NewGameDialog::selectionChanged()
{
	const int curItem = courseList->currentItem();
	remove->setEnabled(!(curItem < 0 || !externCourses.contains(names.at(curItem))));
}

void NewGameDialog::addCourse()
{
	QStringList files = KFileDialog::getOpenFileNames(":kourses", QString::fromLatin1("application/x-kourse"), this, i18n("Pick Kolf Course"));

	bool hasDuplicates = false;

	for (QStringList::Iterator fileIt = files.begin(); fileIt != files.end(); ++fileIt)
	{
		if (names.contains(*fileIt) > 0)
		{
			hasDuplicates = true;
			continue;
		}

		CourseInfo curinfo;
		KolfGame::courseInfo(curinfo, *fileIt);
		info[*fileIt] = curinfo;
		names.prepend(*fileIt);
		externCourses.prepend(*fileIt);

		courseList->insertItem(curinfo.name, 0);
	}
	
	if (hasDuplicates)
		KMessageBox::information(this, i18n("Chosen course is already on course list."));

	courseList->setCurrentItem(0);
	courseSelected(0);
	selectionChanged();
}

void NewGameDialog::addPlayer()
{
	if (editors.count() >= startColors.count())
		return;

	
	PlayerEditor *pe = new PlayerEditor(i18n("Player %1").arg(editors.count() + 1), startColors.at(editors.count()), playersWidget);
	editors.append(pe);
	pe->show();
	playersWidget->layout()->addWidget(pe);
	connect(pe, SIGNAL(deleteEditor(PlayerEditor *)), this, SLOT(deleteEditor(PlayerEditor *)));

	enableButtons();
	playersWidget->setMinimumSize(playersWidget->sizeHint());
}

void NewGameDialog::deleteEditor(PlayerEditor *editor)
{
	if (editors.count() < 2)
		return;

	editors.remove(editor);
	delete editor;

	enableButtons();
	playersWidget->setMinimumSize(playersWidget->sizeHint());
	playersWidget->resize(playersWidget->sizeHint());
}

void NewGameDialog::enableButtons()
{
	addButton->setEnabled(!(editors.count() >= startColors.count()));
}

/////////////////////////

PlayerEditor::PlayerEditor(QString startName, QColor startColor, QWidget *parent, const char *_name)
	: QWidget(parent, _name)
{
	QHBoxLayout *layout = new QHBoxLayout(this, KDialogBase::spacingHint());

	if (!QPixmapCache::find("grass", grass))
	{
		grass.load(locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", grass);
	}
	setBackgroundPixmap(grass);

	editor = new KLineEdit(this);
	layout->addWidget(editor);
	editor->setFrame(false);
	editor->setText(startName);
	layout->addStretch();
	layout->addWidget(colorButton = new KColorButton(startColor, this));
#warning setAutoMask does not exists in Qt4 port
//	colorButton->setAutoMask(true);
	colorButton->setBackgroundPixmap(grass);

	KPushButton *remove = new KPushButton(i18n("Remove"), this);
#warning setAutoMask does not exists in Qt4 port
//	remove->setAutoMask(true);
	layout->addWidget(remove);
	remove->setBackgroundPixmap(grass);
	connect(remove, SIGNAL(clicked()), this, SLOT(removeMe()));
}

void PlayerEditor::removeMe()
{
	emit deleteEditor(this);
}

#include "newgame.moc"
