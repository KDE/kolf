#ifndef KOLF_H
#define KOLF_H

#include <kmainwindow.h>
#include <kurl.h>

#include <qmap.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qwidget.h>
#include <qvaluelist.h>

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

class Kolf : public KMainWindow
{
	Q_OBJECT

public:
	Kolf();
	~Kolf();

protected:
	void closeEvent(QCloseEvent *);

protected slots:
	void startNewGame();
	void tutorial();
	void newGame();
	void closeGame();
	void save();
	void saveAs();
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
	void useMouseChanged(bool);
	void useAdvancedPuttingChanged(bool);	
	void showGuideLineChanged(bool);	
	void soundChanged(bool);	
	void initPlugins();
	void showPlugins();
	void keyBindings();
	void showHighScores();
 
private:
	QWidget *dummy;
	KolfGame *game;
	Editor *editor;
	QWidget *spacer;
	inline void initGUI();
	QString filename;
	PlayerList players;
	QGridLayout *layout;
	ScoreBoard *scoreboard;
	KToggleAction *editingAction;
	KAction *newHoleAction;
	KAction *resetHoleAction;
	KAction *undoShotAction;
	KAction *clearHoleAction;
	KAction *tutorialAction;
	KAction *newAction;
	KAction *endAction;
	KAction *printAction;
	KAction *saveAction;
	KAction *aboutAction;
	KListAction *holeAction;
	KAction *highScoreAction;
	KAction *nextAction;
	KAction *prevAction;
	KAction *firstAction;
	KAction *lastAction;
	KAction *randAction;
	KAction *saveAsAction;
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
