#ifndef BALL_H
#define BALL_H

#include <qcanvas.h>
#include <qcolor.h>

#include <math.h>

#include "rtti.h"

enum BallState {Rolling, Stopped, Holed};

class Ball : public QCanvasEllipse, public CanvasItem
{
public:
	Ball(QCanvas *canvas);
	BallState currentState();

	void resetSize() { setSize(7, 7); }
	virtual void advance(int phase);
	virtual void doAdvance();
	virtual void moveBy(double dx, double dy);

	double curSpeed() { return sqrt(xVelocity() * xVelocity() + yVelocity() * yVelocity()); }
	virtual bool canBeMovedByOthers() const { return true; }

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

#endif
