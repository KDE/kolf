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

/////////////////////////

Floater::Floater(QGraphicsItem *parent)
	: Bridge(parent, "floater"), speedfactor(16)
{
	wall = 0;
	setEnabled(true);
	noUpdateZ = false;
	haventMoved = true;
	wall = new FloaterGuide(this, parent);
	wall->setPoints(100, 100, 200, 200);
	wall->setPen(QPen(wall->pen().color().light(), wall->pen().width() - 1));
	setPos(wall->endPoint().x(), wall->endPoint().y());

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(false);
	setRightWallVisible(false);

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

void Floater::advance(int phase)
{
	if (game && game->isEditing())
		return;

	if (!isEnabled())
		return;

	if (phase == 1 && !velocity().isNull())
	{
		doAdvance();
		if (Vector(origin - QPointF(x(), y())).magnitude() > vector.magnitude())
		{
			vector.setDirection(vector.direction() + M_PI);
			origin = (origin == wall->startPoint()? wall->endPoint() : wall->startPoint());

			setVelocity(-velocity());
		}
	}
}

void Floater::doAdvance()
{
	moveBy(velocity().x(), velocity().y());
}

void Floater::reset()
{
	QPointF start = wall->startPointF() + QPointF(wall->x(), wall->y());
	QPointF end = wall->endPointF() + QPointF(wall->x(), wall->y());

	vector = end - start;
	origin = end;

	setPos(origin);
	moveBy(0, 0);
	setSpeed(speed);
}

QList<QGraphicsItem *> Floater::moveableItems() const
{
	return wall->moveableItems() << wall << point;
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
		setVelocity(Vector());
		return;
	}

	setVelocity(-Vector::fromMagnitudeDirection(speed / 3.5, vector.direction()));
}

void Floater::aboutToSave()
{
	setVelocity(velocity());
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
					ball->moveBy(dx, dy);
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
	QGraphicsItem::moveBy(dx, dy);

	// because we don't do Bridge::moveBy();
	topWall->setPos(x(), y());
	botWall->setPos(x(), y() - 1);
	leftWall->setPos(x(), y());
	rightWall->setPos(x(), y());

	if (game && game->isEditing())
		game->updateHighlighter();
}

void Floater::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("speed", speed);
	cfgGroup->writeEntry("startPoint", (wall->startPointF() + wall->pos()).toPoint());
	cfgGroup->writeEntry("endPoint", (wall->endPointF() + wall->pos()).toPoint());

	doSave(cfgGroup);
}

void Floater::load(KConfigGroup *cfgGroup)
{
	setPos(firstPoint.x(), firstPoint.y());

	QPointF start = wall->startPointF() + wall->pos();
	start = cfgGroup->readEntry("startPoint", start);
	QPointF end = wall->endPointF() + wall->pos();
	end = cfgGroup->readEntry("endPoint", end);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->setPos(0, 0);

	setSpeed(cfgGroup->readEntry("speed", -1));

	doLoad(cfgGroup);
	reset();
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
