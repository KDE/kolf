#ifndef KOLF_BALL_H
#define KOLF_BALL_H

#include <QColor>

#include <math.h>

#include "vector.h"
#include "rtti.h"
#include "game.h"


enum BallState { Rolling = 0, Stopped, Holed };

class Ball : public QGraphicsEllipseItem, public CanvasItem
{
public:
	Ball(QGraphicsScene *scene); 
	virtual void aboutToDie();

	BallState currentState();

	virtual void resetSize(); 
	void resize(double resizeFactor);
	/*
	 * set the position of the ball automatically modifying x and y to take into account resizing
	 */
	void setPos(qreal x, qreal y);
	/*
	 * set the position of the ball automatically modifying pos to take into account resizing
	 */
	void setPos(QPointF pos);
	/*
	 * set the position of the ball to exactly x and y, without taking into account resizing
	 */
	void setResizedPos(qreal x, qreal y);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *); 
	virtual void advance(int phase);
	virtual void doAdvance();
	/*
	 * moves the ball, automatically modifying dx and dy to take into account resizing (so the input dx and dy can use the game's base 400x400 co-ordinated system, and do not need to be modified when the game is resized)
	 */
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(double vx, double vy);

	virtual bool deleteable() const { return false; }

	virtual bool canBeMovedByOthers() const { return true; }

	BallState curState() const { return state; }
	void setState(BallState newState);

	QColor color() const { return m_color; }
	void setColor(QColor color) { m_color = color; setBrush(color); setPen(color); }

	void setMoved(bool yes) { m_moved = yes; }
	bool moved() const { return m_moved; }
	void setBlowUp(bool yes) { m_blowUp = yes; blowUpCount = 0; }
	bool blowUp() const { return m_blowUp; }

	void setFrictionMultiplier(double news) { frictionMultiplier = news; }
	void friction();
	void collisionDetect(double oldx, double oldy);

	int addStroke() const { return m_addStroke; }
	bool placeOnGround(Vector &v) { v = oldVector; return m_placeOnGround; }
	void setAddStroke(int newStrokes) { m_addStroke = newStrokes; }
	void setPlaceOnGround(bool placeOnGround) { m_placeOnGround = placeOnGround; oldVector = m_vector; }

	bool beginningOfHole() const { return m_beginningOfHole; }
	void setBeginningOfHole(bool yes) { m_beginningOfHole = yes; }

	bool forceStillGoing() const { return m_forceStillGoing; }
	void setForceStillGoing(bool yes) { m_forceStillGoing = yes; }

	Vector curVector() const { return m_vector; }
	void setVector(const Vector &newVector);

	bool collisionLock() const { return m_collisionLock; }
	void setCollisionLock(bool yes) { m_collisionLock = yes; }
	virtual void fastAdvanceDone() { setCollisionLock(false); }

	void setDoDetect(bool yes) { m_doDetect = yes; }
	bool doDetect() const { return m_doDetect; }

	virtual void showInfo();
	virtual void hideInfo();
	virtual void setName(const QString &);
	virtual void setVisible(bool yes);

	double width() { return rect().width(); }
	double height() { return rect().height(); }
	double getBaseX() { return baseX; }
	double getBaseY() { return baseY; }

public slots:
	void update() { doAdvance(); }

private:
	BallState state;
	QColor m_color;
	QPixmap pixmap;
	bool pixmapInitialised;
	long int collisionId;
	double frictionMultiplier;
	/*
	 * base numbers are the size or position when no resizing has taken place (i.e. the defaults)
	 */
	double baseDiameter;
	double baseX, baseY; //position of the ball in origional 400x400 co-ordinate system
	double baseFontPixelSize;
	/*
	 * resizeFactor is the number to multiply base numbers by to get their resized value (i.e. if it is 1 then use default size, if it is >1 then everything needs to be bigger, and if it is <1 then everything needs to be smaller)
	 */
	double resizeFactor;

	bool m_blowUp;
	int blowUpCount;
	int m_addStroke;
	bool m_placeOnGround;
	double m_oldvx;
	double m_oldvy;

	bool m_moved;
	bool m_beginningOfHole;
	bool m_forceStillGoing;

	bool ignoreBallCollisions;

	Vector m_vector;
	Vector oldVector;
	bool m_collisionLock;

	bool m_doDetect;
	QList<QGraphicsItem *> m_list;

	QGraphicsSimpleTextItem *label;
};


inline int rad2deg(double theDouble)
{
    return (int)((360L / (2L * M_PI)) * theDouble);
}

inline double deg2rad(double theDouble)
{
	return (((2L * M_PI) / 360L) * theDouble);
}

#endif
