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

#include "landscape.h"
#include "ball.h"
#include "game.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <KConfigGroup>
#include <KLocale>

//BEGIN Kolf::LandscapeItem
//END Kolf::LandscapeItem

Kolf::LandscapeItem::LandscapeItem(const QString& type, QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(false, type, parent, world)
	, m_blinkEnabled(false)
	, m_blinkInterval(50)
	, m_blinkFrame(0)
{
	setSimulationType(CanvasItem::NoSimulation);
}

bool Kolf::LandscapeItem::isBlinkEnabled() const
{
	return m_blinkEnabled;
}

void Kolf::LandscapeItem::setBlinkEnabled(bool blinkEnabled)
{
	m_blinkEnabled = blinkEnabled;
	//reset animation
	m_blinkFrame = 0;
	setVisible(true);
}

int Kolf::LandscapeItem::blinkInterval() const
{
	return m_blinkInterval;
}

void Kolf::LandscapeItem::setBlinkInterval(int blinkInterval)
{
	m_blinkInterval = blinkInterval;
	//reset animation
	m_blinkFrame = 0;
	setVisible(true);
}

void Kolf::LandscapeItem::advance(int phase)
{
	EllipticalCanvasItem::advance(phase);
	if (phase == 1 && m_blinkEnabled)
	{
		const int actualInterval = 1.8 * (10 + m_blinkInterval);
		m_blinkFrame = (m_blinkFrame + 1) % (2 * actualInterval);
		setVisible(m_blinkFrame < actualInterval);
	}
}

void Kolf::LandscapeItem::load(KConfigGroup* group)
{
	EllipticalCanvasItem::loadSize(group);
	setBlinkEnabled(group->readEntry("changeEnabled", m_blinkEnabled));
	setBlinkInterval(group->readEntry("changeEvery", m_blinkInterval));
}

void Kolf::LandscapeItem::save(KConfigGroup* group)
{
	EllipticalCanvasItem::saveSize(group);
	group->writeEntry("changeEnabled", m_blinkEnabled);
	group->writeEntry("changeEvery", m_blinkInterval);
}

Config* Kolf::LandscapeItem::config(QWidget* parent)
{
	return new Kolf::LandscapeConfig(this, parent);
}

Kolf::Overlay* Kolf::LandscapeItem::createOverlay()
{
	return new Kolf::LandscapeOverlay(this);
}

//BEGIN Kolf::LandscapeOverlay

Kolf::LandscapeOverlay::LandscapeOverlay(Kolf::LandscapeItem* item)
	: Kolf::Overlay(item, item)
{
	for (int i = 0; i < 4; ++i)
	{
		Kolf::OverlayHandle* handle = new Kolf::OverlayHandle(Kolf::OverlayHandle::CircleShape, this);
		m_handles << handle;
		addHandle(handle);
		connect(handle, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
	}
}

void Kolf::LandscapeOverlay::update()
{
	Kolf::Overlay::update();
	const QRectF rect = qitem()->boundingRect();
	m_handles[0]->setPos(rect.topLeft());
	m_handles[1]->setPos(rect.topRight());
	m_handles[2]->setPos(rect.bottomLeft());
	m_handles[3]->setPos(rect.bottomRight());
}

void Kolf::LandscapeOverlay::moveHandle(const QPointF& handleScenePos)
{
	const QPointF handlePos = mapFromScene(handleScenePos);
	//factor 2: item bounding rect is always centered around (0,0)
	QSizeF newSize(2 * qAbs(handlePos.x()), 2 * qAbs(handlePos.y()));
	dynamic_cast<Kolf::LandscapeItem*>(qitem())->setSize(newSize);
}

//END Kolf::LandscapeOverlay
//BEGIN Kolf::LandscapeConfig

Kolf::LandscapeConfig::LandscapeConfig(Kolf::LandscapeItem* item, QWidget* parent)
	: Config(parent)
{
	QVBoxLayout* vlayout = new QVBoxLayout(this);
	QCheckBox* checkBox = new QCheckBox(i18n("Enable show/hide"), this);
	vlayout->addWidget(checkBox);

	QHBoxLayout* hlayout = new QHBoxLayout;
	vlayout->addLayout(hlayout);
	QLabel* label1 = new QLabel(i18n("Slow"), this);
	hlayout->addWidget(label1);
	QSlider* slider = new QSlider(Qt::Horizontal, this);
	hlayout->addWidget(slider);
	QLabel* label2 = new QLabel(i18n("Fast"), this);
	hlayout->addWidget(label2);

	vlayout->addStretch();

	checkBox->setChecked(true);
	connect(checkBox, SIGNAL(toggled(bool)), label1, SLOT(setEnabled(bool)));
	connect(checkBox, SIGNAL(toggled(bool)), label2, SLOT(setEnabled(bool)));
	connect(checkBox, SIGNAL(toggled(bool)), slider, SLOT(setEnabled(bool)));
	connect(checkBox, SIGNAL(toggled(bool)), item, SLOT(setBlinkEnabled(bool)));
	checkBox->setChecked(item->isBlinkEnabled());
	slider->setRange(1, 100);
	slider->setPageStep(5);
	slider->setValue(100 - item->blinkInterval());
	connect(slider, SIGNAL(valueChanged(int)), SLOT(setBlinkInterval(int)));
	connect(this, SIGNAL(blinkIntervalChanged(int)), item, SLOT(setBlinkInterval(int)));
}

void Kolf::LandscapeConfig::setBlinkInterval(int sliderValue)
{
	emit blinkIntervalChanged(100 - sliderValue);
}

//END Kolf::LandscapeConfig
//BEGIN Kolf::Puddle

Kolf::Puddle::Puddle(QGraphicsItem* parent, b2World* world)
	: Kolf::LandscapeItem(QLatin1String("puddle"), parent, world)
{
	setData(0, Rtti_DontPlaceOn);
	setSize(QSizeF(45, 30));
	setZValue(-25);
}

bool Kolf::Puddle::collision(Ball* ball)
{
	if (!ball->isVisible())
		return false;
	if (!contains(ball->pos() - pos()))
		return true;
	//ball is visible and has reached the puddle
	playSound("puddle");
	ball->setAddStroke(ball->addStroke() + 1);
	ball->setPlaceOnGround(true);
	ball->setVisible(false);
	ball->setState(Stopped);
	ball->setVelocity(Vector());
	if (game && game->curBall() == ball)
		game->stoppedBall();
	return false;
}

//END Kolf::Puddle
//BEGIN Kolf::Sand

Kolf::Sand::Sand(QGraphicsItem* parent, b2World* world)
	: Kolf::LandscapeItem(QLatin1String("sand"), parent, world)
{
	setSize(QSizeF(45, 40));
	setZValue(-26);
}

bool Kolf::Sand::collision(Ball* ball)
{
	if (contains(ball->pos() - pos()))
		ball->setFrictionMultiplier(7);
	return true;
}

//END Kolf::Sand

#include "landscape.moc"
