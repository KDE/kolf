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

#ifndef KOLF_OBJECTS_H
#define KOLF_OBJECTS_H

#include "canvasitem.h"

namespace Kolf
{
	class Cup : public EllipticalCanvasItem
	{
		public:
			Cup(QGraphicsItem *parent, b2World* world);

			virtual Kolf::Overlay* createOverlay();
			virtual bool canBeMovedByOthers() const { return true; }
			virtual bool collision(Ball *ball);
	};
}

#endif // KOLF_OBJECTS_H
