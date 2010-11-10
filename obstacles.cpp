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

#include "obstacles.h"
#include "ball.h"
#include "shape.h"

#include <QTimer>
#include <KConfigGroup>
#include <KRandom>

//BEGIN Kolf::Bumper

Kolf::Bumper::Bumper(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(false, QLatin1String("bumper_off"), parent, world)
{
	const int diameter = 20;
	setSize(QSizeF(diameter, diameter));
	setZValue(-25);
	setSimulationType(CanvasItem::NoSimulation);
}

bool Kolf::Bumper::collision(Ball* ball)
{
	const double maxSpeed = ball->getMaxBumperBounceSpeed();
	const double speed = qMin(maxSpeed, 1.8 + ball->velocity().magnitude() * .9);
	ball->reduceMaxBumperBounceSpeed();

	Vector betweenVector(ball->pos() - pos());
	betweenVector.setMagnitudeDirection(speed,
		// add some randomness so we don't go indefinetely
		betweenVector.direction() + deg2rad((KRandom::random() % 3) - 1)
	);

	ball->setVelocity(betweenVector);
	ball->setState(Rolling);

	setSpriteKey(QLatin1String("bumper_on"));
	QTimer::singleShot(100, this, SLOT(turnBumperOff()));
	return true;
}

void Kolf::Bumper::turnBumperOff()
{
	setSpriteKey(QLatin1String("bumper_off"));
}

Kolf::Overlay* Kolf::Bumper::createOverlay()
{
	return new Kolf::Overlay(this, this);
}

//END Kolf::Bumper
//BEGIN Kolf::Wall

Kolf::Wall::Wall(QGraphicsItem* parent, b2World* world)
	: QGraphicsLineItem(QLineF(-15, 10, 15, -5), parent)
	, CanvasItem(world)
{
	setPen(QPen(Qt::darkRed, 3));
	setData(0, Rtti_NoCollision);
	setZValue(50);

	m_shape = new Kolf::LineShape(line());
	addShape(m_shape);
}

void Kolf::Wall::load(KConfigGroup* cfgGroup)
{
	const QPoint start = cfgGroup->readEntry("startPoint", QPoint(-15, 10));
	const QPoint end = cfgGroup->readEntry("endPoint", QPoint(15, -5));
	setLine(QLineF(start, end));
}

void Kolf::Wall::save(KConfigGroup* cfgGroup)
{
	const QLineF line = this->line();
	cfgGroup->writeEntry("startPoint", line.p1().toPoint());
	cfgGroup->writeEntry("endPoint", line.p2().toPoint());
}

void Kolf::Wall::setVisible(bool visible)
{
	QGraphicsLineItem::setVisible(visible);
	setSimulationType(visible ? CanvasItem::CollisionSimulation : CanvasItem::NoSimulation);
}

void Kolf::Wall::doAdvance()
{
	moveBy(velocity().x(), velocity().y());
}

void Kolf::Wall::setLine(const QLineF& line)
{
	QGraphicsLineItem::setLine(line);
	m_shape->setLine(line);
	propagateUpdate();
}

void Kolf::Wall::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
}

QPointF Kolf::Wall::getPosition() const
{
	return QGraphicsItem::pos();
}

Kolf::Overlay* Kolf::Wall::createOverlay()
{
	return new Kolf::WallOverlay(this);
}

//END Kolf::Wall
//BEGIN Kolf::WallOverlay

Kolf::WallOverlay::WallOverlay(Kolf::Wall* wall)
	: Kolf::Overlay(wall, wall)
	, m_handle1(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
	, m_handle2(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
{
	addHandle(m_handle1);
	addHandle(m_handle2);
	connect(m_handle1, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
	connect(m_handle2, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
}

void Kolf::WallOverlay::update()
{
	Kolf::Overlay::update();
	const QLineF line = dynamic_cast<Kolf::Wall*>(qitem())->line();
	m_handle1->setPos(line.p1());
	m_handle2->setPos(line.p2());
}

void Kolf::WallOverlay::moveHandle(const QPointF& handleScenePos)
{
	QPointF handlePos = mapFromScene(handleScenePos);
	const QObject* handle = sender();
	//get handle positions
	QPointF handle1Pos = m_handle1->pos();
	QPointF handle2Pos = m_handle2->pos();
	if (handle == m_handle1)
		handle1Pos = handlePos;
	else if (handle == m_handle2)
		handle2Pos = handlePos;
	//ensure minimum length
	static const qreal minLength = Kolf::Overlay::MinimumObjectDimension;
	const QPointF posDiff = handle1Pos - handle2Pos;
	const qreal length = QLineF(QPointF(), posDiff).length();
	if (length < minLength)
	{
		const QPointF additionalExtent = posDiff * (minLength / length - 1);
		if (handle == m_handle1)
			handle1Pos += additionalExtent;
		else if (handle == m_handle2)
			handle2Pos -= additionalExtent;
	}
	//apply to item
	dynamic_cast<Kolf::Wall*>(qitem())->setLine(QLineF(handle1Pos, handle2Pos));
}

//END Kolf::WallOverlay

#include "obstacles.moc"
