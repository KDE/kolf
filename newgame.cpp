#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kscoredialog.h>
#include <kstandarddirs.h>
#include <kseparator.h>
#include <klineedit.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurllabel.h>
#include <ktoolinvocation.h>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <klistbox.h>
#include <QScrollArea>
#include <QPixmapCache>

#include "newgame.h"
#include "game.h"

NewGameDialog::NewGameDialog(bool enableCourses, QWidget *parent, const char *_name)
	: KPageDialog(parent)
{
	setCaption(i18n("New Game"));
	setButtons(Ok | Cancel);
	setDefaultButton(Ok);
	setFaceType(KPageDialog::Tree);
	this->enableCourses = enableCourses;

	KConfig *config = KGlobal::config();

	// lots o' colors :)
	startColors << Qt::yellow << Qt::blue << Qt::red << Qt::lightGray << Qt::cyan << Qt::darkBlue << Qt::magenta << Qt::darkGray << Qt::darkMagenta << Qt::darkYellow;

	playerPage = new QFrame();
    addPage(playerPage, i18n("Players"));

	QVBoxLayout *bigLayout = new QVBoxLayout(playerPage);
        bigLayout->setMargin( marginHint() );
        bigLayout->setSpacing( spacingHint() );

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
		grass.load(KStandardDirs::locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", grass);
	}
        QPalette palette;
        palette.setBrush( scroller->backgroundRole(), QBrush( grass ) );
        scroller->setPalette( palette );

	QMap<QString, QString> entries = config->entryMap("New Game Dialog");
	int i = 0;
	for (QMap<QString, QString>::Iterator it = entries.begin(); it != entries.end(); ++it)
	{
		if (i > startColors.count())
			return;

		addPlayer();
		editors.last()->setName(it.key().right(it.key().length() - 1));
		editors.last()->setColor(QColor(it.value()));
		++i;
	}

	if (editors.isEmpty())
	{
		addPlayer();
		addPlayer();
	}

	enableButtons();
	KPageWidgetItem *pageItem =0L;
	if (enableCourses)
	{
		coursePage = new QFrame();
		pageItem = new KPageWidgetItem( coursePage, i18n("Choose Course to Play") );
		pageItem->setHeader(i18n("Course"));
		addPage(pageItem);
		QVBoxLayout *coursePageLayout = new QVBoxLayout(coursePage);
                coursePageLayout->setMargin( marginHint() );
                coursePageLayout->setSpacing( spacingHint() );

		KUrlLabel *coursesLink = new KUrlLabel("http://katzbrown.com/kolf/Courses/User Uploaded/", "katzbrown.com/kolf/Courses/User Uploaded/", coursePage);
		connect(coursesLink, SIGNAL(leftClickedURL(const QString &)), this, SLOT(invokeBrowser(const QString &)));
		coursePageLayout->addWidget(coursesLink);

		QHBoxLayout *hlayout = new QHBoxLayout;
                hlayout->setSpacing( spacingHint() );
                coursePageLayout->addLayout( hlayout );

		// following use this group
		config->setGroup("New Game Dialog Mode");

		// find other courses
		externCourses = config->readEntry("extra",QStringList());

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

		QVBoxLayout *detailLayout = new QVBoxLayout;
                detailLayout->setSpacing( spacingHint() );
                hlayout->addLayout( detailLayout );
		name = new QLabel(coursePage);
		detailLayout->addWidget(name);
		author = new QLabel(coursePage);
		detailLayout->addWidget(author);

		QHBoxLayout *minorLayout = new QHBoxLayout;
                minorLayout->setSpacing( spacingHint() );
                detailLayout->addLayout( minorLayout );
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

		minorLayout = new QHBoxLayout;
                minorLayout->setSpacing( spacingHint() );
                detailLayout->addLayout( minorLayout );

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
    optionsPage = new QFrame();
    pageItem = new KPageWidgetItem( optionsPage, i18n("Game Options") );
    pageItem->setHeader(i18n("Options"));
    addPage(pageItem);

	QVBoxLayout *vlayout = new QVBoxLayout(optionsPage);
        vlayout->setMargin( marginHint() );
        vlayout->setSpacing( spacingHint() );

	mode = new QCheckBox(i18n("&Strict mode"), optionsPage);
	vlayout->addWidget(mode);
	mode->setChecked(config->readEntry("competition", false));

	QLabel *desc = new QLabel(i18n("In strict mode, undo, editing, and switching holes is not allowed. This is generally for competition. Only in strict mode are highscores kept."), optionsPage);
	desc->setTextFormat(Qt::RichText);
	desc->setWordWrap(true);
	vlayout->addWidget(desc);
}

NewGameDialog::~NewGameDialog()
{
	qDeleteAll(editors);
}

void NewGameDialog::invokeBrowser(const QString &_url)
{
	KToolInvocation::invokeBrowser(_url);
}

void NewGameDialog::slotOk()
{
	KConfig *config = KGlobal::config();

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

	KDialog::accept();
}

void NewGameDialog::courseSelected(int index)
{
	currentCourse = names.at(index);

	CourseInfo &curinfo = info[currentCourse];

	name->setText(QString("<strong>%1</strong>").arg(curinfo.name));

	author->setText(i18n("By %1", curinfo.author));
	par->setText(i18n("Par %1", curinfo.par));
	holes->setText(i18n("%1 Holes", curinfo.holes));
}

void NewGameDialog::showHighscores()
{
	KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
	scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), "Par");
	scoreDialog->setConfigGroup(info[currentCourse].untranslatedName + QString(" Highscores"));
	scoreDialog->setComment(i18n("High Scores for %1", info[currentCourse].name));
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

	names.removeAll(file);
	externCourses.removeAll(file);
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
	QStringList files = KFileDialog::getOpenFileNames( KUrl("kfiledialog:///kourses"),
			 QString::fromLatin1("application/x-kourse"), this, i18n("Pick Kolf Course"));

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


	PlayerEditor *pe = new PlayerEditor(i18n("Player %1", editors.count() + 1), startColors.at(editors.count()), playersWidget);
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

	editors.removeAll(editor);
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

PlayerEditor::PlayerEditor(QString startName, QColor startColor, QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin( KDialog::spacingHint() );

	if (!QPixmapCache::find("grass", grass))
	{
		grass.load(KStandardDirs::locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", grass);
	}
        QPalette palette;
        palette.setBrush( backgroundRole(), QBrush( grass ) );
        setPalette( palette );

	editor = new KLineEdit(this);
	layout->addWidget(editor);
	editor->setFrame(false);
	editor->setText(startName);
	layout->addStretch();
	layout->addWidget(colorButton = new KColorButton(startColor, this));
#warning setAutoMask does not exists in Qt4 port
//	colorButton->setAutoMask(true);
        palette.setBrush( colorButton->backgroundRole(), QBrush( grass ) );
        colorButton->setPalette( palette );

	KPushButton *remove = new KPushButton(i18n("Remove"), this);
#warning setAutoMask does not exists in Qt4 port
//	remove->setAutoMask(true);
	layout->addWidget(remove);
        palette.setBrush( remove->backgroundRole(), QBrush( grass ) );
	remove->setPalette( palette );
	connect(remove, SIGNAL(clicked()), this, SLOT(removeMe()));
}

void PlayerEditor::removeMe()
{
	emit deleteEditor(this);
}

#include "newgame.moc"
