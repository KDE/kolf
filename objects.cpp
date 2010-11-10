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
#include "rtti.h"
#include "tagaro/board.h"

#include <QFormLayout>
#include <QTimer>
#include <KConfigGroup>
#include <KNumInput>
#include <KRandom>

//BEGIN Kolf::BlackHole

Kolf::BlackHole::BlackHole(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QLatin1String("black_hole"), parent, world)
	, exitDeg(0)
{
	setSize(QSizeF(16, 18));
	setZValue(998.1);
	setSimulationType(CanvasItem::NoSimulation);

	infoLine = 0;
	m_minSpeed = 3.0;
	m_maxSpeed = 5.0;
	runs = 0;

	const QColor myColor((QRgb)(KRandom::random() % 0x01000000));
	ellipseItem()->setBrush(myColor);

	exitItem = new BlackHoleExit(this, Kolf::findBoard(this), world);
	exitItem->setPen(QPen(myColor, 6));
	exitItem->setPos(300, 100);

	moveBy(0, 0);

	finishMe();
}

void Kolf::BlackHole::setMinSpeed(double news)
{
	m_minSpeed = news; exitItem->updateArrowLength();
}

void Kolf::BlackHole::setMaxSpeed(double news)
{
	m_maxSpeed = news; exitItem->updateArrowLength();
}

void Kolf::BlackHole::showInfo()
{
	delete infoLine;
	infoLine = new QGraphicsLineItem(Kolf::findBoard(this));
	infoLine->setVisible(true);
	infoLine->setPen(QPen(exitItem->pen().color(), 2));
	infoLine->setZValue(10000);
	infoLine->setLine(QLineF(pos(), exitItem->pos()));

	exitItem->showInfo();
}

void Kolf::BlackHole::hideInfo()
{
	delete infoLine;
	infoLine = 0;

	exitItem->hideInfo();
}

void Kolf::BlackHole::editModeChanged(bool editing)
{
	exitItem->editModeChanged(editing);
}

Config* Kolf::BlackHole::config(QWidget* parent)
{
	return new Kolf::BlackHoleConfig(this, parent);
}

void Kolf::BlackHole::aboutToDie()
{
	//Hole::aboutToDie();
	exitItem->aboutToDie();
	delete exitItem;
}

void Kolf::BlackHole::updateInfo()
{
	if (infoLine)
	{
		infoLine->setVisible(true);
		infoLine->setLine(QLineF(pos(), exitItem->pos()));
		exitItem->showInfo();
	}
}

void Kolf::BlackHole::moveBy(double dx, double dy)
{
	EllipticalCanvasItem::moveBy(dx, dy);
	updateInfo();
}

void Kolf::BlackHole::setExitDeg(int newdeg)
{
	exitDeg = newdeg;
	if (game && game->isEditing() && game->curSelectedItem() == exitItem)
		game->updateHighlighter();

	exitItem->updateArrowAngle();
	finishMe();
}

QList<QGraphicsItem*> Kolf::BlackHole::moveableItems() const
{
	return QList<QGraphicsItem*>() << exitItem;
}

bool Kolf::BlackHole::collision(Ball* ball)
{
	//miss if speed too high
	const double speed = ball->velocity().magnitude();
	if (speed > 3.75)
		return true;
	// is center of ball in cup?
	if (!contains(ball->pos() - pos()))
		return true;
	// warp through blackhole at most 10 times per shot
	if (runs > 10 && game && game->isInPlay())
		return true;

	playSound("blackholeputin");

	const double diff = m_maxSpeed - m_minSpeed;
	const double newSpeed = m_minSpeed + speed / 3.75 * diff;

	ball->setVelocity(Vector());
	ball->setState(Stopped);
	ball->setVisible(false);
	ball->setForceStillGoing(true);

	const double distance = Vector(pos() - exitItem->pos()).magnitude();
	BlackHoleTimer* timer = new BlackHoleTimer(ball, newSpeed, distance * 2.5 - newSpeed * 35 + 500);

	connect(timer, SIGNAL(eject(Ball*, double)), this, SLOT(eject(Ball*, double)));
	connect(timer, SIGNAL(halfway()), this, SLOT(halfway()));

	playSound("blackhole");
	return false;
}

Kolf::BlackHoleTimer::BlackHoleTimer(Ball* ball, double speed, int msec)
	: m_speed(speed), m_ball(ball)
{
	QTimer::singleShot(msec, this, SLOT(mySlot()));
	QTimer::singleShot(msec / 2, this, SLOT(myMidSlot()));
}

void Kolf::BlackHoleTimer::mySlot()
{
	emit eject(m_ball, m_speed);
	delete this;
}

void Kolf::BlackHoleTimer::myMidSlot()
{
	emit halfway();
}

void Kolf::BlackHole::eject(Ball* ball, double speed)
{
	ball->setVisible(true);
	//place ball 10 units after exit, and set exit velocity
	const Vector direction = Vector::fromMagnitudeDirection(1, -deg2rad(exitDeg));
	ball->setPos(exitItem->pos() + 10 * direction);
	ball->setVelocity(speed * direction);

	ball->setForceStillGoing(false);
	ball->setState(Rolling);

	runs++;

	playSound("blackholeeject");
}

void Kolf::BlackHole::halfway()
{
	playSound("blackhole");
}

void Kolf::BlackHole::load(KConfigGroup* cfgGroup)
{
	QPoint exit = cfgGroup->readEntry("exit", exit);
	exitItem->setPos(exit.x(), exit.y());
	exitDeg = cfgGroup->readEntry("exitDeg", exitDeg);
	m_minSpeed = cfgGroup->readEntry("minspeed", m_minSpeed);
	m_maxSpeed = cfgGroup->readEntry("maxspeed", m_maxSpeed);
	exitItem->updateArrowAngle();
	exitItem->updateArrowLength();

	finishMe();
}

void Kolf::BlackHole::finishMe()
{
	const double width = 15; //width of exit line

	double radians = deg2rad(exitDeg);
	QPointF midPoint(0, 0);
	QPointF start;
	QPointF end;

	if (midPoint.y() || !midPoint.x())
	{
		start.setX(midPoint.x() - width*sin(radians));
		start.setY(midPoint.y() - width*cos(radians));
		end.setX(midPoint.x() + width*sin(radians));
		end.setY(midPoint.y() + width*cos(radians));
	}
	else
	{
		start.setX(midPoint.x());
		start.setY(midPoint.y() + width);
		end.setX(midPoint.y() - width);
		end.setY(midPoint.x());
	}

	exitItem->setLine(start.x(), start.y(), end.x(), end.y());
	exitItem->setVisible(true);
}

void Kolf::BlackHole::save(KConfigGroup* cfgGroup)
{
	cfgGroup->writeEntry("exit", exitItem->pos().toPoint());
	cfgGroup->writeEntry("exitDeg", exitDeg);
	cfgGroup->writeEntry("minspeed", m_minSpeed);
	cfgGroup->writeEntry("maxspeed", m_maxSpeed);
}

//END Kolf::BlackHole
//BEGIN Kolf::BlackHoleExit

Kolf::BlackHoleExit::BlackHoleExit(Kolf::BlackHole* blackHole, QGraphicsItem* parent, b2World* world)
	: QGraphicsLineItem(parent)
	, CanvasItem(world)
	, m_blackHole(blackHole)
{
	setData(0, Rtti_NoCollision);
	m_arrow = new ArrowItem(this);
	setZValue(m_blackHole->zValue());
	updateArrowLength();
	m_arrow->setVisible(false);
}

void Kolf::BlackHoleExit::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
	m_blackHole->updateInfo();
}

void Kolf::BlackHoleExit::setPen(const QPen& p)
{
	QGraphicsLineItem::setPen(p);
	m_arrow->setPen(QPen(p.color()));
}

void Kolf::BlackHoleExit::updateArrowAngle()
{
	// arrows work in a different angle system
	m_arrow->setAngle(-deg2rad(m_blackHole->curExitDeg()));
}

void Kolf::BlackHoleExit::updateArrowLength()
{
	m_arrow->setLength(10.0 + 5.0 * (double)(m_blackHole->minSpeed() + m_blackHole->maxSpeed()) / 2.0);
}

void Kolf::BlackHoleExit::editModeChanged(bool editing)
{
	m_arrow->setVisible(editing);
}

void Kolf::BlackHoleExit::showInfo()
{
	m_arrow->setVisible(true);
}

void Kolf::BlackHoleExit::hideInfo()
{
	m_arrow->setVisible(false);
}

Config* Kolf::BlackHoleExit::config(QWidget* parent)
{
	return m_blackHole->config(parent);
}

//END Kolf::BlackHoleExit
//BEGIN Kolf::BlackHoleConfig

Kolf::BlackHoleConfig::BlackHoleConfig(BlackHole* blackHole, QWidget* parent)
	: Config(parent)
	, m_blackHole(blackHole)
{
	QFormLayout* layout = new QFormLayout(this);

	KIntSpinBox* deg = new KIntSpinBox(this);
	deg->setRange(0, 359);
	deg->setSingleStep(10);
	deg->setSuffix(ki18np(" degree", " degrees"));
	deg->setValue(m_blackHole->curExitDeg());
	deg->setWrapping(true);
	layout->addRow(i18n("Exiting ball angle:"), deg);
	connect(deg, SIGNAL(valueChanged(int)), this, SLOT(degChanged(int)));

	KDoubleNumInput* min = new KDoubleNumInput(this);
	min->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	min->setRange(0, 8);
	min->setValue(m_blackHole->minSpeed());
	layout->addRow(i18n("Minimum exit speed:"), min);
	connect(min, SIGNAL(valueChanged(double)), this, SLOT(minChanged(double)));

	KDoubleNumInput* max = new KDoubleNumInput(this);
	max->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	max->setRange(0, 8);
	max->setValue(m_blackHole->maxSpeed());
	layout->addRow(i18n("Maximum exit speed:"), max);
	connect(max, SIGNAL(valueChanged(double)), this, SLOT(maxChanged(double)));
}

void Kolf::BlackHoleConfig::degChanged(int newdeg)
{
	m_blackHole->setExitDeg(newdeg);
	changed();
}

void Kolf::BlackHoleConfig::minChanged(double news)
{
	m_blackHole->setMinSpeed(news);
	changed();
}

void Kolf::BlackHoleConfig::maxChanged(double news)
{
	m_blackHole->setMaxSpeed(news);
	changed();
}

//END Kolf::BlackHoleConfig
//BEGIN Kolf::Cup

Kolf::Cup::Cup(QGraphicsItem* parent, b2World* world)
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

bool Kolf::Cup::collision(Ball* ball)
{
	//miss if speed too high
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

#include "objects.moc"
