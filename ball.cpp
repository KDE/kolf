#include <QGraphicsView>
#include <QColor>
#include <QPen>

#include <kapplication.h>
#include <kdebug.h>
#include "game.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <krandom.h>

#include "rtti.h"
#include "vector.h"
#include "canvasitem.h"
#include "ball.h"

Ball::Ball(QGraphicsScene *scene)
	: QGraphicsEllipseItem(0, scene)
{
	setData(0, Rtti_Ball);
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
	label = new QGraphicsSimpleTextItem("", this, scene);
	label->setFont(font);
	label->setBrush(Qt::white);
	label->setPos(5, 5);
	label->setVisible(false);

	// this sets z
	setState(Stopped);
	label->setZValue(zValue() - .1);
}

void Ball::aboutToDie()
{
	delete label;
}

void Ball::setState(BallState newState)
{
	state = newState;
	if (state == Stopped)
		setZValue(1000);
	else
		setBeginningOfHole(false);
}

void Ball::resetSize()
{
	setRect(rect().x()-3.5, rect().y()-3.5, 7, 7);
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
		setRect(rect().x(), rect().y(), width, height);
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
	CanvasItem::setVelocity(vx, vy);

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

	CanvasItem::setVelocity(cos(newVector.direction()) * newVector.magnitude(), -sin(newVector.direction()) * newVector.magnitude());
}

void Ball::moveBy(double dx, double dy)
{
	double oldx;
	double oldy;
	oldx = x();
	oldy = y();
	QGraphicsEllipseItem::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect(oldx, oldy);
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::doAdvance()
{
	if(getXVelocity()!=0 || getYVelocity()!=0) 
		moveBy(getXVelocity(), getYVelocity());
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
	if (collisionId % 2 && !(getXVelocity() == 0 && getXVelocity() == 0))
		friction();

	const double minSpeed = .06;

	QList<QGraphicsItem *> m_list = collidingItems();

	this->m_list = m_list;

	for (QList<QGraphicsItem *>::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		QGraphicsItem *item = *it;

		if (item->data(0) == Rtti_NoCollision || item->data(0) == Rtti_Putter)
			continue;

		if (item->data(0) == data(0) && !m_collisionLock)
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
				while (collidingItems().contains(item) > 0)
					setPos(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);

				// make a 2 pixel separation
				setPos(x() - 2 * cos(ballAngle), y() + 2 * sin(ballAngle));

				Vector bvector = oball->curVector();
				m_vector -= bvector;

				Vector unit1 = Vector(QPointF(x(), y()), QPointF(oball->x(), oball->y()));
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
		else if (item->data(0) == Rtti_WallPoint)
		{
			//kDebug(12007) << "collided with WallPoint\n";
			// iterate through the rst
			QList<WallPoint *> points;
			for (QList<QGraphicsItem *>::Iterator pit = it; pit != m_list.end(); ++pit)
			{
				if ((*pit)->data(0) == Rtti_WallPoint)
				{
					WallPoint *point = (WallPoint *)(*pit);
					if (point)
						points.prepend(point);
				}
			}

			// ok now we have a list of wall points we are on

			QList<WallPoint *>::const_iterator iterpoint;
			QList<WallPoint *>::const_iterator finalPoint;

			// this wont be least when we're done hopefully
			double leastAngleDifference = 9999;

			for (iterpoint = points.constBegin(); iterpoint != points.constEnd(); ++iterpoint)
			{
				//kDebug(12007) << "-----\n";
				const Wall *parentWall = (*iterpoint)->parentWall();
				const QPointF p(((*iterpoint)->x() + parentWall->x()), ((*iterpoint)->y() + parentWall->y()));
				const QPointF other = QPointF(parentWall->startPoint() == p? parentWall->endPoint() : parentWall->startPoint()) + QPointF(parentWall->x(), parentWall->y());

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
			if (!(*finalPoint))
				continue;

			// collide with our chosen point
			(*finalPoint)->collision(this, collisionId);

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
				// read: if (not do terrain collidingItems)
				if (!citem->collision(this, collisionId))
				{
					// if (skip smart wall test)
					if (citem->vStrut() || item->data(0) == Rtti_Wall)
						goto end;
					else
						goto wallCheck;
				}
			}
			break;
		}
	}

	for (QList<QGraphicsItem *>::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem && citem->terrainCollisions())
		{
			// slopes return false
			// as only one should be processed
			// however that might not always be true

			// read: if (not do terrain collidingItems)
			if (!citem->collision(this, collisionId))
			{
				break;
			}
		}
	}

// Charles's smart wall check:
	
	wallCheck:

	{ // check if I went through a wall
		QList<QGraphicsItem *> items;
		if (game)
			items = game->scene()->items();
		for (QList<QGraphicsItem *>::Iterator i = items.begin(); i != items.end(); ++i)
		{
			if ((*i)->data(0) != Rtti_Wall)
				continue;

			QGraphicsItem *item = (*i);
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

void Ball::setVisible(bool yes)
{
	QGraphicsEllipseItem::setVisible(yes);

	label->setVisible(yes && game && game->isInfoShowing());
}

