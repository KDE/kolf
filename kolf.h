#ifndef KOLF_H
#define KOLF_H

#include <kmainwindow.h>
#include <kurl.h>

#include <qmap.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qwidget.h>
#include <qvaluelist.h>
#include <kdemacros.h>
#include "game.h"

class KolfGame;
class KToggleAction;
class KListAction;
class KAction;
class QGridLayout;
class ScoreBoard;
class QCloseEvent;
class QEvent;
class Player;
class QWidget;
class Editor;

class KDE_EXPORT Kolf : public KMainWindow
{
	Q_OBJECT

public:
	Kolf();
	~Kolf();

	void openURL(KURL url);

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
	void initPlugins();
	void showPlugins();
	void showHighScores();
	void enableAllMessages();
	void createSpacer();

	void emptySlot() {};

	void setCurrentHole(int);

private:
	QWidget *dummy;
	KolfGame *game;
	Editor *editor;
	KolfGame *spacer;
	void initGUI();
	QString filename;
	PlayerList players;
	PlayerList spacerPlayers;
	QGridLayout *layout;
	ScoreBoard *scoreboard;
	KToggleAction *editingAction;
	KAction *newHoleAction;
	KAction *resetHoleAction;
	KAction *undoShotAction;
	//KAction *replayShotAction;
	KAction *clearHoleAction;
	KAction *tutorialAction;
	KAction *newAction;
	KAction *endAction;
	KAction *printAction;
	KAction *saveAction;
	KAction *saveAsAction;
	KAction *saveGameAction;
	KAction *saveGameAsAction;
	KAction *loadGameAction;
	KAction *aboutAction;
	KListAction *holeAction;
	KAction *highScoreAction;
	KAction *nextAction;
	KAction *prevAction;
	KAction *firstAction;
	KAction *lastAction;
	KAction *randAction;
	KToggleAction *showInfoAction;
	KToggleAction *useMouseAction;
	KToggleAction *useAdvancedPuttingAction;
	KToggleAction *showGuideLineAction;
	KToggleAction *soundAction;
	void setHoleMovementEnabled(bool);
	void setHoleOtherEnabled(bool);
	inline void setEditingEnabled(bool);
	bool competition;

	// contains everything
	ObjectList *obj;
	// contains subset of obj
	ObjectList plugins;

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
typedef QValueList<HighScore> HighScoreList;

#endif
