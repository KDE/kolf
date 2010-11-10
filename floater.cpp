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

void FloaterGuide::setLine(const QLineF& line)
{
	if (line.p1() != line.p2())
	{
		Wall::setLine(line);
		if (floater)
			floater->reset();
	}
}

Config *FloaterGuide::config(QWidget *parent)
{
	return floater->config(parent);
}

/////////////////////////

Floater::Floater(QGraphicsItem *parent, b2World* world)
	: Bridge(parent, world, "floater"), speedfactor(16)
{
	wall = 0;
	setEnabled(true);
	noUpdateZ = false;
	haventMoved = true;
	wall = new FloaterGuide(this, parent, world);
	wall->setLine(QLineF(100, 100, 200, 200));
	wall->setPen(QPen(wall->pen().color().light(), wall->pen().width() - 1));
	setPos(wall->line().p2());

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
			const QLineF line = wall->line();
			origin = (origin == line.p1() ? line.p2() : line.p1());

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
	const QLineF motionLine = wall->line().translated(wall->pos());

	vector = motionLine.p2() - motionLine.p1();
	origin = motionLine.p2();

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
	setPos(wall->line().p2() + wall->pos());
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
				Ball *ball = dynamic_cast<Ball *>(*it);
				if (ball)
				{
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
	const QLineF motionLine = wall->line().translated(wall->pos());
	cfgGroup->writeEntry("startPoint", motionLine.p1().toPoint());
	cfgGroup->writeEntry("endPoint", motionLine.p2().toPoint());

	doSave(cfgGroup);
}

void Floater::load(KConfigGroup *cfgGroup)
{
	setPos(firstPoint.x(), firstPoint.y());

	QLineF motionLine = wall->line().translated(wall->pos());
	motionLine.setP1(cfgGroup->readEntry("startPoint", motionLine.p1()));
	motionLine.setP2(cfgGroup->readEntry("endPoint", motionLine.p2()));
	wall->setLine(motionLine);
	wall->setPos(QPointF());

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
