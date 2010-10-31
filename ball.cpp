/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "ball.h"
#include "game.h"
#include "rtti.h"
#include "shape.h"

#include <QApplication>

Ball::Ball(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QLatin1String("ball"), parent, world)
{
	const int diameter = 8;
	setSize(QSizeF(diameter, diameter));
	addShape(new Kolf::EllipseShape(QRectF(-diameter / 2, -diameter / 2, diameter, diameter)));

	setData(0, Rtti_NoCollision);
	m_doDetect = true;
	setBeginningOfHole(false);
	m_collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
	m_forceStillGoing = false;
	frictionMultiplier = 1.0;

	QFont font(QApplication::font());
	font.setPixelSize(12);
	label = new QGraphicsSimpleTextItem(QString(), this);
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
	if (state == Holed)
		setSimulationType(CanvasItem::NoSimulation);
	else
		setSimulationType(CanvasItem::DynamicSimulation);

	if (state == Stopped)
		setZValue(1000);
	else
		setBeginningOfHole(false);
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible())
	{
		setVelocity(Vector());
		return;
	}
	const double subtractAmount = .027 * frictionMultiplier;
	if (m_vector.magnitude() <= subtractAmount)
	{
		state = Stopped;
		setVelocity(Vector());
		game->timeout();
		return;
	}
	m_vector.setMagnitude(m_vector.magnitude() - subtractAmount);
	setVector(m_vector);

	frictionMultiplier = 1.0;
}

void Ball::setVelocity(const Vector& velocity)
{
	CanvasItem::setVelocity(velocity);
	m_vector = QPointF(velocity.x(), -velocity.y());
}

void Ball::setVector(const Vector& newVector)
{
	m_vector = newVector;
	CanvasItem::setVelocity(Vector(newVector.x(), -newVector.y()));
}

void Ball::moveBy(double dx, double dy)
{
	EllipticalCanvasItem::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect();
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::doAdvance()
{
	if (!velocity().isNull())
		moveBy(velocity().x(), velocity().y());
}

void Ball::collisionDetect()
{
	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	// do friction every other time
	m_collisionId = (m_collisionId + 1) % 2;
	if (m_collisionId == 1 && !velocity().isNull())
		friction();

	double initialVector = m_vector.magnitude();
	const double minSpeed = .06;
	bool justCollidedWithWall = false;

	QList<QGraphicsItem *> items = collidingItems();

	bool doTerrainCollisions = true;
	foreach (QGraphicsItem* item, items)
	{
		if (item->data(0) == Rtti_NoCollision || item->data(0) == Rtti_Putter)
			continue;
	
		if (!isVisible() || state == Holed)
			return;

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			if (!citem->terrainCollisions())
			{
				//do collision
				const bool allowTerrainCollisions = citem->collision(this);
				doTerrainCollisions = doTerrainCollisions && allowTerrainCollisions;
			}
			break;
		}
	}

	if (doTerrainCollisions)
	{
		foreach (QGraphicsItem* item, items)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
			if (citem && citem->terrainCollisions())
			{
				// slopes return false
				// as only one should be processed
				// however that might not always be true
				if (!citem->collision(this))
				{
					break;
				}
			}
		}
	}

	double vectorChange = initialVector - m_vector.magnitude();
	if(vectorChange < 0 ) 
		vectorChange *= -1;

	if(m_vector.magnitude() < minSpeed && vectorChange < minSpeed && m_vector.magnitude())
	{
		if( justCollidedWithWall )
		{ //don't want to stop if just hit a wall as may be in the wall
			//problem: could this cause endless ball bouncing between 2 walls?
			m_vector.setMagnitude(minSpeed);
		}
		else
		{
			setVelocity(Vector());
			setState(Stopped);
		}
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
	EllipticalCanvasItem::setVisible(yes);

	label->setVisible(yes && game && game->isInfoShowing());
}
