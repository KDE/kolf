#include <kconfig.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kkeydialog.h>
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
#include <qwidget.h>

#include <stdlib.h>

#include "game.h"
#include "newgame.h"
#include "scoreboard.h"
#include "editor.h"
#include "pluginloader.h"
#include "kolf.h"

Kolf::Kolf()
    : KMainWindow(0)
{
	game = 0;
	editor = 0;
	spacer = 0;
	scoreboard = 0;

	initGUI();

	obj = new ObjectList;
	initPlugins();

	filename = QString::null;
	dummy = new QWidget(this);
	setCentralWidget(dummy);
	layout = new QGridLayout(dummy, 3, 1);

	new Arts::Dispatcher;

	resize(420, 480);
	applyMainWindowSettings(KGlobal::config(), "TopLevelWindow");

	closeGame();

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

Kolf::~Kolf()
{
	// wipe out our objects
	obj->setAutoDelete(true);
	delete obj;
}

void Kolf::initGUI()
{
	newAction = KStdAction::openNew(this, SLOT(newGame()), actionCollection(), "game_new");
	newAction->setText(newAction->text() + QString("..."));

	(void) KStdAction::keyBindings(this, SLOT(keyBindings()), actionCollection());
	endAction = KStdAction::close(this, SLOT(closeGame()), actionCollection(), "game_end");
	endAction->setText(i18n("&Close Current Course"));
	printAction = KStdAction::print(this, SLOT(print()), actionCollection(), "game_print");

	(void) KStdAction::quit(this, SLOT(close()), actionCollection(), "game_quit");
	saveAction = KStdAction::save(this, SLOT(save()), actionCollection(), "game_save");
	saveAsAction = KStdAction::saveAs(this, SLOT(saveAs()), actionCollection(), "game_save_as");

	editingAction = new KToggleAction(i18n("&Edit"), "pencil", CTRL+Key_E, 0, 0, actionCollection(), "editing");
	newHoleAction = new KAction(i18n("&New"), "filenew", CTRL+Key_H, 0, 0, actionCollection(), "newhole");
	clearHoleAction = new KAction(i18n("&Clear"), "locationbar_erase", CTRL+Key_Delete, game, SLOT(clearHole()), actionCollection(), "clearhole");
	resetHoleAction = new KAction(i18n("&Reset"), CTRL+Key_R, 0, 0, actionCollection(), "resethole");
	undoShotAction = KStdAction::undo(0, 0, actionCollection(), "undoshot");
	undoShotAction->setText(i18n("Undo Shot"));

	holeAction = new KListAction(i18n("Switch to Hole"), 0, 0, 0, actionCollection(), "switchhole");
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

	showGuideLineAction = new KToggleAction(i18n("Always Show Putter &Guideline"), 0, 0, 0, actionCollection(), "showguideline");
	connect(showGuideLineAction, SIGNAL(toggled(bool)), this, SLOT(showGuideLineChanged(bool)));
	showGuideLineAction->setChecked(config->readBoolEntry("showGuideLine", true));

	soundAction = new KToggleAction(i18n("Play &Sounds"), 0, 0, 0, actionCollection(), "sound");
	connect(soundAction, SIGNAL(toggled(bool)), this, SLOT(soundChanged(bool)));
	soundAction->setChecked(config->readBoolEntry("sound", true));

	(void) new KAction(i18n("&Reload Plugins"), 0, this, SLOT(initPlugins()), actionCollection(), "reloadplugins");
	(void) new KAction(i18n("&Show Plugins..."), 0, this, SLOT(showPlugins()), actionCollection(), "showplugins");

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
	e->accept();
}

void Kolf::startNewGame()
{
	NewGameDialog *dialog = new NewGameDialog(filename.isNull(), dummy, "New Game Dialog");

	if (dialog->exec() == QDialog::Accepted)
	{
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
		filename = filename.isNull()? dialog->course() : filename;
		
		delete spacer;
		spacer = 0;
		delete game;
		game = new KolfGame(obj, &players, filename, dummy);

		game->setObjects(obj);

		connect(game, SIGNAL(newHole(int)), scoreboard, SLOT(newHole(int)));
		connect(game, SIGNAL(newHole(int)), this, SLOT(parChanged(int)));
		connect(game, SIGNAL(scoreChanged(int, int, int)), scoreboard, SLOT(setScore(int, int, int)));
		connect(game, SIGNAL(parChanged(int, int)), scoreboard, SLOT(parChanged(int, int)));
		connect(game, SIGNAL(newPlayersTurn(Player *)), this, SLOT(newPlayersTurn(Player *)));
		connect(game, SIGNAL(holesDone()), this, SLOT(gameOver()));
		connect(game, SIGNAL(checkEditing()), this, SLOT(checkEditing()));
		connect(game, SIGNAL(editingStarted()), this, SLOT(editingStarted()));
		connect(game, SIGNAL(editingEnded()), this, SLOT(editingEnded()));
		connect(game, SIGNAL(inPlayStart()), this, SLOT(inPlayStart()));
		connect(game, SIGNAL(inPlayEnd()), this, SLOT(inPlayEnd()));
		connect(game, SIGNAL(maxStrokesReached()), this, SLOT(maxStrokesReached()));
		connect(game, SIGNAL(largestHole(int)), this, SLOT(updateHoleMenu(int)));
		connect(game, SIGNAL(titleChanged(const QString &)), this, SLOT(titleChanged(const QString &)));
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
		connect(undoShotAction, SIGNAL(activated()), game, SLOT(undoShot()));
		connect(aboutAction, SIGNAL(activated()), game, SLOT(showInfoDlg()));
		connect(useMouseAction, SIGNAL(toggled(bool)), game, SLOT(setUseMouse(bool)));
		connect(useAdvancedPuttingAction, SIGNAL(toggled(bool)), game, SLOT(setUseAdvancedPutting(bool)));		
		connect(soundAction, SIGNAL(toggled(bool)), game, SLOT(setSound(bool)));		
		connect(showGuideLineAction, SIGNAL(toggled(bool)), game, SLOT(setShowGuideLine(bool)));		

		game->setUseMouse(useMouseAction->isChecked());
		game->setUseAdvancedPutting(useAdvancedPuttingAction->isChecked());		
		game->setShowGuideLine(showGuideLineAction->isChecked());		
		game->setSound(soundAction->isChecked());		

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
		newAction->setEnabled(false);
		tutorialAction->setEnabled(false);
		

		game->addFirstHole();
		game->emitLargestHole();
	}

	delete dialog;
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
	newAction->setEnabled(true);
	tutorialAction->setEnabled(true);

	titleChanged("");
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
	editor = new Editor(obj, dummy, "Editor");
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
	KMessageBox::sorry(this, i18n("The maximum number of strokes for this hole has been reached."));
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
	undoShotAction->setEnabled(yes);
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

void Kolf::titleChanged(const QString &newTitle)
{
	setCaption(newTitle);
}

void Kolf::useMouseChanged(bool yes)
{
	KConfig *config = kapp->config(); config->setGroup("Settings"); config->writeEntry("useMouse", yes); config->sync();
}

void Kolf::useAdvancedPuttingChanged(bool yes)
{
	KConfig *config = kapp->config(); config->setGroup("Settings"); config->writeEntry("useAdvancedPutting", yes); config->sync();
}

void Kolf::showGuideLineChanged(bool yes)
{
	KConfig *config = kapp->config(); config->setGroup("Settings"); config->writeEntry("showGuideLine", yes); config->sync();
}

void Kolf::soundChanged(bool yes)
{
	KConfig *config = kapp->config(); config->setGroup("Settings"); config->writeEntry("sound", yes); config->sync();
}

void Kolf::initPlugins()
{
	//kdDebug() << "initPlugins" << endl;
	if (game)
		game->pause();

	obj->setAutoDelete(true);
	obj->clear();
	plugins.setAutoDelete(false);
	plugins.clear();

	// add prefab objects
	obj->append(new SlopeObj());
	obj->append(new PuddleObj());
	obj->append(new WallObj());
	obj->append(new CupObj());
	obj->append(new SandObj());
	obj->append(new WindmillObj());
	obj->append(new BlackHoleObj());
	obj->append(new FloaterObj());
	obj->append(new BridgeObj());
	obj->append(new SignObj());
	obj->append(new BumperObj());

	ObjectList *other = PluginLoader::loadAll();
	Object *object = 0;
	for (object = other->first(); object; object = other->next())
	{
		obj->append(object);
		plugins.append(object);
	}

	if (game)
	{
		game->setObjects(obj);
		game->unPause();
	}

	//kdDebug() << "end of initPlugins" << endl;
}

void Kolf::showPlugins()
{
	QString text = QString("<h2>%1</h2>").arg(i18n("Currently Loaded"));
	Object *object = 0;
	for (object = plugins.first(); object; object = plugins.next())
		text.append(object->name());
	KMessageBox::information(this, text, i18n("Plugins"));
}

void Kolf::keyBindings()
{
	KKeyDialog::configure(actionCollection());
}

#include "kolf.moc"
