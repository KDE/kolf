#include <qcanvas.h>
#include <qcolor.h>
#include <qpen.h>

#include <kapplication.h>
#include <kdebug.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtti.h"
#include "canvasitem.h"
#include "game.h"
#include "ball.h"

Ball::Ball(QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	setBeginningOfHole(false);
	setBlowUp(false);
	setPen(black);
	resetSize();
	setVelocity(0, 0);
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;

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
	if (xVelocity() == 0 && yVelocity() == 0)
	{
		state = Stopped;
		return;
	}
	double vx = xVelocity();
	double vy = yVelocity();
	double ballAngle = atan(vx / vy);
	if (vy < 0)
		ballAngle -= M_PI;
	ballAngle = M_PI/2 - ballAngle;
	const double frictionFactor = .027;
	vx -= cos(ballAngle) * frictionFactor * frictionMultiplier;
	vy -= sin(ballAngle) * frictionFactor * frictionMultiplier;
	if (vx / xVelocity() < 0)
	{
		vx = vy = 0;
		state = Stopped;
	}
	setVelocity(vx, vy);

	frictionMultiplier = 1;
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
	//const double halfX = xVelocity() / 2;
	//const double halfY = yVelocity() / 2;
	QCanvasEllipse::advance(1);
	
	//moveBy(halfX, halfY);
	//collisionDetect();
	//moveBy(halfX, halfY);
}

void Ball::collisionDetect()
{
	//kdDebug() << "collision detect\n";
	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	// every other time...
	// do friction
	if (collisionId % 2 && !(xVelocity() == 0 && yVelocity() == 0))
		friction();

	QCanvasItemList list = collisions(true);
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

		if (item->rtti() == rtti())
		{
			// it's one of our own kind, a ball
			Ball *oball = dynamic_cast<Ball *>(item);
			if (oball->curState() == Stopped && (oball->x() - x() != 0 && oball->y() - y() != 0) && state == Rolling)
			{
			QPoint oldPos(x(), y());
			double vx = xVelocity();
			double vy = yVelocity();
			double ballAngle;
			if (vy == 0)
				ballAngle = xVelocity() > 0? 0 : M_PI;
			else
				ballAngle = atan(vx / vy);
			if (vy < 0)
				ballAngle -= M_PI;
			ballAngle = M_PI / 2 - ballAngle;
			while (collisions(true).contains(item) > 0)
			{
				move(x() - cos(ballAngle), y() - sin(ballAngle));

				// for debugging
				/*
				   kapp->processEvents();
				   sleep(1);
				 */
			}

			const double mySpeed = curSpeed();

			//oball->setBlowUp(true);
			double angle = atan2(y() - oball->y(), oball->x() - x());
			kdDebug() << "ballAngle is " << (360L / (2L * M_PI)) * ballAngle << endl;
			kdDebug() << "angle is " << (360L / (2L * M_PI)) * angle << endl;

			//double myNewSpeed = mySpeed + oball->curSpeed();
			//setVelocity(cos(ballAngle) * myNewSpeed, sin(ballAngle) * myNewSpeed);

			double angleDifference = fabs(fabs(ballAngle) - fabs(angle));
			while (angleDifference > M_PI / 2)
			{
				kdDebug() << "angleDifference = " << (360L / (2L * M_PI)) * angleDifference << endl;
				angleDifference -= M_PI / 2;
			}
			kdDebug() << "angleDifference = " << (360L / (2L * M_PI)) * angleDifference << endl;
			double newVelocity = mySpeed * ((double)(M_PI / 2 - angleDifference) / (double)(M_PI / 2));

			if (y() == oball->y())
			{
				// horiz
				oball->setVelocity(newVelocity, 0);
				kdDebug() << "horizontal\n";
			}
			else if (x() == oball->x())
			{
				// vert
				oball->setVelocity(0, newVelocity);
				kdDebug() << "vertical\n";
			}
			else
			{
				oball->setVelocity(cos(angle) * newVelocity, -sin(angle) * newVelocity);
			}
			oball->setState(Rolling);
			}
			continue;
		}

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->collision(this, collisionId);
		else
			continue;
		break;
	}
}

BallState Ball::currentState()
{
	return state;
}

