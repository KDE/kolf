/***************************************************************************
 *   Copyright 2008, 2009, 2010 Stefan Majewsky <majewsky@gmx.net>
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

#include "overlay.h"
#include "canvasitem.h"
#include "game.h"
#include "shape.h"
#include "utils-animateditem.h"

#include <QGraphicsSceneEvent>
#include <QPen>
#include <QtMath>

const qreal Kolf::Overlay::MinimumObjectDimension = 10;

static const qreal OverlayHandleSize = 5;
static const QPen HandlePen(Qt::blue);
static const QBrush ActiveHandleBrush(QColor::fromHsv(240, 200, 255, 128));
static const QBrush NormalHandleBrush(QColor::fromHsv(240, 255, 255, 128));

//BEGIN Kolf::OverlayHandle

Kolf::OverlayHandle::OverlayHandle(Kolf::OverlayHandle::Shape shape, QGraphicsItem* parent)
	: QGraphicsPathItem(parent)
{
	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	setPen(HandlePen);
	setBrush(NormalHandleBrush);
	//create shape
	QPainterPath path;
	switch (shape)
	{
		case SquareShape:
			path.addRect(QRectF(-OverlayHandleSize, -OverlayHandleSize, 2 * OverlayHandleSize, 2 * OverlayHandleSize));
			break;
		case CircleShape:
			path.addEllipse(QPointF(), OverlayHandleSize, OverlayHandleSize);
			break;
		case TriangleShape:
			path.moveTo(QPointF(OverlayHandleSize, 0.0));
			path.lineTo(QPointF(-OverlayHandleSize, OverlayHandleSize));
			path.lineTo(QPointF(-OverlayHandleSize, -OverlayHandleSize));
			path.closeSubpath();
			break;
	}
	setPath(path);
}

void Kolf::OverlayHandle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	event->accept();
	setBrush(ActiveHandleBrush);
}

void Kolf::OverlayHandle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	event->accept();
	setBrush(NormalHandleBrush);
}

void Kolf::OverlayHandle::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		event->accept();
		Q_EMIT moveStarted();
	}
}

void Kolf::OverlayHandle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		event->accept();
		Q_EMIT moveRequest(event->scenePos());
	}
}

void Kolf::OverlayHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		event->accept();
		Q_EMIT moveEnded();
	}
}

//END Kolf::OverlayHandle
//BEGIN Kolf::OverlayAreaItem

Kolf::OverlayAreaItem::OverlayAreaItem(Features features, QGraphicsItem* parent)
	: QGraphicsPathItem(parent)
	, m_features(features)
{
	if (m_features & (Clickable | Draggable))
		setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
	if (m_features & Draggable)
		setCursor(Qt::OpenHandCursor);
	if (m_features & Hoverable)
		setAcceptHoverEvents(true);
	//start invisible
	setPen(Qt::NoPen);
	setBrush(Qt::transparent);
}

void Kolf::OverlayAreaItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	if (m_features & Hoverable)
	{
		event->accept();
		Q_EMIT hoverEntered();
	}
	else
		QGraphicsPathItem::hoverEnterEvent(event);
}

void Kolf::OverlayAreaItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	if (m_features & Hoverable)
	{
		event->accept();
		Q_EMIT hoverLeft();
	}
	else
		QGraphicsPathItem::hoverLeaveEvent(event);
}

void Kolf::OverlayAreaItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (m_features & Clickable)
	{
		event->accept();
		Q_EMIT clicked(event->button());
	}
	if (m_features & Draggable)
	{
		event->accept();
		setCursor(Qt::ClosedHandCursor);
	}
	if (!event->isAccepted())
		QGraphicsPathItem::mousePressEvent(event);
}

void Kolf::OverlayAreaItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if (m_features & Draggable)
		Q_EMIT dragged(event->scenePos() - event->lastScenePos());
	else
		QGraphicsItem::mouseMoveEvent(event);
}

void Kolf::OverlayAreaItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (m_features & Draggable)
		setCursor(Qt::OpenHandCursor);
	else
		QGraphicsItem::mouseReleaseEvent(event);
}

//END Kolf::OverlayAreaItem
//BEGIN Kolf::Overlay

Kolf::Overlay::Overlay(CanvasItem* citem, QGraphicsItem* qitem, bool hack_addQitemShapeToOutlines)
	: QGraphicsItem(qitem)
	, m_citem(citem), m_qitem(qitem)
	, m_state(Kolf::Overlay::Passive)
	, m_addQitemShapeToOutlines(hack_addQitemShapeToOutlines)
	, m_activatorItem(new Kolf::OverlayAreaItem(Kolf::OverlayAreaItem::Clickable | Kolf::OverlayAreaItem::Hoverable, this))
	, m_interactorAnimator(new Utils::AnimatedItem(this))
	, m_interactorItem(new Kolf::OverlayAreaItem(Kolf::OverlayAreaItem::Clickable | Kolf::OverlayAreaItem::Draggable, m_interactorAnimator))
	, m_handleAnimator(new Utils::AnimatedItem(this))
{
	//overlays have to be shown explicitly
	hide();
	//overlays themselves do not have mouse interaction, and do not paint anything itself
	setAcceptedMouseButtons(Qt::NoButton);
	setFlag(QGraphicsItem::ItemHasNoContents);
	//initialize activator area item
	m_activatorItem->setZValue(1);
	connect(m_activatorItem, &Kolf::OverlayAreaItem::hoverEntered, this, &Overlay::activatorEntered);
	connect(m_activatorItem, &Kolf::OverlayAreaItem::hoverLeft, this, &Overlay::activatorLeft);
	connect(m_activatorItem, &Kolf::OverlayAreaItem::clicked, this, &Overlay::activatorClicked);
	//initialize interactor area item
	m_interactorAnimator->setZValue(2);
	m_interactorAnimator->setOpacity(0); //not visible at first
	m_interactorItem->setBrush(Qt::green);
	connect(m_interactorItem, &Kolf::OverlayAreaItem::clicked, this, &Overlay::activatorClicked);
	connect(m_interactorItem, &Kolf::OverlayAreaItem::dragged, this, &Overlay::interactorDragged);
	//initialize handle manager
	m_handleAnimator->setZValue(3);
	m_handleAnimator->setHideWhenInvisible(true);
	m_handleAnimator->setOpacity(0); //not visible at first
	//apply passive state - we need to change to some other state to prevent the setState method from returning early
	m_state = Active;
	setState(Passive);
}

CanvasItem* Kolf::Overlay::citem() const
{
	return m_citem;
}

QGraphicsItem* Kolf::Overlay::qitem() const
{
	return m_qitem;
}

void Kolf::Overlay::update()
{
	//update geometry outlines
	QPainterPath activationOutline, interactionOutline;
	foreach (Kolf::Shape* shape, m_citem->shapes())
	{
		activationOutline.addPath(shape->activationOutline());
		interactionOutline.addPath(shape->interactionOutline());
	}
	//HACK for Kolf::Shape
	if (m_addQitemShapeToOutlines)
	{
		const QPainterPath shape = m_qitem->shape();
		activationOutline.addPath(shape);
		interactionOutline.addPath(shape);
	}
	activationOutline.setFillRule(Qt::WindingFill);
	interactionOutline.setFillRule(Qt::WindingFill);
	m_activatorItem->setPath(activationOutline);
	m_interactorItem->setPath(interactionOutline);
}

void Kolf::Overlay::addHandle(QGraphicsItem* handle)
{
	handle->setParentItem(m_handleAnimator);
	handle->show();
}

Kolf::Overlay::State Kolf::Overlay::state() const
{
	return m_state;
}

void Kolf::Overlay::setState(Kolf::Overlay::State state)
{
	if (m_state == state)
		return;
	m_state = state;
	//apply new inner properties
	switch (state)
	{
		case Kolf::Overlay::Passive:
			m_interactorAnimator->setOpacityAnimated(0.0);
			break;
		case Kolf::Overlay::Hovered:
			m_interactorAnimator->setOpacityAnimated(0.3);
			break;
		case Kolf::Overlay::Active:
			m_interactorAnimator->setOpacityAnimated(0.6);
			break;
	}
	m_handleAnimator->setOpacityAnimated(state == Active ? 1.0 : 0.0);
	//propagate changes
	Q_EMIT stateChanged();
	if (state == Kolf::Overlay::Active)
	{
		KolfGame* game = m_citem->game;
		if (game)
			game->setSelectedItem(m_citem);
	}
}

void Kolf::Overlay::activatorEntered()
{
	if (m_state == Kolf::Overlay::Passive)
		setState(Kolf::Overlay::Hovered);
}

void Kolf::Overlay::activatorLeft()
{
	if (m_state == Kolf::Overlay::Hovered)
		setState(Kolf::Overlay::Passive);
}

void Kolf::Overlay::activatorClicked(int button)
{
	Q_UNUSED(button)
	setState(Kolf::Overlay::Active);
}

void Kolf::Overlay::interactorDragged(const QPointF& distance)
{
	if (m_state == Kolf::Overlay::Active)
		m_citem->moveBy(distance.x(), distance.y());
}

//default implementation for QGraphicsItem

QRectF Kolf::Overlay::boundingRect() const
{
	return QRectF();
}

void Kolf::Overlay::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}

//END Kolf::Overlay


