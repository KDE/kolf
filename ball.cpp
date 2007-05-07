#include "ball.h"

#include <QGraphicsView>
#include <QColor>
#include <QPen>
#include <QApplication>

#include <kdebug.h>
#include "game.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <krandom.h>

#include "rtti.h"
#include "vector.h"
#include "canvasitem.h"

Ball::Ball(QGraphicsScene * scene)
	: QGraphicsEllipseItem(0, scene)
{
	baseDiameter = 8;
	resizeFactor = 1;
	setData(0, Rtti_Ball);
	m_doDetect = true;
	m_collisionLock = false;
	setBeginningOfHole(false);
	setBlowUp(false);
	setPen(QPen(Qt::black));
	resetSize();
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;
	m_forceStillGoing = false;
	ignoreBallCollisions = false;
	frictionMultiplier = 1.0;

	QFont font(QApplication::font());
	baseFontPixelSize=12;
	font.setPixelSize((int)(baseFontPixelSize));
	label = new QGraphicsSimpleTextItem("", this, scene);
	label->setFont(font);
	label->setBrush(Qt::white);
	label->setPos(5, 5);
	label->setVisible(false);
	pixmapInitialised=false; //it can't be initialised yet because when a ball is first created it has no game (and therefore no renderer to create the pixmap)

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

void Ball::resetSize()
{
	setRect(baseDiameter*-0.5, baseDiameter*-0.5, baseDiameter, baseDiameter);
}

void Ball::resize(double resizeFactor)
{
	this->resizeFactor = resizeFactor;
	QFont font = label->font();
	font.setPixelSize((int)(baseFontPixelSize*resizeFactor));
	label->setFont(font);
	setPos(baseX, baseY); //not multiplied by resizeFactor since setPos takes care of that for the ball
	setRect(-0.5*baseDiameter*resizeFactor, -0.5*baseDiameter*resizeFactor, baseDiameter*resizeFactor, baseDiameter*resizeFactor);	
	pixmap=game->renderer->renderSvg("ball", (int)rect().width(), (int)rect().height(), 0);
}

void Ball::setPos(qreal x, qreal y)
{
	//for Ball (and only Ball) setPos() itself has been modified to take into account resizing
	//for a procedure that does not automaticaly take into account resizing use setResizedPos()
	setPos(QPointF(x, y));
}

void Ball::setPos(QPointF pos)
{
	//for Ball (and only Ball setPos) itself has been modified to take into account resizing
	//for a procedure that does not automaticaly take into account resizing use setResizedPos
	baseX = pos.x();
	baseY = pos.y();
	QGraphicsEllipseItem::setPos(pos.x()*resizeFactor, pos.y()*resizeFactor);
}

void Ball::setResizedPos(qreal x, qreal y)
{
	//unlike Ball::setPos this does not automatically take into account resizing but instead sets the ball's position to exactly that which is inputted in x and y 
	baseX = x/resizeFactor;
	baseY = y/resizeFactor;
	QGraphicsEllipseItem::setPos(x, y);
}


void Ball::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/ ) 
{
	if(pixmapInitialised == 0) {
		if(game == 0)
			return;
		else {
			pixmap=game->renderer->renderSvg("ball", (int)rect().width(), (int)rect().height(), 0);
			pixmapInitialised=true;
		}
	}
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

void Ball::advance(int /*phase*/)
{
	// not used anymore
	// can be used to make ball wobble
	/*if (phase == 1 && m_blowUp)
	{
		if (blowUpCount >= 50)
		{
			// i should make this a config option
			//setAddStroke(addStroke() + 1);
			setBlowUp(false);
			resetSize();
			return;
		}

		const double diff = 8;
		double randnum = KRandom::random();
		const double width = 6 + randnum * (diff / RAND_MAX);
		randnum = KRandom::random();
		const double height = 6 + randnum * (diff / RAND_MAX);
		setRect(rect().x(), rect().y(), width, height);
		blowUpCount++;
	}*/
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible()) { setVelocity(0, 0); return; }
	const double subtractAmount = .027 * frictionMultiplier;
	if (m_vector.magnitude() <= subtractAmount)
	{
		state = Stopped;
		setVelocity(0, 0);
		game->timeout();
		return;
	}
	m_vector.setMagnitude(m_vector.magnitude() - subtractAmount);
	setVector(m_vector);

	frictionMultiplier = 1.0;
}

void Ball::setVelocity(double vx, double vy)
{
	CanvasItem::setVelocity(vx, vy);

	if (vx == 0 && vy == 0)
	{
		m_vector.setDirection(0);
		m_vector.setMagnitude(0);
		return;
	}

	double ballAngle = atan2(-vy, vx);

	m_vector.setDirection(ballAngle);
	m_vector.setMagnitude(sqrt(pow(vx, 2) + pow(vy, 2)));
}

void Ball::setVector(const Vector &newVector)
{
	m_vector = newVector;

	if (newVector.magnitude() == 0)
	{
		setVelocity(0, 0);
		return;
	}

	CanvasItem::setVelocity(cos(newVector.direction()) * newVector.magnitude(), -sin(newVector.direction()) * newVector.magnitude());
}

void Ball::moveBy(double baseDx, double baseDy)
{
	//this takes as an imput the distance to move in the game's base 400x400 co-ordinate system, so that everything that calls this (friction etc) does not have to worry about the resized co-ordinates
	//NOTE: only Ball::moveBy does this, none of the moving procedures for other items in the game do this. This is inconsistent and likely to cause confusion and future bugs, sorry :(
	double dx = baseDx*resizeFactor;
	double dy = baseDy*resizeFactor;
	double oldx;
	double oldy;
	oldx = x();
	oldy = y();
	QGraphicsEllipseItem::moveBy(dx, dy);
	baseX += baseDx;
	baseY += baseDy;

	if (game && !game->isPaused())
		collisionDetect(oldx, oldy);
		
	if ((dx || dy) && game && game->curBall() == this)
		game->ballMoved();
}

void Ball::doAdvance()
{
	if(getXVelocity()!=0 || getYVelocity()!=0) 
		moveBy(getXVelocity(), getYVelocity());
}

namespace Lines
{
	// provides a point made of doubles

	struct Line
	{
		Point p1, p2;
	};

	int ccw(const Point &p0, const Point &p1, const Point &p2)
	{
		double dx1, dx2, dy1, dy2;
		dx1 = p1.x - p0.x; dy1 = p1.y - p0.y;
		dx2 = p2.x - p0.x; dy2 = p2.y - p0.y;
		if (dx1*dy2 > dy1*dx2) return +1;
		if (dx1*dy2 < dy1*dx2) return -1;
		if ((dx1*dx2 < 0) || (dy1*dy2 < 0)) return -1;
		if ((dx1*dx1+dy1*dy1) < (dx2*dx2+dy2*dy2))
			return +1;
		return 0;
	}

	int intersects(const Line &l1, const Line &l2)
	{
		// Charles says, TODO: Account for vertical lines
		// Jason says, in my testing vertical lines work
		return ((ccw(l1.p1, l1.p2, l2.p1)
				*ccw(l1.p1, l1.p2, l2.p2)) <= 0)
				&& ((ccw(l2.p1, l2.p2, l1.p1)
				*ccw(l2.p1, l2.p2, l1.p2)) <= 0);
	}

	
	bool intersects(
			double xa1, double ya1, double xb1, double yb1,
			double xa2, double ya2, double xb2, double yb2
		)
	{
		Line l1, l2;
		l1.p1.x = xa1;
		l1.p1.y = ya1;
		l1.p2.x = xb1;
		l1.p2.y = yb1;
		
		l2.p1.x = xa2;
		l2.p1.y = ya2;
		l2.p2.x = xb2;
		l2.p2.y = yb2;

		return intersects(l1, l2);
	}
}

void Ball::collisionDetect(double oldx, double oldy)
{
	if (!isVisible() || state == Holed || !m_doDetect)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	//kDebug(12007) << "------" << endl;
	//kDebug(12007) << "Ball::collisionDetect id " << collisionId << endl;

	// every other time...
	// do friction
	if (collisionId % 2 && !(getXVelocity() == 0 && getXVelocity() == 0))
		friction();

	double initialVector = m_vector.magnitude();
	const double minSpeed = .06;

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
						setResizedPos(x() - cos(ballAngle) / 2.0, y() + sin(ballAngle) / 2.0);

					// make a 2 pixel separation
					setResizedPos(x() - 2 * cos(ballAngle), y() + 2 * sin(ballAngle));

					Vector bvector = oball->curVector();
					m_vector -= bvector;

					Vector unit1 = Vector(QPointF(x(), y()), QPointF(oball->x(), oball->y()));
					unit1 = unit1.unit();

					Vector unit2 = m_vector.unit();

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
		else if (item->data(0) == Rtti_WallPoint)
		{
			//kDebug(12007) << "collided with WallPoint\n";
			// iterate through the rst
			QList<WallPoint *> points;
			for (QList<QGraphicsItem *>::Iterator pit = it; pit != m_list.end(); ++pit)
			{
				if ((*pit)->data(0) == Rtti_WallPoint)
				{
					WallPoint *point = (WallPoint *)(*pit);
					if (point)
						points.prepend(point);
				}
			}

			// ok now we have a list of wall points we are on

			QList<WallPoint *>::const_iterator iterpoint;
			QList<WallPoint *>::const_iterator finalPoint;

			// this wont be least when we're done hopefully
			double leastAngleDifference = 9999;

			for (iterpoint = points.constBegin(); iterpoint != points.constEnd(); ++iterpoint)
			{
				//kDebug(12007) << "-----\n";
				const Wall *parentWall = (*iterpoint)->parentWall();
				const QPointF p(((*iterpoint)->x() + parentWall->x()), ((*iterpoint)->y() + parentWall->y()));
				const QPointF other = QPointF(parentWall->startPoint() == p? parentWall->endPoint() : parentWall->startPoint()) + QPointF(parentWall->x(), parentWall->y());

				// vector of wall
				Vector v = Vector(p, other);

				// difference between our path and the wall path
				double ourDir = m_vector.direction();

				double wallDir = M_PI - v.direction();

				//kDebug(12007) << "ourDir: " << rad2deg(ourDir) << endl;
				//kDebug(12007) << "wallDir: " << rad2deg(wallDir) << endl;

				const double angleDifference = fabs(M_PI - fabs(ourDir - wallDir));
				//kDebug(12007) << "computed angleDifference: " << rad2deg(angleDifference) << endl;

				// only if this one is the least of all
				if (angleDifference < leastAngleDifference)
				{
					leastAngleDifference = angleDifference;
					finalPoint = iterpoint;
					//kDebug(12007) << "it's the one\n";
				}
			}

			// this'll never happen
			if (!(*finalPoint))
				continue;

			// collide with our chosen point
			(*finalPoint)->collision(this, collisionId);

			// don't worry about colliding with walls
			// wall points are ok alone
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
		QList<QGraphicsItem *> items;
		if (game)
			items = game->scene()->items();
		for (QList<QGraphicsItem *>::Iterator i = items.begin(); i != items.end(); ++i)
		{
			if ((*i)->data(0) != Rtti_Wall)
				continue;

			QGraphicsItem *item = (*i);
			Wall *wall = dynamic_cast<Wall*>(item);
			if (!wall || !wall->isVisible())
				continue;

			if (Lines::intersects(
					wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y(),
					wall->endPoint().x() + wall->x(),   wall->endPoint().y() + wall->y(),
				
					oldx, oldy, x(), y()
				))
			{
				//kDebug(12007) << "smart wall collision\n";
				wall->collision(this, collisionId);
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
		setVelocity(0, 0);
		setState(Stopped);
	}
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
	QGraphicsEllipseItem::setVisible(yes);

	label->setVisible(yes && game && game->isInfoShowing());
}

