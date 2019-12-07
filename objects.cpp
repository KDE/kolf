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
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <KConfigGroup>
#include <KPluralHandlingSpinBox>
#include <KRandom>
#include <KLocalizedString>
//BEGIN Kolf::BlackHole

Kolf::BlackHole::BlackHole(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(true, QStringLiteral("black_hole"), parent, world)
	, m_minSpeed(3.0)
	, m_maxSpeed(5.0)
	, m_runs(0)
	, m_exitDeg(0)
	, m_exitItem(new QGraphicsLineItem(0, -15, 0, 15, Kolf::findBoard(this)))
	, m_directionItem(new ArrowItem(Kolf::findBoard(this)))
	, m_infoLine(new QGraphicsLineItem(this))
{
	setSize(QSizeF(16, 18));
	setZBehavior(CanvasItem::IsRaisedByStrut, 4);
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

	game->playSound(Sound::BlackHolePutIn);

	const double diff = m_maxSpeed - m_minSpeed;
	const double newSpeed = m_minSpeed + speed / 3.75 * diff;

	ball->setVelocity(Vector());
	ball->setState(Stopped);
	ball->setVisible(false);
	ball->setForceStillGoing(true);

	const double distance = Vector(pos() - m_exitItem->pos()).magnitude();
	BlackHoleTimer* timer = new BlackHoleTimer(ball, newSpeed, distance * 2.5 - newSpeed * 35 + 500);

	connect(timer, &BlackHoleTimer::eject, this, &Kolf::BlackHole::eject);
	connect(timer, &BlackHoleTimer::halfway, this, &Kolf::BlackHole::halfway);

	game->playSound(Sound::BlackHole);
	return false;
}

Kolf::BlackHoleTimer::BlackHoleTimer(Ball* ball, double speed, int msec)
	: m_speed(speed), m_ball(ball)
{
	QTimer::singleShot(msec, this, &Kolf::BlackHoleTimer::emitEject);
	QTimer::singleShot(msec / 2, this, &Kolf::BlackHoleTimer::halfway);
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

	game->playSound(Sound::BlackHoleEject);
}

void Kolf::BlackHole::halfway()
{
	game->playSound(Sound::BlackHole);
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

	KPluralHandlingSpinBox* deg = new KPluralHandlingSpinBox(this);
	deg->setRange(0, 359);
	deg->setSingleStep(10);
	deg->setSuffix(ki18np(" degree", " degrees"));
	deg->setValue(m_blackHole->curExitDeg());
	deg->setWrapping(true);
	layout->addRow(i18n("Exiting ball angle:"), deg);
	connect(deg, QOverload<int>::of(&KPluralHandlingSpinBox::valueChanged), this, &Kolf::BlackHoleConfig::degChanged);

	QSlider* minSlider = new QSlider(Qt::Horizontal, this);
	minSlider->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	minSlider->setRange(0, 800);
	minSlider->setSingleStep(1);
	minSlider->setTickPosition(QSlider::TicksBelow);
	QDoubleSpinBox* minSpinBox = new QDoubleSpinBox(this);
	minSpinBox->setRange(0, 8);
	minSpinBox->setSingleStep(0.01);
	connect(minSlider, &QSlider::valueChanged, [minSlider, minSpinBox] {minSpinBox->setValue(minSlider->value()/100.0);});
	connect(minSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [minSlider, minSpinBox] {minSlider->setValue(qRound(minSpinBox->value()*100));});
	QHBoxLayout *min = new QHBoxLayout;
	min->addWidget(minSlider);
	min->addWidget(minSpinBox);
	layout->addRow(i18n("Minimum exit speed:"), min);
	minSpinBox->setValue(m_blackHole->minSpeed());
	connect(minSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Kolf::BlackHoleConfig::minChanged);

	QSlider* maxSlider = new QSlider(Qt::Horizontal, this);
	maxSlider->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	maxSlider->setRange(0, 800);
	maxSlider->setSingleStep(1);
	maxSlider->setTickPosition(QSlider::TicksBelow);
	QDoubleSpinBox* maxSpinBox = new QDoubleSpinBox(this);
	maxSpinBox->setRange(0, 8);
	maxSpinBox->setSingleStep(0.01);
	connect(maxSlider, &QSlider::valueChanged, [maxSlider, maxSpinBox] {maxSpinBox->setValue(maxSlider->value()/100.0);});
	connect(maxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [maxSlider, maxSpinBox] {maxSlider->setValue(qRound(maxSpinBox->value()*100));});
	QHBoxLayout *max = new QHBoxLayout;
	max->addWidget(maxSlider);
	max->addWidget(maxSpinBox);
	layout->addRow(i18n("Maximum exit speed:"), max);
	maxSpinBox->setValue(m_blackHole->maxSpeed());
	connect(maxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Kolf::BlackHoleConfig::maxChanged);
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
	connect(m_exitHandle, &Kolf::OverlayHandle::moveRequest, this, &Kolf::BlackHoleOverlay::moveHandle);
	connect(m_speedHandle, &Kolf::OverlayHandle::moveRequest, this, &Kolf::BlackHoleOverlay::moveHandle);
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
	: EllipticalCanvasItem(false, QStringLiteral("cup"), parent, world)
{
	const int diameter = 16;
	setSize(QSizeF(diameter, diameter));
	setZBehavior(CanvasItem::IsRaisedByStrut, 4);
	setSimulationType(CanvasItem::NoSimulation);
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
	game->playSound(Sound::Holed);
	ball->setPos(pos());
	ball->setVelocity(Vector());
	return false;
}

//END Kolf::Cup


