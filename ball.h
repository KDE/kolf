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

#ifndef KOLF_BALL_H
#define KOLF_BALL_H

#include "canvasitem.h"

class Wall;

enum BallState { Rolling = 0, Stopped, Holed };

class Ball : public EllipticalCanvasItem
{
public:
	Ball(QGraphicsItem* parent, b2World* world);
	virtual void aboutToDie();

	BallState currentState();

	virtual void doAdvance();
	virtual void moveBy(double dx, double dy);
	virtual void setVelocity(const Vector& velocity);

	virtual bool deleteable() const { return false; }

	virtual bool canBeMovedByOthers() const { return true; }

	BallState curState() const { return state; }
	void setState(BallState newState);

	QColor color() const { return ellipseItem()->brush().color(); }
	void setColor(const QColor& color) { ellipseItem()->setBrush(color); }

	void setMoved(bool yes) { m_moved = yes; }
	bool moved() const { return m_moved; }

	void setFrictionMultiplier(double news) { frictionMultiplier = news; }
	void friction();
	void collisionDetect(double oldx, double oldy);
	void collideWithHaloCollisions( QList< Wall* >& haloWallCollisions );

	int addStroke() const { return m_addStroke; }
	bool placeOnGround(Vector &v) { v = oldVector; return m_placeOnGround; }
	void setAddStroke(int newStrokes) { m_addStroke = newStrokes; }
	void setPlaceOnGround(bool placeOnGround) { m_placeOnGround = placeOnGround; oldVector = m_vector; }

	bool beginningOfHole() const { return m_beginningOfHole; }
	void setBeginningOfHole(bool yes) { m_beginningOfHole = yes; }

	bool forceStillGoing() const { return m_forceStillGoing; }
	void setForceStillGoing(bool yes) { m_forceStillGoing = yes; }

	Vector curVector() const { return m_vector; }
	void setVector(const Vector &newVector);

	bool collisionLock() const { return m_collisionLock; }
	void setCollisionLock(bool yes) { m_collisionLock = yes; }

	void shotStarted() { maxBumperBounceSpeed = 8; }

	void setDoDetect(bool yes) { m_doDetect = yes; }
	bool doDetect() const { return m_doDetect; }

	virtual void showInfo();
	virtual void hideInfo();
	virtual void setName(const QString &);
	virtual void setVisible(bool yes);

	double getMaxBumperBounceSpeed() { return maxBumperBounceSpeed; }
	void reduceMaxBumperBounceSpeed() { if(maxBumperBounceSpeed > 0.4) maxBumperBounceSpeed -= 0.35; }

public slots:
	void update() { doAdvance(); }

private:
	BallState state;
	QColor m_color;

	long int collisionId;
	double frictionMultiplier;

	//the maximum speed of the ball after hitting a bumper, this will decrease ith each bounce so that the ball does not bounce against bumpers forever
	double maxBumperBounceSpeed;

	int m_addStroke;
	bool m_placeOnGround;
	double m_oldvx;
	double m_oldvy;

	bool m_moved;
	bool m_beginningOfHole;
	bool m_forceStillGoing;

	bool ignoreBallCollisions;

	Vector m_vector;
	Vector oldVector;
	bool m_collisionLock;

	bool m_doDetect;
	QList<QGraphicsItem *> m_list;

	QGraphicsSimpleTextItem *label;
};

#endif
