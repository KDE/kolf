#ifndef GAME_H
#define GAME_H

#include <kdebug.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kimageeffect.h>
#include <arts/kplayobject.h>
#include <arts/kartsserver.h>
#include <arts/kartsdispatcher.h>

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

#include <kolf/object.h>
#include <kolf/config.h>
#include <kolf/canvasitem.h>
#include <kolf/ball.h>
#include <kolf/statedb.h>
#include <kolf/rtti.h>

class QLabel;
class QSlider;
class QCheckBox;
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class KConfig;
class KPrinter;
class KolfGame;

enum Direction { D_Left, D_Right, Forwards, Backwards };
enum Amount { Amount_Less, Amount_Normal, Amount_More };
enum HoleResult { Result_Holed, Result_Miss, Result_LipOut };

class Player;

class BallStateInfo
{
public:
	void saveState(KConfig *cfg);
	void loadState(KConfig *cfg);

	int id;
	QPoint spot;
	BallState state;
	bool beginningOfHole;
	int score;
};
class BallStateList : public QValueList<BallStateInfo>
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
	Player() : m_ball(new Ball(0)) {};
	Ball *ball() const { return m_ball; }
	void setBall(Ball *ball) { m_ball = ball; }
	BallStateInfo stateInfo(int hole) const { BallStateInfo ret; ret.spot = QPoint(m_ball->x(), m_ball->y()); ret.state = m_ball->curState(); ret.score = score(hole); ret.beginningOfHole = m_ball->beginningOfHole(); ret.id = m_id; return ret; }

	QValueList<int> scores() const { return m_scores; }
	void setScores(const QValueList<int> &newScores) { m_scores = newScores; }
	int score(int hole) const { return (*m_scores.at(hole - 1)); }
	int lastScore() const { return m_scores.last(); }
	int firstScore() const { return m_scores.first(); }

	void addStrokeToHole(int hole) { (*m_scores.at(hole - 1))++; }
	void setScoreForHole(int score, int hole) { (*m_scores.at(hole - 1)) = score; }
	void subtractStrokeFromHole(int hole) { (*m_scores.at(hole - 1))--; }
	void resetScore(int hole) { (*m_scores.at(hole - 1)) = 0; }
	void addHole() { m_scores.append(0); }
	unsigned int numHoles() const { return m_scores.count(); }

	QString name() const { return m_name; }
	void setName(const QString &name) { m_name = name; }

	void setId(int id) { m_id = id; }
	int id() const { return m_id; }

private:
	Ball *m_ball;
	QValueList<int> m_scores;
	QString m_name;
	int m_id;
};
typedef QValueList<Player> PlayerList;

class Arrow : public QCanvasLine
{
public:
	Arrow(QCanvas *canvas);
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
	virtual void setZ(double newz);

private:
	double m_angle;
	double m_length;
	bool m_reversed;
	QCanvasLine *line1;
	QCanvasLine *line2;
};

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
	void gradeChanged(double);

private:
	Slope *slope;
};
class Slope : public QCanvasRectangle, public CanvasItem, public RectItem
{
public:
	Slope(QRect rect, QCanvas *canvas);
	virtual void aboutToDie();
	virtual int rtti() const { return 1031; }

	virtual void showInfo();
	virtual void hideInfo();
	virtual void editModeChanged(bool changed);
	virtual bool canBeMovedByOthers() const { return !stuckOnGround; }
	virtual QPtrList<QCanvasItem> moveableItems() const;
	virtual Config *config(QWidget *parent) { return new SlopeConfig(this, parent); }
	void setSize(int, int);
	virtual void newSize(int width, int height);

	virtual void moveBy(double dx, double dy);

	virtual void draw(QPainter &painter);
	virtual QPointArray areaPoints() const;

	void setGradient(QString text);
	KImageEffect::GradientType curType() const { return type; }
	void setGrade(double grade);

	double curGrade() const { return grade; }
	void setColor(QColor color) { this->color = color; updatePixmap(); }
	void setReversed(bool reversed) { this->reversed = reversed; updatePixmap(); }
	bool isReversed() const { return reversed; }

	bool isStuckOnGround() const { return stuckOnGround; }
	void setStuckOnGround(bool yes) { stuckOnGround = yes; updateZ(); }

	virtual void load(KConfig *cfg);
	virtual void save(KConfig *cfg);

	virtual bool collision(Ball *ball, long int id);
	virtual bool terrainCollisions() const;

	QMap<KImageEffect::GradientType, QString> gradientI18nKeys;
	QMap<KImageEffect::GradientType, QString> gradientKeys;

	virtual void updateZ(QCanvasRectangle *vStrut = 0);

	void moveArrow();

private:
	KImageEffect::GradientType type;
	inline void setType(KImageEffect::GradientType type);
	bool showingInfo;
	double grade;
	bool reversed;
	QColor color;
	QPixmap pixmap;
	void updatePixmap();
	bool stuckOnGround;
	QPixmap grass;

	void clearArrows();

	QPtrList<Arrow> arrows;
	QCanvasText *text;
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
	RectPoint(QColor color, RectItem *, QCanvas *canvas);
	void dontMove() { dontmove = true; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent);
	virtual bool deleteable() const { return false; }
	virtual bool cornerResize() const { return true; }
	virtual CanvasItem *itemToDelete() { return dynamic_cast<CanvasItem *>(rect); }
	void setSizeFactor(double newFactor) { m_sizeFactor = newFactor; }

private:
	bool dontmove;
	RectItem *rect;
	double m_sizeFactor;
};

class Ellipse : public QCanvasEllipse, public CanvasItem, public RectItem
{
public:
	Ellipse(QCanvas *canvas);
	virtual void advance(int phase);

	int changeEvery() const { return m_changeEvery; }
	void setChangeEvery(int news) { m_changeEvery = news; }
	bool changeEnabled() const { return m_changeEnabled; }
	void setChangeEnabled(bool news) { setAnimated(news); m_changeEnabled = news; }

	virtual void aboutToDie();
	virtual void aboutToSave();
	virtual void savingDone();

	virtual QPtrList<QCanvasItem> moveableItems() const;

	virtual void newSize(int width, int height);
	virtual void moveBy(double dx, double dy);

	virtual void editModeChanged(bool changed);

	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);

	virtual Config *config(QWidget *parent);

protected:
	RectPoint *point;

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
};

class Puddle : public Ellipse
{
public:
	Puddle(QCanvas *canvas);
	virtual bool collision(Ball *ball, long int id);
	virtual int rtti() const { return Rtti_DontPlaceOn; }
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
	virtual bool collision(Ball *ball, long int id);
};
class SandObj : public Object
{
public:
	SandObj() { m_name = i18n("Sand"); m__name = "sand"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Sand(canvas); }
};

class Inside : public QCanvasEllipse, public CanvasItem
{
public:
	Inside(CanvasItem *item, QCanvas *canvas) : QCanvasEllipse(canvas) { this->item = item; }
	virtual bool collision(Ball *ball, long int id) { return item->collision(ball, id); }

private:
	CanvasItem *item;
};

class Bumper : public QCanvasEllipse, public CanvasItem
{
public:
	Bumper(QCanvas *canvas);

	virtual void advance(int phase);
	virtual void aboutToDie();
	virtual void moveBy(double dx, double dy);
	virtual void editModeChanged(bool changed);

	virtual bool collision(Ball *ball, long int id);

private:
	QColor firstColor;
	QColor secondColor;
	int count;
	Inside *inside;
};
class BumperObj : public Object
{
public:
	BumperObj() { m_name = i18n("Bumper"); m__name = "bumper"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Bumper(canvas); }
};

class Hole : public QCanvasEllipse, public CanvasItem
{
public:
	Hole(QColor color, QCanvas *canvas);
	virtual bool place(Ball * /*ball*/, bool /*wasCenter*/) { return true; };

	virtual bool collision(Ball *ball, long int id);

protected:
	virtual HoleResult result(const QPoint, double, bool *wasCenter);
};

class Cup : public Hole
{
public:
	Cup(QCanvas *canvas);
	virtual bool place(Ball *ball, bool wasCenter);
	virtual void save(KConfig *cfg);
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void draw(QPainter &painter);

private:
	QPixmap pixmap;
};
class CupObj : public Object
{
public:
	CupObj() { m_name = i18n("Cup"); m__name = "cup"; m_addOnNewHole = true; }
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
	virtual void aboutToDie();
	virtual void moveBy(double dx, double dy);
	virtual bool deleteable() const { return false; }
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void editModeChanged(bool editing);
	virtual void setPen(QPen p);
	virtual void showInfo();
	virtual void hideInfo();
	void updateArrowAngle();
	void updateArrowLength();
	virtual Config *config(QWidget *parent);
	BlackHole *blackHole;

private:
	Arrow *arrow;
};
class BlackHole : public Hole
{
public:
	BlackHole(QCanvas *canvas);
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void aboutToDie();
	virtual void showInfo();
	virtual void hideInfo();
	virtual bool place(Ball *ball, bool wasCenter);
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);
	virtual Config *config(QWidget *parent) { return new BlackHoleConfig(this, parent); }
	virtual QPtrList<QCanvasItem> moveableItems() const;
	int minSpeed() const { return m_minSpeed; }
	int maxSpeed() const { return m_maxSpeed; }
	void setMinSpeed(int news) { m_minSpeed = news; exitItem->updateArrowLength(); }
	void setMaxSpeed(int news) { m_maxSpeed = news; exitItem->updateArrowLength(); }

	int curExitDeg() const { return exitDeg; }
	void setExitDeg(int newdeg);

	virtual void editModeChanged(bool editing) { exitItem->editModeChanged(editing); }
	void updateInfo();

	virtual void shotStarted() { runs = 0; };

	virtual void moveBy(double dx, double dy);

private:
	int exitDeg;
	BlackHoleExit *exitItem;
	QCanvasEllipse *outside;
	QCanvasLine *infoLine;

	void finishMe();
	int m_minSpeed;
	int m_maxSpeed;

	int runs;
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
	double dampening;

	void setAlwaysShow(bool yes);
	virtual void setZ(double newz);
	virtual void setPen(QPen p);
	virtual bool collision(Ball *ball, long int id);
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);
	virtual void selectedItem(QCanvasItem *item);
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(double vx, double vy);
	virtual void clean();

	// must reimp because we gotta move the end items,
	// and we do that in :moveBy()
	virtual void setPoints(int xa, int ya, int xb, int yb) { QCanvasLine::setPoints(xa, ya, xb, yb); moveBy(0, 0); }

	virtual int rtti() const { return Rtti_DontPlaceOn; }
	virtual QPtrList<QCanvasItem> moveableItems() const;
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
	virtual bool deleteable() const { return false; }
	virtual bool collision(Ball *ball, long int id);
	virtual CanvasItem *itemToDelete() { return wall; }
	virtual void clean();
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
	void go(Direction, Amount amount = Amount_Normal);
	void setOrigin(int x, int y);
	int curLen() const { return len; }
	double curAngle() const { return angle; }
	int curDeg() const { return rad2deg(angle); }
	virtual void showInfo();
	virtual void hideInfo();
	void setAngle(double news) { angle = news; finishMe(); }
	void setDeg(int news) { angle = deg2rad(news); finishMe(); }
	double curMaxAngle() const { return maxAngle; }
	virtual int rtti() const { return Rtti_Putter; }
	virtual void setVisible(bool yes);
	void saveAngle(Ball *ball) { angleMap[ball] = angle; }
	void setAngle(Ball *ball);
	void resetAngles() { angleMap.clear(); setZ(999999); }
	virtual bool canBeMovedByOthers() const { return true; }
	virtual void moveBy(double dx, double dy);
	void setShowGuideLine(bool yes);

private:
	QPoint midPoint;
	double maxAngle;
	double angle;
	double oneDegree;
	QMap<Ball *, double> angleMap;
	int len;
	void finishMe();
	int putterWidth;
	QCanvasLine *guideLine;
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
class Bridge : public QCanvasRectangle, public CanvasItem, public RectItem
{
public:
	Bridge(QRect rect, QCanvas *canvas);
	virtual bool collision(Ball *ball, long int id);
	virtual void aboutToDie();
	virtual void editModeChanged(bool changed);
	virtual void moveBy(double dx, double dy);
	virtual void load(KConfig *cfg);
	virtual void save(KConfig *cfg);
	virtual bool vStrut() const { return true; }
	void doLoad(KConfig *cfg);
	void doSave(KConfig *cfg);
	virtual void newSize(int width, int height);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new BridgeConfig(this, parent); }
	void setSize(int width, int height);
	virtual QPtrList<QCanvasItem> moveableItems() const;

	void setWallColor(QColor color);
	QPen wallPen() const { return topWall->pen(); }

	double wallZ() const { return topWall->z(); }
	void setWallZ(double);

	void setTopWallVisible(bool yes) { topWall->setVisible(yes); }
	void setBotWallVisible(bool yes) { botWall->setVisible(yes); }
	void setLeftWallVisible(bool yes) { leftWall->setVisible(yes); }
	void setRightWallVisible(bool yes) { rightWall->setVisible(yes); }
	bool topWallVisible() const { return topWall->isVisible(); }
	bool botWallVisible() const { return botWall->isVisible(); }
	bool leftWallVisible() const { return leftWall->isVisible(); }
	bool rightWallVisible() const { return rightWall->isVisible(); }
	
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
	QString text() const { return m_text; }
	virtual void draw(QPainter &painter);
	virtual bool vStrut() const { return false; }
	virtual Config *config(QWidget *parent) { return new SignConfig(this, parent); }
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);

protected:
	QString m_text;
};
class SignObj : public Object
{
public:
	SignObj() { m_name = i18n("Sign"); m__name = "sign"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Sign(canvas); }
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
	void endChanged(bool yes);

private:
	Windmill *windmill;
};
class Windmill : public Bridge
{
public:
	Windmill(QRect rect, QCanvas *canvas);
	virtual void aboutToDie();
	virtual void newSize(int width, int height);
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);
	virtual void setGame(KolfGame *game);
	virtual Config *config(QWidget *parent) { return new WindmillConfig(this, parent); }
	void setSize(int width, int height);
	virtual void moveBy(double dx, double dy);
	void setSpeed(int news);
	int curSpeed() const { return speed; }
	void setBottom(bool yes);
	bool bottom() const { return m_bottom; }

private:
	WindmillGuard *guard;
	Wall *left;
	Wall *right;
	int speedfactor;
	int speed;
	bool m_bottom;
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
class FloaterGuide : public Wall
{
public:
	FloaterGuide(Floater *floater, QCanvas *canvas) : Wall(canvas) { this->floater = floater; almostDead = false; }
	virtual void setPoints(int xa, int ya, int xb, int yb);
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent);
	virtual void aboutToDelete();
	virtual void aboutToDie();

private:
	Floater *floater;
	bool almostDead;
};
class Floater : public Bridge
{
public:
	Floater(QRect rect, QCanvas *canvas);
	virtual bool collision(Ball *ball, long int id) { Bridge::collision(ball, id); return false; }
	virtual void saveState(StateDB *db);
	virtual void loadState(StateDB *db);
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);
	virtual bool loadLast() const { return true; }
	virtual void firstMove(int x, int y);
	virtual void aboutToSave();
	virtual void aboutToDie();
	virtual void savingDone();
	virtual void setGame(KolfGame *game);
	virtual void editModeChanged(bool changed);
	virtual bool moveable() const { return false; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent) { return new FloaterConfig(this, parent); }
	virtual QPtrList<QCanvasItem> moveableItems() const;
	virtual void advance(int phase);
	void setSpeed(int news);
	int curSpeed() const { return speed; }

	// called by floaterguide when changed;
	void reset();

private:
	int speedfactor;
	int speed;
	FloaterGuide *wall;
	QPoint start;
	QPoint end;
	bool noUpdateZ;
	bool haventMoved;
	QPoint firstPoint;
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
	QString name() const { return m_name; }
	virtual Config *config(QWidget *parent) { return new HoleConfig(this, parent); }
	void borderWallsChanged(bool yes);
	bool borderWalls() const { return m_borderWalls; }

private:
	QString m_author;
	QString m_name;
	bool m_borderWalls;
	int m_par;
	int m_maxStrokes;
	int m_lowestMaxStrokes;
};

class StrokeCircle : public QCanvasItem
{
public:
	StrokeCircle(QCanvas *canvas);

	void setValue(double v);
	double value();	
	void setMaxValue(double m);
	void setSize(int w, int h);
	void setThickness(int t);
	int thickness() const;
	int width() const;
	int height() const;
	virtual void draw(QPainter &p);
	virtual QRect boundingRect() const;
	virtual bool collidesWith(const QCanvasItem*) const;
	virtual bool collidesWith(const QCanvasSprite*, const QCanvasPolygonalItem*, const QCanvasRectangle*, const QCanvasEllipse*, const QCanvasText*) const;

private:
	double dvalue, dmax;
	int ithickness, iwidth, iheight;
};

struct CourseInfo
{
	CourseInfo(const QString &_name, const QString &_author, unsigned int _holes, unsigned int _par) { name = _name; author = _author; holes = _holes; par = _par; }
	CourseInfo() {}
	QString name;
	QString author;
	unsigned int holes;
	unsigned int par;
};

class KolfGame : public QCanvasView
{
	Q_OBJECT

public:
	KolfGame(ObjectList *obj, PlayerList *players, QString filename, QWidget *parent=0, const char *name=0 );
	~KolfGame();
	void setObjects(ObjectList *obj) { this->obj = obj; }
	void setFilename(const QString &filename);
	QString curFilename() const { return filename; }
	void emitLargestHole() { emit largestHole(highestHole); }
	QCanvas *canvas() const { return course; }
	void removeItem(QCanvasItem *item) { items.setAutoDelete(false); items.removeRef(item); }
	// returns whether it was a cancel
	bool askSave(bool);
	bool isEditing() const { return editing; }
	void setStrict(bool yes) { strict = yes; }
	// returns true when you shouldn't do anything
	bool isPaused() const { return paused; }
	Ball *curBall() const { return (*curPlayer).ball(); }
	void updateMouse();
	//void changeMouse();
	void ballMoved();
	void updateHighlighter();
	void updateCourse() { course->update(); }
	QCanvasItem *curSelectedItem() const { return selectedItem; }
	void setBorderWalls(bool);
	void setInPlay(bool yes) { inPlay = yes; }
	bool isInPlay() { return inPlay; }
	void stoppedBall();
	QString courseName() const { return holeInfo.name(); }
	void hidePutter() { putter->setVisible(false); }
	void ignoreEvents(bool ignore) { m_ignoreEvents = ignore; }

	static void scoresFromSaved(KConfig *, PlayerList &players);
	static void courseInfo(CourseInfo &info, const QString &filename);

public slots:
	void pause();
	void unPause();
	void save();
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
	void showInfoDlg(bool = false);
	void resetHole();
	void clearHole();
	void print(KPrinter &);
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
	void playerHoled(Player *);
	void newSelectedItem(CanvasItem *);
	void checkEditing();
	void editingStarted();
	void editingEnded();
	void inPlayStart();
	void inPlayEnd();
	void maxStrokesReached(const QString &);

private slots:
	void shotDone();
	void holeDone();
	void startNextHole();
	void fastTimeout();
	void putterTimeout();
	void autoSaveTimeout();
	void addItemsToMoveableList(QPtrList<QCanvasItem>);
	void addItemToFastAdvancersList(CanvasItem *);
	void hideInfoText();

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

	QPoint viewportToViewport(const QPoint &p);

private:
	QCanvas *course;
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

	ObjectList *obj;
	QPtrList<QCanvasItem> items;
	QPtrList<QCanvasItem> extraMoveable;
	QPtrList<Wall> borderWalls;

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
	QColor grass;

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
	QCanvasItem *movingItem;
	QCanvasItem *selectedItem;
	QCanvasRectangle *highlighter;

	// sound
	KArtsDispatcher artsDispatcher;
	KArtsServer artsServer;
	QPtrList<KPlayObject> oldPlayObjects;
	bool m_sound;
	bool soundedOnce;
	QString soundDir;

	bool m_ignoreEvents;

	HoleInfo holeInfo;
	QCanvasText *infoText;
	void showInfo();
	StateDB stateDB;

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

	inline void addBorderWall(QPoint start, QPoint end);
	void shotStart();
	void startBall(const Vector &vector);

	bool modified;

	inline bool allPlayersDone();

	bool m_useMouse;
	bool m_useAdvancedPutting;

	QPtrList<CanvasItem> fastAdvancers;
	bool fastAdvancedExist;

	bool fillPrint;
	bool isprinting;;
};

#endif
