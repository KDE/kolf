#include <q3canvas.h>
#include <qcolor.h>
#include <qpen.h>
//Added by qt3to4:
#include <Q3PtrList>

#include <kapplication.h>
#include <kdebug.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <krandom.h>

#include "rtti.h"
#include "vector.h"
#include "canvasitem.h"
#include "game.h"
#include "ball.h"

Ball::Ball(Q3Canvas *canvas)
	: Q3CanvasEllipse(canvas)
{
	m_doDetect = true;
	m_collisionLock = false;
	setBeginningOfHole(false);
	setBlowUp(false);
	setPen(QPen(Qt::black));
	resetSize();
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
	m_forceStillGoing = false;
	frictionMultiplier = 1.0;
	QFont font(kapp->font());
	//font.setPixelSize(10);
	label = new Q3CanvasText("", font, canvas);
	label->setColor(Qt::white);
	label->setVisible(false);

	// this sets z
	setState(Stopped);
	label->setZ(z() - .1);
}

void Ball::aboutToDie()
{
	delete label;
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
	// not used anymore
	// can be used to make ball wobble
	if (phase == 1 && m_blowUp)
	{
		if (blowUpCount >= 50)
		{
			// i should make this a config option
			//setAddStroke(addStroke() + 1);
			setBlowUp(false);
			resetSize();
			return;
		}

		const double diff = 8;
		double randnum = KRandom::random();
		const double width = 6 + randnum * (diff / RAND_MAX);
		randnum = KRandom::random();
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
	Q3CanvasEllipse::setVelocity(vx, vy);

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

	Q3CanvasEllipse::setVelocity(cos(newVector.direction()) * newVector.magnitude(), -sin(newVector.direction()) * newVector.magnitude());
}

void Ball::moveBy(double dx, double dy)
{
	double oldx;
	double oldy;
	oldx = x();
	oldy = y();
	Q3CanvasEllipse::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect(oldx, oldy);
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
	
	label->move(x() + width(), y() + height());
}

void Ball::doAdvance()
{
	Q3CanvasEllipse::advance(1);
}

namespace Lines
{
	// provides a point made of doubles

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
		// Charles says, TODO: Account for vertical lines
		// Jason says, in my testing vertical lines work
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

void Ball::collisionDetect(double oldx, double oldy)
{
	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	//kDebug(12007) << "------" << endl;
	//kDebug(12007) << "Ball::collisionDetect id " << collisionId << endl;

	// every other time...
	// do friction
	if (collisionId % 2 && !(xVelocity() == 0 && yVelocity() == 0))
		friction();

	const double minSpeed = .06;

	Q3CanvasItemList m_list = collisions(true);

	// please don't ask why QCanvas doesn't actually sort its list;
	// it just doesn't.
	m_list.sort();

	this->m_list = m_list;

	for (Q3CanvasItemList::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		Q3CanvasItem *item = *it;

		if (item->rtti() == Rtti_NoCollision || item->rtti() == Rtti_Putter)
			continue;

		if (item->rtti() == rtti() && !m_collisionLock)
		{
			// it's one of our own kind, a ball
			Ball *oball = dynamic_cast<Ball *>(item);
			if (!oball || oball->collisionLock())
				continue;
			oball->setCollisionLock(true);

			if ((oball->x() - x() != 0 && oball->y() - y() != 0) && state == Rolling && oball->curState() != Holed)
			{
				m_collisionLock = true;
				// move this ball to where it was barely touching
				double ballAngle = m_vector.direction();
				while (collisions(true).contains(item) > 0)
					move(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);

				// make a 2 pixel separation
				move(x() - 2 * cos(ballAngle), y() + 2 * sin(ballAngle));

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
		else if (item->rtti() == Rtti_WallPoint)
		{
			//kDebug(12007) << "collided with WallPoint\n";
			// iterate through the rst
			Q3PtrList<WallPoint> points;
			for (Q3CanvasItemList::Iterator pit = it; pit != m_list.end(); ++pit)
			{
				if ((*pit)->rtti() == Rtti_WallPoint)
				{
					WallPoint *point = (WallPoint *)(*pit);
					if (point)
						points.prepend(point);
				}
			}

			// ok now we have a list of wall points we are on

			WallPoint *iterpoint = 0;
			WallPoint *finalPoint = 0;

			// this wont be least when we're done hopefully
			double leastAngleDifference = 9999;

			for (iterpoint = points.first(); iterpoint; iterpoint = points.next())
			{
				//kDebug(12007) << "-----\n";
				const Wall *parentWall = iterpoint->parentWall();
				const QPoint qp(iterpoint->x() + parentWall->x(), iterpoint->y() + parentWall->y());
				const Point p(qp.x(), qp.y());
				const QPoint qother = QPoint(parentWall->startPoint() == qp? parentWall->endPoint() : parentWall->startPoint()) + QPoint(parentWall->x(), parentWall->y());
				const Point other(qother.x(), qother.y());

				// vector of wall
				Vector v = Vector(p, other);

				// difference between our path and the wall path
				double ourDir = m_vector.direction();

				double wallDir = M_PI - v.direction();

				//kDebug(12007) << "ourDir: " << rad2deg(ourDir) << endl;
				//kDebug(12007) << "wallDir: " << rad2deg(wallDir) << endl;

				const double angleDifference = fabs(M_PI - fabs(ourDir - wallDir));
				//kDebug(12007) << "computed angleDifference: " << rad2deg(angleDifference) << endl;

				// only if this one is the least of all
				if (angleDifference < leastAngleDifference)
				{
					leastAngleDifference = angleDifference;
					finalPoint = iterpoint;
					//kDebug(12007) << "it's the one\n";
				}
			}

			// this'll never happen
			if (!finalPoint)
				continue;

			// collide with our chosen point
			finalPoint->collision(this, collisionId);

			// don't worry about colliding with walls
			// wall points are ok alone
			goto end;
		}

		if (!isVisible() || state == Holed)
			return;

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			if (!citem->terrainCollisions())
			{
				// read: if (not do terrain collisions)
				if (!citem->collision(this, collisionId))
				{
					// if (skip smart wall test)
					if (citem->vStrut() || item->rtti() == Rtti_Wall)
						goto end;
					else
						goto wallCheck;
				}
			}
			break;
		}
	}

	for (Q3CanvasItemList::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem && citem->terrainCollisions())
		{
			// slopes return false
			// as only one should be processed
			// however that might not always be true

			// read: if (not do terrain collisions)
			if (!citem->collision(this, collisionId))
			{
				break;
			}
		}
	}

// Charles's smart wall check:
	
	wallCheck:

	{ // check if I went through a wall
		Q3CanvasItemList items;
		if (game)
			items = game->canvas()->allItems();
		for (Q3CanvasItemList::Iterator i = items.begin(); i != items.end(); ++i)
		{
			if ((*i)->rtti() != Rtti_Wall)
				continue;

			Q3CanvasItem *item = (*i);
			Wall *wall = dynamic_cast<Wall*>(item);
			if (!wall || !wall->isVisible())
				continue;

			if (Lines::intersects(
					wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y(),
					wall->endPoint().x() + wall->x(),   wall->endPoint().y() + wall->y(),
				
					oldx, oldy, x(), y()
				))
			{
				//kDebug(12007) << "smart wall collision\n";
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

void Ball::showInfo()
{
	label->setVisible(isVisible());
}

void Ball::hideInfo()
{
	label->setVisible(false);
}

void Ball::setName(const QString &name)
{
	label->setText(name);
}

void Ball::setCanvas(Q3Canvas *c)
{
	Q3CanvasEllipse::setCanvas(c);
	label->setCanvas(c);
}

void Ball::setVisible(bool yes)
{
	Q3CanvasEllipse::setVisible(yes);

	label->setVisible(yes && game && game->isInfoShowing());
}

