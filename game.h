#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <arts/soundserver.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kpixmapeffect.h>

#include <math.h>

#include <qcanvas.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qframe.h>
#include <qmap.h>
#include <qpen.h>
#include <qpoint.h>
#include <qpointarray.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qwidget.h>

class QLabel;
class QSlider;
class QCheckBox;
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class KSimpleConfig;
class KolfGame;

enum BallState {Rolling, Stopped, Holed};
enum RttiCodes { Rtti_NoCollision = 1001, Rtti_DontPlaceOn = 1002, Rtti_Ball = 1003, Rtti_Putter = 1004, Rtti_WallPoint = 1005 };
enum Direction { D_Left, D_Right, Forwards, Backwards };
enum HoleResult { Result_Holed, Result_Miss, Result_LipOut };

class Ball;
class Player;

class Config : public QFrame
{
	Q_OBJECT

public:
	Config(QWidget *parent, const char *name = 0) : QFrame(parent, name) { startedUp = false; }
	void ctorDone() { startedUp = true; }

signals:
	void modified();

protected:
	inline int spacingHint();
	inline int marginHint();
	bool startedUp;
	inline void changed();
};

class DefaultConfig : public Config
{
public:
	DefaultConfig(QWidget *parent);
};

class CanvasItem
{
public:
	CanvasItem() { game = 0; }
	/**
	 * load your settings from the KSimpleConfig.
	 * You'll have to call KSimpleConfig::setGroup(this->makeGroup(...))
	 *
	 */
	virtual void load(KSimpleConfig *, int /*hole*/) {}
	/**
	 * returns a bool that is true if your item needs to load after other items
	 */
	virtual bool loadLast() { return false; }
	/**
	 * called after all items have had load() called.
	 */
	virtual void finalLoad() {}
	/**
	 * called after the item is moved the very first time by the game
	 */
	virtual void firstMove(int /*x*/, int /*y*/) {}
	/**
	 * same as load (you must call setGroup), but save your settings.
	 */
	virtual void save(KSimpleConfig *, int /*hole*/) {}
	/**
	 * called right before any items are saved.
	 */
	virtual void aboutToSave() {}
	/**
	 * called right after all items are saved.
	 */
	virtual void savingDone() {}
	/**
	 * called when the edit mode has been changed.
	 */
	virtual void editModeChanged(bool /*changed*/) {}
	/**
	 * the item should delete any other objects it's created.
	 * DO NOT DO THIS IN THE DESTRUCTOR!
	 */
	virtual void aboutToDie() {}
	/**
	 * called when user presses delete key while editing. This is very rarely reimplemented, and generally shouldn't be.
	 */
	virtual void aboutToDelete() {}
	/** returns whether this item should be able to be deleted by user while editing.
	 */
	virtual bool deleteable() { return true; }
	/**
	 * returns whether or not this item lifts items on top of it.
	 */
	virtual bool vStrut() { return false; }
	/**
	 * show extra item info
	 */
	virtual void showInfo() {};
	/**
	 * hide extra item info
	 */
	virtual void hideInfo() {};
	/**
	 * update your Z value (this is called by various things when perhaps the value should change)
	 */
	virtual void updateZ() {};
	/**
	 * returns whether this item can be moved by others (if you want to move an item, you should honor this!)
	 */
	virtual bool canBeMovedByOthers() { return false; }
	/**
	 * returns a Config that can be used to configure this item by the user.
	 * The default implementation returns one that says 'No configuration options'.
	 */
	virtual Config *config(QWidget *parent) { return new DefaultConfig(parent); }
	/**
	 * returns other items that should be moveable (besides this one of course).
	 */
	virtual QPtrList<QCanvasItem> moveableItems() { return QPtrList<QCanvasItem>(); }
	/**
	 * returns whether this can be moved by the user while editing.
	 */
	virtual bool moveable() { return true; }
	/**
	 * no need to reimplement.
	 */
	void setId(int newId) { id = newId; }
	/**
	 * no need to reimplement.
	 */
	void playSound(QString file);

	virtual void collision(Ball * /*ball*/, long int /*id*/) {};

	/**
	 * use this to KSimpleConfig::setGroup() in your load and save functions.
	 */
	inline QString makeGroup(int hole, QString name, int x, int y) { return QString("%1-%2@%3,%4|%5").arg(hole).arg(name).arg(x).arg(y).arg(id); }

	/**
	 * reimplement if you want extra items to have access to the game object.
	 * playSound() relies on having this.
	 */
	virtual void setGame(KolfGame *game) { this->game = game; }

protected:
	/**
	 * pointer to main KolfGame
	 */
	KolfGame *game;

	/**
	 * returns the highest vertical strut the item is on
	 */
	QCanvasItem *onVStrut();

private:
	int id;
};

class Ball : public QCanvasEllipse, public CanvasItem
{
public:
	Ball(QCanvas *canvas);
	BallState currentState();

	void resetSize() { setSize(7, 7); }
	virtual void advance(int phase);
	virtual void doAdvance() { QCanvasEllipse::advance(1); }

	double curSpeed() { return sqrt(xVelocity() * xVelocity() + yVelocity() * yVelocity()); }
	virtual bool canBeMovedByOthers() { return true; }

	BallState curState() { return state; }
	void setState(BallState newState);

	QColor color() { return m_color; }
	void setColor(QColor color) { m_color = color; setBrush(color); }

	void setMoved(bool yes) { m_moved = yes; }
	bool moved() { return m_moved; }
	void setBlowUp(bool yes) { m_blowUp = yes; blowUpCount = 0; }
	bool blowUp() { return m_blowUp; }

	void setFrictionMultiplier(double news) { frictionMultiplier = news; };
	void friction();
	void collisionDetect();

	virtual int rtti() const { return Rtti_Ball; };

	bool addStroke() { return m_addStroke; }
	bool placeOnGround(double &oldvx, double &oldvy) { oldvx = m_oldvx; oldvy = m_oldvy; return m_placeOnGround; }
	void setAddStroke(int newStrokes) { m_addStroke = newStrokes; }
	void setPlaceOnGround(bool placeOnGround, double oldvx, double oldvy) { m_placeOnGround = placeOnGround; m_oldvx = oldvx; m_oldvy = oldvy;}

	bool beginningOfHole() { return m_beginningOfHole; }
	void setBeginningOfHole(bool yes) { m_beginningOfHole = yes; }

private:
	BallState state;
	QColor m_color;
	long int collisionId;
	double frictionMultiplier;

	bool m_blowUp;
	int blowUpCount;
	int m_addStroke;
	bool m_placeOnGround;
	double m_oldvx;
	double m_oldvy;

	bool m_moved;
	bool m_beginningOfHole;
};

class Player
{
public:
	Player() : m_ball(new Ball(0)) {};
	Ball *ball() { return m_ball; }
	void setBall(Ball *ball) { m_ball = ball; }

	QValueList<int> scores() { return m_scores; }
	int score(int hole) { return (*m_scores.at(hole - 1)); }
	int lastScore() { return m_scores.last(); }
	int firstScore() { return m_scores.first(); }

	void addStrokeToHole(int hole) { (*m_scores.at(hole - 1))++; }
	void subtractStrokeFromHole(int hole) { (*m_scores.at(hole - 1))--; }
	void resetScore(int hole) { (*m_scores.at(hole - 1)) = 0; }
	void addHole() { m_scores.append(0); }
	unsigned int numHoles() { return m_scores.count(); }

	QString name() { return m_name; }
	void setName(const QString &name) { m_name = name; }

	void setId(int id) { m_id = id; }
	int id() { return m_id; }

private:
	Ball *m_ball;
	QValueList<int> m_scores;
	QString m_name;
	int m_id;
};
typedef QValueList<Player> PlayerList;

class Object
{
public:
	Object() {};
	virtual ~Object() {};
	virtual QCanvasItem *newObject(QCanvas * /*canvas*/) { return 0; };
	virtual QString name() { return m_name; }
	virtual QString _name() { return m__name; }

protected:
	QString m_name;
	QString m__name;
	QPtrList<CanvasItem> items;
};
typedef QPtrList<Object> ObjectList;

class RectPoint;
class RectItem
{
public:
	virtual void newSize(int /*width*/, int /*height*/) {};
};

class Slope;
class SlopeConfig : public Config
{
	Q_OBJECT

public:
	SlopeConfig(Slope *slope, QWidget *parent);

private slots:
	void setGradient(const QString &text);
	void setReversed(bool);
	void setStuckOnGround(bool);
	void gradeChanged(int);

private:
	Slope *slope;
};
class Slope : public QCanvasRectangle, public CanvasItem, public RectItem
{
public:
	Slope(QRect rect, QCanvas *canvas);
	virtual void aboutToDie();

	virtual void showInfo();
	virtual void hideInfo();
	virtual void editModeChanged(bool changed);
	virtual bool canBeMovedByOthers() { return !stuckOnGround; }
	virtual QPtrList<QCanvasItem> moveableItems();
	virtual Config *config(QWidget *parent) { return new SlopeConfig(this, parent); }
	void setSize(int, int);
	virtual void newSize(int width, int height);

	virtual void moveBy(double dx, double dy);

	virtual void draw(QPainter &painter);
	virtual QPointArray areaPoints() const;

	void setGradient(QString text);
	KPixmapEffect::GradientType curType() { return type; }
	void setGrade(int grade) { if (grade > 0 && grade < 11) { this->grade = grade; updatePixmap(); } }

	int curGrade() { return grade; }
	void setGradeVisible(bool visible) { showGrade = visible; update(); }
	void setColor(QColor color) { this->color = color; updatePixmap(); }
	void setReversed(bool reversed) { this->reversed = reversed; updatePixmap(); }
	bool isReversed() { return reversed; }

	bool isStuckOnGround() { return stuckOnGround; }
	void setStuckOnGround(bool yes) { stuckOnGround = yes; updateZ(); }

	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void save(KSimpleConfig *cfg, int hole);

	virtual void collision(Ball *ball, long int id);

	QMap<KPixmapEffect::GradientType, QString> gradientI18nKeys;
	QMap<KPixmapEffect::GradientType, QString> gradientKeys;

	virtual void updateZ();

private:
	KPixmapEffect::GradientType type;
	inline void setType(KPixmapEffect::GradientType type);
	int grade;
	bool showGrade;
	bool reversed;
	QColor color;
	KPixmap pixmap;
	void updatePixmap();
	bool stuckOnGround;

	RectPoint *point;
};
class SlopeObj : public Object
{
public:
	SlopeObj() { m_name = i18n("Slope"); m__name = "slope"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Slope(QRect(0, 0, 40, 40), canvas); }
};

class RectPoint : public QCanvasEllipse, public CanvasItem
{
public:
	RectPoint(QColor color, QCanvasRectangle *slope, QCanvas *canvas);
	void dontMove() { dontmove = true; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent) { return dynamic_cast<CanvasItem *>(rect)->config(parent); }
	virtual bool deleteable() { return false; }

private:
	bool dontmove;
	QCanvasRectangle *rect;
};

class Ellipse : public QCanvasEllipse, public CanvasItem
{
public:
	Ellipse(QCanvas *canvas);
	virtual void advance(int phase);

	int changeEvery() { return m_changeEvery; }
	void setChangeEvery(int news) { m_changeEvery = news; }
	bool changeEnabled() { return m_changeEnabled; }
	void setChangeEnabled(bool news) { setAnimated(news); m_changeEnabled = news; }

	virtual void aboutToSave();
	virtual void savingDone();

	virtual void doSave(KSimpleConfig *cfg, int hole);
	virtual void doLoad(KSimpleConfig *cfg, int hole);

	virtual Config *config(QWidget *parent);

private:
	int count;
	int m_changeEvery;
	bool m_changeEnabled;
	bool dontHide;
};
class EllipseConfig : public Config
{
	Q_OBJECT

public:
	EllipseConfig(Ellipse *ellipse, QWidget *);

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
	Ellipse *ellipse;
	bool startup;
};

class Puddle : public Ellipse
{
public:
	Puddle(QCanvas *canvas);
	virtual void collision(Ball *ball, long int id);
	virtual int rtti() const { return Rtti_DontPlaceOn; }
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void save(KSimpleConfig *cfg, int hole);
};
class PuddleObj : public Object
{
public:
	PuddleObj() { m_name = i18n("Puddle"); m__name = "puddle"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Puddle(canvas); }
};

class Sand : public Ellipse
{
public:
	Sand(QCanvas *canvas);
	virtual void collision(Ball *ball, long int id);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void save(KSimpleConfig *cfg, int hole);
};
class SandObj : public Object
{
public:
	SandObj() { m_name = i18n("Sand"); m__name = "sand"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Sand(canvas); }
};

class Hole : public QCanvasEllipse, public CanvasItem
{
public:
	Hole(QColor color, QCanvas *canvas);
	virtual bool place(Ball * /*ball*/, bool /*wasCenter*/) { return true; };

	virtual void collision(Ball *ball, long int id);

protected:
	virtual HoleResult result(const QPoint, double, bool *wasCenter);
};

class Cup : public Hole
{
public:
	Cup(QCanvas *canvas) : Hole(QColor("#FF923F"), canvas) {}
	virtual bool place(Ball *ball, bool wasCenter);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual bool canBeMovedByOthers() { return true; }
};
class CupObj : public Object
{
public:
	CupObj() { m_name = i18n("Cup"); m__name = "cup"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Cup(canvas); }
};

class BlackHole;
class BlackHoleConfig : public Config
{
	Q_OBJECT

public:
	BlackHoleConfig(BlackHole *blackHole, QWidget *parent);

private slots:
	void degChanged(int);
	void minChanged(int);
	void maxChanged(int);

private:
	BlackHole *blackHole;
};
class BlackHoleExit : public QCanvasLine, public CanvasItem
{
public:
	BlackHoleExit(BlackHole *blackHole, QCanvas *canvas);
	virtual int rtti() const { return Rtti_NoCollision; }
	virtual void showInfo();
	virtual void hideInfo();
	virtual bool deleteable() { return false; }
	virtual bool canBeMovedByOthers() { return true; }
	virtual Config *config(QWidget *parent);
	BlackHole *blackHole;

private:
	QCanvasLine *infoLine;
};
class BlackHole : public Hole
{
public:
	BlackHole(QCanvas *canvas);
	virtual bool canBeMovedByOthers() { return true; }
	virtual void aboutToDie();
	virtual void showInfo();
	virtual void hideInfo();
	virtual bool place(Ball *ball, bool wasCenter);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual Config *config(QWidget *parent) { return new BlackHoleConfig(this, parent); }
	virtual QPtrList<QCanvasItem> moveableItems();
	int minSpeed() { return m_minSpeed; }
	int maxSpeed() { return m_maxSpeed; }
	void setMinSpeed(int news) { m_minSpeed = news; }
	void setMaxSpeed(int news) { m_maxSpeed = news; }

	int curExitDeg() { return exitDeg; }
	void setExitDeg(int newdeg) { exitDeg = newdeg; finishMe(); }

private:
	int exitDeg;
	BlackHoleExit *exitItem;
	QCanvasLine *infoLine;

	void finishMe();
	int m_minSpeed;
	int m_maxSpeed;
};
class BlackHoleObj : public Object
{
public:
	BlackHoleObj() { m_name = i18n("Black Hole"); m__name = "blackhole"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new BlackHole(canvas); }
};

class WallPoint;
class Wall : public QCanvasLine, public CanvasItem
{
public:
	Wall(QCanvas *canvas);
	virtual void aboutToDie();
	double dampening() { return 1.22; }

	void setAlwaysShow(bool yes);
	virtual void setZ(double newz);
	virtual void setPen(QPen p);
	virtual void collision(Ball *ball, long int id);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(double vx, double vy);
	virtual void finalLoad();
	// must reimp because we gotta move the end items,
	// and we do that in :moveBy()
	virtual void setPoints(int xa, int ya, int xb, int yb) { QCanvasLine::setPoints(xa, ya, xb, yb); moveBy(0, 0); }
	virtual int rtti() const { return Rtti_DontPlaceOn; }
	virtual QPtrList<QCanvasItem> moveableItems();
	virtual void setGame(KolfGame *game);
	virtual void setVisible(bool);

	virtual QPointArray areaPoints() const;

private:
	int lastId;
	bool editing;
	WallPoint *startItem;
	WallPoint *endItem;
};
class WallPoint : public QCanvasEllipse, public CanvasItem
{
public:
	WallPoint(bool start, Wall *wall, QCanvas *canvas);
	//virtual int rtti() const { return Rtti_NoCollision; }
	void setAlwaysShow(bool yes) { alwaysShow = yes; updateVisible(); }
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual int rtti() const { return Rtti_WallPoint; }
	virtual bool deleteable() { return false; }
	virtual void collision(Ball *ball, long int id);
	virtual Config *config(QWidget *parent) { return wall->config(parent); }
	void dontMove() { dontmove = true; };
	void updateVisible();
	
private:
	bool alwaysShow;
	Wall *wall;
	bool editing;
	bool start;
	bool dontmove;
	bool visible;
	int lastId;
};
class WallObj : public Object
{
public:
	WallObj() { m_name = i18n("Wall"); m__name = "wall"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Wall(canvas); }
};

class Putter : public QCanvasLine, public CanvasItem
{
public:
	Putter(QCanvas *canvas);
	void go(Direction, bool more = false);
	void setOrigin(int x, int y);
	int curLen() { return len; }
	int curDeg() { return deg; }
	virtual void showInfo();
	virtual void hideInfo();
	void setDeg(int news) { deg = news; finishMe(); }
	int curMaxDeg() { return maxDeg; }
	virtual int rtti() const { return Rtti_Putter; }
	virtual void setVisible(bool yes);
	void saveDegrees(Ball *ball) { degMap[ball] = deg; }
	void setDegrees(Ball *ball);
	void resetDegrees() { degMap.clear(); setZ(999999); }
	virtual bool canBeMovedByOthers() { return true; }
	virtual void moveBy(double dx, double dy);

private:
	QPoint midPoint;
	int maxDeg;
	int deg;
	int len;
	void finishMe();
	int putterWidth;
	QCanvasLine *guideLine;
	QMap<Ball *, int> degMap;
};

class Bridge;
class BridgeConfig : public Config
{
	Q_OBJECT

public:
	BridgeConfig(Bridge *bridge, QWidget *);

private slots:
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
class Bridge : public QCanvasRectangle, public CanvasItem, public RectItem
{
public:
	Bridge(QRect rect, QCanvas *canvas);
	virtual void collision(Ball *ball, long int id);
	virtual void aboutToDie();
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(double vx, double vy);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual bool vStrut() { return true; }
	void doLoad(KSimpleConfig *cfg, int hole);
	void doSave(KSimpleConfig *cfg, int hole);
	virtual void newSize(int width, int height);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new BridgeConfig(this, parent); }
	void setSize(int width, int height);
	virtual QPointArray areaPoints() const;
	virtual QPtrList<QCanvasItem> moveableItems();
	void setWallColor(QColor color);
	QPen wallPen() { return topWall->pen(); }
	double wallZ() { return topWall->z(); }
	void setWallZ(double);
	void setTopWallVisible(bool yes) { topWall->setVisible(yes); }
	void setBotWallVisible(bool yes) { botWall->setVisible(yes); }
	void setLeftWallVisible(bool yes) { leftWall->setVisible(yes); }
	void setRightWallVisible(bool yes) { rightWall->setVisible(yes); }
	bool topWallVisible() { return topWall->isVisible(); }
	bool botWallVisible() { return botWall->isVisible(); }
	bool leftWallVisible() { return leftWall->isVisible(); }
	bool rightWallVisible() { return rightWall->isVisible(); }
	
protected:
	Wall *topWall;
	Wall *botWall;
	Wall *leftWall;
	Wall *rightWall;
	RectPoint *point;
};
class BridgeObj : public Object
{
public:
	BridgeObj() { m_name = i18n("Bridge"); m__name = "bridge"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Bridge(QRect(0, 0, 80, 40), canvas); }
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
	Sign(QCanvas *canvas);
	void setText(const QString &text) { m_text = text; update(); }
	QString text() { return m_text; }
	virtual int rtti() const { return Rtti_NoCollision; }
	virtual void draw(QPainter &painter);
	virtual bool vStrut() { return false; }
	virtual Config *config(QWidget *parent) { return new SignConfig(this, parent); }
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual void load(KSimpleConfig *cfg, int hole);

protected:
	QString m_text;
};
class SignObj : public Object
{
public:
	SignObj() { m_name = i18n("Sign"); m__name = "sign"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Sign(canvas); }
	virtual int rtti() const { return Rtti_NoCollision; }
};

class Windmill;
class WindmillGuard : public Wall
{
public:
	WindmillGuard(QCanvas *canvas) : Wall(canvas) {};
	void setBetween(int newmin, int newmax) { max = newmax; min = newmin; }
	virtual void advance(int phase);

private:
	int max;
	int min;
};
class WindmillConfig : public BridgeConfig
{
	Q_OBJECT

public:
	WindmillConfig(Windmill *windmill, QWidget *parent);

private slots:
	void speedChanged(int news);

private:
	Windmill *windmill;
};
class Windmill : public Bridge
{
public:
	Windmill(QRect rect, QCanvas *canvas);
	virtual void aboutToDie();
	virtual void newSize(int width, int height);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new WindmillConfig(this, parent); }
	void setSize(int width, int height);
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(double vx, double vy);
	void setSpeed(int news);
	int curSpeed() { return speed; }

private:
	WindmillGuard *guard;
	Wall *left;
	Wall *right;
	int speedfactor;
	int speed;
};
class WindmillObj : public Object
{
public:
	WindmillObj() { m_name = i18n("Windmill"); m__name = "windmill"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Windmill(QRect(0, 0, 80, 40), canvas); }
};

class Floater;
class FloaterConfig : public BridgeConfig
{
	Q_OBJECT

public:
	FloaterConfig(Floater *floater, QWidget *parent);

private slots:
	void speedChanged(int news);

private:
	Floater *floater;
};
class FloaterGuide;
class Floater : public Bridge
{
public:
	Floater(QRect rect, QCanvas *canvas);
	virtual void save(KSimpleConfig *cfg, int hole);
	virtual void load(KSimpleConfig *cfg, int hole);
	virtual bool loadLast() { return true; }
	virtual void firstMove(int x, int y);
	virtual void aboutToSave();
	virtual void aboutToDie();
	virtual void savingDone();
	virtual void setGame(KolfGame *game);
	virtual void editModeChanged(bool changed);
	virtual bool moveable() { return false; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent) { return new FloaterConfig(this, parent); }
	virtual QPtrList<QCanvasItem> moveableItems();
	virtual void advance(int phase);
	void setSpeed(int news);
	int curSpeed() { return speed; }

private:
	int speedfactor;
	int speed;
	FloaterGuide *wall;
	QPoint lastStart;
	QPoint lastEnd;
	QPoint lastWall;
	bool noUpdateZ;
	bool haventMoved;
	QPoint firstPoint;
};
class FloaterGuide : public Wall
{
public:
	FloaterGuide(Floater *floater, QCanvas *canvas) : Wall(canvas) { this->floater = floater; almostDead = false; }
	virtual Config *config(QWidget *parent) { return floater->config(parent); }
	virtual void aboutToDelete();
	virtual void aboutToDie();

private:
	Floater *floater;
	bool almostDead;
};
class FloaterObj : public Object
{
public:
	FloaterObj() { m_name = i18n("Floater"); m__name = "floater"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Floater(QRect(0, 0, 80, 40), canvas); }
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
	virtual ~HoleInfo() {}
	void setPar(int newpar) { m_par = newpar; }
	int par() { return m_par; }
	void setMaxStrokes(int newMaxStrokes) { m_maxStrokes = newMaxStrokes; }
	int maxStrokes() { return m_maxStrokes; }
	void setAuthor(QString newauthor) { m_author = newauthor; }
	QString author() { return m_author; }
	void setName(QString newname) { m_name = newname; }
	QString name() { return m_name; }
	virtual Config *config(QWidget *parent) { return new HoleConfig(this, parent); }
	void borderWallsChanged(bool yes);
	bool borderWalls() { return m_borderWalls; }

private:
	QString m_author;
	QString m_name;
	bool m_borderWalls;
	int m_par;
	int m_maxStrokes;
};

class KolfGame : public QCanvasView
{
	Q_OBJECT

public:
	KolfGame(PlayerList *players, QString filename, QWidget *parent=0, const char *name=0 );
	~KolfGame();
	ObjectList *objectList() { return &obj; }
	void setFilename(const QString &filename);
	QString curFilename() const { return filename; }
	void emitLargestHole() { emit largestHole(highestHole); }
	QCanvas *canvas() const { return course; }
	void removeItem(QCanvasItem *item) { items.setAutoDelete(false); items.removeRef(item); }
	// returns whether it was a cancel
	bool askSave(bool);
	bool isEditing() const { return editing; }
	Ball *curBall() { return (*curPlayer).ball(); }
	void updateMouse();
	void setBorderWalls(bool);

public slots:
	void pause();
	void unPause();
	void save();
	void addFirstHole() { emit newHole(curPar); }
	void toggleEditMode();
	void setModified() { modified = true; }
	void addNewObject(Object *newObj);
	void addNewHole();
	void switchHole(int);
	void switchHole(const QString &);
	void nextHole();
	void prevHole();
	void firstHole();
	void lastHole();
	void randHole();
	void playSound(QString file);
	void showInfo();
	void showInfoDlg(bool = false);
	void resetHole();
	void clearHole();
	void print(QPainter &);
	void setUseMouse(bool yes) { m_useMouse = yes; }

signals:
	void holesDone();
	void newHole(int);
	void largestHole(int);
	void scoreChanged(int, int, int);
	void newPlayersTurn(Player *);
	void playerHoled(Player *);
	void newSelectedItem(CanvasItem *);
	void checkEditing();
	void editingStarted();
	void editingEnded();
	void inPlayStart();
	void inPlayEnd();
	void maxStrokesReached();

private slots:
	void shotDone();
	void holeDone();
	void timeout();
	void fastTimeout();
	void frictionTimeout();
	void putterTimeout();
	void autoSaveTimeout();
	void addItemsToMoveableList(QPtrList<QCanvasItem>);
	void hideInfoText();

protected:
	void contentsMousePressEvent(QMouseEvent *e);
	void contentsMouseMoveEvent(QMouseEvent *e);
	void contentsMouseReleaseEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);

private:
	QCanvas *course;
	Putter *putter;
	PlayerList *players;
	PlayerList::Iterator curPlayer;
	Ball *whiteBall;

	QTimer *timer;
	QTimer *autoSaveTimer;
	QTimer *fastTimer;
	QTimer *frictionTimer;
	QTimer *putterTimer;

	ObjectList obj;
	QPtrList<QCanvasItem> items;
	QPtrList<QCanvasItem> extraMoveable;
	QPtrList<Wall> borderWalls;

	int timerMsec;
	int autoSaveMsec;
	int fastTimerMsec;
	int frictionTimerMsec;
	int putterTimerMsec;

	void puttPress();
	void puttRelease();
	bool inPlay;
	bool putting;
	bool stroking;
	double strength;
	double maxStrength;
	int puttCount;

	int curHole;
	int highestHole;
	int curPar;

	int wallWidth;
	int height;
	int width;
	QColor grass;

	int advancePeriod;

	bool paused;

	QString filename;
	bool recalcHighestHole;
	void openFile();

	bool editing;
	QPoint storedMousePos;
	bool moving;
	QCanvasItem *movingItem;
	QCanvasItem *selectedItem;
	QCanvasRectangle *highlighter;

	// sound
	Arts::SimpleSoundServer soundserver;
	Arts::PlayObjectFactory playObjectFactory;
	Arts::PlayObject playObject;
	void initSoundServer();
	bool m_serverRunning;
	bool m_soundError;
	QString soundDir;

	HoleInfo holeInfo;
	QCanvasText *infoText;

	bool addingNewHole;
	int scoreboardHoles;
	inline void resetHoleScores();

	bool infoShown;

	KSimpleConfig *cfg;

	inline void addBorderWall(QPoint start, QPoint end);
	inline void shotStart();

	void showInfoPress();
	void showInfoRelease();

	bool modified;

	inline bool allPlayersDone();

	bool m_useMouse;
};

#endif
