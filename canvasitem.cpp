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

#include "canvasitem.h"
#include "game.h"

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

//BEGIN EllipticalCanvasItem

EllipticalCanvasItem::EllipticalCanvasItem(bool withEllipse, const QString& spriteKey, QGraphicsItem* parent)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), spriteKey, parent)
	, m_ellipseItem(0)
{
	if (withEllipse)
	{
		m_ellipseItem = new QGraphicsEllipseItem(this);
		m_ellipseItem->setFlag(QGraphicsItem::ItemStacksBehindParent);
		//won't appear unless pen/brush is configured
		m_ellipseItem->setPen(Qt::NoPen);
		m_ellipseItem->setBrush(Qt::NoBrush);
	}
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
