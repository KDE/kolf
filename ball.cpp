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

#include "ball.h"
#include "game.h"
#include "rtti.h"

#include <QApplication>

Ball::Ball(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QLatin1String("ball"), parent, world)
{
	const int diameter = 8;
	setSize(QSizeF(diameter, diameter));

	setData(0, Rtti_Ball);
	m_doDetect = true;
	m_collisionLock = false;
	setBeginningOfHole(false);
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
	m_forceStillGoing = false;
	ignoreBallCollisions = false;
	frictionMultiplier = 1.0;

	QFont font(QApplication::font());
	font.setPixelSize(12);
	label = new QGraphicsSimpleTextItem(QString(), this);
	label->setFont(font);
	label->setBrush(Qt::white);
	label->setPos(5, 5);
	label->setVisible(false);

	// this sets z
	setState(Stopped);
	label->setZValue(zValue() - .1);
}

void Ball::aboutToDie()
{
	delete label;
}

void Ball::setState(BallState newState)
{
	state = newState;
	if (state == Stopped)
		setZValue(1000);
	else
		setBeginningOfHole(false);
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible())
	{
		setVelocity(Vector());
		return;
	}
	const double subtractAmount = .027 * frictionMultiplier;
	if (m_vector.magnitude() <= subtractAmount)
	{
		state = Stopped;
		setVelocity(Vector());
		game->timeout();
		return;
	}
	m_vector.setMagnitude(m_vector.magnitude() - subtractAmount);
	setVector(m_vector);

	frictionMultiplier = 1.0;
}

void Ball::setVelocity(const Vector& velocity)
{
	CanvasItem::setVelocity(velocity);
	m_vector = QPointF(velocity.x(), -velocity.y());
}

void Ball::setVector(const Vector& newVector)
{
	m_vector = newVector;
	CanvasItem::setVelocity(Vector(newVector.x(), -newVector.y()));
}

void Ball::moveBy(double dx, double dy)
{
	const QPointF oldPos = pos();
	EllipticalCanvasItem::moveBy(dx, dy);

	if (game && !game->isPaused())
		collisionDetect(oldPos.x(), oldPos.y());
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::doAdvance()
{
	if (!velocity().isNull())
		moveBy(velocity().x(), velocity().y());
}

void Ball::collisionDetect(double oldx, double oldy)
{
	QGraphicsEllipseItem* halo = NULL;

	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	//kDebug(12007) << "------";
	//kDebug(12007) << "Ball::collisionDetect id" << collisionId;

	// every other time...
	// do friction
	if (collisionId % 2 && !velocity().isNull())
		friction();

	double initialVector = m_vector.magnitude();
	const double minSpeed = .06;
	bool justCollidedWithWall = false;

	QList<QGraphicsItem *> m_list = collidingItems();
	bool collidingWithABall=0;

	this->m_list = m_list;

	for (QList<QGraphicsItem *>::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		QGraphicsItem *item = *it;

		if (item->data(0) == Rtti_NoCollision || item->data(0) == Rtti_Putter)
			continue;

		if (item->data(0) == data(0) && !m_collisionLock)
		{
			// it's one of our own kind, a ball
			collidingWithABall=1;
			if(ignoreBallCollisions)
				continue;

			Ball *oball = dynamic_cast<Ball *>(item);
			if (!oball || oball->collisionLock())
				continue;
			oball->setCollisionLock(true);

			if (oball->curState() != Holed) 
			{
				if (oball->x() - x() == 0 && oball->y() - y() == 0 && state != Rolling) 
				{
					//both balls have same position, therefore other ball must have spawned on top of this one (possibly after going into water on the first shot, and then being reset)
					ignoreBallCollisions=1;
				}
				else
				{
					m_collisionLock = true;
					// move this ball to where it was barely touching
					double ballAngle = m_vector.direction();
					while (collidingItems().contains(item) > 0)
						setPos(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);

					// make a 2 pixel separation
					setPos(x() - 2 * cos(ballAngle), y() + 2 * sin(ballAngle));

					Vector bvector = oball->curVector();
					m_vector -= bvector;

					Vector unit1(x() - oball->x(), y() - oball->y());
					unit1 = unit1.unitVector();

					Vector unit2 = m_vector.unitVector();

					double cos = unit1 * unit2;

					unit1 *= m_vector.magnitude() * cos;
					m_vector -= unit1;
					m_vector += bvector;

					bvector += unit1;

					oball->setVector(bvector);
					setVector(m_vector);

					oball->setState(Rolling);
					setState(Rolling);

					oball->doAdvance();
				}
			}

			continue;
		}
		else if (item->data(0) == Rtti_WallPoint || item->data(0) == Rtti_Wall )
		{
			static int tempLastId = collisionId - 25;
			double ballVectorMagnitude = m_vector.magnitude();
			int allowableDifference = 1;
			if (ballVectorMagnitude < .30)
				allowableDifference = 8;
			else if (ballVectorMagnitude < .50)
				allowableDifference = 6;
			else if (ballVectorMagnitude < .75)
				allowableDifference = 4;
			else if (ballVectorMagnitude < .95)
				allowableDifference = 2;

			if (abs(collisionId - tempLastId) <= allowableDifference)
			{
				//kDebug(12007) << "Clever wall and wall point collision detection is skipping. AllowableDifference is:" << abs(collisionId - tempLastId);
				goto end; //skip this AND smart wall collision
			}

			//Create the halo. This is an ellipse centered around the ball, and bigger than it. This allows us to detect walls which we could be about to collide into, and react intelligently to them, even though we are not colliding with them quite yet
			const double haloSizeFactor = 2;

			halo = new QGraphicsEllipseItem( QRectF(pos() * haloSizeFactor, size() * haloSizeFactor), this );
			halo->hide();

			QList<QGraphicsItem *> haloCollisions = halo->collidingItems();
			QList< Wall* > haloWallCollisions;

			for( QList<QGraphicsItem *>::Iterator hIter = haloCollisions.begin(); hIter != haloCollisions.end(); ++hIter )
			{
				if ((*hIter)->data(0) == Rtti_Wall)
				{
					Wall* wall = dynamic_cast< Wall* >(*hIter);
					if( wall )
					{
						haloWallCollisions.push_back( wall );
					}
				}
			}

			if( haloWallCollisions.size() == 0 )
			{
				tempLastId = collisionId;
				//not found any walls to collide off, so I must be colliding with a wall point already, will collide off that instead
				WallPoint* wp = dynamic_cast< WallPoint* >(item);
				if( wp )
				{
					wp->collision(this, collisionId);
					justCollidedWithWall = true;
				}
				else
				{
					//this should not happen
					break;
				}
			}
			else if( haloWallCollisions.size() == 1 )
			{
				tempLastId = collisionId;
				Wall* w = dynamic_cast< Wall* >(haloWallCollisions[0]);
				if( w )
				{
					w->collision(this, collisionId);
					justCollidedWithWall = true;
					goto end;
				}
				else
				{
					//this should not happen
					break;
				}
			}
			else //haloWallCollisions.size() >= 2
			{
				tempLastId = collisionId;
				collideWithHaloCollisions( haloWallCollisions );
				justCollidedWithWall = true;
			}

			goto end;
		}
	
		if (!isVisible() || state == Holed)
			return;

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			if (!citem->terrainCollisions())
			{
				// read: if (not do terrain collidingItems)
				if (!citem->collision(this, collisionId))
				{
					// if (skip smart wall test)
					if (citem->vStrut() || item->data(0) == Rtti_Wall)
						goto end;
					else
						goto wallCheck;
				}
			}
			break;
		}
	}

	if(ignoreBallCollisions && !collidingWithABall) 
		ignoreBallCollisions=0; //bad ball collision must be over now  since we are no longer colliding with a ball, so stop ignoring ball collisions

	for (QList<QGraphicsItem *>::Iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem && citem->terrainCollisions())
		{
			// slopes return false
			// as only one should be processed
			// however that might not always be true

			// read: if (not do terrain collidingItems)
			if (!citem->collision(this, collisionId))
			{
				break;
			}
		}
	}

// Charles's smart wall check:
	
	wallCheck:

	{ // check if I went through a wall
		QList<QGraphicsItem *> items = parentItem()->children();
		for (QList<QGraphicsItem *>::Iterator i = items.begin(); i != items.end(); ++i)
		{
			if ((*i)->data(0) != Rtti_Wall)
				continue;

			QGraphicsItem *item = (*i);
			Wall *wall = dynamic_cast<Wall*>(item);
			if (!wall || !wall->isVisible())
				continue;

			const QLineF wallLine(
				wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y(),
				wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y()
			);
			const QLineF localTrajectory(oldx, oldy, x(), y());
			if (wallLine.intersect(localTrajectory, 0) == QLineF::BoundedIntersection)
			{
				//kDebug(12007) << "smart wall collision\n";
				wall->collision(this, collisionId);
				justCollidedWithWall = true;
				break;
			}
		}
	}

	end:


	double vectorChange = initialVector - m_vector.magnitude();
	if(vectorChange < 0 ) 
		vectorChange *= -1;

	if(m_vector.magnitude() < minSpeed && vectorChange < minSpeed && m_vector.magnitude())
	{
		if( justCollidedWithWall )
		{ //don't want to stop if just hit a wall as may be in the wall
			//problem: could this cause endless ball bouncing between 2 walls?
			m_vector.setMagnitude(minSpeed);
		}
		else
		{
			setVelocity(Vector());
			setState(Stopped);
		}
	}
	delete halo;
}

//this rather ugly bit of code takes list of walls that the ball is very close to. It then looks at their angles relative to the ball and works out which two are closest to the ball in each direction (clockwise and anti-clockwise)
//it then works out the average angle of those two walls and collides with them as if that were the only wall here.
void Ball::collideWithHaloCollisions( QList< Wall* >& haloWallCollisions )
{
	double ballAngle = -m_vector.direction();

	double closestNegativeAngleDiff = -181;
	double furthestNegativeAngleDiff = 1;
	double closestPositiveAngleDiff = 181;
	double furthestPositiveAngleDiff = -1;

	for ( QList< Wall* >::iterator Iter = haloWallCollisions.begin(); Iter != haloWallCollisions.end(); ++Iter )
	{
		//find which point on the wall is closest to the ball and get the wall's vector accordingly (need this to get the right angle of the wall relative to the ball)
		//kDebug(12007) << "------";
		QPointF p1 = (*Iter)->startPoint();
		QPointF p2 = (*Iter)->endPoint();
		QPointF ballPos = QPointF( x(), y() );

		int dist1 = abs((int)( ballPos.x() - p1.x() )) + abs((int)( ballPos.y() - p1.y() ));
		int dist2 = abs((int)( ballPos.x() - p2.x() )) + abs((int)( ballPos.y() - p2.y() ));

		double wallAngle;
		if( dist1 < dist2 )
		{
			wallAngle = Vector(p1 - p2).direction();
		}
		else
		{
			wallAngle = Vector(p2 - p1).direction();
		}

		//kDebug(12007) << "ballAngle:" << rad2deg(ballAngle);
		//kDebug(12007) << "wallAngle:" << rad2deg(wallAngle);

		double angleDiff = rad2deg(ballAngle) - rad2deg(wallAngle);
		while( angleDiff > 180 ) 
		{
			angleDiff -= 360;
		}
		while( angleDiff < -180 )
		{
			angleDiff += 360;
		}
		//kDebug(12007) << "computed angleDifference:" << angleDiff;

		if( angleDiff > 0 )
		{
			if( angleDiff < closestPositiveAngleDiff )
			{
				closestPositiveAngleDiff = angleDiff;
			}
			if( angleDiff > furthestPositiveAngleDiff )
			{
				furthestPositiveAngleDiff = angleDiff;
			}
		}
		else
		{
			if( angleDiff > closestNegativeAngleDiff )
			{
				closestNegativeAngleDiff = angleDiff;
			}
			if( angleDiff < furthestNegativeAngleDiff )
			{
				furthestNegativeAngleDiff = angleDiff;
			}
		}
	}

	//if all the walls have a negative angle difference or a positive angle difference then we want to collide with the two walls at each extreme of the angel difference type we do have
	float closestNegativeAngle, closestPositiveAngle;
	if( closestNegativeAngleDiff == -181 )
	{
		closestNegativeAngle = rad2deg(ballAngle) - furthestPositiveAngleDiff;
	}
	else
	{
		closestNegativeAngle = rad2deg(ballAngle) - closestPositiveAngleDiff;
	}

	if( closestPositiveAngleDiff == 181 )
	{
		closestPositiveAngle = rad2deg(ballAngle) - furthestNegativeAngleDiff;
	}
	else
	{
		closestPositiveAngle = rad2deg(ballAngle) - closestNegativeAngleDiff;
	}

	//kDebug(12007) << "closest pos:" << closestPositiveAngle << "closest neg:" << closestNegativeAngle;

	double averageAngleOfTwoClosestWalls = ( closestPositiveAngle + closestNegativeAngle ) / 2;

	//kDebug(12007) << "average angle:" << averageAngleOfTwoClosestWalls;

	ballAngle = m_vector.direction();
	//kDebug(12007) << "old ball angle:" << rad2deg(ballAngle);

	double newBallAngle = M_PI + ballAngle  - 2 * ( ballAngle + deg2rad( averageAngleOfTwoClosestWalls ) );

	//kDebug(12007) << "new ball angle:" << rad2deg(newBallAngle);

	Vector ballVector(curVector());
	const double dampening = 1.2;
	ballVector /= dampening;
	ballVector.setDirection(newBallAngle);
	setVector(ballVector);

	playSound("wall", curVector().magnitude() / 10.0);
}

BallState Ball::currentState()
{
	return state;
}

void Ball::showInfo()
{
	label->setVisible(isVisible());
}

void Ball::hideInfo()
{
	label->setVisible(false);
}

void Ball::setName(const QString &name)
{
	label->setText(name);
}

void Ball::setVisible(bool yes)
{
	EllipticalCanvasItem::setVisible(yes);

	label->setVisible(yes && game && game->isInfoShowing());
}
