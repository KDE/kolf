#include <qcanvas.h>
#include <qcolor.h>
#include <qpen.h>

#include <kapplication.h>

#include <math.h>
#include <stdlib.h>

#include "rtti.h"
#include "canvasitem.h"
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
	if (state == Stopped)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	// every other time...
	// do friction
	if (collisionId % 2)
		friction();

	QCanvasItemList list = collisions(true);
	if (list.isEmpty())
		return;

	// please don't ask why QCanvas doesn't actually sort its list
	// it just doesn't.
	list.sort();

	QCanvasItem *item = 0;

	for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
	{
		item = *it;

		if (item->rtti() == Rtti_NoCollision || item->rtti() == Rtti_Putter)
			continue;
		if (!collidesWith(item))
			continue;

		if (item->rtti() == rtti())
		{
			if (curSpeed() > 2.7)
			{
				// it's one of our own kind, a ball, and we're hitting it
				// sorta hard
				Ball *oball = dynamic_cast<Ball *>(item);
				if (/*oball->curState() != Stopped && */oball->curState() != Holed)
					oball->setBlowUp(true);
				continue;
			}
		}

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->collision(this, collisionId);
		break;
	}
}

BallState Ball::currentState()
{
	return state;
}

