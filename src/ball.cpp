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
#include "overlay.h"
#include "shape.h"

#include <QApplication>

Ball::Ball(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QStringLiteral("ball"), parent, world)
{
	const int diameter = 8;
	setSize(QSizeF(diameter, diameter));
	setZBehavior(CanvasItem::IsRaisedByStrut, 10);

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
}

void Ball::setState(BallState newState)
{
	state = newState;
	if (state == Holed || !EllipticalCanvasItem::isVisible())
		setSimulationType(CanvasItem::NoSimulation);
	else
		setSimulationType(CanvasItem::DynamicSimulation);

	if (state != Stopped)
		setBeginningOfHole(false);
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible())
	{
		setVelocity(QPointF());
		return;
	}
	const double subtractAmount = .027 * frictionMultiplier;
	Vector velocity = this->velocity();
	if (velocity.magnitude() <= subtractAmount)
	{
		state = Stopped;
		setVelocity(QPointF());
		game->timeout();
		return;
	}
	velocity.setMagnitude(velocity.magnitude() - subtractAmount);
	setVelocity(velocity);

	frictionMultiplier = 1.0;
}

void Ball::moveBy(double dx, double dy)
{
	EllipticalCanvasItem::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect();

	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::endSimulation()
{
	CanvasItem::endSimulation();
	if (state == Stopped) {
		if (!qFuzzyIsNull(Vector(velocity()).magnitude())) {
			//ball was resting, but received some momentum from collision with other ball
			setState(Rolling);
		}
	} else if (state == Rolling) {
		if (qFuzzyIsNull(Vector(velocity()).magnitude())) {
			//ball was moving, but stopped moving from collision with other ball
			setState(Stopped);
		}
	}
}

void Ball::collisionDetect()
{
	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	// do friction every other time
	m_collisionId = (m_collisionId + 1) % 2;
	if (m_collisionId == 1 && !velocity().isNull())
		friction();

	const double initialVelocity = Vector(velocity()).magnitude();
	const double minSpeed = .06;

	const QList<QGraphicsItem *> items = collidingItems();

	bool doTerrainCollisions = true;
	for (QGraphicsItem* item : items) {
		if (item->data(0) == Rtti_NoCollision || item->data(0) == Rtti_Putter)
		{
			if (item->data(0) == Rtti_NoCollision)
				game->playSound(Sound::Wall);
			continue;
		}
	
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
		for (QGraphicsItem* item : items) {
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

	const double currentVelocity = Vector(velocity()).magnitude();
	const double velocityChange = qAbs(initialVelocity - currentVelocity);

	if(currentVelocity < minSpeed && velocityChange < minSpeed && currentVelocity)
	{
		//cutoff low velocities
		setVelocity(Vector());
		setState(Stopped);
	}
}

BallState Ball::currentState()
{
	return state;
}

QList<QGraphicsItem*> Ball::infoItems() const
{
	return QList<QGraphicsItem*>() << label;
}

void Ball::setName(const QString &name)
{
	label->setText(name);
}

void Ball::setVisible(bool yes)
{
	EllipticalCanvasItem::setVisible(yes);
	setState(state);
}

Kolf::Overlay* Ball::createOverlay()
{
	return new Kolf::Overlay(this, this);
}
