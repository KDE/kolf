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

//BEGIN Kolf::Shape

const qreal Kolf::Shape::ActivationOutlinePadding = 5;

Kolf::Shape::Shape()
	: m_citem(0)
{
}

Kolf::Shape::~Shape()
{
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
	return true;
}

void Kolf::Shape::update()
{
	m_interactionOutline = m_activationOutline = QPainterPath();
	createOutlines(m_activationOutline, m_interactionOutline);
	if (m_citem)
	{
		Kolf::Overlay* overlay = m_citem->overlay(false);
		if (overlay) //may be 0 if the overlay has not yet been instantiated
			overlay->update();
	}
}

//END Kolf::Shape
//BEGIN Kolf::CircleShape

Kolf::CircleShape::CircleShape(qreal radius, const QPointF& center)
	: m_center(center)
	, m_radius(radius)
{
	update();
}

QPointF Kolf::CircleShape::center() const
{
	return m_center;
}

void Kolf::CircleShape::setCenter(const QPointF& center)
{
	if (m_center != center)
	{
		m_center = center;
		update();
	}
}

qreal Kolf::CircleShape::radius() const
{
	return m_radius;
}

void Kolf::CircleShape::setRadius(qreal radius)
{
	if (m_radius != radius)
	{
		m_radius = radius;
		update();
	}
}

void Kolf::CircleShape::createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline)
{
	const qreal activationRadius = m_radius + Kolf::Shape::ActivationOutlinePadding;
	interactionOutline.addEllipse(m_center, m_radius, m_radius);
	activationOutline.addEllipse(m_center, activationRadius, activationRadius);
}

//END Kolf::CircleShape
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
