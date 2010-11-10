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

#ifndef GAME_H
#define GAME_H

//#define SOUND

#include <kolflib_export.h>
#include "ball.h"
#include "obstacles.h"

#include "tagaro/scene.h"

#include <KLocale>
#include <KConfigGroup>

class QLabel;
class QSlider;
class QCheckBox;
class QVBoxLayout;
class KolfGame;
class KGameRenderer;

namespace Kolf
{
	class LineShape;
};
namespace Tagaro
{
	class Board;
}
namespace Phonon
{
    class MediaObject;
}
namespace Kolf
{
	class ItemFactory;
	KGameRenderer* renderer();
	Tagaro::Board* findBoard(QGraphicsItem* item); //TODO: temporary HACK
	b2World* world(); //TODO: temporary HACK (should be inside KolfGame, but various places outside the game need to create CanvasItems)
}

enum Direction { D_Left, D_Right, Forwards, Backwards };
enum Amount { Amount_Less, Amount_Normal, Amount_More };

class BallStateInfo
{
public:
	void saveState(KConfigGroup *cfgGroup);
	void loadState(KConfigGroup *cfgGroup);

	int id;
	QPoint spot;
	BallState state;
	bool beginningOfHole;
	int score;
};
class BallStateList : public QList<BallStateInfo>
{
public:
	int hole;
	int player;
	bool canUndo;
	Vector vector;
};

class Player
{
public:
	Player() : m_ball(new Ball(0, Kolf::world())) {}
	Ball *ball() const { return m_ball; }
	void setBall(Ball *ball) { m_ball = ball; }
	BallStateInfo stateInfo(int hole) const { BallStateInfo ret; ret.spot = m_ball->pos().toPoint(); ret.state = m_ball->curState(); ret.score = score(hole); ret.beginningOfHole = m_ball->beginningOfHole(); ret.id = m_id; return ret; }

	QList<int> scores() const { return m_scores; }
	void setScores(const QList<int> &newScores) { m_scores = newScores; }
	int score(int hole) const { return m_scores.at(hole - 1); }
	int lastScore() const { return m_scores.last(); }
	int firstScore() const { return m_scores.first(); }

	void addStrokeToHole(int hole) { (*(m_scores.begin() + (hole -1)))++; }
	void setScoreForHole(int score, int hole) { (*(m_scores.begin() + (hole - 1))) = score; }
	void subtractStrokeFromHole(int hole) { (*(m_scores.begin() + (hole -1))--); }
	void resetScore(int hole) { (*(m_scores.begin() + (hole - 1))) = 0; }
	void addHole() { m_scores.append(0); }
	unsigned int numHoles() const { return m_scores.count(); }

	QString name() const { return m_name; }
	void setName(const QString &name) { m_name = name; m_ball->setName(name); }

	void setId(int id) { m_id = id; }
	int id() const { return m_id; }

private:
	Ball *m_ball;
	QList<int> m_scores;
	QString m_name;
	int m_id;
};

typedef QList<Player> PlayerList;

class Putter : public QGraphicsLineItem, public CanvasItem
{
public:
	Putter(QGraphicsItem* parent, b2World* world);

	void go(Direction, Amount amount = Amount_Normal);
	void setOrigin(double x, double y);
	double curLen() const { return guideLineLength; }
	double curAngle() const { return angle; }
	int curDeg() const { return rad2deg(angle); }
	virtual void showInfo();
	virtual void hideInfo();
	void setAngle(double news) { angle = news; finishMe(); }
	void setDeg(int news) { angle = deg2rad(news); finishMe(); }
	double curMaxAngle() const { return maxAngle; }
	virtual void setVisible(bool yes);
	void saveAngle(Ball *ball) { angleMap[ball] = angle; }
	void setAngle(Ball *ball);
	void resetAngles() { angleMap.clear(); setZValue(999999); }
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void moveBy(double dx, double dy);
	void setShowGuideLine(bool yes);

	virtual QPointF getPosition() const { return QGraphicsItem::pos(); }
private:
	QPointF midPoint;
	double maxAngle;
	double angle;
	double oneDegree;
	QMap<Ball *, double> angleMap;
	double guideLineLength, putterWidth;
	void finishMe();
	QGraphicsLineItem *guideLine;
	bool m_showGuideLine;
};

class HoleInfo;
class HoleConfig : public Config
{
	Q_OBJECT

public:
	HoleConfig(HoleInfo *holeInfo, QWidget *);

private slots:
	void authorChanged(const QString &);
	void parChanged(int);
	void maxStrokesChanged(int);
	void nameChanged(const QString &);
	void borderWallsChanged(bool);

private:
	HoleInfo *holeInfo;
};
class HoleInfo : public CanvasItem
{
public:
	HoleInfo(b2World* world) : CanvasItem(world) { setSimulationType(CanvasItem::NoSimulation); m_lowestMaxStrokes = 4; }
	virtual ~HoleInfo() {}
	void setPar(int newpar) { m_par = newpar; }
	int par() const { return m_par; }
	void setMaxStrokes(int newMaxStrokes) { m_maxStrokes = newMaxStrokes; }
	int lowestMaxStrokes() const { return m_lowestMaxStrokes; }
	int maxStrokes() const { return m_maxStrokes; }
	bool hasMaxStrokes() const { return m_maxStrokes != m_lowestMaxStrokes; }
	void setAuthor(QString newauthor) { m_author = newauthor; }
	QString author() const { return m_author; }

	void setName(QString newname) { m_name = newname; }
	void setUntranslatedName(QString newname) { m_untranslatedName = newname; }
	QString name() const { return m_name; }
	QString untranslatedName() const { return m_untranslatedName; }

	virtual Config *config(QWidget *parent) { return new HoleConfig(this, parent); }
	void borderWallsChanged(bool yes);
	bool borderWalls() const { return m_borderWalls; }

	virtual void moveBy(double , double) {}
	virtual QPointF getPosition() const { return QPointF(); }
private:
	QString m_author;
	QString m_name;
	QString m_untranslatedName;
	bool m_borderWalls;
	int m_par;
	int m_maxStrokes;
	int m_lowestMaxStrokes;
};

class StrokeCircle : public QGraphicsItem
{
public:
	StrokeCircle(QGraphicsItem *parent);

	void setValue(double v);
	double value();
	void setMaxValue(double m);
	void setSize(const QSizeF& size);
	void setThickness(double t);
	double thickness() const;
	double width() const;
	double height() const;
	virtual void paint (QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	virtual QRectF boundingRect() const;
	virtual bool collidesWithItem(const QGraphicsItem*, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;

private:
	double dvalue, dmax;
	double ithickness, iwidth, iheight;
};

struct KOLFLIB_EXPORT CourseInfo
{
	CourseInfo(const QString &_name, const QString &_untranslatedName, const QString &_author, unsigned int _holes, unsigned int _par) { name = _name; untranslatedName = _untranslatedName, author = _author; holes = _holes; par = _par; }
	CourseInfo();

	QString name;
	QString untranslatedName;
	QString author;
	unsigned int holes;
	unsigned int par;
};

class KOLFLIB_EXPORT KolfGame : public QGraphicsView
{
	Q_OBJECT

public:
	KolfGame(const Kolf::ItemFactory& factory, PlayerList *players, const QString &filename, QWidget *parent=0);
	~KolfGame();
	void setFilename(const QString &filename);
	QString curFilename() const { return filename; }
	void emitLargestHole() { emit largestHole(highestHole); }
	QGraphicsScene *scene() const { return course; }
	void removeItem(QGraphicsItem *item) { m_topLevelQItems.removeAll(item); }
	bool askSave(bool);
	bool isEditing() const { return editing; }
	int currentHole() { return curHole; }
	void setStrict(bool yes) { strict = yes; }
	// returns true when you shouldn't do anything
	bool isPaused() const { return paused; }
	Ball *curBall() const { return (*curPlayer).ball(); }
	void updateMouse();
	void ballMoved();
	void updateHighlighter();
	void setBorderWalls(bool);
	void setInPlay(bool yes) { inPlay = yes; }
	bool isInPlay() { return inPlay; }
	bool isInfoShowing() { return m_showInfo; }
	void stoppedBall();
	QString courseName() const { return holeInfo.name(); }
	void hidePutter() { putter->setVisible(false); }
	void ignoreEvents(bool ignore) { m_ignoreEvents = ignore; }

	void overlayStateChanged(CanvasItem* citem);

	static void scoresFromSaved(KConfig*, PlayerList &players);
	static void courseInfo(CourseInfo &info, const QString &filename);

public slots:
	void pause();
	void unPause();
	void save();
	void toggleEditMode();
	void setModified(bool mod = true);
	void addNewObject(const QString& identifier);
	void addNewHole();
	void switchHole(int);
	void switchHole(const QString &);
	void nextHole();
	void prevHole();
	void firstHole();
	void lastHole();
	void randHole();
	void playSound(const QString &file, float vol = 1);
	void showInfoDlg(bool = false);
	void resetHole();
	void clearHole();
	void setShowInfo(bool yes);
	void toggleShowInfo();
	void updateShowInfo();
	void setUseMouse(bool yes) { m_useMouse = yes; }
	void setUseAdvancedPutting(bool yes);
	void setShowGuideLine(bool yes);
	void setSound(bool yes);
	void undoShot();
	void timeout();
	void saveScores(KConfig *);
	void startFirstHole(int hole);
	void sayWhosGoing();

signals:
	void holesDone();
	void newHole(int);
	void parChanged(int, int);
	void titleChanged(const QString &);
	void largestHole(int);
	void scoreChanged(int, int, int);
	void newPlayersTurn(Player *);
	void newSelectedItem(CanvasItem *);
	void checkEditing();
	void editingStarted();
	void editingEnded();
	void inPlayStart();
	void inPlayEnd();
	void maxStrokesReached(const QString &);
	void currentHole(int);
	void modifiedChanged(bool);
	void newStatusText(const QString &);

private slots:
	void shotDone();
	void holeDone();
	void startNextHole();
	void fastTimeout();
	void putterTimeout();
	void autoSaveTimeout();

	void emitMax();

protected:
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);

	void handleMousePressEvent(QMouseEvent *e);
	void handleMouseDoubleClickEvent(QMouseEvent *e);
	void handleMouseMoveEvent(QMouseEvent *e);
	void handleMouseReleaseEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);

	//resizes view to make sure it is square and calls resizeAllItems
	void resizeEvent(QResizeEvent *);

	QPoint viewportToViewport(const QPoint &p);

private:
	Tagaro::Scene *course;
	Tagaro::Board *courseBoard;
	Putter *putter;
	PlayerList *players;
	PlayerList::Iterator curPlayer;
	Ball *whiteBall;
	StrokeCircle *strokeCircle; 

	QTimer *timer;
	QTimer *autoSaveTimer;
	QTimer *fastTimer;
	QTimer *putterTimer;
	bool regAdv;

	const Kolf::ItemFactory& m_factory;
	QList<QGraphicsItem*> m_topLevelQItems; //includes balls, but not putter and highlighter
	QList<QGraphicsItem*> m_moveableQItems;

	QList<Kolf::Wall *> borderWalls;

	int timerMsec;
	int autoSaveMsec;
	int fastTimerMsec;
	int putterTimerMsec;

	void puttPress();
	void puttRelease();
	bool inPlay;
	bool putting;
	bool stroking;
	bool finishStroking;
	double strength;
	double maxStrength;
	int puttCount;
	bool puttReverse;

	int curHole;
	int highestHole;
	int curPar;

	int wallWidth;
	int height;
	int width;
	int margin;

	int advancePeriod;

	int lastDelId;

	bool paused;

	QString filename;
	bool recalcHighestHole;
	void openFile();

	bool strict;

	bool editing;
	QPoint storedMousePos;
	bool moving;
	bool dragging;
	QGraphicsItem *movingItem;
	CanvasItem *movingCanvasItem;
	QGraphicsItem *selectedItem;
	QGraphicsRectItem *highlighter;
	
	//For intro banner
	Tagaro::SpriteObjectItem *banner;

#ifdef SOUND
    Phonon::MediaObject *m_player;
#endif
	bool m_sound;
	QString soundDir;

	bool m_ignoreEvents;

	HoleInfo holeInfo;
	QMap<QString, QPointF> savedState;

	BallStateList ballStateList;
	void loadStateList();
	void recreateStateList();
	void addHoleInfo(BallStateList &list);

	bool dontAddStroke;

	bool addingNewHole;
	int scoreboardHoles;
	inline void resetHoleScores();

	bool m_showInfo;

	bool infoShown;

	KConfig *cfg;
	KConfigGroup cfgGroup;

	inline void addBorderWall(const QPoint &start, const QPoint &end);
	void shotStart();
	void startBall(const Vector &vector);

	bool modified;

	inline bool allPlayersDone();

	bool m_useMouse;
	bool m_useAdvancedPutting;

	QString playerWhoMaxed;
};

#endif
