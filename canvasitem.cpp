/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2008, 2009, 2010 Stefan Majewsky <majewsky@gmx.net>

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

#include "canvasitem.h"
#include "game.h"
#include "landscape.h"
#include "overlay.h"
#include "shape.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2World.h>

//this is how much a strut and the items on it are raised
static const int ZValueStep = 100;

CanvasItem::CanvasItem(b2World* world)
	: game(0)
	, m_zBehavior(CanvasItem::FixedZValue)
	, m_zValue(0)
	, m_strut(0)
	, m_body(0)
	, m_overlay(0)
	, m_simulationType((CanvasItem::SimulationType) -1)
{
	b2BodyDef bodyDef;
	bodyDef.userData = this;
	m_body = world->CreateBody(&bodyDef);
	setSimulationType(CanvasItem::CollisionSimulation);
}

CanvasItem::~CanvasItem()
{
	//disconnect struts
	if (m_strut)
		m_strut->m_struttedItems.removeAll(this);
	foreach (CanvasItem* item, m_struttedItems)
		item->m_strut = 0;
	//The overlay is deleted first, because it might interact with all other parts of the object.
	delete m_overlay;
	//NOTE: Box2D objects will need to be destroyed in the following order:
	//subobjects, shapes, own b2Body
	qDeleteAll(m_shapes);
	m_body->GetWorld()->DestroyBody(m_body);
}

void CanvasItem::setZBehavior(CanvasItem::ZBehavior behavior, qreal zValue)
{
	m_zBehavior = behavior;
	m_zValue = zValue;
	QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(this);
	if (qitem)
	{
		if (m_zBehavior == CanvasItem::FixedZValue)
			qitem->setZValue(m_zValue);
		else
			updateZ(qitem);
	}
}

void CanvasItem::updateZ(QGraphicsItem* self)
{
	//disconnect from old strut (if any)
	if (m_strut)
	{
		m_strut->m_struttedItems.removeAll(this);
		m_strut = 0;
	}
	//simple behavior
	if (m_zBehavior == CanvasItem::FixedZValue)
		return;
	if (m_zBehavior == CanvasItem::IsStrut)
	{
		self->setZValue(ZValueStep);
		return;
	}
	//determine new strut
	foreach (QGraphicsItem* qitem, self->collidingItems())
	{
		CanvasItem* citem = dynamic_cast<CanvasItem*>(qitem);
		if (citem && citem->m_zBehavior == CanvasItem::IsStrut)
		{
			//special condition for slopes: they must lie inside the strut's area, not only touch it
			Kolf::Slope* slope = dynamic_cast<Kolf::Slope*>(this);
			if (slope)
				if (!slope->collidesWithItem(qitem, Qt::ContainsItemBoundingRect))
					continue;
			//strut found
			m_strut = citem;
			m_strut->m_struttedItems << this;
			self->setZValue(m_zValue + ZValueStep);
			return;
		}
	}
	//no strut found -> set default zValue
	self->setZValue(m_zValue);
}

void CanvasItem::moveItemsOnStrut(const QPointF& posDiff)
{
	foreach (CanvasItem* citem, m_struttedItems)
	{
		QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(citem);
		if (!qitem || qitem->data(0) == Rtti_Putter)
			continue;
		citem->moveBy(posDiff.x(), posDiff.y());
		Ball* ball = dynamic_cast<Ball*>(citem);
		if (ball && game && !game->isEditing() && game->curBall() == ball)
			game->ballMoved();
	}
}

/*static*/ bool CanvasItem::mayCollide(CanvasItem* citem1, CanvasItem* citem2)
{
	//which one is the ball?
	Ball* ball = dynamic_cast<Ball*>(citem1);
	CanvasItem* citem = citem2;
	if (!ball)
	{
		ball = dynamic_cast<Ball*>(citem2);
		citem = citem1;
	}
	if (!ball)
		//huh, no ball involved? then don't restrict anything, because
		//that likely introduces weird bugs later
		return true;
	//if both items are graphicsitems, restrict collisions of ball to thos
	//objects on same strut level or above (i.e. don't collide with
	//stuff below the current strut)
	const QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(citem);
	if (!qitem)
		return true;
	const int ballStrutLevel = int(ball->zValue()) / ZValueStep;
	const int itemStrutLevel = int(qitem->zValue()) / ZValueStep;
	return ballStrutLevel <= itemStrutLevel;
}

void CanvasItem::moveBy(double dx, double dy)
{
	Q_UNUSED(dx) Q_UNUSED(dy)
	QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(this);
	if (qitem)
		updateZ(qitem);
}

void CanvasItem::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("dummykey", true);
}

void CanvasItem::playSound(const QString &file, double vol)
{
	if (game)
		game->playSound(file, vol);
}

void CanvasItem::editModeChanged(bool editing)
{
	Kolf::Overlay* overlay = this->overlay();
	if (overlay)
		overlay->setVisible(editing);
}

b2World* CanvasItem::world() const
{
	return m_body->GetWorld();
}

void CanvasItem::addShape(Kolf::Shape* shape)
{
	if (shape->attach(this)) //this will fail if the shape is already attached to some object
		m_shapes << shape;
}

void CanvasItem::setSimulationType(CanvasItem::SimulationType type)
{
	if (m_simulationType != type)
	{
		m_simulationType = type;
		//write type into b2Body
		b2BodyType b2type; bool b2active;
		switch (type)
		{
			case CanvasItem::NoSimulation:
				b2type = b2_staticBody;
				b2active = false;
				break;
			case CanvasItem::CollisionSimulation:
				b2type = b2_staticBody;
				b2active = true;
				break;
			case CanvasItem::KinematicSimulation:
				b2type = b2_kinematicBody;
				b2active = true;
				break;
			case CanvasItem::DynamicSimulation: default:
				b2type = b2_dynamicBody;
				b2active = true;
				break;
		}
		m_body->SetType(b2type);
		m_body->SetActive(b2active);
	}
}

QPointF CanvasItem::velocity() const
{
	b2Vec2 v = m_body->GetLinearVelocity();
	return QPointF(v.x, v.y);
}

void CanvasItem::setVelocity(const QPointF& newVelocity)
{
	const QPointF currentVelocity = this->velocity();
	if (newVelocity != currentVelocity)
	{
		const qreal mass = m_body->GetMass();
		//WARNING: Velocities are NOT scaled. The timestep is scaled, instead.
		//See where b2World::Step() gets called for more info.
		if (mass == 0 || m_simulationType != CanvasItem::DynamicSimulation)
		{
			m_body->SetLinearVelocity(b2Vec2(newVelocity.x(), newVelocity.y()));
		}
		else
		{
			const QPointF impulse = (newVelocity - currentVelocity) * mass;
			m_body->ApplyLinearImpulse(b2Vec2(impulse.x(), impulse.y()), m_body->GetPosition());
		}
	}
}

void CanvasItem::startSimulation()
{
	const QPointF position = getPosition() * Kolf::Box2DScaleFactor;
	m_body->SetTransform(b2Vec2(position.x(), position.y()), 0);
}

void CanvasItem::endSimulation()
{
	//read position
	b2Vec2 v = m_body->GetPosition();
	QPointF position = QPointF(v.x, v.y) / Kolf::Box2DScaleFactor;
	if (position != getPosition())
		//HACK: The above condition can be removed later, but for now we need to
		//prevent moveBy() from being called with (0, 0) arguments because such
		//have a non-standard behavior with some classes (e.g. Ball), i.e. these
		//arguments trigger some black magic
		setPosition(position);
}

Kolf::Overlay* CanvasItem::overlay(bool createIfNecessary)
{
	//the overlay is created once it is requested
	if (!m_overlay && createIfNecessary)
	{
		m_overlay = createOverlay();
		if (m_overlay)
		{
			//should be above object representation
			m_overlay->setZValue(m_overlay->qitem()->zValue() + 100);
			//initialize the overlay's parameters
			m_overlay->update();
		}
	}
	return m_overlay;
}

void CanvasItem::propagateUpdate()
{
	if (m_overlay)
		m_overlay->update();
}

//BEGIN EllipticalCanvasItem

EllipticalCanvasItem::EllipticalCanvasItem(bool withEllipse, const QString& spriteKey, QGraphicsItem* parent, b2World* world)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), spriteKey, parent)
	, CanvasItem(world)
	, m_ellipseItem(0)
	, m_shape(0)
{
	if (withEllipse)
	{
		m_ellipseItem = new QGraphicsEllipseItem(this);
		m_ellipseItem->setFlag(QGraphicsItem::ItemStacksBehindParent);
		//won't appear unless pen/brush is configured
		m_ellipseItem->setPen(Qt::NoPen);
		m_ellipseItem->setBrush(Qt::NoBrush);
	}
	m_shape = new Kolf::EllipseShape(QRectF());
	addShape(m_shape);
}

bool EllipticalCanvasItem::contains(const QPointF& point) const
{
	const QSizeF halfSize = size() / 2;
	const qreal xScaled = point.x() / halfSize.width();
	const qreal yScaled = point.y() / halfSize.height();
	return xScaled * xScaled + yScaled * yScaled < 1;
}

QPainterPath EllipticalCanvasItem::shape() const
{
	QPainterPath path;
	path.addEllipse(rect());
	return path;
}

QRectF EllipticalCanvasItem::rect() const
{
	return Tagaro::SpriteObjectItem::boundingRect();
}

void EllipticalCanvasItem::setSize(const QSizeF& size)
{
	setOffset(QPointF(-0.5 * size.width(), -0.5 * size.height()));
	Tagaro::SpriteObjectItem::setSize(size);
	if (m_ellipseItem)
		m_ellipseItem->setRect(this->rect());
	m_shape->setRect(this->rect());
}

void EllipticalCanvasItem::moveBy(double dx, double dy)
{
	Tagaro::SpriteObjectItem::moveBy(dx, dy);
	CanvasItem::moveBy(dx, dy);
}

void EllipticalCanvasItem::saveSize(KConfigGroup* group)
{
	const QSizeF size = this->size();
	group->writeEntry("width", size.width());
	group->writeEntry("height", size.height());
}

void EllipticalCanvasItem::loadSize(KConfigGroup* group)
{
	QSizeF size = this->size();
	size.rwidth() = group->readEntry("width", size.width());
	size.rheight() = group->readEntry("height", size.height());
	setSize(size);
}

//END EllipticalCanvasItem
//BEGIN ArrowItem

ArrowItem::ArrowItem(QGraphicsItem* parent)
	: QGraphicsPathItem(parent)
	, m_angle(0), m_length(20)
	, m_reversed(false)
{
	updatePath();
	setPen(QPen(Qt::black));
	setBrush(Qt::NoBrush);
}

qreal ArrowItem::angle() const
{
	return m_angle;
}

void ArrowItem::setAngle(qreal angle)
{
	if (m_angle != angle)
	{
		m_angle = angle;
		updatePath();
	}
}

qreal ArrowItem::length() const
{
	return m_length;
}

void ArrowItem::setLength(qreal length)
{
	if (m_length != length)
	{
		m_length = qMax<qreal>(length, 0.0);
		updatePath();
	}
}

bool ArrowItem::isReversed() const
{
	return m_reversed;
}

void ArrowItem::setReversed(bool reversed)
{
	if (m_reversed != reversed)
	{
		m_reversed = reversed;
		updatePath();
	}
}

Vector ArrowItem::vector() const
{
	return Vector::fromMagnitudeDirection(m_length, m_angle);
}

void ArrowItem::updatePath()
{
	if (m_length == 0)
	{
		setPath(QPainterPath());
		return;
	}
	//the following three points define the arrow tip
	const QPointF extent = Vector::fromMagnitudeDirection(m_length, m_angle);
	const QPointF startPoint = m_reversed ? extent : QPointF();
	const QPointF endPoint = m_reversed ? QPointF() : extent;
	const QPointF point1 = endPoint - Vector::fromMagnitudeDirection(m_length / 2, m_angle + M_PI / 12);
	const QPointF point2 = endPoint - Vector::fromMagnitudeDirection(m_length / 2, m_angle - M_PI / 12);
	QPainterPath path;
	path.addPolygon(QPolygonF() << startPoint << endPoint);
	path.addPolygon(QPolygonF() << point1 << endPoint << point2);
	setPath(path);
}

//END ArrowItem
