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

#include "objects.h"
#include "ball.h"
#include "overlay.h"

//BEGIN Kolf::Cup

Kolf::Cup::Cup(QGraphicsItem *parent, b2World* world)
	: EllipticalCanvasItem(false, QLatin1String("cup"), parent, world)
{
	const int diameter = 16;
	setSize(QSizeF(diameter, diameter));
	setSimulationType(CanvasItem::NoSimulation);
	setZValue(998.1);
}

Kolf::Overlay* Kolf::Cup::createOverlay()
{
	return new Kolf::Overlay(this, this);
}

bool Kolf::Cup::collision(Ball *ball)
{
	//miss if speed to high
	const double speed = ball->velocity().magnitude();
	if (speed > 3.75)
		return true;
	//miss if center of ball not inside cup
	if (!contains(ball->pos() - pos()))
		return true;
	//place ball in hole
	ball->setState(Holed);
	playSound("holed");
	ball->setPos(pos());
	ball->setVelocity(Vector());
	return false;
}

//END Kolf::Cup
