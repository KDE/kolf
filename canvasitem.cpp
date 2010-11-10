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
#include "overlay.h"
#include "shape.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2World.h>

CanvasItem::CanvasItem(b2World* world)
	: game(0)
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
	//The overlay is deleted first, because it might interact with all other parts of the object.
	delete m_overlay;
	//NOTE: Box2D objects will need to be destroyed in the following order:
	//subobjects, shapes, own b2Body
	qDeleteAll(m_shapes);
	m_body->GetWorld()->DestroyBody(m_body);
}

QGraphicsRectItem *CanvasItem::onVStrut()
{
	QGraphicsItem *qthis = dynamic_cast<QGraphicsItem *>(this);
	if (!qthis) 
		return 0;
	QList<QGraphicsItem *> l = qthis->collidingItems();
	bool aboveVStrut = false;
	CanvasItem *item = 0;
	QGraphicsItem *qitem = 0;
	for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
	{
		item = dynamic_cast<CanvasItem *>(*it);
		if (item)
		{
			qitem = *it;
			if (item->vStrut())
			{
				//kDebug(12007) << "above vstrut\n";
				aboveVStrut = true;
				break;
			}
		}
	}

	QGraphicsRectItem *ritem = dynamic_cast<QGraphicsRectItem *>(qitem);

	return aboveVStrut && ritem? ritem : 0;
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

QPointF CanvasItem::physicalVelocity() const
{
	b2Vec2 v = m_body->GetLinearVelocity();
	return QPointF(v.x, v.y);
}

void CanvasItem::setPhysicalVelocity(const QPointF& newVelocity)
{
	const QPointF currentVelocity = this->physicalVelocity();
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
