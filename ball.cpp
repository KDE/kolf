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

inline double rad2deg(double theDouble)
{
	return ((360L / (2L * M_PI)) * theDouble);
}

Ball::Ball(QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	m_collisionLock = false;
	setBeginningOfHole(false);
	setBlowUp(false);
	setPen(black);
	resetSize();
	setVelocity(0, 0);
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
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
	//kdDebug() << "Friction, magnitude is " << m_vector.magnitude() << ", subtractAmount is " << subtractAmount << endl;
	if (m_vector.magnitude() <= subtractAmount)
	{
		//kdDebug() << "Ball::friction stopping\n";
		//kdDebug() << "magnitude is " << m_vector.magnitude() << endl;
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
	//kdDebug() << "Ball::setVelocity(" << vx << ", " << vy << ")" << endl;

	QCanvasEllipse::setVelocity(vx, vy);

	if (vx == 0 && vy == 0)
	{
		m_vector.setDirection(0);
		m_vector.setMagnitude(0);
		return;
	}

	double ballAngle = atan2(-vy, vx);
	//kdDebug() << "ballAngle calculated as " << rad2deg(ballAngle) << endl;

	m_vector.setDirection(ballAngle);
	m_vector.setMagnitude(sqrt(pow(vx, 2) + pow(vy, 2))); 
}

void Ball::setVector(Vector newVector)
{
	//kdDebug() << "Ball::setVector; Magnitude = " << newVector.magnitude() << ", Direction = " << newVector.direction() << endl;

	m_vector = newVector;

	if (newVector.magnitude() == 0)
	{
		setVelocity(0, 0);
		return;
	}

	QCanvasEllipse::setVelocity(cos(newVector.direction()) * newVector.magnitude(), -sin(newVector.direction()) * newVector.magnitude());

	//kdDebug() << "new velocities calculated as " << xVelocity() << ", " << yVelocity() << endl;
}

void Ball::moveBy(double dx, double dy)
{
	QCanvasEllipse::moveBy(dx, dy);
	collisionDetect();
	if (game)
		if (game->curBall() == this)
			game->ballMoved();
}

void Ball::doAdvance()
{
	QCanvasEllipse::advance(1);
}

void Ball::collisionDetect()
{
	if (!isVisible() || state == Holed)
		return;

	//kdDebug() << "collision detect\n";
	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	// every other time...
	// do friction
	if (collisionId % 2 && !(xVelocity() == 0 && yVelocity() == 0))
		friction();

	//kdDebug() << "velocitiees: " << xVelocity() << ", " << yVelocity() << endl;

	QCanvasItemList list = collisions(true);
	//kdDebug() << "collisions done\n";
	if (list.isEmpty())
	{
		//kdDebug() << "collision list empty\n";
		return;
	}

	// please don't ask why QCanvas doesn't actually sort its list
	// it just doesn't.
	list.sort();

	QCanvasItem *item = 0;

	for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
	{
		item = *it;

		if (item->rtti() == Rtti_NoCollision || item->rtti() == Rtti_Putter)
			continue;

		if (item->rtti() == rtti() && !m_collisionLock)
		{
			// it's one of our own kind, a ball
			Ball *oball = dynamic_cast<Ball *>(item);
			if (oball->collisionLock())
				continue;

			if ((oball->x() - x() != 0 && oball->y() - y() != 0) && state == Rolling)
			{
				m_collisionLock = true;
				//kdDebug() << "collision with other ball\n";
				// move this ball to where it was barely touching
				double ballAngle = m_vector.direction();
				while (collisions(true).contains(item) > 0)
				{
					move(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);
					//kdDebug() << "moved to " << x() << ", " << y() << endl;
				}
				//move(x() - cos(ballAngle), y() - sin(ballAngle));

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

				double distance = 0;

				/*
				distance = sqrt(pow(x() - item->x(), 2) + pow(y() - item->y(), 2));
				//kdDebug() << "distance between balls is " << distance << endl;
				double delta = (width() - distance) / 2.0;
				//kdDebug() << "delta is " << delta << endl;
				double _angle = atan2(y() - item->y(), x() - item->x());
				//kdDebug() << "angle between balls is " << rad2deg(distance) << endl;
				double delta_x = ::cos(_angle) * delta;
				double delta_y = sin(_angle) * delta;

				//kdDebug() << "moving balls\n";
				setX(x() - delta_x);
				setY(y() - delta_y);
				item->setX(item->x() + delta_x);
				item->setY(item->y() + delta_y);
				//kdDebug() << "done moving balls\n";
				*/

				distance = sqrt(pow(x() - item->x(), 2) + pow(y() - item->y(), 2));
				//kdDebug() << "distance between balls is " << distance << endl;
				//kdDebug() << "---------\n";

				oball->setState(Rolling);
				setState(Rolling);
			}

			continue;
		}

		if (!isVisible() || state == Holed)
			return;

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->collision(this, collisionId);
		else
			break;

		continue;
	}
}

BallState Ball::currentState()
{
	return state;
}

