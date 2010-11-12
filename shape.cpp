/*
    Copyright 2008-2010 Stefan Majewsky <majewsky@gmx.net>

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

#include "shape.h"
#include "canvasitem.h"
#include "overlay.h"

#include <QtCore/qmath.h>
#include <QtCore/QVarLengthArray>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2EdgeShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2Fixture.h>

static inline b2Vec2 toB2Vec2(const QPointF& p)
{
	return b2Vec2(p.x(), p.y());
}

//BEGIN Kolf::Shape

const qreal Kolf::Shape::ActivationOutlinePadding = 5;

Kolf::Shape::Shape()
	: m_traits(Kolf::Shape::ParticipatesInPhysicalSimulation)
	, m_citem(0)
	, m_body(0)
	, m_fixtureDef(new b2FixtureDef)
	, m_fixture(0)
	, m_shape(0)
{
	m_fixtureDef->density = 1;
	m_fixtureDef->restitution = 1;
	m_fixtureDef->friction = 0;
	m_fixtureDef->userData = this;
}

Kolf::Shape::~Shape()
{
	delete m_fixtureDef;
	updateFixture(0); //clear fixture and shape
}

QPainterPath Kolf::Shape::activationOutline() const
{
	return m_activationOutline;
}

QPainterPath Kolf::Shape::interactionOutline() const
{
	return m_interactionOutline;
}

bool Kolf::Shape::attach(CanvasItem* item)
{
	if (m_citem)
		return false;
	m_citem = item;
	m_body = item->m_body;
	updateFixture(createShape());
	return true;
}

Kolf::Shape::Traits Kolf::Shape::traits() const
{
	return m_traits;
}

void Kolf::Shape::setTraits(Kolf::Shape::Traits traits)
{
	if (m_traits == traits)
		return;
	m_traits = traits;
	updateFixture(createShape());
}

void Kolf::Shape::updateFixture(b2Shape* newShape)
{
	if (!m_body)
		return;
	//destroy old fixture
	if (m_fixture)
		m_body->DestroyFixture(m_fixture);
	delete m_shape;
	m_shape = 0;
	//create new fixture
	if (m_traits & Kolf::Shape::CollisionDetectionFlag)
	{
		m_shape = newShape;
		if (m_shape)
		{
			b2FixtureDef fixtureDef = *m_fixtureDef;
			fixtureDef.shape = m_shape;
			fixtureDef.isSensor = !(m_traits & Kolf::Shape::PhysicalSimulationFlag);
			m_fixture = m_body->CreateFixture(&fixtureDef);
		}
	}
	else
		delete newShape; //TODO: inefficient
}

void Kolf::Shape::update()
{
	updateFixture(createShape());
	m_interactionOutline = m_activationOutline = QPainterPath();
	createOutlines(m_activationOutline, m_interactionOutline);
	//propagate update to overlays
	if (m_citem)
	{
		Kolf::Overlay* overlay = m_citem->overlay(false);
		if (overlay) //may be 0 if the overlay has not yet been instantiated
			overlay->update();
	}
}

//END Kolf::Shape
//BEGIN Kolf::EllipseShape

Kolf::EllipseShape::EllipseShape(const QRectF& rect)
	: m_rect(rect)
{
	update();
}

QRectF Kolf::EllipseShape::rect() const
{
	return m_rect;
}

void Kolf::EllipseShape::setRect(const QRectF& rect)
{
	if (m_rect != rect)
	{
		m_rect = rect;
		update();
	}
}

b2Shape* Kolf::EllipseShape::createShape()
{
	const b2Vec2 c = toB2Vec2(m_rect.center() * Kolf::Box2DScaleFactor);
	const qreal rx = m_rect.width() * Kolf::Box2DScaleFactor / 2;
	const qreal ry = m_rect.height() * Kolf::Box2DScaleFactor / 2;
	if (rx == ry)
	{
		//use circle shape when possible because it's cheaper and exact
		b2CircleShape* shape = new b2CircleShape;
		shape->m_p = c;
		shape->m_radius = rx;
		return shape;
	}
	else
	{
		//elliptical shape is not pre-made in Box2D, so create a polygon instead
		b2PolygonShape* shape = new b2PolygonShape;
		static const int N = qMin(20, b2_maxPolygonVertices);
		//increase N if the approximation turns out to be too bad
		//TODO: calculate the (cos, sin) pairs only once
		QVarLengthArray<b2Vec2, 20> vertices(N);
		static const qreal angleStep = 2 * M_PI / N;
		for (int i = 0; i < N; ++i)
		{
			const qreal angle = -i * angleStep; //CCW order as required by Box2D
			vertices[i].x = c.x + rx * cos(angle);
			vertices[i].y = c.y + ry * sin(angle);
		}
		shape->Set(vertices.data(), N);
		return shape;
	}
}

void Kolf::EllipseShape::createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline)
{
	interactionOutline.addEllipse(m_rect);
	const qreal& p = Kolf::Shape::ActivationOutlinePadding;
	activationOutline.addEllipse(m_rect.adjusted(-p, -p, p, p));
}

//END Kolf::EllipseShape
//BEGIN Kolf::RectShape

Kolf::RectShape::RectShape(const QRectF& rect)
	: m_rect(rect)
{
	update();
}

QRectF Kolf::RectShape::rect() const
{
	return m_rect;
}

void Kolf::RectShape::setRect(const QRectF& rect)
{
	if (m_rect != rect)
	{
		m_rect = rect;
		update();
	}
}

b2Shape* Kolf::RectShape::createShape()
{
	b2PolygonShape* shape = new b2PolygonShape;
	shape->SetAsBox(
		m_rect.width() * Kolf::Box2DScaleFactor / 2,
		m_rect.height() * Kolf::Box2DScaleFactor / 2,
		toB2Vec2(m_rect.center() * Kolf::Box2DScaleFactor),
		0 //intrinsic rotation angle
	);
	return shape;
}

void Kolf::RectShape::createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline)
{
	interactionOutline.addRect(m_rect);
	const qreal& p = Kolf::Shape::ActivationOutlinePadding;
	activationOutline.addRect(m_rect.adjusted(-p, -p, p, p));
}

//END Kolf::RectShape
//BEGIN Kolf::LineShape

Kolf::LineShape::LineShape(const QLineF& line)
	: m_line(line)
{
	update();
}

QLineF Kolf::LineShape::line() const
{
	return m_line;
}

void Kolf::LineShape::setLine(const QLineF& line)
{
	if (m_line != line)
	{
		m_line = line;
		update();
	}
}

b2Shape* Kolf::LineShape::createShape()
{
	b2EdgeShape* shape = new b2EdgeShape;
	shape->Set(
		toB2Vec2(m_line.p1() * Kolf::Box2DScaleFactor),
		toB2Vec2(m_line.p2() * Kolf::Box2DScaleFactor)
	);
	return shape;
}

void Kolf::LineShape::createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline)
{
	const QPointF extent = m_line.p2() - m_line.p1();
	const qreal angle = atan2(extent.y(), extent.x());
	const qreal paddingAngle = angle + M_PI / 2;
	const qreal padding = Kolf::Shape::ActivationOutlinePadding;
	const QPointF paddingVector(padding * cos(paddingAngle), padding * sin(paddingAngle));
	//interaction outline: a rectangle that is aligned with the wall
	interactionOutline.moveTo(m_line.p1() + paddingVector);
	interactionOutline.lineTo(m_line.p1() - paddingVector);
	interactionOutline.lineTo(m_line.p2() - paddingVector);
	interactionOutline.lineTo(m_line.p2() + paddingVector);
	interactionOutline.closeSubpath();
	//activation outline: the same rectangle with additional half-circles at the ends
	activationOutline = interactionOutline;
	activationOutline.addEllipse(m_line.p1(), padding, padding);
	activationOutline.addEllipse(m_line.p2(), padding, padding);
}

//END Kolf::LineShape
