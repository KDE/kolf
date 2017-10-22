/***************************************************************************
 *   Copyright 2010 Stefan Majewsky <majewsky@gmx.net>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "utils-animateditem.h"

#include <QPropertyAnimation>

Utils::AnimatedItem::AnimatedItem(QGraphicsItem* parent)
	: QGraphicsObject(parent)
	, m_animationTime(200)
	, m_hideWhenInvisible(false)
	, m_animator(new QPropertyAnimation(this, "correct_opacity", this))
{
}

int Utils::AnimatedItem::animationTime() const
{
	return m_animationTime;
}

void Utils::AnimatedItem::setAnimationTime(int time)
{
	m_animationTime = time;
}

bool Utils::AnimatedItem::hideWhenInvisible() const
{
	return m_hideWhenInvisible;
}

void Utils::AnimatedItem::setHideWhenInvisible(bool hideWhenInvisible)
{
	m_hideWhenInvisible = hideWhenInvisible;
}

void Utils::AnimatedItem::setOpacityAnimated(qreal targetOpacity)
{
	qreal currentOpacity = opacity();
	m_animator->setDuration(m_animationTime);
	m_animator->setStartValue(currentOpacity);
	m_animator->setEndValue(targetOpacity);
	m_animator->start();
}

void Utils::AnimatedItem::setOpacity(qreal opacity)
{
	QGraphicsItem::setOpacity(opacity);
	if (m_hideWhenInvisible)
		QGraphicsItem::setVisible(!qFuzzyIsNull(opacity));
}

//empty QGraphicsItem reimplementation

QRectF Utils::AnimatedItem::boundingRect() const
{
	return QRectF();
}

void Utils::AnimatedItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}


