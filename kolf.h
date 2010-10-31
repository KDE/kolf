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

#ifndef KOLF_H
#define KOLF_H

#include <kolflib_export.h>
#include <KXmlGuiWindow>

#include "game.h"
#include "itemfactory.h"

class QGridLayout;
class KAction;
class KSelectAction;
class KToggleAction;

class Editor;
class ScoreBoard;

class KOLFLIB_EXPORT KolfWindow : public KXmlGuiWindow
{
	Q_OBJECT

public:
	KolfWindow();
	~KolfWindow();

	void openUrl(KUrl url);

public slots:
	void closeGame();
	void updateModified(bool);

protected:
	virtual bool queryClose();

protected slots:
	void startNewGame();
	void loadGame();
	void tutorial();
	void newGame();
	void save();
	void saveAs();
	void saveGame();
	void saveGameAs();
	void print();
	void newPlayersTurn(Player *);
	void gameOver();
	void editingStarted();
	void editingEnded();
	void checkEditing();
	void setHoleFocus() { game->setFocus(); }
	void inPlayStart();
	void inPlayEnd();
	void maxStrokesReached(const QString &);
	void updateHoleMenu(int);
	void titleChanged(const QString &);
	void newStatusText(const QString &);
	void showInfoChanged(bool);
	void useMouseChanged(bool);
	void useAdvancedPuttingChanged(bool);
	void showGuideLineChanged(bool);
	void soundChanged(bool);
	void showHighScores();
	void enableAllMessages();
	void createSpacer();

	void emptySlot() {}

	void setCurrentHole(int);

private:
	QWidget *dummy;
	KolfGame *game;
	Editor *editor;
	KolfGame *spacer;
	void setupActions();
	QString filename;
	PlayerList players;
	PlayerList spacerPlayers;
	QGridLayout *layout;
	ScoreBoard *scoreboard;
	KToggleAction *editingAction;
	KAction *newHoleAction;
	KAction *resetHoleAction;
	QAction *undoShotAction;
	//QAction *replayShotAction;
	KAction *clearHoleAction;
	QAction *tutorialAction;
	QAction *newAction;
	QAction *endAction;
	//QAction *printAction;
	QAction *saveAction;
	QAction *saveAsAction;
	QAction *saveGameAction;
	QAction *saveGameAsAction;
	QAction *loadGameAction;
	QAction *aboutAction;
	KSelectAction *holeAction;
	QAction *highScoreAction;
	KAction *nextAction;
	KAction *prevAction;
	KAction *firstAction;
	KAction *lastAction;
	QAction *randAction;
	KToggleAction *showInfoAction;
	KToggleAction *useMouseAction;
	KToggleAction *useAdvancedPuttingAction;
	KToggleAction *showGuideLineAction;
	KToggleAction *soundAction;
	void setHoleMovementEnabled(bool);
	void setHoleOtherEnabled(bool);
	inline void setEditingEnabled(bool);
	bool competition;

	Kolf::ItemFactory m_itemFactory;

	QString loadedGame;

	bool isTutorial;
	bool courseModified;
	QString title;
	QString tempStatusBarText;
};

struct HighScore
{
	HighScore() {}
	HighScore(const QString &name, int score) { this->name = name; this->score = score; }
	QString name;
	int score;
};
typedef QList<HighScore> HighScoreList;

#endif
