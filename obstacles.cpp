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

#include "obstacles.h"
#include "ball.h"
#include "game.h"
#include "shape.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <KConfigGroup>
#include <KLineEdit>
#include <KRandom>

//BEGIN Kolf::Bumper

Kolf::Bumper::Bumper(QGraphicsItem* parent, b2World* world)
	: EllipticalCanvasItem(false, QLatin1String("bumper_off"), parent, world)
{
	const int diameter = 20;
	setSize(QSizeF(diameter, diameter));
	setZBehavior(CanvasItem::IsRaisedByStrut, 4);
	setSimulationType(CanvasItem::NoSimulation);
}

bool Kolf::Bumper::collision(Ball* ball)
{
	const double maxSpeed = ball->getMaxBumperBounceSpeed();
	const double speed = qMin(maxSpeed, 1.8 + Vector(ball->velocity()).magnitude() * .9);
	ball->reduceMaxBumperBounceSpeed();

	Vector betweenVector(ball->pos() - pos());
	betweenVector.setMagnitudeDirection(speed,
		// add some randomness so we don't go indefinetely
		betweenVector.direction() + deg2rad((KRandom::random() % 3) - 1)
	);

	ball->setVelocity(betweenVector);
	ball->setState(Rolling);

	setSpriteKey(QLatin1String("bumper_on"));
	QTimer::singleShot(100, this, SLOT(turnBumperOff()));
	return true;
}

void Kolf::Bumper::turnBumperOff()
{
	setSpriteKey(QLatin1String("bumper_off"));
}

Kolf::Overlay* Kolf::Bumper::createOverlay()
{
	return new Kolf::Overlay(this, this);
}

//END Kolf::Bumper
//BEGIN Kolf::Wall

Kolf::Wall::Wall(QGraphicsItem* parent, b2World* world)
	: QGraphicsLineItem(QLineF(-15, 10, 15, -5), parent)
	, CanvasItem(world)
{
	setPen(QPen(Qt::darkRed, 3));
	setData(0, Rtti_NoCollision);
	setZBehavior(CanvasItem::FixedZValue, 5);

	m_shape = new Kolf::LineShape(line());
	addShape(m_shape);
}

void Kolf::Wall::load(KConfigGroup* cfgGroup)
{
	const QPoint start = cfgGroup->readEntry("startPoint", QPoint(-15, 10));
	const QPoint end = cfgGroup->readEntry("endPoint", QPoint(15, -5));
	setLine(QLineF(start, end));
}

void Kolf::Wall::save(KConfigGroup* cfgGroup)
{
	const QLineF line = this->line();
	cfgGroup->writeEntry("startPoint", line.p1().toPoint());
	cfgGroup->writeEntry("endPoint", line.p2().toPoint());
}

void Kolf::Wall::setVisible(bool visible)
{
	QGraphicsLineItem::setVisible(visible);
	setSimulationType(visible ? CanvasItem::CollisionSimulation : CanvasItem::NoSimulation);
}

void Kolf::Wall::setLine(const QLineF& line)
{
	QGraphicsLineItem::setLine(line);
	m_shape->setLine(line);
	propagateUpdate();
}

void Kolf::Wall::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
	CanvasItem::moveBy(dx, dy);
}

QPointF Kolf::Wall::getPosition() const
{
	return QGraphicsItem::pos();
}

Kolf::Overlay* Kolf::Wall::createOverlay()
{
	return new Kolf::WallOverlay(this);
}

//END Kolf::Wall
//BEGIN Kolf::WallOverlay

Kolf::WallOverlay::WallOverlay(Kolf::Wall* wall)
	: Kolf::Overlay(wall, wall)
	, m_handle1(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
	, m_handle2(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
{
	addHandle(m_handle1);
	addHandle(m_handle2);
	connect(m_handle1, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
	connect(m_handle2, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
}

void Kolf::WallOverlay::update()
{
	Kolf::Overlay::update();
	const QLineF line = dynamic_cast<Kolf::Wall*>(qitem())->line();
	m_handle1->setPos(line.p1());
	m_handle2->setPos(line.p2());
}

void Kolf::WallOverlay::moveHandle(const QPointF& handleScenePos)
{
	//TODO: code duplication to Kolf::FloaterOverlay
	QPointF handlePos = mapFromScene(handleScenePos);
	const QObject* handle = sender();
	//get handle positions
	QPointF handle1Pos = m_handle1->pos();
	QPointF handle2Pos = m_handle2->pos();
	if (handle == m_handle1)
		handle1Pos = handlePos;
	else if (handle == m_handle2)
		handle2Pos = handlePos;
	//ensure minimum length
	static const qreal minLength = Kolf::Overlay::MinimumObjectDimension;
	const QPointF posDiff = handle1Pos - handle2Pos;
	const qreal length = QLineF(QPointF(), posDiff).length();
	if (length < minLength)
	{
		const QPointF additionalExtent = posDiff * (minLength / length - 1);
		if (handle == m_handle1)
			handle1Pos += additionalExtent;
		else if (handle == m_handle2)
			handle2Pos -= additionalExtent;
	}
	//apply to item
	dynamic_cast<Kolf::Wall*>(qitem())->setLine(QLineF(handle1Pos, handle2Pos));
}

//END Kolf::WallOverlay
//BEGIN Kolf::RectangleItem

Kolf::RectangleItem::RectangleItem(const QString& type, QGraphicsItem* parent, b2World* world)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), type, parent)
	, CanvasItem(world)
	, m_wallPen(QColor("#92772D").darker(), 3)
	, m_wallAllowed(Kolf::RectangleWallCount, true)
	, m_walls(Kolf::RectangleWallCount, 0)
	, m_shape(new Kolf::RectShape(QRectF(0, 0, 1, 1)))
{
	setZValue(998);
	addShape(m_shape);
	setSimulationType(CanvasItem::NoSimulation);
	//default size
	setSize(type == "sign" ? QSize(110, 40) : QSize(80, 40));
}

Kolf::RectangleItem::~RectangleItem()
{
	qDeleteAll(m_walls);
}

bool Kolf::RectangleItem::hasWall(Kolf::WallIndex index) const
{
	return (bool) m_walls[index];
}

bool Kolf::RectangleItem::isWallAllowed(Kolf::WallIndex index) const
{
	return m_wallAllowed[index];
}

void Kolf::RectangleItem::setWall(Kolf::WallIndex index, bool hasWall)
{
	const bool oldHasWall = (bool) m_walls[index];
	if (oldHasWall == hasWall)
		return;
	if (hasWall && !m_wallAllowed[index])
		return;
	if (hasWall)
	{
		Kolf::Wall* wall = m_walls[index] = new Kolf::Wall(parentItem(), world());
		wall->setPos(pos());
		applyWallStyle(wall);
		updateWallPosition();
	}
	else
	{
		delete m_walls[index];
		m_walls[index] = 0;
	}
	propagateUpdate();
	emit wallChanged(index, hasWall, m_wallAllowed[index]);
}

void Kolf::RectangleItem::setWallAllowed(Kolf::WallIndex index, bool wallAllowed)
{
	m_wallAllowed[index] = wallAllowed;
	//delete wall if one exists at this position currently
	if (!wallAllowed)
		setWall(index, false);
	emit wallChanged(index, hasWall(index), wallAllowed);
}

void Kolf::RectangleItem::updateWallPosition()
{
	const QRectF rect(QPointF(), size());
	Kolf::Wall* const topWall = m_walls[Kolf::TopWallIndex];
	Kolf::Wall* const leftWall = m_walls[Kolf::LeftWallIndex];
	Kolf::Wall* const rightWall = m_walls[Kolf::RightWallIndex];
	Kolf::Wall* const bottomWall = m_walls[Kolf::BottomWallIndex];
	if (topWall)
		topWall->setLine(QLineF(rect.topLeft(), rect.topRight()));
	if (leftWall)
		leftWall->setLine(QLineF(rect.topLeft(), rect.bottomLeft()));
	if (rightWall)
		rightWall->setLine(QLineF(rect.topRight(), rect.bottomRight()));
	if (bottomWall)
		bottomWall->setLine(QLineF(rect.bottomLeft(), rect.bottomRight()));
}

void Kolf::RectangleItem::setSize(const QSizeF& size)
{
	Tagaro::SpriteObjectItem::setSize(size);
	m_shape->setRect(QRectF(QPointF(), size));
	updateWallPosition();
	propagateUpdate();
}

QPointF Kolf::RectangleItem::getPosition() const
{
	return QGraphicsItem::pos();
}

void Kolf::RectangleItem::moveBy(double dx, double dy)
{
	Tagaro::SpriteObjectItem::moveBy(dx, dy);
	//move myself
	const QPointF pos = this->pos();
	foreach (Kolf::Wall* wall, m_walls)
		if (wall)
			wall->setPos(pos);
	//update Z order
	CanvasItem::moveBy(dx, dy);
	foreach (QGraphicsItem* qitem, collidingItems())
	{
		CanvasItem* citem = dynamic_cast<CanvasItem*>(qitem);
		if (citem)
			citem->updateZ(qitem);
	}
}

void Kolf::RectangleItem::setWallColor(const QColor& color)
{
	m_wallPen = QPen(color.darker(), 3);
	foreach (Kolf::Wall* wall, m_walls)
		applyWallStyle(wall);
}

void Kolf::RectangleItem::applyWallStyle(Kolf::Wall* wall)
{
	if (!wall) //explicitly allowed, see e.g. setWallColor()
		return;
	wall->setPen(m_wallPen);
	wall->setZValue(zValue() + 0.001);
}

void Kolf::RectangleItem::setZValue(qreal zValue)
{
	QGraphicsItem::setZValue(zValue);
	foreach (Kolf::Wall* wall, m_walls)
		applyWallStyle(wall);
}

static const char* wallPropNames[] = { "topWallVisible", "leftWallVisible", "rightWallVisible", "botWallVisible" };

void Kolf::RectangleItem::load(KConfigGroup* group)
{
	QSize size = Tagaro::SpriteObjectItem::size().toSize();
	size.setWidth(group->readEntry("width", size.width()));
	size.setHeight(group->readEntry("height", size.height()));
	setSize(size);
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
	{
		bool hasWall = this->hasWall((Kolf::WallIndex) i);
		hasWall = group->readEntry(wallPropNames[i], hasWall);
		setWall((Kolf::WallIndex) i, hasWall);
	}
}

void Kolf::RectangleItem::save(KConfigGroup* group)
{
	const QSize size = Tagaro::SpriteObjectItem::size().toSize();
	group->writeEntry("width", size.width());
	group->writeEntry("height", size.height());
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
	{
		const bool hasWall = this->hasWall((Kolf::WallIndex) i);
		group->writeEntry(wallPropNames[i], hasWall);
	}
}

Config* Kolf::RectangleItem::config(QWidget* parent)
{
	return new Kolf::RectangleConfig(this, parent);
}

Kolf::Overlay* Kolf::RectangleItem::createOverlay()
{
	return new Kolf::RectangleOverlay(this);
}

//END Kolf::RectangleItem
//BEGIN Kolf::RectangleOverlay

Kolf::RectangleOverlay::RectangleOverlay(Kolf::RectangleItem* item)
	: Kolf::Overlay(item, item)
{
	//TODO: code duplication to Kolf::LandscapeOverlay and Kolf::SlopeOverlay
	for (int i = 0; i < 4; ++i)
	{
		Kolf::OverlayHandle* handle = new Kolf::OverlayHandle(Kolf::OverlayHandle::CircleShape, this);
		m_handles << handle;
		addHandle(handle);
		connect(handle, SIGNAL(moveRequest(QPointF)), this, SLOT(moveHandle(QPointF)));
	}
}

void Kolf::RectangleOverlay::update()
{
	Kolf::Overlay::update();
	const QRectF rect = qitem()->boundingRect();
	m_handles[0]->setPos(rect.topLeft());
	m_handles[1]->setPos(rect.topRight());
	m_handles[2]->setPos(rect.bottomLeft());
	m_handles[3]->setPos(rect.bottomRight());
}

void Kolf::RectangleOverlay::moveHandle(const QPointF& handleScenePos)
{
	Kolf::OverlayHandle* handle = qobject_cast<Kolf::OverlayHandle*>(sender());
	const int handleIndex = m_handles.indexOf(handle);
	Kolf::RectangleItem* item = dynamic_cast<Kolf::RectangleItem*>(qitem());
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

//END Kolf::RectangleOverlay
//BEGIN Kolf::RectangleConfig

Kolf::RectangleConfig::RectangleConfig(Kolf::RectangleItem* item, QWidget* parent)
	: Config(parent)
	, m_layout(new QGridLayout(this))
	, m_wallCheckBoxes(Kolf::RectangleWallCount, 0)
	, m_item(item)
{
	static const char* captions[] = { I18N_NOOP("&Top"), I18N_NOOP("&Left"), I18N_NOOP("&Right"), I18N_NOOP("&Bottom") };
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
	{
		QCheckBox* checkBox = m_wallCheckBoxes[i] = new QCheckBox(i18n(captions[i]), this);
		checkBox->setEnabled(item->isWallAllowed((Kolf::WallIndex) i));
		checkBox->setChecked(item->hasWall((Kolf::WallIndex) i));
		connect(checkBox, SIGNAL(toggled(bool)), SLOT(setWall(bool)));
	}
	connect(item, SIGNAL(wallChanged(Kolf::WallIndex, bool,bool)), SLOT(wallChanged(Kolf::WallIndex, bool,bool)));
	m_layout->addWidget(new QLabel(i18n("Walls on:")), 0, 0);
	m_layout->addWidget(m_wallCheckBoxes[0], 0, 1);
	m_layout->addWidget(m_wallCheckBoxes[1], 1, 0);
	m_layout->addWidget(m_wallCheckBoxes[2], 1, 2);
	m_layout->addWidget(m_wallCheckBoxes[3], 1, 1);
	m_layout->setRowStretch(2, 10);
	//Kolf::Sign does not have a special Config class
	Kolf::Sign* sign = qobject_cast<Kolf::Sign*>(item);
	if (sign)
	{
		m_layout->addWidget(new QLabel(i18n("Sign HTML:")), 3, 0, 1, 3);
		KLineEdit* edit = new KLineEdit(sign->text(), this);
		m_layout->addWidget(edit, 4, 0, 1, 3);
		connect(edit, SIGNAL(textChanged(QString)), sign, SLOT(setText(QString)));
	}
	//Kolf::Windmill does not have a special Config class
	Kolf::Windmill* windmill = qobject_cast<Kolf::Windmill*>(item);
	if (windmill)
	{
		QCheckBox* checkBox = new QCheckBox(i18n("Windmill on top"), this);
		m_layout->addWidget(checkBox, 4, 0, 1, 3);
		checkBox->setChecked(windmill->guardAtTop());
		connect(checkBox, SIGNAL(toggled(bool)), windmill, SLOT(setGuardAtTop(bool)));
		QHBoxLayout* hlayout = new QHBoxLayout;
		m_layout->addLayout(hlayout, 5, 0, 1, 3);
		QLabel* label1 = new QLabel(i18n("Slow"), this);
		hlayout->addWidget(label1);
		QSlider* slider = new QSlider(Qt::Horizontal, this);
		hlayout->addWidget(slider);
		QLabel* label2 = new QLabel(i18n("Fast"), this);
		hlayout->addWidget(label2);
		slider->setRange(1, 10);
		slider->setPageStep(1);
		slider->setValue(windmill->speed());
		connect(slider, SIGNAL(valueChanged(int)), windmill, SLOT(setSpeed(int)));
	}
	//Kolf::Floater does not have a special Config class
	Kolf::Floater* floater = qobject_cast<Kolf::Floater*>(item);
	if (floater)
	{
		m_layout->addWidget(new QLabel(i18n("Moving speed"), this), 4, 0, 1, 3);
		QHBoxLayout* hlayout = new QHBoxLayout;
		m_layout->addLayout(hlayout, 5, 0, 1, 3);
		QLabel* label1 = new QLabel(i18n("Slow"), this);
		hlayout->addWidget(label1);
		QSlider* slider = new QSlider(Qt::Horizontal, this);
		hlayout->addWidget(slider);
		QLabel* label2 = new QLabel(i18n("Fast"), this);
		hlayout->addWidget(label2);
		slider->setRange(0, 20);
		slider->setPageStep(2);
		slider->setValue(floater->speed());
		connect(slider, SIGNAL(valueChanged(int)), floater, SLOT(setSpeed(int)));
	}
}

void Kolf::RectangleConfig::setWall(bool hasWall)
{
	const int wallIndex = m_wallCheckBoxes.indexOf(qobject_cast<QCheckBox*>(sender()));
	if (wallIndex >= 0)
	{
		m_item->setWall((Kolf::WallIndex) wallIndex, hasWall);
		changed();
	}
}

void Kolf::RectangleConfig::wallChanged(Kolf::WallIndex index, bool hasWall, bool wallAllowed)
{
	m_wallCheckBoxes[index]->setEnabled(wallAllowed);
	m_wallCheckBoxes[index]->setChecked(hasWall);
}

//END Kolf::RectangleConfig
//BEGIN Kolf::Bridge

Kolf::Bridge::Bridge(QGraphicsItem* parent, b2World* world)
	: Kolf::RectangleItem(QLatin1String("bridge"), parent, world)
{
	setZBehavior(CanvasItem::IsStrut, 0);
}

bool Kolf::Bridge::collision(Ball* ball)
{
	ball->setFrictionMultiplier(.63);
	return false;
}

//END Kolf::Bridge
//BEGIN Kolf::Floater

Kolf::Floater::Floater(QGraphicsItem* parent, b2World* world)
	: Kolf::RectangleItem(QLatin1String("floater"), parent, world)
	, m_motionLine(QLineF(200, 200, 100, 100))
	, m_speed(0)
	, m_velocity(0)
	, m_position(0)
	, m_moveByMovesMotionLine(true)
	, m_animated(true)
{
	setMlPosition(m_position);
	setZBehavior(CanvasItem::IsStrut, 0);
}

void Kolf::Floater::editModeChanged(bool editing)
{
	Kolf::RectangleItem::editModeChanged(editing);
	m_animated = !editing;
	if (editing)
		setMlPosition(0);
}

void Kolf::Floater::moveBy(double dx, double dy)
{
	moveItemsOnStrut(QPointF(dx, dy));
	Kolf::RectangleItem::moveBy(dx, dy);
	if (m_moveByMovesMotionLine)
		m_motionLine.translate(dx, dy);
	propagateUpdate();
}

QLineF Kolf::Floater::motionLine() const
{
	return m_motionLine;
}

void Kolf::Floater::setMotionLine(const QLineF& motionLine)
{
	m_motionLine = motionLine;
	setMlPosition(m_position);
	propagateUpdate();
}

void Kolf::Floater::setMlPosition(qreal position)
{
	m_moveByMovesMotionLine = false;
	setPosition(m_motionLine.pointAt(position));
	m_position = position;
	m_moveByMovesMotionLine = true;
}

int Kolf::Floater::speed() const
{
	return m_speed;
}

void Kolf::Floater::setSpeed(int speed)
{
	m_speed = speed;
	const qreal velocity = speed / 3.5;
	m_velocity = (m_velocity < 0) ? -velocity : velocity;
	propagateUpdate();
}

void Kolf::Floater::advance(int phase)
{
	if (phase != 1 || !m_animated)
		return;
	//determine movement step
	const qreal mlLength = m_motionLine.length();
	const qreal parameterDiff = m_velocity / mlLength;
	//determine new position (mirror on end point if end point passed)
	m_position += parameterDiff;
	if (m_position < 0)
	{
		m_velocity = qAbs(m_velocity);
		m_position = -m_position;
	}
	else if (m_position > 1)
	{
		m_velocity = -qAbs(m_velocity);
		m_position = 2 - m_position;
	}
	//apply position
	setMlPosition(m_position);
}

void Kolf::Floater::load(KConfigGroup* group)
{
	Kolf::RectangleItem::load(group);
	QLineF motionLine = m_motionLine;
	motionLine.setP1(group->readEntry("startPoint", m_motionLine.p1()));
	motionLine.setP2(group->readEntry("endPoint", m_motionLine.p2()));
	setMotionLine(motionLine);
	setSpeed(group->readEntry("speed", m_speed));
}

void Kolf::Floater::save(KConfigGroup* group)
{
	Kolf::RectangleItem::save(group);
	group->writeEntry("startPoint", m_motionLine.p1());
	group->writeEntry("endPoint", m_motionLine.p2());
	group->writeEntry("speed", m_speed);
}

Kolf::Overlay* Kolf::Floater::createOverlay()
{
	return new Kolf::FloaterOverlay(this);
}

//END Kolf::Floater
//BEGIN Kolf::FloaterOverlay

Kolf::FloaterOverlay::FloaterOverlay(Kolf::Floater* floater)
	: Kolf::RectangleOverlay(floater)
	, m_handle1(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
	, m_handle2(new Kolf::OverlayHandle(Kolf::OverlayHandle::SquareShape, this))
	, m_motionLineItem(new QGraphicsLineItem(this))
{
	addHandle(m_handle1);
	addHandle(m_handle2);
	connect(m_handle1, SIGNAL(moveRequest(QPointF)), this, SLOT(moveMotionLineHandle(QPointF)));
	connect(m_handle2, SIGNAL(moveRequest(QPointF)), this, SLOT(moveMotionLineHandle(QPointF)));
	addHandle(m_motionLineItem);
	QPen pen = m_motionLineItem->pen();
	pen.setStyle(Qt::DashLine);
	m_motionLineItem->setPen(pen);
}

void Kolf::FloaterOverlay::update()
{
	Kolf::RectangleOverlay::update();
	const QLineF line = dynamic_cast<Kolf::Floater*>(qitem())->motionLine().translated(-qitem()->pos());
	m_handle1->setPos(line.p1());
	m_handle2->setPos(line.p2());
	m_motionLineItem->setLine(line);
}

void Kolf::FloaterOverlay::moveMotionLineHandle(const QPointF& handleScenePos)
{
	//TODO: code duplication to Kolf::WallOverlay
	QPointF handlePos = mapFromScene(handleScenePos) + qitem()->pos();
	const QObject* handle = sender();
	//get handle positions
	QPointF handle1Pos = m_handle1->pos() + qitem()->pos();
	QPointF handle2Pos = m_handle2->pos() + qitem()->pos();
	if (handle == m_handle1)
		handle1Pos = handlePos;
	else if (handle == m_handle2)
		handle2Pos = handlePos;
	//ensure minimum length
	static const qreal minLength = Kolf::Overlay::MinimumObjectDimension;
	const QPointF posDiff = handle1Pos - handle2Pos;
	const qreal length = QLineF(QPointF(), posDiff).length();
	if (length < minLength)
	{
		const QPointF additionalExtent = posDiff * (minLength / length - 1);
		if (handle == m_handle1)
			handle1Pos += additionalExtent;
		else if (handle == m_handle2)
			handle2Pos -= additionalExtent;
	}
	//apply to item
	dynamic_cast<Kolf::Floater*>(qitem())->setMotionLine(QLineF(handle1Pos, handle2Pos));
}

//END Kolf::FloaterOverlay
//BEGIN Kolf::Sign

Kolf::Sign::Sign(QGraphicsItem* parent, b2World* world)
	: Kolf::RectangleItem(QLatin1String("sign"), parent, world)
	, m_text(i18n("New Text"))
	, m_textItem(new QGraphicsTextItem(m_text, this))
{
	setZBehavior(CanvasItem::FixedZValue, 3);
	setWallColor(Qt::black);
	for (int i = 0; i < Kolf::RectangleWallCount; ++i)
		setWall((Kolf::WallIndex) i, true);
	//Z value 1 should be enough to keep text above overlay
	m_textItem->setZValue(1);
	m_textItem->setAcceptedMouseButtons(0);
	//TODO: activate QGraphicsItem::ItemClipsChildrenToShape flag after
	//refactoring (only after it is clear that the text is the only child)
}

QString Kolf::Sign::text() const
{
	return m_text;
}

void Kolf::Sign::setText(const QString& text)
{
	m_text = text;
	m_textItem->setHtml(text);
}

void Kolf::Sign::setSize(const QSizeF& size)
{
	Kolf::RectangleItem::setSize(size);
	m_textItem->setTextWidth(size.width());
}

void Kolf::Sign::load(KConfigGroup* group)
{
	Kolf::RectangleItem::load(group);
	setText(group->readEntry("Comment", m_text));
}

void Kolf::Sign::save(KConfigGroup* group)
{
	Kolf::RectangleItem::save(group);
	group->writeEntry("Comment", m_text);
}

//END Kolf::Sign
//BEGIN Kolf::Windmill

Kolf::Windmill::Windmill(QGraphicsItem* parent, b2World* world)
	: Kolf::RectangleItem(QLatin1String("windmill"), parent, world)
	  , m_leftWall(new Kolf::Wall(parent, world))
	  , m_rightWall(new Kolf::Wall(parent, world))
	  , m_guardWall(new Kolf::Wall(parent, world))
	  , m_guardAtTop(false)
	  , m_speed(0), m_velocity(0)
{
	setZBehavior(CanvasItem::IsStrut, 0);
	setSpeed(5); //initialize m_speed and m_velocity properly
	applyWallStyle(m_leftWall);
	applyWallStyle(m_rightWall);
	applyWallStyle(m_guardWall); //Z-ordering!
	m_guardWall->setPen(QPen(Qt::black, 5));
	setWall(Kolf::TopWallIndex, false);
	setWall(Kolf::LeftWallIndex, true);
	setWall(Kolf::RightWallIndex, true);
	setWallAllowed(Kolf::BottomWallIndex, false);
	m_guardWall->setLine(QLineF());
	updateWallPosition();
}

Kolf::Windmill::~Windmill()
{
	delete m_leftWall;
	delete m_rightWall;
	delete m_guardWall;
}

bool Kolf::Windmill::guardAtTop() const
{
	return m_guardAtTop;
}

void Kolf::Windmill::setGuardAtTop(bool guardAtTop)
{
	if (m_guardAtTop == guardAtTop)
		return;
	m_guardAtTop = guardAtTop;
	//exchange top and bottom walls
	if (guardAtTop)
	{
		const bool hasWall = this->hasWall(Kolf::TopWallIndex);
		setWallAllowed(Kolf::BottomWallIndex, true);
		setWallAllowed(Kolf::TopWallIndex, false);
		setWall(Kolf::BottomWallIndex, hasWall);
	}
	else
	{
		const bool hasWall = this->hasWall(Kolf::BottomWallIndex);
		setWallAllowed(Kolf::BottomWallIndex, false);
		setWallAllowed(Kolf::TopWallIndex, true);
		setWall(Kolf::TopWallIndex, hasWall);
	}
	//recalculate position of guard walls etc.
	updateWallPosition();
	propagateUpdate();
}

int Kolf::Windmill::speed() const
{
	return m_speed;
}

void Kolf::Windmill::setSpeed(int speed)
{
	m_speed = speed;
	const qreal velocity = speed / 3.0;
	m_velocity = (m_velocity < 0) ? -velocity : velocity;
	propagateUpdate();
}

void Kolf::Windmill::advance(int phase)
{
	if (phase == 1)
	{
		QLineF guardLine = m_guardWall->line().translated(m_velocity, 0);
		const qreal maxX = qMax(guardLine.x1(), guardLine.x2());
		const qreal minX = qMin(guardLine.x1(), guardLine.x2());
		QRectF rect(QPointF(), size());
		if (minX < rect.left())
		{
			guardLine.translate(rect.left() - minX, 0);
			m_velocity = qAbs(m_velocity);
		}
		else if (maxX > rect.right())
		{
			guardLine.translate(rect.right() - maxX, 0);
			m_velocity = -qAbs(m_velocity);
		}
		m_guardWall->setLine(guardLine);
	}
}

void Kolf::Windmill::moveBy(double dx, double dy)
{
	Kolf::RectangleItem::moveBy(dx, dy);
	const QPointF pos = this->pos();
	m_leftWall->setPos(pos);
	m_rightWall->setPos(pos);
	m_guardWall->setPos(pos);
}

void Kolf::Windmill::updateWallPosition()
{
	Kolf::RectangleItem::updateWallPosition();
	//parametrize position of guard relative to old rect
	qreal t = 0.5;
	if (!m_guardWall->line().isNull())
	{
		//this branch is taken unless this method gets called from the ctor
		const qreal oldLeft = m_leftWall->line().x1();
		const qreal oldRight = m_rightWall->line().x1();
		const qreal oldGCenter = m_guardWall->line().pointAt(0.5).x();
		t = (oldGCenter - oldLeft) / (oldRight - oldLeft);
	}
	//set new positions
	const QRectF rect(QPointF(), size());
	const QPointF leftEnd = m_guardAtTop ? rect.topLeft() : rect.bottomLeft();
	const QPointF rightEnd = m_guardAtTop ? rect.topRight() : rect.bottomRight();
	const QPointF wallExtent(rect.width() / 4, 0);
	m_leftWall->setLine(QLineF(leftEnd, leftEnd + wallExtent));
	m_rightWall->setLine(QLineF(rightEnd, rightEnd - wallExtent));
	//set position of guard to the same relative coordinate as before
	const qreal gWidth = wallExtent.x() / 1.07 - 2;
	const qreal gY = m_guardAtTop ? rect.top() - 4 : rect.bottom() + 4;
	QLineF gLine(rect.left(), gY, rect.left() + gWidth, gY);
	const qreal currentGCenter = gLine.pointAt(0.5).x();
	const qreal desiredGCenter = rect.left() + t * rect.width();
	gLine.translate(desiredGCenter - currentGCenter, 0);
	m_guardWall->setLine(gLine);
}

void Kolf::Windmill::load(KConfigGroup* group)
{
	Kolf::RectangleItem::load(group);
	setSpeed(group->readEntry("speed", m_speed));
	setGuardAtTop(!group->readEntry("bottom", !m_guardAtTop));
}

void Kolf::Windmill::save(KConfigGroup* group)
{
	Kolf::RectangleItem::save(group);
	group->writeEntry("speed", m_speed);
	group->writeEntry("bottom", !m_guardAtTop);
}

//END Kolf::Windmill

#include "obstacles.moc"
