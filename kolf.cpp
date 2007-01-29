#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kscoredialog.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstandardshortcut.h>
#include <kstandardaction.h>
#include <kstandardgameaction.h>
#include <KStandardGuiItem>
#include <kicon.h>

#include <QTimer>
#include <QGridLayout>

#include <stdlib.h>

#include "game.h"
#include "floater.h"
#include "slope.h"
#include "newgame.h"
#include "scoreboard.h"
#include "editor.h"
#include "pluginloader.h"
#include "printdialogpage.h"
#include "kolf.h"

Kolf::Kolf()
    : KMainWindow(0)
{
	setObjectName("Kolf");
	competition = false;
	game = 0;
	editor = 0;
	spacer = 0;
	scoreboard = 0;
	isTutorial = false;

	initGUI();

	obj = new ObjectList;
	initPlugins();

	filename = QString::null;
	dummy = new QWidget(this);
	setCentralWidget(dummy);
	layout = new QGridLayout(dummy);

	resize(420, 480);
}

Kolf::~Kolf()
{
	// wipe out our objects
	while (!obj->isEmpty())
		delete obj->takeFirst();
	delete obj;
}

void Kolf::initGUI()
{
	newAction = KStandardGameAction::gameNew(this, SLOT(newGame()), this);
        actionCollection()->addAction(newAction->objectName(), newAction);
	newAction->setText(newAction->text() + QString("..."));

	endAction = KStandardGameAction::end(this, SLOT(closeGame()), this);
        actionCollection()->addAction(endAction->objectName(), endAction);
	printAction = KStandardGameAction::print(this, SLOT(print()), this);
        actionCollection()->addAction(printAction->objectName(), printAction);

	QAction *action = KStandardGameAction::quit(this, SLOT(close()), this);
        actionCollection()->addAction(action->objectName(), action);
	saveAction = actionCollection()->addAction(KStandardAction::Save, "game_save", this, SLOT(save()));
	saveAction->setText(i18n("Save &Course"));
	saveAsAction = actionCollection()->addAction(KStandardAction::SaveAs, "game_save_as", this, SLOT(saveAs()));
	saveAsAction->setText(i18n("Save &Course As..."));

	saveGameAction = actionCollection()->addAction("savegame");
        saveGameAction->setText(i18n("&Save Game"));
	connect(saveGameAction, SIGNAL(triggered(bool) ), SLOT(saveGame()));
	saveGameAsAction = actionCollection()->addAction("savegameas");
        saveGameAsAction->setText(i18n("&Save Game As..."));
	connect(saveGameAsAction, SIGNAL(triggered(bool) ), SLOT(saveGameAs()));

	loadGameAction = KStandardGameAction::load(this, SLOT(loadGame()), this);
        actionCollection()->addAction(loadGameAction->objectName(), loadGameAction);
	loadGameAction->setText(i18n("Load Saved Game..."));

	highScoreAction = KStandardGameAction::highscores(this, SLOT(showHighScores()), actionCollection());

	editingAction = new KToggleAction(KIcon("pencil"), i18n("&Edit"), this);
        actionCollection()->addAction("editing", editingAction);
	connect(editingAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	editingAction->setShortcut(Qt::CTRL+Qt::Key_E);
	newHoleAction = actionCollection()->addAction("newhole");
        newHoleAction->setIcon(KIcon("filenew"));
        newHoleAction->setText(i18n("&New"));
	connect(newHoleAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	newHoleAction->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_N);
	clearHoleAction = actionCollection()->addAction("clearhole");
        clearHoleAction->setIcon(KIcon("locationbar_erase"));
        clearHoleAction->setText(KStandardGuiItem::clear().text());
	connect(clearHoleAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	clearHoleAction->setShortcut(Qt::CTRL+Qt::Key_Delete);
	resetHoleAction = actionCollection()->addAction("resethole");
        resetHoleAction->setText(i18n("&Reset"));
	connect(resetHoleAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	resetHoleAction->setShortcut(Qt::CTRL+Qt::Key_R);
	undoShotAction = KStandardAction::undo(this, SLOT(emptySlot()), this);
        actionCollection()->addAction("undoshot", undoShotAction);
	undoShotAction->setText(i18n("&Undo Shot"));
	//replayShotAction = new KAction(i18n("&Replay Shot"), 0, this, SLOT(emptySlot()), actionCollection(), "replay");

	holeAction = new KSelectAction(i18n("Switch to Hole"), this);
        actionCollection()->addAction("switchhole", holeAction);
	connect(holeAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	nextAction = actionCollection()->addAction("nexthole");
        nextAction->setIcon(KIcon("forward"));
        nextAction->setText(i18n("&Next Hole"));
	connect(nextAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	nextAction->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Forward));
	prevAction = actionCollection()->addAction("prevhole");
        prevAction->setIcon(KIcon("back"));
        prevAction->setText(i18n("&Previous Hole"));
	connect(prevAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	prevAction->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Back));
	firstAction = actionCollection()->addAction("firsthole");
        firstAction->setIcon(KIcon("gohome"));
        firstAction->setText(i18n("&First Hole"));
	connect(firstAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));
	firstAction->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Home));
	lastAction = actionCollection()->addAction("lasthole");
        lastAction->setText(i18n("&Last Hole"));
	connect(lastAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	lastAction->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_End);
	randAction = actionCollection()->addAction("randhole");
        randAction->setIcon(KIcon("goto"));
        randAction->setText(i18n("&Random Hole"));
	connect(randAction, SIGNAL(triggered(bool)), SLOT(emptySlot()));

	useMouseAction = new KToggleAction(i18n("Enable &Mouse for Moving Putter"), this);
        actionCollection()->addAction("usemouse", useMouseAction);
	connect(useMouseAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	useMouseAction->setCheckedState(KGuiItem(i18n("Disable &Mouse for Moving Putter")));
	connect(useMouseAction, SIGNAL(toggled(bool)), this, SLOT(useMouseChanged(bool)));
	KSharedConfig::Ptr config = KGlobal::config();
	config->setGroup("Settings");
	useMouseAction->setChecked(config->readEntry("useMouse", true));

	useAdvancedPuttingAction = new KToggleAction(i18n("Enable &Advanced Putting"), this);
        actionCollection()->addAction("useadvancedputting", useAdvancedPuttingAction);
	connect(useAdvancedPuttingAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	useAdvancedPuttingAction->setCheckedState(KGuiItem(i18n("Disable &Advanced Putting")));
	connect(useAdvancedPuttingAction, SIGNAL(toggled(bool)), this, SLOT(useAdvancedPuttingChanged(bool)));
	useAdvancedPuttingAction->setChecked(config->readEntry("useAdvancedPutting", false));

	showInfoAction = new KToggleAction(KIcon("info"), i18n("Show &Info"), this);
        actionCollection()->addAction("showinfo", showInfoAction);
	connect(showInfoAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	showInfoAction->setShortcut(Qt::CTRL+Qt::Key_I);
	showInfoAction->setCheckedState(KGuiItem(i18n("Hide &Info")));
	connect(showInfoAction, SIGNAL(toggled(bool)), this, SLOT(showInfoChanged(bool)));
	showInfoAction->setChecked(config->readEntry("showInfo", false));

	showGuideLineAction = new KToggleAction(i18n("Show Putter &Guideline"), this);
        actionCollection()->addAction("showguideline", showGuideLineAction);
	connect(showGuideLineAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	showGuideLineAction->setCheckedState(KGuiItem(i18n("Hide Putter &Guideline")));
	connect(showGuideLineAction, SIGNAL(toggled(bool)), this, SLOT(showGuideLineChanged(bool)));
	showGuideLineAction->setChecked(config->readEntry("showGuideLine", true));

	KToggleAction *act = new KToggleAction(i18n("Enable All Dialog Boxes"), this);
        actionCollection()->addAction("enableAll", act);
	connect(act, SIGNAL(triggered(bool) ), SLOT(enableAllMessages()));
	act->setCheckedState(KGuiItem(i18n("Disable All Dialog Boxes")));

	soundAction = new KToggleAction(i18n("Play &Sounds"), this);
        actionCollection()->addAction("sound", soundAction);
	connect(soundAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	connect(soundAction, SIGNAL(toggled(bool)), this, SLOT(soundChanged(bool)));
	soundAction->setChecked(config->readEntry("sound", true));

	action = actionCollection()->addAction("reloadplugins");
        action->setText(i18n("&Reload Plugins"));
	connect(action, SIGNAL(triggered(bool) ), SLOT(initPlugins()));
	action = actionCollection()->addAction("showplugins");
        action->setText(i18n("Show &Plugins"));
	connect(action, SIGNAL(triggered(bool) ), SLOT(showPlugins()));

	aboutAction = actionCollection()->addAction("aboutcourse");
        action->setText(i18n("&About Course"));
	connect(aboutAction, SIGNAL(triggered(bool) ), SLOT(emptySlot()));
	tutorialAction = actionCollection()->addAction("tutorial");
        action->setText(i18n("&Tutorial"));
	connect(tutorialAction, SIGNAL(triggered(bool) ), SLOT(tutorial()));

	statusBar();
	setupGUI();
}

bool Kolf::queryClose()
{
	if (game)
		if (game->askSave(true))
			return false;
	return true;
}

void Kolf::startNewGame()
{
	NewGameDialog *dialog = 0;
	int firstHole = 1;

	if (loadedGame.isNull())
	{
		dialog = new NewGameDialog(filename.isNull());
		if (dialog->exec() != QDialog::Accepted)
			goto end;
	}

	players.clear();
	delete scoreboard;
	scoreboard = new ScoreBoard(dummy);
	layout->addWidget(scoreboard, 1, 0);
	scoreboard->show();

	if (loadedGame.isNull())
	{
		PlayerEditor *curEditor = 0;
		int newId = 1;
		for (curEditor = dialog->players()->at(newId-1); newId <= dialog->players()->count(); ++newId)
		{
			players.append(Player());
			players.last().ball()->setColor(dialog->players()->at(newId-1)->color());
                        players.last().setName(dialog->players()->at(newId-1)->name());
			players.last().setId(newId);
		}

		competition = dialog->competition();
		filename = filename.isNull()? dialog->course() : filename;
	}
	else
	{
		KConfig config(loadedGame);
		config.setGroup("0 Saved Game");

		if (isTutorial)
			filename = KGlobal::dirs()->findResource("appdata", "tutorial.kolf");
		else
			filename = config.readEntry("Course", QString());

		if (filename.isNull())
			return;

		competition = config.readEntry("Competition", false);
		firstHole = config.readEntry("Current Hole", 1);

		players.clear();
		KolfGame::scoresFromSaved(&config, players);
	}

	for (PlayerList::Iterator it = players.begin(); it != players.end(); ++it)
		scoreboard->newPlayer((*it).name());

	delete spacer;
	spacer = 0;
	delete game;
	game = new KolfGame(obj, &players, filename, dummy);
	game->setStrict(competition);

	connect(game, SIGNAL(newHole(int)), scoreboard, SLOT(newHole(int)));
	connect(game, SIGNAL(scoreChanged(int, int, int)), scoreboard, SLOT(setScore(int, int, int)));
	connect(game, SIGNAL(parChanged(int, int)), scoreboard, SLOT(parChanged(int, int)));
	connect(game, SIGNAL(modifiedChanged(bool)), this, SLOT(updateModified(bool)));
	connect(game, SIGNAL(newPlayersTurn(Player *)), this, SLOT(newPlayersTurn(Player *)));
	connect(game, SIGNAL(holesDone()), this, SLOT(gameOver()));
	connect(game, SIGNAL(checkEditing()), this, SLOT(checkEditing()));
	connect(game, SIGNAL(editingStarted()), this, SLOT(editingStarted()));
	connect(game, SIGNAL(editingEnded()), this, SLOT(editingEnded()));
	connect(game, SIGNAL(inPlayStart()), this, SLOT(inPlayStart()));
	connect(game, SIGNAL(inPlayEnd()), this, SLOT(inPlayEnd()));
	connect(game, SIGNAL(maxStrokesReached(const QString &)), this, SLOT(maxStrokesReached(const QString &)));
	connect(game, SIGNAL(largestHole(int)), this, SLOT(updateHoleMenu(int)));
	connect(game, SIGNAL(titleChanged(const QString &)), this, SLOT(titleChanged(const QString &)));
	connect(game, SIGNAL(newStatusText(const QString &)), this, SLOT(newStatusText(const QString &)));
	connect(game, SIGNAL(currentHole(int)), this, SLOT(setCurrentHole(int)));
	connect(holeAction, SIGNAL(triggered(const QString &)), game, SLOT(switchHole(const QString &)));
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
	//connect(replayShotAction, SIGNAL(activated()), game, SLOT(replay()));
	connect(aboutAction, SIGNAL(activated()), game, SLOT(showInfoDlg()));
	connect(useMouseAction, SIGNAL(toggled(bool)), game, SLOT(setUseMouse(bool)));
	connect(useAdvancedPuttingAction, SIGNAL(toggled(bool)), game, SLOT(setUseAdvancedPutting(bool)));
	connect(soundAction, SIGNAL(toggled(bool)), game, SLOT(setSound(bool)));
	connect(showGuideLineAction, SIGNAL(toggled(bool)), game, SLOT(setShowGuideLine(bool)));
	connect(showInfoAction, SIGNAL(toggled(bool)), game, SLOT(setShowInfo(bool)));

	game->setUseMouse(useMouseAction->isChecked());
	game->setUseAdvancedPutting(useAdvancedPuttingAction->isChecked());
	game->setShowInfo(showInfoAction->isChecked());
	game->setShowGuideLine(showGuideLineAction->isChecked());
	game->setSound(soundAction->isChecked());

	layout->addWidget(game, 0, 0, Qt::AlignCenter);

	game->show();
	game->setFocus();

	setEditingEnabled(true);
	endAction->setEnabled(true);
	setHoleMovementEnabled(true);
	setHoleOtherEnabled(true);
	aboutAction->setEnabled(true);
	highScoreAction->setEnabled(true);
	printAction->setEnabled(true);
	saveAction->setEnabled(true);
	saveAsAction->setEnabled(true);
	saveGameAction->setEnabled(true);
	saveGameAsAction->setEnabled(true);

	clearHoleAction->setEnabled(false);
	newHoleAction->setEnabled(false);
	newAction->setEnabled(false);
	loadGameAction->setEnabled(false);
	tutorialAction->setEnabled(false);


	// so game can do stuff that needs to be done
	// after things above are connected
	game->startFirstHole(firstHole);

	end:
	delete dialog;
}

void Kolf::newGame()
{
	isTutorial = false;
	filename = QString::null;
	startNewGame();
}

void Kolf::tutorial()
{
	QString newfilename = KGlobal::dirs()->findResource("appdata", "tutorial.kolfgame");
	if (newfilename.isNull())
	        return;

	filename = QString::null;
	loadedGame = newfilename;
	isTutorial = true;

	startNewGame();

	loadedGame = QString::null;
}

void Kolf::closeGame()
{
	if (game)
	{
		if (game->askSave(true))
			return;
		game->pause();
	}

	filename = QString::null;

	editingEnded();
	delete game;
	game = 0;
	loadedGame = QString::null;

	editingAction->setChecked(false);
	setEditingEnabled(false);
	endAction->setEnabled(false);
	aboutAction->setEnabled(false);
	highScoreAction->setEnabled(false);
	printAction->setEnabled(false);
	saveAction->setEnabled(false);
	saveAsAction->setEnabled(false);
	saveGameAction->setEnabled(false);
	saveGameAsAction->setEnabled(false);
	setHoleMovementEnabled(false);
	setHoleOtherEnabled(false);

	clearHoleAction->setEnabled(false);
	newHoleAction->setEnabled(false);
	newAction->setEnabled(true);
	loadGameAction->setEnabled(true);
	tutorialAction->setEnabled(true);

	titleChanged(QString::null);
	updateModified(false);

	QTimer::singleShot(100, this, SLOT(createSpacer()));
}

void Kolf::createSpacer()
{
	// make a player to play the spacer hole
	spacerPlayers.clear();
	spacerPlayers.append(Player());
	spacerPlayers.last().ball()->setColor(Qt::yellow);
	spacerPlayers.last().setName("player");
	spacerPlayers.last().setId(1);

	delete spacer;
	spacer = new KolfGame(obj, &spacerPlayers, KGlobal::dirs()->findResource("appdata", "intro"), dummy);
	spacer->setSound(false);
	spacer->startFirstHole(1);
	layout->addWidget(spacer, 0, 0, Qt::AlignCenter);
	spacer->hidePutter();
	spacer->ignoreEvents(true);

	spacer->show();
}

void Kolf::gameOver()
{
	int curPar = 0;
	int lowScore = INT_MAX; // let's hope it doesn't stay this way!
	int curScore = 1;

	// names of people who had the lowest score
	QStringList names;

	HighScoreList highScores;
	int scoreBoardIndex = 1;

	while (curScore != 0)
	{
		QString curName;

		// name taken as a reference and filled out
		curScore = scoreboard->total(scoreBoardIndex, curName);

		scoreBoardIndex++;

		if (curName == i18n("Par"))
		{
			curPar = curScore;
			curScore = 0;
			continue;
		}

		if (curScore == 0)
			continue;

		// attempt to add everybody to the highscore list
		// (ignored if we aren't competing down below)
		highScores.append(HighScore(curName, curScore));

		if (curScore < lowScore)
		{
			names.clear();
			lowScore = curScore;
			names.append(curName);
		}
		else if (curScore == lowScore)
			names.append(curName);
	}

	// only announce a winner if more than two entries
	// (player and par) are on the scoreboard + one to go past end
	// + 1 for koodoo
	if (scoreBoardIndex > 4)
	{
		if (names.count() > 1)
		{
			QString winners = names.join(i18n(" and "));
			KMessageBox::information(this, i18n("%1 tied", winners));
		}
		else
			KMessageBox::information(this, i18n("%1 won!", names.first()));
	}

	if (competition)
	{
		// deal with highscores
		// KScoreDialog makes it very easy :-))

		KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
		scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), "Par");

		CourseInfo courseInfo;
		game->courseInfo(courseInfo, game->curFilename());

		scoreDialog->setConfigGroup(courseInfo.untranslatedName + QString(" Highscores"));

		for (HighScoreList::Iterator it = highScores.begin(); it != highScores.end(); ++it)
		{
			KScoreDialog::FieldInfo info;
			info[KScoreDialog::Name] = (*it).name;
			info[KScoreDialog::Custom1] = QString::number(curPar);

			scoreDialog->addScore((*it).score, info, false, true);
		}

		scoreDialog->setComment(i18n("High Scores for %1", courseInfo.name));
		scoreDialog->show();
	}

	QTimer::singleShot(700, this, SLOT(closeGame()));
}

void Kolf::showHighScores()
{
	KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
	scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), "Par");

	CourseInfo courseInfo;
	game->courseInfo(courseInfo, game->curFilename());

	scoreDialog->setConfigGroup(courseInfo.untranslatedName + QString(" Highscores"));
	scoreDialog->setComment(i18n("High Scores for %1", courseInfo.name));
	scoreDialog->show();
}

void Kolf::save()
{
	if (filename.isNull())
	{
		saveAs();
		return;
	}

	if (game) {
		game->save();
		game->setFocus();
	}
}

void Kolf::saveAs()
{
	QString newfilename = KFileDialog::getSaveFileName( KUrl("kfiledialog:///kourses"),
                                  "application/x-kourse", this, i18n("Pick Kolf Course to Save To"));
	if (!newfilename.isNull())
	{
		filename = newfilename;
		game->setFilename(filename);
		game->save();
		game->setFocus();
	}
}

void Kolf::saveGameAs()
{
	QString newfilename = KFileDialog::getSaveFileName( KUrl("kfiledialog:///savedkolf"),
                                    "application/x-kolf", this, i18n("Pick Saved Game to Save To"));
	if (newfilename.isNull())
		return;

	loadedGame = newfilename;

	saveGame();
}

void Kolf::saveGame()
{
	if (loadedGame.isNull())
	{
		saveGameAs();
		return;
	}

	KConfig config(loadedGame);
	config.setGroup("0 Saved Game");

	config.writeEntry("Competition", competition);
	config.writeEntry("Course", filename);

	game->saveScores(&config);

	config.sync();
}

void Kolf::loadGame()
{
	loadedGame = KFileDialog::getOpenFileName( KUrl("kfiledialog:///savedkolf"),
			 QLatin1String("application/x-kolf"), this, i18n("Pick Kolf Saved Game"));

	if (loadedGame.isNull())
		return;

	isTutorial = false;
	startNewGame();
}

// called by main for command line files
void Kolf::openUrl(KUrl url)
{
	QString target;
	if (KIO::NetAccess::download(url, target, this))
	{
		isTutorial = false;
		QString mimeType = KMimeType::findByPath(target)->name();
		if (mimeType == "application/x-kourse")
			filename = target;
		else if (mimeType == "application/x-kolf")
			loadedGame = target;
		else
		{
			closeGame();
			return;
		}

		QTimer::singleShot(10, this, SLOT(startNewGame()));
	}
	else
		closeGame();
}

void Kolf::newPlayersTurn(Player *player)
{
	tempStatusBarText = i18n("%1's turn", player->name());

	if (showInfoAction->isChecked())
		statusBar()->showMessage(tempStatusBarText, 5 * 1000);
	else
		statusBar()->showMessage(tempStatusBarText);

	scoreboard->setCurrentCell(player->id() - 1, game->currentHole() - 1);
}

void Kolf::newStatusText(const QString &text)
{
	if (text.isEmpty())
		statusBar()->showMessage(tempStatusBarText);
	else
		statusBar()->showMessage(text);
}

void Kolf::editingStarted()
{
	delete editor;
	editor = new Editor(obj, dummy);
        editor->setObjectName( "Editor" );
	connect(editor, SIGNAL(addNewItem(Object *)), game, SLOT(addNewObject(Object *)));
	connect(editor, SIGNAL(changed()), game, SLOT(setModified()));
	connect(editor, SIGNAL(addNewItem(Object *)), this, SLOT(setHoleFocus()));
	connect(game, SIGNAL(newSelectedItem(CanvasItem *)), editor, SLOT(setItem(CanvasItem *)));

	scoreboard->hide();

	layout->addWidget(editor, 1, 0);
	editor->show();

	clearHoleAction->setEnabled(true);
	newHoleAction->setEnabled(true);
	setHoleOtherEnabled(false);

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
	setHoleOtherEnabled(true);

	if (game)
		game->setFocus();
}

void Kolf::inPlayStart()
{
	setEditingEnabled(false);
	setHoleOtherEnabled(false);
	setHoleMovementEnabled(false);
}

void Kolf::inPlayEnd()
{
	setEditingEnabled(true);
	setHoleOtherEnabled(true);
	setHoleMovementEnabled(true);
}

void Kolf::maxStrokesReached(const QString &name)
{
	KMessageBox::sorry(this, i18n("%1's score has reached the maximum for this hole.", name));
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
}

void Kolf::setHoleOtherEnabled(bool yes)
{
	if (competition)
		yes = false;

	resetHoleAction->setEnabled(yes);
	undoShotAction->setEnabled(yes);
	//replayShotAction->setEnabled(yes);
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
	if (!game)
		return;

	KPrinter pr;
	pr.addDialogPage(new PrintDialogPage());

    if (pr.setup(this, i18n("Print %1 - Hole %2", game->courseName(), game->currentHole())))
	{
		pr.newPage();
		game->print(pr);
	}
}

void Kolf::updateModified(bool mod)
{
	courseModified = mod;
	titleChanged(title);
}

void Kolf::titleChanged(const QString &newTitle)
{
	title = newTitle;
	setCaption(title, courseModified);
}

void Kolf::useMouseChanged(bool yes)
{
	KSharedConfig::Ptr config = KGlobal::config(); config->setGroup("Settings"); config->writeEntry("useMouse", yes); config->sync();
}

void Kolf::useAdvancedPuttingChanged(bool yes)
{
	KSharedConfig::Ptr config = KGlobal::config(); config->setGroup("Settings"); config->writeEntry("useAdvancedPutting", yes); config->sync();
}

void Kolf::showInfoChanged(bool yes)
{
	KSharedConfig::Ptr config = KGlobal::config(); config->setGroup("Settings"); config->writeEntry("showInfo", yes); config->sync();
}

void Kolf::showGuideLineChanged(bool yes)
{
	KSharedConfig::Ptr config = KGlobal::config(); config->setGroup("Settings"); config->writeEntry("showGuideLine", yes); config->sync();
}

void Kolf::soundChanged(bool yes)
{
	KSharedConfig::Ptr config = KGlobal::config(); config->setGroup("Settings"); config->writeEntry("sound", yes); config->sync();
}

void Kolf::initPlugins()
{
	//kDebug(12007) << "initPlugins" << endl;
	if (game)
		game->pause();

	while (!obj->isEmpty())
		delete obj->takeFirst();
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
	QList<Object *>::const_iterator object;
        for (object = other->constBegin(); object != other->constEnd(); ++object)
        {
                obj->append(*object);
                plugins.append(*object);
        }

	if (game)
	{
		game->setObjects(obj);
		game->unPause();
	}

	//kDebug(12007) << "end of initPlugins" << endl;
}

void Kolf::showPlugins()
{
	QString text = QString("<h2>%1</h2><ol>").arg(i18n("Currently Loaded Plugins"));
	QList<Object *>::const_iterator object;
        for (object = plugins.constBegin(); object != plugins.constEnd(); ++object)
	{
		text.append("<li>");
		text.append((*object)->name());
		text.append(" - ");
		text.append(i18n("by %1", (*object)->author()));
		text.append("</li>");
	}
	/*Object *object = 0;
	for (object = plugins.first(); object; object = plugins.next())
	{
		text.append("<li>");
		text.append(object->name());
		text.append(" - ");
		text.append(i18n("by %1", object->author()));
		text.append("</li>");
	}*/
	text.append("</ol>");
	KMessageBox::information(this, text, i18n("Plugins"));
}

void Kolf::enableAllMessages()
{
	KMessageBox::enableAllMessages();
}

void Kolf::setCurrentHole(int hole)
{
	if (!holeAction || holeAction->items().count() < hole)
		return;
	// Golf is 1-based, KListAction is 0-based
	holeAction->setCurrentItem(hole - 1);
}

#include "kolf.moc"
