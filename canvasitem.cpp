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

