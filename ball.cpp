#include <qcanvas.h>
#include <qcolor.h>
#include <qpen.h>

#include <kapplication.h>
#include <kdebug.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtti.h"
#include "vector.h"
#include "canvasitem.h"
#include "game.h"
#include "ball.h"

Ball::Ball(QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	m_doDetect = true;
	m_collisionLock = false;
	setBeginningOfHole(false);
	setBlowUp(false);
	setPen(black);
	resetSize();
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
	m_forceStillGoing = false;
	frictionMultiplier = 1.0;

	// this sets z
	setState(Stopped);
}

void Ball::setState(BallState newState)
{
	state = newState;
	if (state == Stopped)
		setZ(1000);
	else
		setBeginningOfHole(false);
}

void Ball::advance(int phase)
{
	if (phase == 1 && m_blowUp)
	{
		if (blowUpCount >= 50)
		{
			//TODO make this a config option
			//setAddStroke(addStroke() + 1);
			setBlowUp(false);
			resetSize();
			return;
		}

		const double diff = 8;
		double randnum = kapp->random();
		const double width = 6 + randnum * (diff / RAND_MAX);
		randnum = kapp->random();
		const double height = 6 + randnum * (diff / RAND_MAX);
		setSize(width, height);
		blowUpCount++;
	}
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible()) { setVelocity(0, 0); return; }
	const double subtractAmount = .027 * frictionMultiplier;
	if (m_vector.magnitude() <= subtractAmount)
	{
		state = Stopped;
		setVelocity(0, 0);
		game->timeout();
		return;
	}
	m_vector.setMagnitude(m_vector.magnitude() - subtractAmount);
	setVector(m_vector);

	frictionMultiplier = 1.0;
}

void Ball::setVelocity(double vx, double vy)
{
	QCanvasEllipse::setVelocity(vx, vy);

	if (vx == 0 && vy == 0)
	{
		m_vector.setDirection(0);
		m_vector.setMagnitude(0);
		return;
	}

	double ballAngle = atan2(-vy, vx);

	m_vector.setDirection(ballAngle);
	m_vector.setMagnitude(sqrt(pow(vx, 2) + pow(vy, 2)));
}

void Ball::setVector(const Vector &newVector)
{
	m_vector = newVector;

	if (newVector.magnitude() == 0)
	{
		setVelocity(0, 0);
		return;
	}

	QCanvasEllipse::setVelocity(cos(newVector.direction()) * newVector.magnitude(), -sin(newVector.direction()) * newVector.magnitude());
}

void Ball::moveBy(double dx, double dy)
{
	double oldx;
	double oldy;
	oldx = x();
	oldy = y();
	QCanvasEllipse::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect(oldx, oldy);
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::doAdvance()
{
	QCanvasEllipse::advance(1);
}

namespace Lines
{

	struct Point
	{
		double x;
		double y;
	};

	struct Line
	{
		Point p1, p2;
	};

	int ccw(const Point &p0, const Point &p1, const Point &p2)
	{
		double dx1, dx2, dy1, dy2;
		dx1 = p1.x - p0.x; dy1 = p1.y - p0.y;
		dx2 = p2.x - p0.x; dy2 = p2.y - p0.y;
		if (dx1*dy2 > dy1*dx2) return +1;
		if (dx1*dy2 < dy1*dx2) return -1;
		if ((dx1*dx2 < 0) || (dy1*dy2 < 0)) return -1;
		if ((dx1*dx1+dy1*dy1) < (dx2*dx2+dy2*dy2))
			return +1;
		return 0;
	}

	int intersects(const Line &l1, const Line &l2)
	{
		// TODO: Account for vertical lines
		return ((ccw(l1.p1, l1.p2, l2.p1)
				*ccw(l1.p1, l1.p2, l2.p2)) <= 0)
				&& ((ccw(l2.p1, l2.p2, l1.p1)
				*ccw(l2.p1, l2.p2, l1.p2)) <= 0);
	}

	
	bool intersects(
			double xa1, double ya1, double xb1, double yb1,
			double xa2, double ya2, double xb2, double yb2
		)
	{
		Line l1, l2;
		l1.p1.x = xa1;
		l1.p1.y = ya1;
		l1.p2.x = xb1;
		l1.p2.y = yb1;
		
		l2.p1.x = xa2;
		l2.p1.y = ya2;
		l2.p2.x = xb2;
		l2.p2.y = yb2;

		return intersects(l1, l2);
	}
}

#include <iostream.h>

void Ball::collisionDetect(double oldx, double oldy)
{
	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	// every other time...
	// do friction
	if (collisionId % 2 && !(xVelocity() == 0 && yVelocity() == 0))
		friction();

	const double minSpeed = .06;

	QCanvasItemList m_list = collisions(true);

	// please don't ask why QCanvas doesn't actually sort its list;
	// it just doesn't.
	m_list.sort();

	this->m_list = m_list;

	for (QCanvasItemList::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		QCanvasItem *item = *it;

		if (item->rtti() == Rtti_NoCollision || item->rtti() == Rtti_Putter)
			continue;

		if (item->rtti() == rtti() && !m_collisionLock)
		{
			// it's one of our own kind, a ball
			Ball *oball = dynamic_cast<Ball *>(item);
			if (oball->collisionLock())
				continue;
			oball->setCollisionLock(true);

			if ((oball->x() - x() != 0 && oball->y() - y() != 0) && state == Rolling && oball->curState() != Holed)
			{
				m_collisionLock = true;
				// move this ball to where it was barely touching
				double ballAngle = m_vector.direction();
				while (collisions(true).contains(item) > 0)
					move(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);
				// make a 1 pixel separation
				move(x() - cos(ballAngle), y() + sin(ballAngle));

				Vector bvector = oball->curVector();
				m_vector -= bvector;

				Vector unit1 = Vector(QPoint(x(), y()), QPoint(oball->x(), oball->y()));
				unit1 = unit1.unit();

				Vector unit2 = m_vector.unit();

				double cos = unit1 * unit2;

				unit1 *= m_vector.magnitude() * cos;
				m_vector -= unit1;
				m_vector += bvector;

				bvector += unit1;

				oball->setVector(bvector);
				setVector(m_vector);

				oball->setState(Rolling);
				setState(Rolling);

				oball->doAdvance();
			}

			continue;
		}

		if (!isVisible() || state == Holed)
			return;

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			if (!citem->terrainCollisions())
				if (!citem->collision(this, collisionId))
					goto end;
			break;
		}
	}


	for (QCanvasItemList::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem)
			if (citem->terrainCollisions())
				citem->collision(this, collisionId);
	}

	{ // check if I went through a wall
		QCanvasItemList items;
		if (game)
			items = game->canvas()->allItems();
		for (QCanvasItemList::Iterator i = items.begin(); i != items.end(); ++i)
		{
			QCanvasItem *item = *i;
			Wall *wall = dynamic_cast<Wall*>(item);
			if (!wall) continue;

			if (Lines::intersects(
					wall->startPoint().x(), wall->startPoint().y(),
					wall->endPoint().x(),   wall->endPoint().y(),
				
					oldx, oldy, x(), y()
				))
			{
				wall->collision(this, collisionId);
				break;
			}

		
		}
	}
	


	end:

	if (m_vector.magnitude() < minSpeed && m_vector.magnitude())
	{
		setVelocity(0, 0);
		setState(Stopped);
	}
}

BallState Ball::currentState()
{
	return state;
}

