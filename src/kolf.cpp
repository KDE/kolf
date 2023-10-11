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

#include "kolf.h"
#include "editor.h"
#include "landscape.h"
#include "newgame.h"
#include "objects.h"
#include "obstacles.h"
#include "scoreboard.h"

#include <KActionCollection>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KScoreDialog>
#include <KSelectAction>
#include <KSharedConfig>
#include <KStandardAction>
#include <KStandardGameAction>
#include <KStandardGuiItem>
#include <KToggleAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTemporaryFile>
#include <QTimer>

KolfWindow::KolfWindow()
    : KXmlGuiWindow(nullptr)
{
	setObjectName( QStringLiteral("Kolf" ));
	competition = false;
	game = nullptr;
	editor = nullptr;
	spacer = nullptr;
	scoreboard = nullptr;
	isTutorial = false;

	setupActions();

	m_itemFactory.registerType<Kolf::Slope>(QStringLiteral("slope"), i18n("Slope"));
	m_itemFactory.registerType<Kolf::Puddle>(QStringLiteral("puddle"), i18n("Puddle"));
	m_itemFactory.registerType<Kolf::Wall>(QStringLiteral("wall"), i18n("Wall"));
	m_itemFactory.registerType<Kolf::Cup>(QStringLiteral("cup"), i18n("Cup"), true); //true == addOnNewHole
	m_itemFactory.registerType<Kolf::Sand>(QStringLiteral("sand"), i18n("Sand"));
	m_itemFactory.registerType<Kolf::Windmill>(QStringLiteral("windmill"), i18n("Windmill"));
	m_itemFactory.registerType<Kolf::BlackHole>(QStringLiteral("blackhole"), i18n("Black Hole"));
	m_itemFactory.registerType<Kolf::Floater>(QStringLiteral("floater"), i18n("Floater"));
	m_itemFactory.registerType<Kolf::Bridge>(QStringLiteral("bridge"), i18n("Bridge"));
	m_itemFactory.registerType<Kolf::Sign>(QStringLiteral("sign"), i18n("Sign"));
	m_itemFactory.registerType<Kolf::Bumper>(QStringLiteral("bumper"), i18n("Bumper"));
	//NOTE: The plugin mechanism has been removed because it is not used anyway.

	filename = QString();
	dummy = new QWidget(this);
	setCentralWidget(dummy);
	layout = new QGridLayout(dummy);

	resize(420, 480);
}

KolfWindow::~KolfWindow()
{
}

void KolfWindow::setupActions()
{
	// Game
	newAction = KStandardGameAction::gameNew(this, &KolfWindow::newGame, actionCollection());
	endAction = KStandardGameAction::end(this, &KolfWindow::closeGame, actionCollection());
	KStandardGameAction::quit(this, &KolfWindow::close, actionCollection());

	saveAction = actionCollection()->addAction(KStandardAction::Save, QStringLiteral("game_save"), this, &KolfWindow::save);
	saveAction->setText(i18n("Save &Course"));
	saveAsAction = actionCollection()->addAction(KStandardAction::SaveAs, QStringLiteral("game_save_as"), this, &KolfWindow::saveAs);
	saveAsAction->setText(i18n("Save &Course As..."));

	saveGameAction = actionCollection()->addAction(QStringLiteral("savegame"));
	saveGameAction->setText(i18n("&Save Game"));
	connect(saveGameAction, &QAction::triggered, this, &KolfWindow::saveGame);
	saveGameAsAction = actionCollection()->addAction(QStringLiteral("savegameas"));
	saveGameAsAction->setText(i18n("&Save Game As..."));
	connect(saveGameAsAction, &QAction::triggered, this, &KolfWindow::saveGameAs);

	loadGameAction = KStandardGameAction::load(this, &KolfWindow::loadGame, actionCollection());
	highScoreAction = KStandardGameAction::highscores(this, &KolfWindow::showHighScores, actionCollection());

	// Hole
	editingAction = new KToggleAction(QIcon::fromTheme( QStringLiteral( "document-properties") ), i18n("&Edit"), this);
	actionCollection()->addAction(QStringLiteral("editing"), editingAction);
	connect(editingAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(editingAction, Qt::CTRL|Qt::Key_E);
	newHoleAction = actionCollection()->addAction(QStringLiteral("newhole"));
	newHoleAction->setIcon(QIcon::fromTheme( QStringLiteral( "document-new" )));
	newHoleAction->setText(i18n("&New"));
	connect(newHoleAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(newHoleAction, Qt::CTRL|Qt::SHIFT|Qt::Key_N);
	clearHoleAction = actionCollection()->addAction(QStringLiteral("clearhole"));
	clearHoleAction->setIcon(QIcon::fromTheme( QStringLiteral( "edit-clear-locationbar-ltr" )));
	clearHoleAction->setText(KStandardGuiItem::clear().text());
	connect(clearHoleAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(clearHoleAction, Qt::CTRL|Qt::Key_Delete);
	resetHoleAction = actionCollection()->addAction(QStringLiteral("resethole"));
	resetHoleAction->setText(i18n("&Reset"));
	connect(resetHoleAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(resetHoleAction, Qt::CTRL|Qt::Key_R);
	undoShotAction = KStandardAction::undo(this, &KolfWindow::emptySlot, this);
	actionCollection()->addAction(QStringLiteral("undoshot"), undoShotAction);
	undoShotAction->setText(i18n("&Undo Shot"));
	//replayShotAction = new QAction(i18n("&Replay Shot"), 0, this, SLOT(emptySlot()), actionCollection(), "replay");

	// Go
	holeAction = new KSelectAction(i18n("Switch to Hole"), this);
	actionCollection()->addAction(QStringLiteral("switchhole"), holeAction);
	connect(holeAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	nextAction = actionCollection()->addAction(QStringLiteral("nexthole"));
	nextAction->setIcon(QIcon::fromTheme( QStringLiteral( "go-next" )));
	nextAction->setText(i18n("&Next Hole"));
	connect(nextAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	actionCollection()->setDefaultShortcuts(nextAction, KStandardShortcut::forward());
	prevAction = actionCollection()->addAction(QStringLiteral("prevhole"));
	prevAction->setIcon(QIcon::fromTheme( QStringLiteral( "go-previous" )));
	prevAction->setText(i18n("&Previous Hole"));
	connect(prevAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	actionCollection()->setDefaultShortcuts(prevAction, KStandardShortcut::back());
	firstAction = actionCollection()->addAction(QStringLiteral("firsthole"));
	firstAction->setIcon(QIcon::fromTheme( QStringLiteral( "go-home" )));
	firstAction->setText(i18n("&First Hole"));
	connect(firstAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	actionCollection()->setDefaultShortcuts(firstAction, KStandardShortcut::begin());
	lastAction = actionCollection()->addAction(QStringLiteral("lasthole"));
	lastAction->setText(i18n("&Last Hole"));
	connect(lastAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(lastAction, Qt::CTRL|Qt::SHIFT|Qt::Key_End); // why not KStandardShortcut::End (Ctrl+End)?
	randAction = actionCollection()->addAction(QStringLiteral("randhole"));
	randAction->setIcon(QIcon::fromTheme( QStringLiteral( "go-jump" )));
	randAction->setText(i18n("&Random Hole"));
	connect(randAction, &QAction::triggered, this, &KolfWindow::emptySlot);

	// Settings
	useMouseAction = new KToggleAction(i18n("Enable &Mouse for Moving Putter"), this);
	actionCollection()->addAction(QStringLiteral("usemouse"), useMouseAction);
	connect(useMouseAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	connect(useMouseAction, &QAction::toggled, this, &KolfWindow::useMouseChanged);
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings");
	useMouseAction->setChecked(configGroup.readEntry("useMouse", true));

	useAdvancedPuttingAction = new KToggleAction(i18n("Enable &Advanced Putting"), this);
	actionCollection()->addAction(QStringLiteral("useadvancedputting"), useAdvancedPuttingAction);
	connect(useAdvancedPuttingAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	connect(useAdvancedPuttingAction, &QAction::toggled, this, &KolfWindow::useAdvancedPuttingChanged);
	useAdvancedPuttingAction->setChecked(configGroup.readEntry("useAdvancedPutting", false));

	showInfoAction = new KToggleAction(QIcon::fromTheme( QStringLiteral( "help-about")), i18n("Show &Info"), this);
	actionCollection()->addAction(QStringLiteral("showinfo"), showInfoAction);
	connect(showInfoAction, &QAction::triggered, this, &KolfWindow::emptySlot);
    actionCollection()->setDefaultShortcut(showInfoAction, Qt::CTRL|Qt::Key_I);
	connect(showInfoAction, &QAction::toggled, this, &KolfWindow::showInfoChanged);
	showInfoAction->setChecked(configGroup.readEntry("showInfo", true));

	showGuideLineAction = new KToggleAction(i18n("Show Putter &Guideline"), this);
	actionCollection()->addAction(QStringLiteral("showguideline"), showGuideLineAction);
	connect(showGuideLineAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	connect(showGuideLineAction, &QAction::toggled, this, &KolfWindow::showGuideLineChanged);
	showGuideLineAction->setChecked(configGroup.readEntry("showGuideLine", true));

	KToggleAction *act = new KToggleAction(i18n("Enable All Dialog Boxes"), this);
	actionCollection()->addAction(QStringLiteral("enableAll"), act);
	connect(act, &QAction::triggered, this, &KolfWindow::enableAllMessages);

	soundAction = new KToggleAction(i18n("Play &Sounds"), this);
	actionCollection()->addAction(QStringLiteral("sound"), soundAction);
	connect(soundAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	connect(soundAction, &QAction::toggled, this, &KolfWindow::soundChanged);
	soundAction->setChecked(configGroup.readEntry("sound", true));

	aboutAction = actionCollection()->addAction(QStringLiteral("aboutcourse"));
	aboutAction->setText(i18n("&About Course..."));
	connect(aboutAction, &QAction::triggered, this, &KolfWindow::emptySlot);
	tutorialAction = actionCollection()->addAction(QStringLiteral("tutorial"));
	tutorialAction->setText(i18n("&Tutorial"));
	connect(tutorialAction, &QAction::triggered, this, &KolfWindow::tutorial);

	setupGUI();
}

bool KolfWindow::queryClose()
{
	if (game)
		if (game->askSave(true))
			return false;
	return true;
}

void KolfWindow::startNewGame()
{
        NewGameDialog *dialog = nullptr;
	int firstHole = 1;

	if (loadedGame.isNull())
	{
		dialog = new NewGameDialog(filename.isNull());
                if (dialog->exec() != QDialog::Accepted) {
                    delete dialog;
                    return;
               }
	}

	players.clear();
	delete scoreboard;
	scoreboard = new ScoreBoard(dummy);
	layout->addWidget(scoreboard, 1, 0);
	scoreboard->show();

	if (loadedGame.isNull())
	{
		for (int newId = 1; newId <= dialog->players()->count(); ++newId)
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
		KConfigGroup configGroup(config.group("0 Saved Game"));

		if (isTutorial)
			filename = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("tutorial.kolf"));
		else
			filename = configGroup.readEntry("Course", QString());

		if (filename.isNull())
			return;

		competition = configGroup.readEntry("Competition", false);
		firstHole = configGroup.readEntry("Current Hole", 1);

		players.clear();
		KolfGame::scoresFromSaved(&config, players);
	}

	for (PlayerList::Iterator it = players.begin(); it != players.end(); ++it)
		scoreboard->newPlayer((*it).name());

	delete spacer;
        spacer = nullptr;
	delete game;
        game = new KolfGame(m_itemFactory, &players, filename, dummy);
	game->setStrict(competition);

	connect(game, &KolfGame::newHole, scoreboard, &ScoreBoard::newHole);
	connect(game, &KolfGame::scoreChanged, scoreboard, &ScoreBoard::setScore);
	connect(game, &KolfGame::parChanged, scoreboard, &ScoreBoard::parChanged);
	connect(game, &KolfGame::modifiedChanged, this, &KolfWindow::updateModified);
	connect(game, &KolfGame::newPlayersTurn, this, &KolfWindow::newPlayersTurn);
	connect(game, &KolfGame::holesDone, this, &KolfWindow::gameOver);
	connect(game, &KolfGame::checkEditing, this, &KolfWindow::checkEditing);
	connect(game, &KolfGame::editingStarted, this, &KolfWindow::editingStarted);
	connect(game, &KolfGame::editingEnded, this, &KolfWindow::editingEnded);
	connect(game, &KolfGame::inPlayStart, this, &KolfWindow::inPlayStart);
	connect(game, &KolfGame::inPlayEnd, this, &KolfWindow::inPlayEnd);
	connect(game, &KolfGame::maxStrokesReached, this, &KolfWindow::maxStrokesReached);
	connect(game, &KolfGame::largestHole, this, &KolfWindow::updateHoleMenu);
	connect(game, &KolfGame::titleChanged, this, &KolfWindow::titleChanged);
	connect(game, &KolfGame::newStatusText, this, &KolfWindow::newStatusText);
        connect(game, qOverload<int>(&KolfGame::currentHole), this,
                &KolfWindow::setCurrentHole);
        connect(holeAction, &QAction::triggered, game,
                qOverload<int>(&KolfGame::switchHole));
        connect(nextAction, &QAction::triggered, game, &KolfGame::nextHole);
	connect(prevAction, &QAction::triggered, game, &KolfGame::prevHole);
	connect(firstAction, &QAction::triggered, game, &KolfGame::firstHole);
	connect(lastAction, &QAction::triggered, game, &KolfGame::lastHole);
	connect(randAction, &QAction::triggered, game, &KolfGame::randHole);
	connect(editingAction, &QAction::triggered, game, &KolfGame::toggleEditMode);
	connect(newHoleAction, &QAction::triggered, game, &KolfGame::addNewHole);
	connect(clearHoleAction, &QAction::triggered, game, &KolfGame::clearHole);
	connect(resetHoleAction, &QAction::triggered, game, &KolfGame::resetHole);
	connect(undoShotAction, &QAction::triggered, game, &KolfGame::undoShot);
	//connect(replayShotAction, &QAction::triggered, game, &KolfGame::replay);
	connect(aboutAction, &QAction::triggered, game, &KolfGame::showInfoDlg);
	connect(useMouseAction, &QAction::toggled, game, &KolfGame::setUseMouse);
	connect(useAdvancedPuttingAction, &QAction::toggled, game, &KolfGame::setUseAdvancedPutting);
	connect(soundAction, &QAction::toggled, game, &KolfGame::setSound);
	connect(showGuideLineAction, &QAction::toggled, game, &KolfGame::setShowGuideLine);
	connect(showInfoAction, &QAction::toggled, game, &KolfGame::setShowInfo);

	game->setUseMouse(useMouseAction->isChecked());
	game->setUseAdvancedPutting(useAdvancedPuttingAction->isChecked());
	game->setShowInfo(showInfoAction->isChecked());
	game->setShowGuideLine(showGuideLineAction->isChecked());
	game->setSound(soundAction->isChecked());

	layout->addWidget(game, 0, 0);//, Qt::AlignCenter);

	game->show();
	game->setFocus();

	setEditingEnabled(true);
	endAction->setEnabled(true);
	setHoleMovementEnabled(true);
	setHoleOtherEnabled(true);
	aboutAction->setEnabled(true);
	highScoreAction->setEnabled(true);
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
	delete dialog;
}

void KolfWindow::newGame()
{
	isTutorial = false;
	filename = QString();
	startNewGame();
}

void KolfWindow::tutorial()
{
	QString newfilename = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("tutorial.kolfgame"));
	if (newfilename.isNull())
	        return;

	filename = QString();
	loadedGame = newfilename;
	isTutorial = true;

	startNewGame();

	loadedGame = QString();
}

void KolfWindow::closeGame()
{
	if (game)
	{
		if (game->askSave(true))
			return;
		game->pause();
	}

	filename = QString();

	editingEnded();
	delete game;
	game = nullptr;
	loadedGame = QString();

	editingAction->setChecked(false);
	setEditingEnabled(false);
	endAction->setEnabled(false);
	aboutAction->setEnabled(false);
	highScoreAction->setEnabled(false);
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

	titleChanged(QString());
	updateModified(false);

	QTimer::singleShot(100, this, &KolfWindow::createSpacer);
}

void KolfWindow::createSpacer()
{
	// make a player to play the spacer hole
	spacerPlayers.clear();
	spacerPlayers.append(Player());
	spacerPlayers.last().ball()->setColor(Qt::yellow);
	spacerPlayers.last().setName(QStringLiteral("player"));
	spacerPlayers.last().setId(1);

	delete spacer;
	spacer = new KolfGame(m_itemFactory, &spacerPlayers, QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("intro")), dummy);
	spacer->setSound(false);
	layout->addWidget(spacer, 0, 0);//, Qt::AlignCenter);
	spacer->ignoreEvents(true);

	spacer->show();
	spacer->startFirstHole(1);
	spacer->hidePutter();
}

void KolfWindow::gameOver()
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
			QString winners = names.join( i18n(" and " ));
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
		scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), QStringLiteral("Par"));

		CourseInfo courseInfo;
		game->courseInfo(courseInfo, game->curFilename());

		scoreDialog->setConfigGroup(qMakePair(QByteArray(courseInfo.untranslatedName.toUtf8() + " Highscores"), i18n("High Scores for %1", courseInfo.name)));

		for (HighScoreList::Iterator it = highScores.begin(); it != highScores.end(); ++it)
		{
			KScoreDialog::FieldInfo info;
                        info[KScoreDialog::Name] = (*it).name;
                        info[KScoreDialog::Score].setNum((*it).score);
			info[KScoreDialog::Custom1] = QString::number(curPar);

                        scoreDialog->addScore(info, KScoreDialog::LessIsMore);
		}

		scoreDialog->setComment(i18n("High Scores for %1", courseInfo.name));
		scoreDialog->show();
	}

	QTimer::singleShot(700, this, &KolfWindow::closeGame);
}

void KolfWindow::showHighScores()
{
	KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
	scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), QStringLiteral("Par"));

	CourseInfo courseInfo;
	game->courseInfo(courseInfo, game->curFilename());

	scoreDialog->setConfigGroup(qMakePair(QByteArray(courseInfo.untranslatedName.toUtf8() + " Highscores"), i18n("High Scores for %1", courseInfo.name)));
	scoreDialog->setComment(i18n("High Scores for %1", courseInfo.name));
	scoreDialog->show();
}

void KolfWindow::save()
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

void KolfWindow::saveAs()
{
	QPointer<QFileDialog> fileSaveDialog = new QFileDialog(this);
	fileSaveDialog->setWindowTitle(i18nc("@title:window", "Pick Kolf Course to Save To"));
	fileSaveDialog->setMimeTypeFilters(QStringList(QStringLiteral("application/x-kourse")));
	fileSaveDialog->setAcceptMode(QFileDialog::AcceptSave);
	if (fileSaveDialog->exec() == QDialog::Accepted) {
		QUrl newfile = fileSaveDialog->selectedUrls().first();
		if (!newfile.isEmpty()) {
			filename = newfile.toLocalFile();
			game->setFilename(filename);
			game->save();
			game->setFocus();
		}
	}
	delete fileSaveDialog;
}

void KolfWindow::saveGameAs()
{
		QPointer<QFileDialog> fileSaveDialog = new QFileDialog(this);
		fileSaveDialog->setWindowTitle(i18nc("@title:window", "Pick Saved Game to Save To"));
		fileSaveDialog->setMimeTypeFilters(QStringList(QStringLiteral("application/x-kolf")));
		fileSaveDialog->setAcceptMode(QFileDialog::AcceptSave);
		if (fileSaveDialog->exec() == QDialog::Accepted) {
			QUrl newfile = fileSaveDialog->selectedUrls().first();
			if (newfile.isEmpty()) {
				return;
			}
			else {
				loadedGame = newfile.toLocalFile();
				saveGame();
			}
		}
		delete fileSaveDialog;
}

void KolfWindow::saveGame()
{
	if (loadedGame.isNull())
	{
		saveGameAs();
		return;
	}

	KConfig config(loadedGame);
	KConfigGroup configGroup(config.group("0 Saved Game"));

	configGroup.writeEntry("Competition", competition);
	configGroup.writeEntry("Course", filename);

	game->saveScores(&config);

	configGroup.sync();
}

void KolfWindow::loadGame()
{
	QPointer<QFileDialog> fileLoadDialog = new QFileDialog(this);
	fileLoadDialog->setWindowTitle(i18nc("@title:window", "Pick Kolf Saved Game"));
	fileLoadDialog->setMimeTypeFilters(QStringList(QStringLiteral("application/x-kolf")));
	fileLoadDialog->setAcceptMode(QFileDialog::AcceptOpen);
	fileLoadDialog->setFileMode(QFileDialog::ExistingFile);
	if (fileLoadDialog->exec() == QDialog::Accepted) {
		loadedGame = fileLoadDialog->selectedUrls().first().toLocalFile();
		if (loadedGame.isEmpty()) {
			return;
		}
		else {
			isTutorial = false;
			startNewGame();
		}
	}
	delete fileLoadDialog;
}

// called by main for command line files
void KolfWindow::openUrl(const QUrl &url)
{
	QTemporaryFile tempFile;
	tempFile.open();
	KIO::FileCopyJob *job = KIO::file_copy(url, QUrl::fromLocalFile(tempFile.fileName()), -1, KIO::Overwrite);
	KJobWidgets::setWindow(job, this);
	job->exec();
	if (!job->error())
	{
		isTutorial = false;
		QMimeDatabase db;
        QString mimeType = db.mimeTypeForFile(tempFile.fileName()).name();
		if (mimeType == QLatin1String("application/x-kourse"))
			filename = tempFile.fileName();
		else if (mimeType == QLatin1String("application/x-kolf"))
			loadedGame = tempFile.fileName();
		else
		{
			closeGame();
			return;
		}

		QTimer::singleShot(10, this, &KolfWindow::startNewGame);
	}
	else
		closeGame();
}

void KolfWindow::newPlayersTurn(Player *player)
{
	tempStatusBarText = i18n("%1's turn", player->name());

	if (showInfoAction->isChecked())
		statusBar()->showMessage(tempStatusBarText, 5 * 1000);
	else
		statusBar()->showMessage(tempStatusBarText);

	scoreboard->setCurrentCell(player->id() - 1, game->currentHole() - 1);
}

void KolfWindow::newStatusText(const QString &text)
{
	if (text.isEmpty())
		statusBar()->showMessage(tempStatusBarText);
	else
		statusBar()->showMessage(text);
}

void KolfWindow::editingStarted()
{
	delete editor;
	editor = new Editor(m_itemFactory, dummy);
	editor->setObjectName( QStringLiteral( "Editor" ) );
	connect(editor, &Editor::addNewItem, game, &KolfGame::addNewObject);
	connect(editor, &Editor::changed, game, &KolfGame::setModified);
	connect(editor, &Editor::addNewItem, this, &KolfWindow::setHoleFocus);
	connect(game, &KolfGame::newSelectedItem, editor, &Editor::setItem);

	scoreboard->hide();

	layout->addWidget(editor, 1, 0);
	editor->show();

	clearHoleAction->setEnabled(true);
	newHoleAction->setEnabled(true);
	setHoleOtherEnabled(false);

	game->setFocus();
}

void KolfWindow::editingEnded()
{
	delete editor;
	editor = nullptr;

	if (scoreboard)
		scoreboard->show();

	clearHoleAction->setEnabled(false);
	newHoleAction->setEnabled(false);
	setHoleOtherEnabled(true);

	if (game)
		game->setFocus();
}

void KolfWindow::inPlayStart()
{
	setEditingEnabled(false);
	setHoleOtherEnabled(false);
	setHoleMovementEnabled(false);
}

void KolfWindow::inPlayEnd()
{
	setEditingEnabled(true);
	setHoleOtherEnabled(true);
	setHoleMovementEnabled(true);
}

void KolfWindow::maxStrokesReached(const QString &name)
{
	KMessageBox::information(this, i18n("%1's score has reached the maximum for this hole.", name));
}

void KolfWindow::updateHoleMenu(int largest)
{
	QStringList items;
	for (int i = 1; i <= largest; ++i)
		items.append(QString::number(i));

	// setItems for some reason enables the action
	bool shouldbe = holeAction->isEnabled();
	holeAction->setItems(items);
	holeAction->setEnabled(shouldbe);
}

void KolfWindow::setHoleMovementEnabled(bool yes)
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

void KolfWindow::setHoleOtherEnabled(bool yes)
{
	if (competition)
		yes = false;

	resetHoleAction->setEnabled(yes);
	undoShotAction->setEnabled(yes);
	//replayShotAction->setEnabled(yes);
}

void KolfWindow::setEditingEnabled(bool yes)
{
	editingAction->setEnabled(competition? false : yes);
}

void KolfWindow::checkEditing()
{
	editingAction->setChecked(true);
}

void KolfWindow::updateModified(bool mod)
{
	courseModified = mod;
	titleChanged(title);
}

void KolfWindow::titleChanged(const QString &newTitle)
{
	title = newTitle;
	setCaption(title, courseModified);
}

void KolfWindow::useMouseChanged(bool yes)
{
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings"); configGroup.writeEntry("useMouse", yes); configGroup.sync();
}

void KolfWindow::useAdvancedPuttingChanged(bool yes)
{
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings"); configGroup.writeEntry("useAdvancedPutting", yes); configGroup.sync();
}

void KolfWindow::showInfoChanged(bool yes)
{
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings"); configGroup.writeEntry("showInfo", yes); configGroup.sync();
}

void KolfWindow::showGuideLineChanged(bool yes)
{
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings"); configGroup.writeEntry("showGuideLine", yes); configGroup.sync();
}

void KolfWindow::soundChanged(bool yes)
{
	KConfigGroup configGroup(KSharedConfig::openConfig(), "Settings"); configGroup.writeEntry("sound", yes); configGroup.sync();
}

void KolfWindow::enableAllMessages()
{
	KMessageBox::enableAllMessages();
}

void KolfWindow::setCurrentHole(int hole)
{
	if (!holeAction || holeAction->items().count() < hole)
		return;
	// Golf is 1-based, KListAction is 0-based
	holeAction->setCurrentItem(hole - 1);
}

#include "moc_kolf.cpp"
