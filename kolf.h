#ifndef KJUMP_H_INCLUDED
#define KJUMP_H_INCLUDED

#include <kmainwindow.h>
#include <kurl.h>

#include <qmap.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qwidget.h>

#include "game.h"

class KolfGame;
class KToggleAction;
class KListAction;
class KRecentFilesAction;
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

protected:
	void closeEvent(QCloseEvent *);

protected slots:
	void startNewGame();
	void tutorial();
	void newGame();
	void newSameGame();
	void closeGame();
	void openDefaultCourse(const QString &);
	void open();
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
	void maxStrokesReached();
	void updateHoleMenu(int);
	void openRecent(const KURL &);
	void useMouseChanged(bool);
 
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
	KAction *clearHoleAction;
	KAction *tutorialAction;
	KAction *newAction;
	KAction *newSameAction;
	KAction *endAction;
	KAction *openAction;
	KAction *printAction;
	KListAction *newDefaultAction;
	KRecentFilesAction *recentAction;
	KAction *saveAction;
	KAction *aboutAction;
	KListAction *holeAction;
	KAction *nextAction;
	KAction *prevAction;
	KAction *firstAction;
	KAction *lastAction;
	KAction *randAction;
	KAction *saveAsAction;
	KToggleAction *useMouseAction;
	void setHoleMovementEnabled(bool);
	inline void setEditingEnabled(bool);
	bool competition;

	QMap<QString, QString> defaults;
};

#endif
