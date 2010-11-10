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
#include "game.h"
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
//BEGIN Kolf::RectangleItem

Kolf::RectangleItem::RectangleItem(const QString& type, QGraphicsItem* parent, b2World* world)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), type, parent)
	, CanvasItem(world)
	, m_wallPen(QColor("#92772D").darker(), 3)
	, m_walls(Kolf::RectangleWallCount, 0)
{
	setZValue(998);
	//default size
	setSize(type == "sign" ? QSize(110, 40) : QSize(80, 40));
}

Kolf::RectangleItem::~RectangleItem()
{
	qDeleteAll(m_walls);
}

bool Kolf::RectangleItem::hasWall(Kolf::WallIndex index) const
{
	return (bool) m_walls[index];
}

void Kolf::RectangleItem::setWall(Kolf::WallIndex index, bool hasWall)
{
	const bool oldHasWall = (bool) m_walls[index];
	if (oldHasWall == hasWall)
		return;
	if (hasWall)
	{
		Kolf::Wall* wall = m_walls[index] = new Kolf::Wall(parentItem(), world());
		wall->setPos(pos());
		applyWallStyle(wall);
		updateWallPosition();
	}
	else
	{
		delete m_walls[index];
		m_walls[index] = 0;
	}
	propagateUpdate();
}

void Kolf::RectangleItem::updateWallPosition()
{
	const QRectF rect(QPointF(), size());
	Kolf::Wall* const topWall = m_walls[Kolf::TopWallIndex];
	Kolf::Wall* const leftWall = m_walls[Kolf::LeftWallIndex];
	Kolf::Wall* const rightWall = m_walls[Kolf::RightWallIndex];
	Kolf::Wall* const bottomWall = m_walls[Kolf::BottomWallIndex];
	if (topWall)
		topWall->setLine(QLineF(rect.topLeft(), rect.topRight()));
	if (leftWall)
		leftWall->setLine(QLineF(rect.topLeft(), rect.bottomLeft()));
	if (rightWall)
		rightWall->setLine(QLineF(rect.topRight(), rect.bottomRight()));
	if (bottomWall)
		bottomWall->setLine(QLineF(rect.bottomLeft(), rect.bottomRight()));
}

void Kolf::RectangleItem::setSize(const QSizeF& size)
{
	Tagaro::SpriteObjectItem::setSize(size);
	updateWallPosition();
	propagateUpdate();
}

QPointF Kolf::RectangleItem::getPosition() const
{
	return QGraphicsItem::pos();
}

void Kolf::RectangleItem::moveBy(double dx, double dy)
{
	Tagaro::SpriteObjectItem::moveBy(dx, dy);
	//move myself
	const QPointF pos = this->pos();
	foreach (Kolf::Wall* wall, m_walls)
		if (wall)
			wall->setPos(pos);
	//update Z order of items on top of vStrut
	foreach (QGraphicsItem* qitem, collidingItems())
	{
		CanvasItem* citem = dynamic_cast<CanvasItem*>(qitem);
		if (citem)
			citem->updateZ();
	}
}

void Kolf::RectangleItem::setWallColor(const QColor& color)
{
	m_wallPen = QPen(color.darker(), 3);
	foreach (Kolf::Wall* wall, m_walls)
		applyWallStyle(wall);
}

void Kolf::RectangleItem::applyWallStyle(Kolf::Wall* wall)
{
	if (!wall) //explicitly allowed, see e.g. setWallColor()
		return;
	wall->setPen(m_wallPen);
	wall->setZValue(zValue() + 0.001);
}

void Kolf::RectangleItem::setZValue(qreal zValue)
{
	QGraphicsItem::setZValue(zValue);
	foreach (Kolf::Wall* wall, m_walls)
		applyWallStyle(wall);
}

static const char* wallPropNames[] = { "topWallVisible", "leftWallVisible", "rightWallVisible", "botWallVisible" };

void Kolf::RectangleItem::load(KConfigGroup* group)
{
	QSize size = Tagaro::SpriteObjectItem::size().toSize();
	size.setWidth(group->readEntry("width", size.width()));
	size.setHeight(group->readEntry("height", size.height()));
	setSize(size);
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
	{
		bool hasWall = this->hasWall((Kolf::WallIndex) i);
		hasWall = group->readEntry(wallPropNames[i], hasWall);
		setWall((Kolf::WallIndex) i, hasWall);
	}
}

void Kolf::RectangleItem::save(KConfigGroup* group)
{
	const QSize size = Tagaro::SpriteObjectItem::size().toSize();
	group->writeEntry("width", size.width());
	group->writeEntry("height", size.height());
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
	{
		const bool hasWall = this->hasWall((Kolf::WallIndex) i);
		group->writeEntry(wallPropNames[i], hasWall);
	}
}

Config* Kolf::RectangleItem::config(QWidget* parent)
{
	return CanvasItem::config(parent);
	//return new Kolf::RectangleConfig(parent);
}

Kolf::Overlay* Kolf::RectangleItem::createOverlay()
{
	return new Kolf::Overlay(this, this);
	//return new Kolf::RectangleOverlay(this);
}

//END Kolf::RectangleItem
//BEGIN Kolf::Bridge

Kolf::Bridge::Bridge(QGraphicsItem* parent, b2World* world)
	: Kolf::RectangleItem(QLatin1String("bridge"), parent, world)
{
}

bool Kolf::Bridge::collision(Ball* ball)
{
	ball->setFrictionMultiplier(.63);
	return false;
}

//END Kolf::Bridge

#include "obstacles.moc"
