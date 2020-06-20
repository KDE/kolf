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

#include <QDoubleSpinBox>
#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <KComboBox>
#include <KConfigGroup>
#include <KLocalizedString>

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
	//TODO: code duplication to Kolf::RectangleOverlay and Kolf::SlopeOverlay
	for (int i = 0; i < 4; ++i)
	{
		Kolf::OverlayHandle* handle = new Kolf::OverlayHandle(Kolf::OverlayHandle::CircleShape, this);
		m_handles << handle;
		addHandle(handle);
		connect(handle, &Kolf::OverlayHandle::moveRequest, this, &Kolf::LandscapeOverlay::moveHandle);
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
	connect(checkBox, &QCheckBox::toggled, label1, &QLabel::setEnabled);
	connect(checkBox, &QCheckBox::toggled, label2, &QLabel::setEnabled);
	connect(checkBox, &QCheckBox::toggled, slider, &QSlider::setEnabled);
	connect(checkBox, &QCheckBox::toggled, item, &Kolf::LandscapeItem::setBlinkEnabled);
	checkBox->setChecked(item->isBlinkEnabled());
	slider->setRange(1, 100);
	slider->setPageStep(5);
	slider->setValue(100 - item->blinkInterval());
	connect(slider, &QSlider::valueChanged, this, &Kolf::LandscapeConfig::setBlinkInterval);
	connect(this, &Kolf::LandscapeConfig::blinkIntervalChanged, item, &Kolf::LandscapeItem::setBlinkInterval);
}

void Kolf::LandscapeConfig::setBlinkInterval(int sliderValue)
{
	Q_EMIT blinkIntervalChanged(100 - sliderValue);
}

//END Kolf::LandscapeConfig
//BEGIN Kolf::Puddle

Kolf::Puddle::Puddle(QGraphicsItem* parent, b2World* world)
	: Kolf::LandscapeItem(QStringLiteral("puddle"), parent, world)
{
	setData(0, Rtti_DontPlaceOn);
	setSize(QSizeF(45, 30));
	setZBehavior(CanvasItem::FixedZValue, 3);
}

bool Kolf::Puddle::collision(Ball* ball)
{
	if (!ball->isVisible())
		return false;
	if (!contains(ball->pos() - pos()))
		return true;
	//ball is visible and has reached the puddle
	game->playSound(Sound::Puddle);
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
	: Kolf::LandscapeItem(QStringLiteral("sand"), parent, world)
{
	setSize(QSizeF(45, 40));
	setZBehavior(CanvasItem::FixedZValue, 2);
}

bool Kolf::Sand::collision(Ball* ball)
{
	if (contains(ball->pos() - pos()))
		ball->setFrictionMultiplier(7);
	return true;
}

//END Kolf::Sand
//BEGIN Kolf::Slope

struct SlopeData
{
	QStringList gradientKeys, translatedGradientKeys;
	QStringList spriteKeys, reversedSpriteKeys;
	SlopeData()
	{
		gradientKeys << QStringLiteral("Vertical")
		             << QStringLiteral("Horizontal")
		             << QStringLiteral("Diagonal")
		             << QStringLiteral("Opposite Diagonal")
		             << QStringLiteral("Elliptic");
		translatedGradientKeys << i18n("Vertical")
		             << i18n("Horizontal")
		             << i18n("Diagonal")
		             << i18n("Opposite Diagonal")
		             << i18n("Elliptic");
		spriteKeys   << QStringLiteral("slope_n")
			         << QStringLiteral("slope_w")
			         << QStringLiteral("slope_nw")
			         << QStringLiteral("slope_ne")
			         << QStringLiteral("slope_bump");
		reversedSpriteKeys << QStringLiteral("slope_s")
			         << QStringLiteral("slope_e")
			         << QStringLiteral("slope_se")
			         << QStringLiteral("slope_sw")
			         << QStringLiteral("slope_dip");
	}
};
Q_GLOBAL_STATIC(SlopeData, g_slopeData)

Kolf::Slope::Slope(QGraphicsItem* parent, b2World* world)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), QString(), parent)
	, CanvasItem(world)
	, m_grade(4)
	, m_reversed(false)
	, m_stuckOnGround(false)
	, m_type(Kolf::VerticalSlope)
	, m_gradeItem(new QGraphicsSimpleTextItem(this))
{
	m_gradeItem->setBrush(Qt::white);
	m_gradeItem->setVisible(false);
	m_gradeItem->setZValue(1);
	for (int i = 0; i < 4; ++i)
	{
		ArrowItem* arrow = new ArrowItem(this);
		arrow->setLength(0);
		arrow->setVisible(false);
		m_arrows << arrow;
	}
	setSize(QSizeF(40, 40));
	m_stuckOnGround = true; //so that the following call does not return early
	setStuckOnGround(false); //initializes Z behavior
	updateAppearance();
}

double Kolf::Slope::grade() const
{
	return m_grade;
}

void Kolf::Slope::setGrade(double grade)
{
	if (m_grade != grade && grade > 0)
	{
		m_grade = grade;
		updateAppearance();
		propagateUpdate();
	}
}

bool Kolf::Slope::isReversed() const
{
	return m_reversed;
}

void Kolf::Slope::setReversed(bool reversed)
{
	if (m_reversed != reversed)
	{
		m_reversed = reversed;
		updateAppearance();
		propagateUpdate();
	}
}

Kolf::SlopeType Kolf::Slope::slopeType() const
{
	return m_type;
}

void Kolf::Slope::setSlopeType(int type)
{
	if (m_type != type && type >= 0)
	{
		m_type = (Kolf::SlopeType) type;
		updateAppearance();
		propagateUpdate();
	}
}

bool Kolf::Slope::isStuckOnGround() const
{
	return m_stuckOnGround;
}

void Kolf::Slope::setStuckOnGround(bool stuckOnGround)
{
	if (m_stuckOnGround != stuckOnGround)
	{
		m_stuckOnGround = stuckOnGround;
		setZBehavior(m_stuckOnGround ? CanvasItem::FixedZValue : CanvasItem::IsRaisedByStrut, 1);
		propagateUpdate();
	}
}

QPainterPath Kolf::Slope::shape() const
{
	const QRectF rect = boundingRect();
	QPainterPath path;
	if (m_type == Kolf::CrossDiagonalSlope) {
		QPolygonF polygon(3);
		polygon[0] = rect.topLeft();
		polygon[1] = rect.bottomRight();
		polygon[2] = m_reversed ? rect.topRight() : rect.bottomLeft();
		path.addPolygon(polygon);
	} else if (m_type == Kolf::DiagonalSlope) {
		QPolygonF polygon(3);
		polygon[0] = rect.topRight();
		polygon[1] = rect.bottomLeft();
		polygon[2] = m_reversed ? rect.topLeft() : rect.bottomRight();
		path.addPolygon(polygon);
	} else if (m_type == Kolf::EllipticSlope) {
		path.addEllipse(rect);
	} else {
		path.addRect(rect);
	}
	return path;
}

void Kolf::Slope::setSize(const QSizeF& size)
{
	if (m_type == Kolf::EllipticSlope)
	{
		const double extent = qMin(size.width(), size.height());
		Tagaro::SpriteObjectItem::setSize(extent, extent);
	}
	else
		Tagaro::SpriteObjectItem::setSize(size);
	updateInfo();
	propagateUpdate();
	updateZ(this);
}

QPointF Kolf::Slope::getPosition() const
{
	return Tagaro::SpriteObjectItem::pos();
}

void Kolf::Slope::moveBy(double dx, double dy)
{
	Tagaro::SpriteObjectItem::moveBy(dx, dy);
	CanvasItem::moveBy(dx, dy);
}

void Kolf::Slope::load(KConfigGroup* group)
{
	setGrade(group->readEntry("grade", m_grade));
	setReversed(group->readEntry("reversed", m_reversed));
	setStuckOnGround(group->readEntry("stuckOnGround", m_stuckOnGround));
	//gradient is a bit more complicated
	const QString type = group->readEntry("gradient", g_slopeData->gradientKeys.value(m_type));
	setSlopeType(g_slopeData->gradientKeys.indexOf(type));
	//read size
	QSizeF size = Tagaro::SpriteObjectItem::size();
	size.setWidth(group->readEntry("width", size.width()));
	size.setHeight(group->readEntry("height", size.height()));
	setSize(size);
}

void Kolf::Slope::save(KConfigGroup* group)
{
	group->writeEntry("grade", m_grade);
	group->writeEntry("reversed", m_reversed);
	group->writeEntry("stuckOnGround", m_stuckOnGround);
	group->writeEntry("gradient", g_slopeData->gradientKeys.value(m_type));
	const QSizeF size = Tagaro::SpriteObjectItem::size();
	group->writeEntry("width", size.width());
	group->writeEntry("height", size.height());
}

void Kolf::Slope::updateAppearance()
{
	updateInfo();
	//set pixmap
	setSpriteKey((m_reversed ? g_slopeData->reversedSpriteKeys : g_slopeData->spriteKeys).value(m_type));
}

void Kolf::Slope::updateInfo()
{
	m_gradeItem->setText(QString::number(m_grade));
	const QPointF textOffset = m_gradeItem->boundingRect().center();
	//update arrows
	const QSizeF size = Tagaro::SpriteObjectItem::size();
	const double width = size.width(), height = size.height();
	const double length = sqrt(width * width + height * height) / 4;
	if (m_type == Kolf::EllipticSlope)
	{
		double angle = 0;
		for (int i = 0; i < 4; ++i, angle += M_PI / 2)
		{
			ArrowItem* arrow = m_arrows[i];
			arrow->setLength(length);
			arrow->setAngle(angle);
			arrow->setReversed(m_reversed);
			arrow->setPos(QPointF(width / 2, height / 2));
		}
		m_gradeItem->setPos(QPointF(width / 2, height / 2) - textOffset);
	}
	else
	{
		double angle = 0;
		double x = .5 * width, y = .5 * height;
		switch ((int) m_type)
		{
			case Kolf::HorizontalSlope:
				angle = 0;
				break;
			case Kolf::VerticalSlope:
				angle = M_PI / 2;
				break;
			case Kolf::DiagonalSlope:
				angle = atan(width / height);
				x = m_reversed ? .25 * width : .75 * width;
				y = m_reversed ? .25 * height : .75 * height;
				break;
			case Kolf::CrossDiagonalSlope:
				angle = M_PI - atan(width / height);
				x = m_reversed ? .75 * width : .25 * width;
				y = m_reversed ? .25 * height : .75 * height;
				break;
		}
		//only one arrow needed - hide all others
		for (int i = 1; i < 4; ++i)
			m_arrows[i]->setLength(0);
		ArrowItem* arrow = m_arrows[0];
		arrow->setLength(length);
		arrow->setAngle(m_reversed ? angle : angle + M_PI);
		arrow->setPos(QPointF(x, y));
		m_gradeItem->setPos(QPointF(x, y) - textOffset);
	}
}

bool Kolf::Slope::collision(Ball* ball)
{
	Vector v = ball->velocity();
	double addto = 0.013 * m_grade;

	const bool diag = m_type == Kolf::DiagonalSlope || m_type == Kolf::CrossDiagonalSlope;
	const bool circle = m_type == Kolf::EllipticSlope;

	double slopeAngle = 0;
	const double width = size().width(), height = size().height();

	if (diag)
		slopeAngle = atan(width / height);
	else if (circle)
	{
		const QPointF start = pos() + QPointF(width, height) / 2.0;
		const QPointF end = ball->pos();

		Vector betweenVector = start - end;
		const double factor = betweenVector.magnitude() / (width / 2.0);
		slopeAngle = betweenVector.direction();

		// this little bit by Daniel
		addto *= factor * M_PI / 2;
		addto = sin(addto);
	}

	if (!m_reversed)
		addto = -addto;
	switch ((int) m_type)
	{
		case Kolf::HorizontalSlope:
			v.rx() += addto;
			break;
		case Kolf::VerticalSlope:
			v.ry() += addto;
			break;
		case Kolf::DiagonalSlope:
		case Kolf::EllipticSlope:
			v.rx() += cos(slopeAngle) * addto;
			v.ry() += sin(slopeAngle) * addto;
			break;
		case Kolf::CrossDiagonalSlope:
			v.rx() -= cos(slopeAngle) * addto;
			v.ry() += sin(slopeAngle) * addto;
			break;
	}
	ball->setVelocity(v);
	ball->setState(v.isNull() ? Stopped : Rolling);
	// do NOT do terrain collidingItems
	return false;
}

bool Kolf::Slope::terrainCollisions() const
{
	return true;
}

QList<QGraphicsItem*> Kolf::Slope::infoItems() const
{
	QList<QGraphicsItem*> result;
	for (ArrowItem* arrow : m_arrows)
		result << arrow;
	result << m_gradeItem;
	return result;
}

Config* Kolf::Slope::config(QWidget* parent)
{
	return new Kolf::SlopeConfig(this, parent);
}

Kolf::Overlay* Kolf::Slope::createOverlay()
{
	return new Kolf::SlopeOverlay(this);
}

//END Kolf::Slope
//BEGIN Kolf::SlopeConfig

Kolf::SlopeConfig::SlopeConfig(Kolf::Slope* slope, QWidget* parent)
	: Config(parent)
{
	QGridLayout* layout = new QGridLayout(this);

	KComboBox* typeBox = new KComboBox(this);
	typeBox->addItems(g_slopeData->translatedGradientKeys);
	typeBox->setCurrentIndex(slope->slopeType());
	connect(typeBox, QOverload<int>::of(&KComboBox::currentIndexChanged), slope, &Kolf::Slope::setSlopeType);
	layout->addWidget(typeBox, 0, 0, 1, 2);

	QCheckBox* reversed = new QCheckBox(i18n("Reverse direction"), this);
	reversed->setChecked(slope->isReversed());
	connect(reversed, &QCheckBox::toggled, slope, &Kolf::Slope::setReversed);
	layout->addWidget(reversed, 1, 0);

	QCheckBox* stuck = new QCheckBox(i18n("Unmovable"), this);
	stuck->setChecked(slope->isStuckOnGround());
	stuck->setWhatsThis(i18n("Whether or not this slope can be moved by other objects, like floaters."));
	connect(stuck, &QCheckBox::toggled, slope, &Kolf::Slope::setStuckOnGround);
	layout->addWidget(stuck, 1, 1);

	layout->addWidget(new QLabel(i18n("Grade:"), this), 2, 0);

	QDoubleSpinBox* grade = new QDoubleSpinBox(this);
	grade->setRange(0, 8);
	grade->setSingleStep(1);
	grade->setValue(slope->grade());
	connect(grade, QOverload<double>::of(&QDoubleSpinBox::valueChanged), slope, &Kolf::Slope::setGrade);
	layout->addWidget(grade, 2, 1);

	layout->setRowStretch(4, 10);
}

//END Kolf::SlopeConfig
//BEGIN Kolf::SlopeOverlay

Kolf::SlopeOverlay::SlopeOverlay(Kolf::Slope* slope)
	: Kolf::Overlay(slope, slope, true) //true = add shape to outlines
{
	//TODO: code duplication to Kolf::LandscapeOverlay and Kolf::RectangleOverlay
	for (int i = 0; i < 4; ++i)
	{
		Kolf::OverlayHandle* handle = new Kolf::OverlayHandle(Kolf::OverlayHandle::CircleShape, this);
		m_handles << handle;
		addHandle(handle);
		connect(handle, &Kolf::OverlayHandle::moveRequest, this, &Kolf::SlopeOverlay::moveHandle);
	}
}

void Kolf::SlopeOverlay::update()
{
	Kolf::Overlay::update();
	const QRectF rect = qitem()->boundingRect();
	m_handles[0]->setPos(rect.topLeft());
	m_handles[1]->setPos(rect.topRight());
	m_handles[2]->setPos(rect.bottomLeft());
	m_handles[3]->setPos(rect.bottomRight());
}

void Kolf::SlopeOverlay::moveHandle(const QPointF& handleScenePos)
{
	Kolf::OverlayHandle* handle = qobject_cast<Kolf::OverlayHandle*>(sender());
	const int handleIndex = m_handles.indexOf(handle);
	Kolf::Slope* item = dynamic_cast<Kolf::Slope*>(qitem());
	const QPointF handlePos = mapFromScene(handleScenePos);
	//modify bounding rect using new handlePos
	QRectF rect(QPointF(), item->size());
	if (handleIndex % 2 == 0)
		rect.setLeft(qMin(handlePos.x(), rect.right()));
	else
		rect.setRight(qMax(handlePos.x(), rect.left()));
	if (handleIndex < 2)
		rect.setTop(qMin(handlePos.y(), rect.bottom()));
	else
		rect.setBottom(qMax(handlePos.y(), rect.top()));
	item->moveBy(rect.x(), rect.y());
	item->setSize(rect.size());
}

//END Kolf::SlopeOverlay


