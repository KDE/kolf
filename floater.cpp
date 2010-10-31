/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

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

#include "floater.h"
#include "rtti.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

void FloaterGuide::aboutToDelete()
{
	game->removeItem(floater);
	aboutToDie();
	floater->aboutToDie();
	delete floater;
	almostDead = true;
}

void FloaterGuide::aboutToDie()
{
	if (almostDead)
		return;
	else
		Wall::aboutToDie();
}

void FloaterGuide::moveBy(double dx, double dy)
{
	Wall::moveBy(dx, dy);

	if (floater)
		floater->reset();
}

void FloaterGuide::setPoints(double xa, double ya, double xb, double yb)
{
	baseX1 = xa / resizeFactor;
	baseY1 = ya / resizeFactor;
	baseX2 = xb / resizeFactor;
	baseY2 = yb / resizeFactor;

	if (qAbs(xa - xb) > 0 || qAbs(ya - yb) > 0)
	{
		Wall::setPoints(xa, ya, xb, yb);
		if (floater)
			floater->reset();
	}
}

Config *FloaterGuide::config(QWidget *parent)
{
	return floater->config(parent);
}

void FloaterGuide::resize(double resizeFactor)
{
	this->resizeFactor = resizeFactor;
	setPoints(baseX1*resizeFactor, baseY1*resizeFactor, baseX2*resizeFactor, baseY2*resizeFactor);
}

void FloaterGuide::updateBaseResizeInfo()
{
	baseX1 = line().x1() / resizeFactor;
	baseY1 = line().y1() / resizeFactor;
	baseX2 = line().x2() / resizeFactor;
	baseY2 = line().y2() / resizeFactor;
	floater->updateBaseResizeInfo();
}

/////////////////////////

Floater::Floater(QGraphicsItem *parent, QGraphicsScene *scene)
	: Bridge(parent, scene, "floater"), speedfactor(16)
{
	wall = 0;
	setEnabled(true);
	noUpdateZ = false;
	haventMoved = true;
	resizeFactor = 1;
	wall = new FloaterGuide(this, parent, scene);
	wall->setPoints(100, 100, 200, 200);
	wall->setPen(QPen(wall->pen().color().light(), wall->pen().width() - 1));
	setPos(wall->endPoint().x(), wall->endPoint().y());

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(false);
	setRightWallVisible(false);

	newSize(width(), height());
	moveBy(0, 0);
	setSpeed(0);

	editModeChanged(false);
	reset();
}

void Floater::setGame(KolfGame *game)
{
	Bridge::setGame(game);

	wall->setGame(game);
}

void Floater::editModeChanged(bool changed)
{
	if (changed)
		wall->editModeChanged(true);
	Bridge::editModeChanged(changed);
	wall->setVisible(changed);
}

void Floater::resize(double resizeFactor)
{
	this->resizeFactor = resizeFactor;
	setSpeed(speed); //update speed taking into account new resizeFactor
	wall->resize( resizeFactor );
	Bridge::resize(resizeFactor);
}

void Floater::advance(int phase)
{
	if (game && game->isEditing())
		return;

	if (!isEnabled())
		return;

	if (phase == 1 && (getXVelocity() || getXVelocity()))
	{
		doAdvance();
		if (Vector(origin - QPointF(x(), y())).magnitude() > vector.magnitude())
		{
			vector.setDirection(vector.direction() + M_PI);
			origin = (origin == wall->startPoint()? wall->endPoint() : wall->startPoint());

			setVelocity(-getXVelocity(), -getYVelocity());
		}
	}
}

void Floater::doAdvance()
{
	moveBy(getXVelocity(), getYVelocity());
}

void Floater::reset()
{
	QPointF startf = wall->startPoint() + QPointF(wall->x(), wall->y());
	QPointF endf = wall->endPoint() + QPointF(wall->x(), wall->y());
	QPoint start = (QPoint((int)startf.x(), (int)startf.y()));
	QPoint end = (QPoint((int)endf.x(), (int)endf.y()));

	vector = end - start;
	origin = end;

	setPos(origin.x(), origin.y());
	moveBy(0, 0);
	setSpeed(speed);
}

QList<QGraphicsItem *> Floater::moveableItems() const
{
	QList<QGraphicsItem *> ret(wall->moveableItems());
	ret.append(wall);
	ret.append(point);
	return ret;
}

void Floater::aboutToDie()
{
	if (wall)
		wall->setVisible(false);
	Bridge::aboutToDie();
	setEnabled(false);
}

void Floater::setSpeed(int news)
{
	if (!wall || news < 0)
		return;
	speed = news;

	if (news == 0)
	{
		setVelocity(0, 0);
		return;
	}

	const double factor = (double)speed / 3.5;
	setVelocity(-cos(vector.direction()) * factor * resizeFactor, -sin(vector.direction()) * factor * resizeFactor);
}

void Floater::aboutToSave()
{
	setVelocity(0, 0);
	noUpdateZ = true;
	setPos(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
	noUpdateZ = false;
}

void Floater::savingDone()
{
	setSpeed(speed);
}

void Floater::moveBy(double dx, double dy)
{
	if (!isEnabled())
		return;

	QList<QGraphicsItem *> l = collidingItems();
	for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
	{
		CanvasItem *item = dynamic_cast<CanvasItem *>(*it);

		if (!noUpdateZ && item && item->canBeMovedByOthers())
			item->updateZ(this);

		if ((*it)->zValue() >= zValue())
		{
			if (item && item->canBeMovedByOthers() && collidesWithItem(*it))
			{
				if ((*it)->data(0) == Rtti_Ball)
				{
					//((Ball *)(*it))->setState(Rolling);
					Ball *ball = dynamic_cast<Ball *>(*it);
                                        Q_ASSERT(ball);
					ball->moveByResizedDistance(dx, dy);
					if (game && /*game->hasFocus() &&*/ !game->isEditing() && game->curBall() == (Ball *)(*it))
							game->ballMoved();
				}
				else if ((*it)->data(0) != Rtti_Putter) {
					item->moveBy(dx, dy);
				}
			}
		}
	}

	point->dontMove();
	point->setPos(x() + width(), y() + height());

	// this call must come after we have tested for collidingItems, otherwise we skip them when saving!
	// that's a bad thing
	QGraphicsRectItem::moveBy(dx, dy);

	// because we don't do Bridge::moveBy();
	topWall->setPos(x(), y());
	botWall->setPos(x(), y() - 1);
	leftWall->setPos(x(), y());
	rightWall->setPos(x(), y());

	if (game && game->isEditing())
		game->updateHighlighter();
}

void Floater::saveState(StateDB *db)
{
	db->setPoint(QPointF(x()/resizeFactor, y()/resizeFactor));
}

void Floater::loadState(StateDB *db)
{
	const QPointF moveTo = db->point();
	moveBy(moveTo.x()*resizeFactor - x(), moveTo.y()*resizeFactor - y());
}

void Floater::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("speed", speed);
	cfgGroup->writeEntry("startPoint", QPoint((int)(wall->startPoint().x() + wall->x()), (int)(wall->startPoint().y() + wall->y())));
	cfgGroup->writeEntry("endPoint", QPoint((int)(wall->endPoint().x() + wall->x()), (int)(wall->endPoint().y() + wall->y())));

	doSave(cfgGroup);
}

void Floater::load(KConfigGroup *cfgGroup)
{
	setPos(firstPoint.x(), firstPoint.y());

	QPointF start(wall->startPoint() + QPointF(wall->x(), wall->y()));
	start = cfgGroup->readEntry("startPoint", start);
	QPointF end(wall->endPoint() + QPointF(wall->x(), wall->y()));
	end = cfgGroup->readEntry("endPoint", end);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->setPos(0, 0);

	setSpeed(cfgGroup->readEntry("speed", -1));

	doLoad(cfgGroup);
	reset();
}

void Floater::firstMove(int x, int y)
{
	firstPoint = QPoint(x, y);
}

void Floater::updateBaseResizeInfo()
{
	Bridge::updateBaseResizeInfo();
}

/////////////////////////

FloaterConfig::FloaterConfig(Floater *floater, QWidget *parent)
	: BridgeConfig(floater, parent)
{
	this->floater = floater;
	m_vlayout->addStretch();

	m_vlayout->addWidget(new QLabel(i18n("Moving speed"), this));
	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        m_vlayout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange( 0, 20 );
        slider->setPageStep( 2 );
        slider->setValue( floater->curSpeed() );
	hlayout->addWidget(slider);
	hlayout->addWidget(new QLabel(i18n("Fast"), this));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(speedChanged(int)));
}

void FloaterConfig::speedChanged(int news)
{
	floater->setSpeed(news);
	changed();
}

#include "floater.moc"
