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
#include "game.h"
#include "tagaro/board.h"

#include <QFormLayout>
#include <QTimer>
#include <KConfigGroup>
#include <KNumInput>
#include <KRandom>

//BEGIN Kolf::BlackHole

Kolf::BlackHole::BlackHole(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QLatin1String("black_hole"), parent, world)
	, m_minSpeed(3.0)
	, m_maxSpeed(5.0)
	, m_runs(0)
	, m_exitDeg(0)
	, m_exitItem(new QGraphicsLineItem(0, -15, 0, 15, Kolf::findBoard(this)))
	, m_directionItem(new ArrowItem(Kolf::findBoard(this)))
	, m_infoLine(new QGraphicsLineItem(this))
{
	setSize(QSizeF(16, 18));
	setZValue(998.1);
	setSimulationType(CanvasItem::NoSimulation);

	const QColor myColor((QRgb)(KRandom::random() % 0x01000000));
	ellipseItem()->setBrush(myColor);
	m_exitItem->setPen(QPen(myColor, 6));
	m_directionItem->setPen(myColor);
	m_infoLine->setPen(QPen(myColor, 2));

	setExitPos(QPointF(300, 100));
	m_directionItem->setVisible(false);
	m_infoLine->setVisible(false);
	moveBy(0, 0); //initializes line item
}

Kolf::BlackHole::~BlackHole()
{
	//these items are not direct childs
	delete m_directionItem;
	delete m_exitItem;
}

double Kolf::BlackHole::minSpeed() const
{
	return m_minSpeed;
}

void Kolf::BlackHole::setMinSpeed(double news)
{
	m_minSpeed = news;
	m_directionItem->setLength(10.0 + 2.5 * (m_minSpeed + m_maxSpeed));
	propagateUpdate();
}

double Kolf::BlackHole::maxSpeed() const
{
	return m_maxSpeed;
}

void Kolf::BlackHole::setMaxSpeed(double news)
{
	m_maxSpeed = news;
	m_directionItem->setLength(10.0 + 2.5 * (m_minSpeed + m_maxSpeed));
	propagateUpdate();
}

QList<QGraphicsItem*> Kolf::BlackHole::infoItems() const
{
	return QList<QGraphicsItem*>() << m_infoLine << m_directionItem;
}

Kolf::Overlay* Kolf::BlackHole::createOverlay()
{
	return new Kolf::BlackHoleOverlay(this);
}

Config* Kolf::BlackHole::config(QWidget* parent)
{
	return new Kolf::BlackHoleConfig(this, parent);
}

void Kolf::BlackHole::moveBy(double dx, double dy)
{
	EllipticalCanvasItem::moveBy(dx, dy);
	m_infoLine->setLine(QLineF(QPointF(), m_exitItem->pos() - pos()));
	propagateUpdate();
}

int Kolf::BlackHole::curExitDeg() const
{
	return m_exitDeg;
}

void Kolf::BlackHole::setExitDeg(int newdeg)
{
	m_exitDeg = newdeg;
	m_exitItem->setRotation(-newdeg);
	m_directionItem->setAngle(-deg2rad(newdeg));
	propagateUpdate();
}

QPointF Kolf::BlackHole::exitPos() const
{
	return m_exitItem->pos();
}

void Kolf::BlackHole::setExitPos(const QPointF& exitPos)
{
	m_exitItem->setPos(exitPos);
	m_directionItem->setPos(exitPos);
	moveBy(0, 0); //updates line item, and calls propagateUpdate()
}

Vector Kolf::BlackHole::exitDirection() const
{
	return m_directionItem->vector();
}

void Kolf::BlackHole::shotStarted()
{
	m_runs = 0;
}

bool Kolf::BlackHole::collision(Ball* ball)
{
	//miss if speed too high
	const double speed = Vector(ball->velocity()).magnitude();
	if (speed > 3.75)
		return true;
	// is center of ball in cup?
	if (!contains(ball->pos() - pos()))
		return true;
	// warp through blackhole at most 10 times per shot
	if (m_runs > 10 && game && game->isInPlay())
		return true;

	playSound("blackholeputin");

	const double diff = m_maxSpeed - m_minSpeed;
	const double newSpeed = m_minSpeed + speed / 3.75 * diff;

	ball->setVelocity(Vector());
	ball->setState(Stopped);
	ball->setVisible(false);
	ball->setForceStillGoing(true);

	const double distance = Vector(pos() - m_exitItem->pos()).magnitude();
	BlackHoleTimer* timer = new BlackHoleTimer(ball, newSpeed, distance * 2.5 - newSpeed * 35 + 500);

	connect(timer, SIGNAL(eject(Ball*, double)), this, SLOT(eject(Ball*, double)));
	connect(timer, SIGNAL(halfway()), this, SLOT(halfway()));

	playSound("blackhole");
	return false;
}

Kolf::BlackHoleTimer::BlackHoleTimer(Ball* ball, double speed, int msec)
	: m_speed(speed), m_ball(ball)
{
	QTimer::singleShot(msec, this, SLOT(emitEject()));
	QTimer::singleShot(msec / 2, this, SIGNAL(halfway()));
}

void Kolf::BlackHoleTimer::emitEject()
{
	emit eject(m_ball, m_speed);
	deleteLater();
}

void Kolf::BlackHole::eject(Ball* ball, double speed)
{
	ball->setVisible(true);
	//place ball 10 units after exit, and set exit velocity
	const Vector direction = Vector::fromMagnitudeDirection(1, -deg2rad(m_exitDeg));
	ball->setPos(m_exitItem->pos() + 10 * direction);
	ball->setVelocity(speed * direction);

	ball->setForceStillGoing(false);
	ball->setState(Rolling);

	m_runs++;

	playSound("blackholeeject");
}

void Kolf::BlackHole::halfway()
{
	playSound("blackhole");
}

void Kolf::BlackHole::load(KConfigGroup* cfgGroup)
{
	setExitPos(cfgGroup->readEntry("exit", exitPos().toPoint()));
	setExitDeg(cfgGroup->readEntry("exitDeg", m_exitDeg));
	setMinSpeed(cfgGroup->readEntry("minspeed", m_minSpeed));
	setMaxSpeed(cfgGroup->readEntry("maxspeed", m_maxSpeed));
}

void Kolf::BlackHole::save(KConfigGroup* cfgGroup)
{
	cfgGroup->writeEntry("exit", m_exitItem->pos().toPoint());
	cfgGroup->writeEntry("exitDeg", m_exitDeg);
	cfgGroup->writeEntry("minspeed", m_minSpeed);
	cfgGroup->writeEntry("maxspeed", m_maxSpeed);
}

//END Kolf::BlackHole
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
//BEGIN Kolf::BlackHoleOverlay

Kolf::BlackHoleOverlay::BlackHoleOverlay(Kolf::BlackHole* blackHole)
	: Kolf::Overlay(blackHole, blackHole)
	, m_exitIndicator(new QGraphicsLineItem(this))
	, m_exitHandle(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
	, m_speedHandle(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
{
	addHandle(m_exitIndicator);
	addHandle(m_exitHandle);
	addHandle(m_speedHandle);
	connect(m_exitHandle, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
	connect(m_speedHandle, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
}

void Kolf::BlackHoleOverlay::update()
{
	Kolf::Overlay::update();
	Kolf::BlackHole* blackHole = dynamic_cast<Kolf::BlackHole*>(qitem());
	m_exitHandle->setPos(blackHole->exitPos() - blackHole->pos());
	m_speedHandle->setPos(m_exitHandle->pos() + blackHole->exitDirection());
	m_exitIndicator->setLine(QLineF(m_exitHandle->pos(), m_speedHandle->pos()));
	const qreal exitAngle = -deg2rad(blackHole->curExitDeg());
	m_exitHandle->setRotation(exitAngle);
	m_speedHandle->setRotation(exitAngle);
}

void Kolf::BlackHoleOverlay::moveHandle(const QPointF& handleScenePos)
{
	Kolf::BlackHole* blackHole = dynamic_cast<Kolf::BlackHole*>(qitem());
	if (sender() == m_exitHandle)
		blackHole->setExitPos(handleScenePos);
	else if (sender() == m_speedHandle)
	{
		//this modifies only exit direction, not speed
		Vector dir = handleScenePos - blackHole->exitPos();
		blackHole->setExitDeg(-rad2deg(dir.direction()));
	}
}

//END Kolf::BlackHoleOverlay
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
	const double speed = Vector(ball->velocity()).magnitude();
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
