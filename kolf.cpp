#include <kconfig.h>
#include <kaction.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kprinter.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kurl.h>

#include <qevent.h>
#include <qfile.h>
#include <qobject.h>
#include <qmap.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qptrlist.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qwidget.h>

#include <stdlib.h>

#include "game.h"
#include "kolf.h"
#include "newgame.h"
#include "scoreboard.h"
#include "editor.h"

Kolf::Kolf()
    : KMainWindow(0)
{
	game = 0;
	editor = 0;
	spacer = 0;
	scoreboard = 0;

	initGUI();

	filename = QString::null;
	dummy = new QWidget(this);
	setCentralWidget(dummy);
	layout = new QGridLayout(dummy, 3, 1);

	new Arts::Dispatcher;

	resize(420, 480);
	applyMainWindowSettings(KGlobal::config(), "TopLevelWindow");

	closeGame();
	newSameAction->setEnabled(false);

	KConfig *config = kapp->config();
	config->setGroup("App");
	if (!config->readBoolEntry("beenRun", false))
	{
		switch (KMessageBox::questionYesNo(this, i18n("Since it's your first time playing Kolf, would you like to play on the tutorial course once?")))
		{
			case KMessageBox::Yes:
				QTimer::singleShot(100, this, SLOT(tutorial()));
				break;
			case KMessageBox::No:
			default:
				break;
		}
		config->writeEntry("beenRun", true);
	}

	config->sync();
}

void Kolf::initGUI()
{
	newAction = KStdAction::openNew(this, SLOT(newGame()), actionCollection());

	newDefaultAction = new KListAction(i18n("Open Default Course"), 0, 0, 0, actionCollection(), "newdefault");
	QStringList items = KGlobal::dirs()->findAllResources("appdata", "courses/*.kolf");
	for (QStringList::Iterator it = items.begin(); it != items.end(); ++it)
	{
		KSimpleConfig cfg(*it);
		cfg.setGroup("0-course@-50,-50");
		QString newName = cfg.readEntry("name", "");
		defaults[newName] = *it;
		*it = newName;
		//(*it) = QFileInfo(*it).baseName();
	}
	newDefaultAction->setItems(items);
	connect(newDefaultAction, SIGNAL(activated(const QString &)), this, SLOT(openDefaultCourse(const QString &)));

	newAction->setText(newAction->text() + QString("..."));
	newSameAction = new KAction(i18n("&Play Course Again..."), "filenew", CTRL+SHIFT+Key_N, this, SLOT(newSameGame()), actionCollection(), "samecourse");
	newSameAction->setStatusText(i18n("Open the last course you opened."));
	endAction = KStdAction::close(this, SLOT(closeGame()), actionCollection());
	endAction->setText(i18n("&Close Current Course"));
	recentAction = KStdAction::openRecent(0, 0, actionCollection());
	recentAction->setText(recentAction->text() + QString("..."));
	connect(recentAction, SIGNAL(urlSelected(const KURL &)), this, SLOT(openRecent(const KURL &)));
	recentAction->loadEntries(KGlobal::config(), "Kolf recent files");
	printAction = KStdAction::print(this, SLOT(print()), actionCollection());

	editingAction = new KToggleAction(i18n("&Edit"), "pencil", CTRL+Key_E, 0, 0, actionCollection(), "editing");
	newHoleAction = new KAction(i18n("&New"), "filenew", CTRL+Key_H, 0, 0, actionCollection(), "newhole");
	clearHoleAction = new KAction(i18n("&Clear"), "locationbar_erase", CTRL+Key_Delete, game, SLOT(clearHole()), actionCollection(), "clearhole");
	resetHoleAction = new KAction(i18n("&Reset"), CTRL+Key_R, 0, 0, actionCollection(), "resethole");

	(void) KStdAction::quit(this, SLOT(close()), actionCollection());
	openAction = KStdAction::open(this, SLOT(open()), actionCollection());
	saveAction = KStdAction::save(this, SLOT(save()), actionCollection());
	saveAsAction = KStdAction::saveAs(this, SLOT(saveAs()), actionCollection());

	holeAction = new KListAction(i18n("Switch To Hole"), 0, 0, 0, actionCollection(), "switchhole");
	nextAction = new KAction(i18n("&Next Hole"), "forward", KStdAccel::key(KStdAccel::Forward), 0, 0, actionCollection(), "nexthole");
	prevAction = new KAction(i18n("&Previous Hole"), "back", KStdAccel::key(KStdAccel::Back), 0, 0, actionCollection(), "prevhole");
	firstAction = new KAction(i18n("&First Hole"), "gohome", KStdAccel::key(KStdAccel::Home), 0, 0, actionCollection(), "firsthole");
	lastAction = new KAction(i18n("&Last Hole"), CTRL+Key_End, 0, 0, actionCollection(), "lasthole");
	randAction = new KAction(i18n("&Random Hole"), "goto", 0, 0, 0, actionCollection(), "randhole");

	useMouseAction = new KToggleAction(i18n("Enable &Mouse for Moving Putter"), 0, 0, 0, actionCollection(), "usemouse");
	connect(useMouseAction, SIGNAL(toggled(bool)), this, SLOT(useMouseChanged(bool)));
	KConfig *config = kapp->config();
	config->setGroup("Settings");
	useMouseAction->setChecked(config->readBoolEntry("useMouse", true));

	useAdvancedPuttingAction = new KToggleAction(i18n("Enable &Advanced Putting"), 0, 0, 0, actionCollection(), "useadvancedputting");
	connect(useAdvancedPuttingAction, SIGNAL(toggled(bool)), this, SLOT(useAdvancedPuttingChanged(bool)));
	useAdvancedPuttingAction->setChecked(config->readBoolEntry("useAdvancedPutting", false));

	aboutAction = new KAction(i18n("&About Course..."), 0, 0, 0, actionCollection(), "aboutcourse");
	tutorialAction = new KAction(i18n("&Tutorial..."), 0, this, SLOT(tutorial()), actionCollection(), "tutorial");

	statusBar();
	createGUI();
}

void Kolf::closeEvent(QCloseEvent *e)
{
	if (game)
		if (game->askSave(true))
			return;
	saveMainWindowSettings(KGlobal::config(), "TopLevelWindow");
	recentAction->saveEntries(KGlobal::config(), "Kolf recent files");
	e->accept();
}

void Kolf::startNewGame()
{
	NewGameDialog *dialog = new NewGameDialog(dummy, "New Game Dialog");

	if (dialog->exec() == QDialog::Accepted)
	{
		//statusBar()->message(i18n("Loading course..."));
		players.clear();
		delete scoreboard;
		scoreboard = new ScoreBoard(dummy, "Score Board");
		layout->addWidget(scoreboard, 1, 0);
		scoreboard->show();
		
		PlayerEditor *curEditor = 0;
		int newId = 1;
		for (curEditor = dialog->players()->first(); curEditor; curEditor = dialog->players()->next())
		{
			players.append(Player());
			players.last().ball()->setColor(curEditor->color());
			players.last().setName(curEditor->name());
			players.last().setId(newId);
			scoreboard->newPlayer(curEditor->name());
			newId++;
		}
		competition = dialog->competition();
		
		delete spacer;
		spacer = 0;
		delete game;
		game = new KolfGame(&players, filename, dummy);
		connect(game, SIGNAL(newHole(int)), scoreboard, SLOT(newHole(int)));
		connect(game, SIGNAL(newHole(int)), this, SLOT(parChanged(int)));
		connect(game, SIGNAL(scoreChanged(int, int, int)), scoreboard, SLOT(setScore(int, int, int)));
		connect(game, SIGNAL(newPlayersTurn(Player *)), this, SLOT(newPlayersTurn(Player *)));
		connect(game, SIGNAL(holesDone()), this, SLOT(gameOver()));
		connect(game, SIGNAL(checkEditing()), this, SLOT(checkEditing()));
		connect(game, SIGNAL(editingStarted()), this, SLOT(editingStarted()));
		connect(game, SIGNAL(editingEnded()), this, SLOT(editingEnded()));
		connect(game, SIGNAL(inPlayStart()), this, SLOT(inPlayStart()));
		connect(game, SIGNAL(inPlayEnd()), this, SLOT(inPlayEnd()));
		connect(game, SIGNAL(maxStrokesReached()), this, SLOT(maxStrokesReached()));
		connect(game, SIGNAL(largestHole(int)), this, SLOT(updateHoleMenu(int)));
		connect(holeAction, SIGNAL(activated(const QString &)), game, SLOT(switchHole(const QString &)));
		connect(nextAction, SIGNAL(activated()), game, SLOT(nextHole()));
		connect(prevAction, SIGNAL(activated()), game, SLOT(prevHole()));
		connect(firstAction, SIGNAL(activated()), game, SLOT(firstHole()));
		connect(lastAction, SIGNAL(activated()), game, SLOT(lastHole()));
		connect(randAction, SIGNAL(activated()), game, SLOT(randHole()));
		connect(editingAction, SIGNAL(activated()), game, SLOT(toggleEditMode()));
		connect(newHoleAction, SIGNAL(activated()), game, SLOT(addNewHole()));
		connect(clearHoleAction, SIGNAL(activated()), game, SLOT(clearHole()));
		connect(resetHoleAction, SIGNAL(activated()), game, SLOT(resetHole()));
		connect(aboutAction, SIGNAL(activated()), game, SLOT(showInfoDlg()));
		connect(useMouseAction, SIGNAL(toggled(bool)), game, SLOT(setUseMouse(bool)));
		connect(useAdvancedPuttingAction, SIGNAL(toggled(bool)), game, SLOT(setUseAdvancedPutting(bool)));		

		game->setUseMouse(useMouseAction->isChecked());
		game->setUseAdvancedPutting(useAdvancedPuttingAction->isChecked());		

		layout->addWidget(game, 0, 0);

		game->show();
		game->setFocus();

		setEditingEnabled(true);
		endAction->setEnabled(true);
		setHoleMovementEnabled(true);
		aboutAction->setEnabled(true);
		printAction->setEnabled(true);
		saveAction->setEnabled(true);
		saveAsAction->setEnabled(true);

		clearHoleAction->setEnabled(false);
		newHoleAction->setEnabled(false);
		openAction->setEnabled(false);
		recentAction->setEnabled(false);
		newAction->setEnabled(false);
		newDefaultAction->setEnabled(false);
		newSameAction->setEnabled(false);
		tutorialAction->setEnabled(false);
		

		game->addFirstHole();
		game->emitLargestHole();
		//statusBar()->message(i18n("Good luck!"));
	}

	delete dialog;
}

void Kolf::open()
{
	QString oldfilename = filename;
	filename = KFileDialog::getOpenFileName(QString::null, QString::fromLatin1("*.kolf"), this, i18n("Pick Kolf Course"));
	if (!filename.isNull())
	{
		recentAction->addURL(QString("file:") + filename);
		startNewGame();
	}
	else
		filename = oldfilename;
}

void Kolf::openRecent(const KURL &url)
{
	if (url.isLocalFile() && QFile::exists(url.path()))
	{
		filename = url.path();
		startNewGame();
	}
	else
	{
		recentAction->removeURL(url);
	}

	recentAction->setCurrentItem(-1);
}

void Kolf::newGame()
{
	filename = QString::null;
	startNewGame();
}

void Kolf::tutorial()
{
	QString newfilename = KGlobal::dirs()->findResource("appdata", "tutorial.kolf");
	if (newfilename.isNull())
		return;
	filename = newfilename;
	startNewGame();
}

void Kolf::openDefaultCourse(const QString &course)
{
	QString newfilename = defaults[course];
	if (newfilename.isNull())
		return;
	filename = newfilename;
	startNewGame();
	newDefaultAction->setCurrentItem(-1);
}

// ahhhhh easy func :-)
void Kolf::newSameGame()
{
	startNewGame();
}

void Kolf::closeGame()
{
	if (game)
	{
		if (game->askSave(true))
			return;
		game->pause();
	}
	editingEnded();
	delete game;
	game = 0;
	delete spacer;
	spacer = new QWidget(dummy);
	layout->addWidget(spacer, 0, 0);
	spacer->setMinimumSize(410, 410);
	spacer->show();

	editingAction->setChecked(false);
	setEditingEnabled(false);
	endAction->setEnabled(false);
	aboutAction->setEnabled(false);
	printAction->setEnabled(false);
	saveAction->setEnabled(false);
	saveAsAction->setEnabled(false);
	setHoleMovementEnabled(false);

	clearHoleAction->setEnabled(false);
	newHoleAction->setEnabled(false);
	openAction->setEnabled(true);
	recentAction->setEnabled(true);
	newAction->setEnabled(true);
	newDefaultAction->setEnabled(true);
	newSameAction->setEnabled(true);
	tutorialAction->setEnabled(true);
}

void Kolf::gameOver()
{
	int lowScore = INT_MAX;
	int curScore = 1;
	QStringList names;
	int i = 1;
	while (curScore != 0)
	{
		QString curName;
		curScore = scoreboard->total(i, curName);

		i++;

		if (curName == i18n("Par") || curScore == 0)
			continue;

		if (curScore < lowScore)
		{
			names.clear();
			lowScore = curScore;
			names.append(curName);
		}
		else if (curScore == lowScore)
			names.append(curName);
	}

	if (names.count() > 1)
	{
		QString winners = names.join(i18n(" and "));
		statusBar()->message(i18n("%1 tied").arg(winners));
	}
	else
	{
		statusBar()->message(i18n("%1 won!").arg(names.first()));
	}

	QTimer::singleShot(700, this, SLOT(closeGame()));
}

void Kolf::save()
{
	if (filename.isNull())
	{
		saveAs();
		return;
	}
	//kdDebug() << "Kolf::save\n";
	if (game)
		game->save();
	game->setFocus();
}

void Kolf::saveAs()
{
	QString newfilename = KFileDialog::getSaveFileName(QString::null, "*.kolf", this, i18n("Pick Kolf Course to Save To"));
	if (!newfilename.isNull())
	{
		filename = newfilename;
		recentAction->addURL(QString("file:") + filename);
		game->setFilename(filename);
		game->save();
		game->setFocus();
	}
}

void Kolf::newPlayersTurn(Player *player)
{
	statusBar()->message(i18n("%1's turn").arg(player->name()));
}

void Kolf::editingStarted()
{
	delete editor;
	editor = new Editor(game->objectList(), dummy, "Editor");
	connect(editor, SIGNAL(addNewItem(Object *)), game, SLOT(addNewObject(Object *)));
	connect(editor, SIGNAL(changed()), game, SLOT(setModified()));
	connect(editor, SIGNAL(addNewItem(Object *)), this, SLOT(setHoleFocus()));
	connect(game, SIGNAL(newSelectedItem(CanvasItem *)), editor, SLOT(setItem(CanvasItem *)));

	scoreboard->hide();

	layout->addWidget(editor, 1, 0);
	editor->show();

	clearHoleAction->setEnabled(true);
	newHoleAction->setEnabled(true);
	setHoleMovementEnabled(false);

	game->setFocus();
}

void Kolf::editingEnded()
{
	delete editor;
	editor = 0;

	if (scoreboard)
		scoreboard->show();

	clearHoleAction->setEnabled(false);
	newHoleAction->setEnabled(false);
	setHoleMovementEnabled(true);

	if (game)
		game->setFocus();
}

void Kolf::inPlayStart()
{
	setEditingEnabled(false);
	setHoleMovementEnabled(false);
}

void Kolf::inPlayEnd()
{
	setEditingEnabled(true);
	setHoleMovementEnabled(true);
}

void Kolf::maxStrokesReached()
{
	statusBar()->message(i18n("Sorry, maximum number of strokes for this hole reached"));
}

void Kolf::updateHoleMenu(int largest)
{
	QStringList items;
	for (int i = 1; i <= largest; ++i)
		items.append(QString::number(i));

	// setItems for some reason enables the action
	bool shouldbe = holeAction->isEnabled();
	holeAction->setItems(items);
	holeAction->setEnabled(shouldbe);
}

void Kolf::setHoleMovementEnabled(bool yes)
{
	if (competition)
		yes = false;

	holeAction->setEnabled(yes);

	nextAction->setEnabled(yes);
	prevAction->setEnabled(yes);
	firstAction->setEnabled(yes);
	lastAction->setEnabled(yes);
	randAction->setEnabled(yes);

	resetHoleAction->setEnabled(yes);
}

void Kolf::setEditingEnabled(bool yes)
{
	editingAction->setEnabled(competition? false : yes);
}

void Kolf::checkEditing()
{
	editingAction->setChecked(true);
}

void Kolf::print()
{
	KPrinter pr;
	if (pr.setup())
	{
		pr.newPage();
		QPainter p(&pr);
		if (game)
			game->print(p);
	}
}

void Kolf::useMouseChanged(bool yes)
{
	KConfig *config = kapp->config();
	config->setGroup("Settings");
	config->writeEntry("useMouse", yes);
	config->sync();
}

void Kolf::useAdvancedPuttingChanged(bool yes)
{
	KConfig *config = kapp->config();
	config->setGroup("Settings");
	config->writeEntry("useAdvancedPutting", yes);
	config->sync();
}

#include "kolf.moc"
