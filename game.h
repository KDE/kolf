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

#include "tagaro/scene.h"

#include <KLocale>
#include <KConfigGroup>

class QLabel;
class QSlider;
class QCheckBox;
class QVBoxLayout;
class KolfGame;
class KGameRenderer;

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
}

enum Direction { D_Left, D_Right, Forwards, Backwards };
enum Amount { Amount_Less, Amount_Normal, Amount_More };
enum HoleResult { Result_Holed, Result_Miss, Result_LipOut };

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
	Player() : m_ball(new Ball(0)) {}
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

class HintedLineItem : public QGraphicsLineItem
{
public:
	HintedLineItem(bool antialiased, QGraphicsItem *parent);

	virtual void paint(QPainter *p, const QStyleOptionGraphicsItem *style, QWidget *widget);
private:
	bool m_antialiased;
};

class Arrow : public HintedLineItem
{
public:
	Arrow(QGraphicsItem *parent);
	void setAngle(double newAngle) { m_angle = newAngle; }
	double angle() const { return m_angle; }
	void setLength(double newLength) { m_length = newLength; }
	double length() const { return m_length; }
	void setReversed(bool yes) { m_reversed = yes; }
	bool reversed() const { return m_reversed; }
	virtual void setVisible(bool);
	virtual void setPen(QPen p);
	void aboutToDie();
	virtual void moveBy(double, double);
	void updateSelf();
	virtual void setZValue(double newz);

private:
	/*
	 * base numbers are the size or position when no resizing has taken place (i.e. the defaults)
	 */
	double baseLineThickness;
	double m_angle;
	double m_length;
	bool m_reversed;
	QGraphicsLineItem *line1;
	QGraphicsLineItem *line2;
};

class RectPoint : public QGraphicsEllipseItem, public CanvasItem
{
public:
	RectPoint(const QColor &color, Tagaro::SpriteObjectItem *rect, QGraphicsItem *parent);
	void dontMove() { dontmove = true; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent);
	virtual bool deleteable() const { return false; }
	virtual bool cornerResize() const { return true; }
	virtual CanvasItem *itemToDelete() { return dynamic_cast<CanvasItem *>(rect); }
	void setSizeFactor(double newFactor) { m_sizeFactor = newFactor; }
	virtual void setSize(const QSizeF& size);

protected:
	Tagaro::SpriteObjectItem *rect;
	double m_sizeFactor;

private:
	bool dontmove;
};

class KolfEllipse : public EllipticalCanvasItem
{
public:
	KolfEllipse(QGraphicsItem *parent, const QString &type);
	virtual void advance(int phase);

	int changeEvery() const { return m_changeEvery; }
	void setChangeEvery(int news) { m_changeEvery = news; }
	bool changeEnabled() const { return m_changeEnabled; }
	void setChangeEnabled(bool news);

	virtual void aboutToDie();
	virtual void aboutToSave();
	virtual void savingDone();

	virtual QList<QGraphicsItem *> moveableItems() const;

	virtual void moveBy(double dx, double dy);

	virtual void editModeChanged(bool changed);

	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);

	virtual Config *config(QWidget *parent);

protected:
	RectPoint *point;
	int m_changeEvery;
	bool m_changeEnabled;

private:
	int count;
	bool dontHide;
};
class EllipseConfig : public Config
{
	Q_OBJECT

public:
	EllipseConfig(KolfEllipse *ellipse, QWidget *);

private slots:
	void value1Changed(int news);
	void value2Changed(int news);
	void check1Changed(bool on);
	void check2Changed(bool on);

protected:
	QVBoxLayout *m_vlayout;

private:
	QLabel *slow1;
	QLabel *fast1;
	QLabel *slow2;
	QLabel *fast2;
	QSlider *slider1;
	QSlider *slider2;
	KolfEllipse *ellipse;
};

class Puddle : public KolfEllipse
{
public:
	Puddle(QGraphicsItem *parent);
	virtual bool collision(Ball *ball, long int id);
};

class Sand : public KolfEllipse
{
public:
	Sand(QGraphicsItem *parent);
	virtual bool collision(Ball *ball, long int id);
};

class Bumper : public EllipticalCanvasItem
{
Q_OBJECT
public:
	Bumper(QGraphicsItem *parent);

	virtual bool collision(Ball *ball, long int id);
public Q_SLOTS:
	void turnBumperOff();
};

 class Cup : public EllipticalCanvasItem
{
public:
	Cup(QGraphicsItem *parent);

	virtual bool place(Ball *ball, bool wasCenter);
	virtual bool canBeMovedByOthers() const { return true; }
	virtual bool collision(Ball *ball, long int id);

protected:
	virtual HoleResult result(const QPointF, double, bool *wasCenter);
};

class BlackHole;
class BlackHoleConfig : public Config
{
	Q_OBJECT

public:
	BlackHoleConfig(BlackHole *blackHole, QWidget *parent);

private slots:
	void degChanged(int);
	void minChanged(double);
	void maxChanged(double);

private:
	BlackHole *blackHole;
};

class BlackHoleExit : public HintedLineItem, public CanvasItem
{
public:
	BlackHoleExit(BlackHole *blackHole, QGraphicsItem *parent);
	virtual void aboutToDie();
	virtual void moveBy(double dx, double dy);
	virtual bool deleteable() const { return false; }
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void editModeChanged(bool editing);
	virtual void setPen(const QPen& p);
	virtual void showInfo();
	virtual void hideInfo();
	void updateArrowAngle();
	void updateArrowLength();
	void setArrowPen(QPen pen) { arrow->setPen(pen); }
	virtual Config *config(QWidget *parent);
	BlackHole *blackHole;
protected:
	Arrow *arrow;
};
class BlackHoleTimer : public QObject
{
Q_OBJECT

public:
	BlackHoleTimer(Ball *ball, double speed, int msec);

signals:
	void eject(Ball *ball, double speed);
	void halfway();

protected slots:
	void mySlot();
	void myMidSlot();

protected:
	double m_speed;
	Ball *m_ball;
};
class BlackHole : public EllipticalCanvasItem
{
Q_OBJECT

public:
	BlackHole(QGraphicsItem *parent);
	virtual bool canBeMovedByOthers() const { return true; }

	virtual void aboutToDie();
	virtual void showInfo();
	virtual void hideInfo();
	virtual bool place(Ball *ball, bool wasCenter);
	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);
	virtual Config *config(QWidget *parent) { return new BlackHoleConfig(this, parent); }
	virtual QList<QGraphicsItem *> moveableItems() const;
	double minSpeed() const { return m_minSpeed; }
	double maxSpeed() const { return m_maxSpeed; }
	void setMinSpeed(double news) { m_minSpeed = news; exitItem->updateArrowLength(); }
	void setMaxSpeed(double news) { m_maxSpeed = news; exitItem->updateArrowLength(); }

	int curExitDeg() const { return exitDeg; }
	void setExitDeg(int newdeg);

	virtual void editModeChanged(bool editing) { exitItem->editModeChanged(editing); }
	void updateInfo();

	virtual void shotStarted() { runs = 0; }

	virtual void moveBy(double dx, double dy);

	virtual bool collision(Ball *ball, long int id);

public slots:
	void eject(Ball *ball, double speed);
	void halfway();

protected:
	int exitDeg;
	BlackHoleExit *exitItem;
	double m_minSpeed;
	double m_maxSpeed;
	virtual HoleResult result(const QPointF, double, bool *wasCenter);

private:
	int runs;
	QGraphicsLineItem *infoLine;
	void finishMe();
};

class WallPoint;
class Wall : public HintedLineItem, public CanvasItem
{
public:
	Wall(QGraphicsItem *parent, bool antialiasing = true);
	virtual void aboutToDie();
	double dampening;

	void setAlwaysShow(bool yes);
	virtual void setZValue(double newz);
	virtual void setPen(QPen p);
	virtual bool collision(Ball *ball, long int id);
	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);
	virtual void selectedItem(QGraphicsItem *item);
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void setPos(double x, double y);
	void setPos(const QPointF& p) { setPos(p.x(), p.y()); }
	virtual void clean();

	// must reimp because we gotta move the end items,
	// and we do that in moveBy()
	virtual void setPoints(double xa, double ya, double xb, double yb) { setLine(xa, ya, xb, yb); moveBy(0, 0); }

	virtual QList<QGraphicsItem *> moveableItems() const;
	virtual void setGame(KolfGame *game);
	virtual void setVisible(bool);

	QPointF startPointF() const { return line().p1(); }
	QPointF endPointF() const { return line().p2(); }
	QPoint startPoint() const { return line().p1().toPoint(); }
	QPoint endPoint() const { return line().p2().toPoint(); }

	void doAdvance();
protected:
	WallPoint *startItem;
	WallPoint *endItem;
	bool editing;

private:
	long int lastId;

	friend class WallPoint;
};
class WallPoint : public QGraphicsEllipseItem, public CanvasItem
{
public:
	WallPoint(bool start, Wall *wall, QGraphicsItem *parent);
	void setAlwaysShow(bool yes) { alwaysShow = yes; updateVisible(); }
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual bool deleteable() const { return false; }
	virtual bool collision(Ball *ball, long int id);
	virtual CanvasItem *itemToDelete() { return wall; }
	virtual void clean();
	virtual Config *config(QWidget *parent) { return wall->config(parent); }
	void dontMove() { dontmove = true; }
	void updateVisible();

	virtual void setSize(const QSizeF& size) { setRect(QRectF(rect().topLeft(), size)); }
	double width() { return rect().width(); }
	double height() { return rect().height(); }

	Wall *parentWall() { return wall; }
protected:
	Wall *wall;
	bool editing;
	bool visible;

private:
	bool alwaysShow;
	bool start;
	bool dontmove;
	long int lastId;

	friend class Wall;
};

class Putter : public HintedLineItem, public CanvasItem
{
public:
	Putter(QGraphicsItem* parent);

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


class Bridge;
class BridgeConfig : public Config
{
	Q_OBJECT

public:
	BridgeConfig(Bridge *bridge, QWidget *);

protected slots:
	void topWallChanged(bool);
	void botWallChanged(bool);
	void leftWallChanged(bool);
	void rightWallChanged(bool);

protected:
	QVBoxLayout *m_vlayout;
	QCheckBox *top;
	QCheckBox *bot;
	QCheckBox *left;
	QCheckBox *right;

private:
	Bridge *bridge;
};

class Bridge : public Tagaro::SpriteObjectItem, public CanvasItem
{
public:
	Bridge(QGraphicsItem *parent, const QString &type = QLatin1String("bridge"));

	virtual bool collision(Ball *ball, long int id);
	virtual void aboutToDie();
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void load(KConfigGroup *cfgGroup);
	virtual void save(KConfigGroup *cfgGroup);
	virtual bool vStrut() const { return true; }
	void doLoad(KConfigGroup *cfgGroup);
	void doSave(KConfigGroup *cfgGroup);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new BridgeConfig(this, parent); }
	virtual void setSize(const QSizeF& size);
	virtual QList<QGraphicsItem *> moveableItems() const;

	void setWallColor(const QColor &color);
	QPen wallPen() const { return topWall->pen(); }

	double wallZ() const { return topWall->zValue(); }
	void setWallZ(double);

	void setTopWallVisible(bool yes) { topWall->setVisible(yes); }
	void setBotWallVisible(bool yes) { botWall->setVisible(yes); }
	void setLeftWallVisible(bool yes) { leftWall->setVisible(yes); }
	void setRightWallVisible(bool yes) { rightWall->setVisible(yes); }
	bool topWallVisible() const { return topWall->isVisible(); }
	bool botWallVisible() const { return botWall->isVisible(); }
	bool leftWallVisible() const { return leftWall->isVisible(); }
	bool rightWallVisible() const { return rightWall->isVisible(); }
	
	double width() {return Tagaro::SpriteObjectItem::size().width(); }
	double height() {return Tagaro::SpriteObjectItem::size().height(); }

protected:
	Wall *topWall;
	Wall *botWall;
	Wall *leftWall;
	Wall *rightWall;
	RectPoint *point;
};

class Sign;
class SignConfig : public BridgeConfig
{
	Q_OBJECT

public:
	SignConfig(Sign *sign, QWidget *parent);

private slots:
	void textChanged(const QString &);

private:
	Sign *sign;
};
class Sign : public Bridge
{
public:
	Sign(QGraphicsItem *parent);

	void setText(const QString &text);
	QString text() const { return m_text; }
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
	virtual bool vStrut() const { return false; }
	virtual Config *config(QWidget *parent) { return new SignConfig(this, parent); }
	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);

protected:
	QString m_text;
	QString m_untranslatedText;
};

class Windmill;
class WindmillGuard : public Wall
{
public:
	WindmillGuard(QGraphicsItem *parent) : Wall(parent, false) {}
	void setBetween(double newmin, double newmax) { max = newmax; min = newmin; }
	virtual void advance(int phase);
	double getMax() { return max; }
	double getMin() { return min; }

protected:
	double max;
	double min;
};
class WindmillConfig : public BridgeConfig
{
	Q_OBJECT

public:
	WindmillConfig(Windmill *windmill, QWidget *parent);

private slots:
	void speedChanged(int news);
	void endChanged(bool yes);

private:
	Windmill *windmill;
};
class Windmill : public Bridge
{
public:
	Windmill(QGraphicsItem *parent);
	virtual void aboutToDie();
	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new WindmillConfig(this, parent); }

	virtual void setSize(const QSizeF& size);
	virtual void moveBy(double dx, double dy);
	void setSpeed(double news);
	double curSpeed() const { return speed; }
	void setBottom(bool yes);
	bool bottom() const { return m_bottom; }

protected:
	WindmillGuard *guard;
	Wall *left;
	Wall *right;

	int speedfactor;
	double speed;
	bool m_bottom;
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
	HoleInfo() { m_lowestMaxStrokes = 4; }
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
	void removeItem(QGraphicsItem *item) { items.removeAll(item); }
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
	QGraphicsItem *curSelectedItem() const { return selectedItem; }
	void setBorderWalls(bool);
	void setInPlay(bool yes) { inPlay = yes; }
	bool isInPlay() { return inPlay; }
	bool isInfoShowing() { return m_showInfo; }
	void stoppedBall();
	QString courseName() const { return holeInfo.name(); }
	void hidePutter() { putter->setVisible(false); }
	void ignoreEvents(bool ignore) { m_ignoreEvents = ignore; }

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
	void addItemsToMoveableList(QList<QGraphicsItem *>);

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
	QList<QGraphicsItem *> items;
	QList<QGraphicsItem *> extraMoveable;
	QList<Wall *> borderWalls;

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
