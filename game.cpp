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

#include "game.h"
#include "itemfactory.h"
#include "kcomboboxdialog.h"
#include "obstacles.h"
#include "shape.h"

#include "tagaro/board.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFileDialog>
#include <QGlobalStatic>
#include <QLabel>
#include <QMouseEvent>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <KGameRenderer>
#include <KLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRandom>
#include <KgTheme>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/Contacts/b2Contact.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>

inline QString makeGroup(int id, int hole, const QString &name, int x, int y)
{
	return QStringLiteral("%1-%2@%3,%4|%5").arg(hole).arg(name).arg(x).arg(y).arg(id);
}

inline QString makeStateGroup(int id, const QString &name)
{
	return QStringLiteral("%1|%2").arg(name).arg(id);
}

class KolfContactListener : public b2ContactListener
{
	public:
		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) Q_DECL_OVERRIDE
		{
			Q_UNUSED(oldManifold)
			CanvasItem* citemA = static_cast<CanvasItem*>(contact->GetFixtureA()->GetBody()->GetUserData());
			CanvasItem* citemB = static_cast<CanvasItem*>(contact->GetFixtureB()->GetBody()->GetUserData());
			if (!CanvasItem::mayCollide(citemA, citemB))
				contact->SetEnabled(false);
		}
};

class KolfWorld : public b2World
{
	public:
		KolfWorld()
			: b2World(b2Vec2(0, 0), true) //parameters: no gravity, objects are allowed to sleep
		{
			SetContactListener(&m_listener);
		}
	private:
		KolfContactListener m_listener;
};

class KolfTheme : public KgTheme
{
	public:
		KolfTheme() : KgTheme("pics/default_theme.desktop")
		{
			setGraphicsPath(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("pics/default_theme.svgz")));
		}
};

class KolfRenderer : public KGameRenderer
{
	public:
		KolfRenderer() : KGameRenderer(new KolfTheme)
		{
			setStrategyEnabled(KGameRenderer::UseDiskCache, false);
			setStrategyEnabled(KGameRenderer::UseRenderingThreads, false);
		}
};

Q_GLOBAL_STATIC(KolfRenderer, g_renderer)
Q_GLOBAL_STATIC(KolfWorld, g_world)

KGameRenderer* Kolf::renderer()
{
	return g_renderer;
}

Tagaro::Board* Kolf::findBoard(QGraphicsItem* item_)
{
	//This returns the toplevel board instance in which the given parent resides.
	return item_ ? dynamic_cast<Tagaro::Board*>(item_->topLevelItem()) : 0;
}

b2World* Kolf::world()
{
	return g_world;
}

/////////////////////////

Putter::Putter(QGraphicsItem* parent, b2World* world)
: QGraphicsLineItem(parent)
, CanvasItem(world)
{
	setData(0, Rtti_Putter);
	setZBehavior(CanvasItem::FixedZValue, 10001);
	m_showGuideLine = true;
	oneDegree = M_PI / 180;
	guideLineLength = 9;
	putterWidth = 11;
	angle = 0;

	guideLine = new QGraphicsLineItem(this);
	guideLine->setPen(QPen(Qt::white));
	guideLine->setZValue(998.8);

	setPen(QPen(Qt::black, 4));
	maxAngle = 2 * M_PI;

	hideInfo();

	// this also sets Z
	resetAngles();
}

void Putter::showInfo()
{
	guideLine->setVisible(isVisible());
}

void Putter::hideInfo()
{
	guideLine->setVisible(m_showGuideLine? isVisible() : false);
}

void Putter::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
	guideLine->setPos(x(), y());
	CanvasItem::moveBy(dx, dy);
}

void Putter::setShowGuideLine(bool yes)
{
	m_showGuideLine = yes;
	setVisible(isVisible());
}

void Putter::setVisible(bool yes)
{
	QGraphicsLineItem::setVisible(yes);
	guideLine->setVisible(m_showGuideLine? yes : false);
}

void Putter::setOrigin(double _x, double _y)
{
	setVisible(true);
	setPos(_x, _y);
	guideLineLength = 9; //reset to default
	finishMe();
}

void Putter::setAngle(Ball *ball)
{
	angle = angleMap.contains(ball)? angleMap[ball] : 0;
	finishMe();
}

void Putter::go(Direction d, Amount amount)
{
	double addition = (amount == Amount_More? 6 * oneDegree : amount == Amount_Less? .5 * oneDegree : 2 * oneDegree);

	switch (d)
	{
		case Forwards:
			guideLineLength -= 1;
			guideLine->setVisible(false);
			break;
		case Backwards:
			guideLineLength += 1;
			guideLine->setVisible(false);
			break;
		case D_Left:
			angle += addition;
			if (angle > maxAngle)
				angle -= maxAngle;
			break;
		case D_Right:
			angle -= addition;
			if (angle < 0)
				angle = maxAngle - fabs(angle);
			break;
	}

	finishMe();
}

void Putter::finishMe()
{
	midPoint.setX(cos(angle) * guideLineLength);
	midPoint.setY(-sin(angle) * guideLineLength);

	QPointF start;
	QPointF end;

	if (midPoint.y() || !midPoint.x())
	{
		start.setX(midPoint.x() - putterWidth * sin(angle));
		start.setY(midPoint.y() - putterWidth * cos(angle));
		end.setX(midPoint.x() + putterWidth * sin(angle));
		end.setY(midPoint.y() + putterWidth * cos(angle));
	}
	else
	{
		start.setX(midPoint.x());
		start.setY(midPoint.y() + putterWidth);
		end.setY(midPoint.y() - putterWidth);
		end.setX(midPoint.x());
	}

	guideLine->setLine(midPoint.x(), midPoint.y(), -cos(angle) * guideLineLength * 4, sin(angle) * guideLineLength * 4);

	setLine(start.x(), start.y(), end.x(), end.y());
}

/////////////////////////

HoleConfig::HoleConfig(HoleInfo *holeInfo, QWidget *parent)
: Config(parent)
{
	this->holeInfo = holeInfo;

	QVBoxLayout *layout = new QVBoxLayout(this);

	QHBoxLayout *hlayout = new QHBoxLayout;
	layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Course name: "), this));
	KLineEdit *nameEdit = new KLineEdit(holeInfo->untranslatedName(), this);
	hlayout->addWidget(nameEdit);
	connect(nameEdit, &KLineEdit::textChanged, this, &HoleConfig::nameChanged);

	hlayout = new QHBoxLayout;
	layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Course author: "), this));
	KLineEdit *authorEdit = new KLineEdit(holeInfo->author(), this);
	hlayout->addWidget(authorEdit);
	connect(authorEdit, &KLineEdit::textChanged, this, &HoleConfig::authorChanged);

	layout->addStretch();

	hlayout = new QHBoxLayout;
	layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Par:"), this));
	QSpinBox *par = new QSpinBox(this);
	par->setRange( 1, 15 );
	par->setSingleStep( 1 );
	par->setValue(holeInfo->par());
	hlayout->addWidget(par);
	connect(par, QOverload<int>::of(&QSpinBox::valueChanged), this, &HoleConfig::parChanged);
	hlayout->addStretch();

	hlayout->addWidget(new QLabel(i18n("Maximum:"), this));
	QSpinBox *maxstrokes = new QSpinBox(this);
	maxstrokes->setRange( holeInfo->lowestMaxStrokes(), 30 );
	maxstrokes->setSingleStep( 1 );
	maxstrokes->setWhatsThis( i18n("Maximum number of strokes player can take on this hole."));
	maxstrokes->setToolTip( i18n("Maximum number of strokes"));
	maxstrokes->setSpecialValueText(i18n("Unlimited"));
	maxstrokes->setValue(holeInfo->maxStrokes());
	hlayout->addWidget(maxstrokes);
	connect(maxstrokes, QOverload<int>::of(&QSpinBox::valueChanged), this, &HoleConfig::maxStrokesChanged);

	QCheckBox *check = new QCheckBox(i18n("Show border walls"), this);
	check->setChecked(holeInfo->borderWalls());
	layout->addWidget(check);
	connect(check, &QCheckBox::toggled, this, &HoleConfig::borderWallsChanged);
}

void HoleConfig::authorChanged(const QString &newauthor)
{
	holeInfo->setAuthor(newauthor);
	changed();
}

void HoleConfig::nameChanged(const QString &newname)
{
	holeInfo->setName(newname);
	holeInfo->setUntranslatedName(newname);
	changed();
}

void HoleConfig::parChanged(int newpar)
{
	holeInfo->setPar(newpar);
	changed();
}

void HoleConfig::maxStrokesChanged(int newms)
{
	holeInfo->setMaxStrokes(newms);
	changed();
}

void HoleConfig::borderWallsChanged(bool yes)
{
	holeInfo->borderWallsChanged(yes);
	changed();
}

/////////////////////////

StrokeCircle::StrokeCircle(QGraphicsItem *parent)
: QGraphicsItem(parent)
{
	dvalue = 0;
	dmax = 360;
	iwidth = 100;
	iheight = 100;
	ithickness = 8;
	setZValue(10000);

	setSize(QSizeF(80, 80));
	setThickness(8);
}

void StrokeCircle::setValue(double v)
{
	dvalue = v;
	if (dvalue > dmax)
		dvalue = dmax;

	update();
}

double StrokeCircle::value()
{
	return dvalue;
}

bool StrokeCircle::collidesWithItem(const QGraphicsItem*, Qt::ItemSelectionMode) const { return false; }

QRectF StrokeCircle::boundingRect() const { return QRectF(x(), y(), iwidth, iheight); }

void StrokeCircle::setMaxValue(double m)
{
	dmax = m;
	if (dvalue > dmax)
		dvalue = dmax;
}
void StrokeCircle::setSize(const QSizeF& size)
{
	if (size.width() > 0)
		iwidth = size.width();
	if (size.height() > 0)
		iheight = size.height();
}
void StrokeCircle::setThickness(double t)
{
	if (t > 0)
		ithickness = t;
}

double StrokeCircle::thickness() const
{
	return ithickness;
}

double StrokeCircle::width() const
{
	return iwidth;
}

double StrokeCircle::height() const
{
	return iheight;
}

void StrokeCircle::paint (QPainter *p, const QStyleOptionGraphicsItem *, QWidget * )
{
	int al = (int)((dvalue * 360 * 16) / dmax);
	int length, deg;
	if (al < 0)
	{
		deg = 270 * 16;
		length = -al;
	}
	else if (al <= (270 * 16))
	{
		deg = 270 * 16 - al;
		length = al;
	}
	else
	{
		deg = (360 * 16) - (al - (270 * 16));
		length = al;
	}

	p->setBrush(QBrush(Qt::black, Qt::NoBrush));
	p->setPen(QPen(Qt::white, ithickness / 2));
	p->drawEllipse(QRectF(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness));

	if(dvalue>=0)
		p->setPen(QPen(QColor((int)((0xff * dvalue) / dmax), 0, (int)(0xff - (0xff * dvalue) / dmax)), ithickness));
	else
		p->setPen(QPen(QColor("black"), ithickness));

	p->drawArc(QRectF(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness), deg, length);

	p->setPen(QPen(Qt::white, 1));
	p->drawEllipse(QRectF(x(), y(), iwidth, iheight));
	p->drawEllipse(QRectF(x() + ithickness, y() + ithickness, iwidth - ithickness * 2, iheight - ithickness * 2));
	p->setPen(QPen(Qt::white, 3));
	p->drawLine(QPointF(x() + iwidth / 2, y() + iheight - ithickness * 1.5), QPointF(x() + iwidth / 2, y() + iheight));
	p->drawLine(QPointF(x() + iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 + iheight / 20), QPointF(x() + iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 - iheight / 20));
	p->drawLine(QPointF(x() + iwidth - iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 + iheight / 20), QPointF(x() + iwidth - iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 - iheight / 20));
}
/////////////////////////////////////////

KolfGame::KolfGame(const Kolf::ItemFactory& factory, PlayerList *players, const QString &filename, QWidget *parent)
: QGraphicsView(parent),
 m_factory(factory),
 m_soundBlackHole(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/blackhole.wav"))),
 m_soundBlackHoleEject(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/blackholeeject.wav"))),
 m_soundBlackHolePutIn(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/blackholeputin.wav"))),
 m_soundBumper(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/bumper.wav"))),
 m_soundHit(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/hit.wav"))),
 m_soundHoled(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/holed.wav"))),
 m_soundHoleINone(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/holeinone.wav"))),
 m_soundPuddle(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/puddle.wav"))),
 m_soundWall(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/wall.wav"))),
 m_soundWooHoo(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("sounds/woohoo.wav"))),
 holeInfo(g_world)
{
	setRenderHint(QPainter::Antialiasing);
	// for mouse control
	setMouseTracking(true);
	viewport()->setMouseTracking(true);
	setFrameShape(NoFrame);

	regAdv = false;
	curHole = 0; // will get ++'d
	cfg = 0;
	setFilename(filename);
	this->players = players;
	curPlayer = players->end();
	curPlayer--; // will get ++'d to end and sent back
	// to beginning
	paused = false;
	modified = false;
	inPlay = false;
	putting = false;
	stroking = false;
	editing = false;
	strict = false;
	lastDelId = -1;
	m_showInfo = false;
	ballStateList.canUndo = false;
	dontAddStroke = false;
	addingNewHole = false;
	scoreboardHoles = 0;
	infoShown = false;
	m_useMouse = true;
	m_useAdvancedPutting = true;
	m_sound = true;
	m_ignoreEvents = false;
	highestHole = 0;
	recalcHighestHole = false;
	banner = 0;

	holeInfo.setGame(this);
	holeInfo.setAuthor(i18n("Course Author"));
	holeInfo.setName(i18n("Course Name"));
	holeInfo.setUntranslatedName(i18n("Course Name"));
	holeInfo.setMaxStrokes(10);
	holeInfo.borderWallsChanged(true);

	// width and height are the width and height of the scene
	// in easy storage
	width = 400;
	height = 400;

	margin = 10;

	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(width, height);
	QSizePolicy sizePolicy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setSizePolicy(sizePolicy);

	setContentsMargins(margin, margin, margin, margin);

	course = new Tagaro::Scene(Kolf::renderer(), QStringLiteral("grass"));
	course->setMainView(this); //this does this->setScene(course)
	courseBoard = new Tagaro::Board;
	courseBoard->setLogicalSize(QSizeF(400, 400));
	course->addItem(courseBoard);

	if( filename.contains( QLatin1String("intro") ) )
	{
		banner = new Tagaro::SpriteObjectItem(Kolf::renderer(), QStringLiteral("intro_foreground"), courseBoard);
		banner->setSize(400, 132);
		banner->setPos(0, 32);
		banner->setZValue(3); //on the height of a puddle (above slopes and sands, below any objects)
	}

	adjustSize();

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		Ball* ball = (*it).ball();
		ball->setParentItem(courseBoard);
		m_topLevelQItems << ball;
		m_moveableQItems << ball;
	}

	QFont font = QApplication::font();
	font.setPixelSize(12);

	// create the advanced putting indicator
	strokeCircle = new StrokeCircle(courseBoard);
	strokeCircle->setPos(width - 90, height - 90);
	strokeCircle->setVisible(false);
	strokeCircle->setValue(0);
	strokeCircle->setMaxValue(360); 

	// whiteBall marks the spot of the whole whilst editing
	whiteBall = new Ball(courseBoard, g_world);
	whiteBall->setGame(this);
	whiteBall->setColor(Qt::white);
	whiteBall->setVisible(false);
	whiteBall->setDoDetect(false);
	m_topLevelQItems << whiteBall;
	m_moveableQItems << whiteBall;

	int highestLog = 0;

	// if players have scores from loaded game, move to last hole
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		if ((int)(*it).scores().count() > highestLog)
			highestLog = (*it).scores().count();

		(*it).ball()->setGame(this);
	}

	// here only for saved games
	if (highestLog)
		curHole = highestLog;

	putter = new Putter(courseBoard, g_world);

	// border walls:

	// horiz
	addBorderWall(QPoint(margin, margin), QPoint(width - margin, margin));
	addBorderWall(QPoint(margin, height - margin - 1), QPoint(width - margin, height - margin - 1));

	// vert
	addBorderWall(QPoint(margin, margin), QPoint(margin, height - margin));
	addBorderWall(QPoint(width - margin - 1, margin), QPoint(width - margin - 1, height - margin));

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &KolfGame::timeout);
	timerMsec = 300;

	fastTimer = new QTimer(this);
	connect(fastTimer, &QTimer::timeout, this, &KolfGame::fastTimeout);
	fastTimerMsec = 11;

	autoSaveTimer = new QTimer(this);
	connect(autoSaveTimer, &QTimer::timeout, this, &KolfGame::autoSaveTimeout);
	autoSaveMsec = 5 * 1000 * 60; // 5 min autosave

	// setUseAdvancedPutting() sets maxStrength!
	setUseAdvancedPutting(false);

	putting = false;
	putterTimer = new QTimer(this);
	connect(putterTimer, &QTimer::timeout, this, &KolfGame::putterTimeout);
	putterTimerMsec = 20;
}

void KolfGame::playSound(Sound soundType)
{
	if (m_sound) {
		switch (soundType) {
			case Sound::BlackHole:
				m_soundBlackHole.start();
				break;
			case Sound::BlackHoleEject:
				m_soundBlackHoleEject.start();
				break;
			case Sound::BlackHolePutIn:
				m_soundBlackHolePutIn.start();
				break;
			case Sound::Bumper:
				m_soundBumper.start();
				break;
			case Sound::Hit:
				m_soundHit.start();
				break;
			case Sound::Holed:
				m_soundHoled.start();
				break;
			case Sound::HoleINone:
				m_soundHoleINone.start();
				break;
			case Sound::Puddle:
				m_soundPuddle.start();
				break;
			case Sound::Wall:
				m_soundWall.start();
				break;
			case Sound::WooHoo:
				m_soundWooHoo.start();
				break;
			default:
				qWarning() << "There was a request to play an unknown sound.";
				break;
		}
	}
}

void KolfGame::startFirstHole(int hole)
{
	if (curHole > 0) // if there was saved game, sync scoreboard
		// with number of holes
	{
		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			cfgGroup = KConfigGroup(cfg->group(QStringLiteral("%1-hole@-50,-50|0").arg(scoreboardHoles + 1)));
			emit newHole(cfgGroup.readEntry("par", 3));
		}

		// lets load all of the scores from saved game if there are any
		for (int hole = 1; hole <= curHole; ++hole)
			for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
				emit scoreChanged((*it).id(), hole, (*it).score(hole));
	}

	curHole = hole - 1;

	// this increments curHole, etc
	recalcHighestHole = true;
	startNextHole();
	paused = true;
	unPause();
}

void KolfGame::setFilename(const QString &filename)
{
	this->filename = filename;
	delete cfg;
	cfg = new KConfig(filename, KConfig::NoGlobals);
}

KolfGame::~KolfGame()
{
	QList<QGraphicsItem*> itemsCopy(m_topLevelQItems); //this list will be modified soon, so take a copy
	foreach (QGraphicsItem* item, itemsCopy)
	{
		CanvasItem* citem = dynamic_cast<CanvasItem*>(item);
		delete citem;
	}

	delete cfg;
}

void KolfGame::setModified(bool mod)
{
	modified = mod;
	emit modifiedChanged(mod);
}

void KolfGame::pause()
{
	if (paused)
	{
		// play along with people who call pause() again, instead of unPause()
		unPause();
		return;
	}

	paused = true;
	timer->stop();
	fastTimer->stop();
	putterTimer->stop();
}

void KolfGame::unPause()
{
	if (!paused)
		return;

	paused = false;

	timer->start(timerMsec);
	fastTimer->start(fastTimerMsec);

	if (putting || stroking)
		putterTimer->start(putterTimerMsec);
}

void KolfGame::addBorderWall(const QPoint &start, const QPoint &end)
{
	Kolf::Wall *wall = new Kolf::Wall(courseBoard, g_world);
	wall->setLine(QLineF(start, end));
	wall->setVisible(true);
	wall->setGame(this);
	//change Z value to something very high so that border walls
	//really keep the balls inside the course
	wall->setZBehavior(CanvasItem::FixedZValue, 10000);
	borderWalls.append(wall);
}

void KolfGame::handleMouseDoubleClickEvent(QMouseEvent *e)
{
	// allow two fast single clicks
	handleMousePressEvent(e);
}

void KolfGame::handleMousePressEvent(QMouseEvent *e)
{
	if (m_ignoreEvents)
		return;

	if (editing)
	{
		//at this point, QGV::mousePressEvent and thus the interaction
		//with overlays has already been done; we therefore know that
		//the user has clicked into free space
		setSelectedItem(0);
		return;
	}
	else
	{
		if (m_useMouse)
		{
			if (!inPlay && e->button() == Qt::LeftButton)
				puttPress();
			else if (e->button() == Qt::RightButton)
				toggleShowInfo();
		}
	}

	setFocus();
}

QPoint KolfGame::viewportToViewport(const QPoint &p)
{
	//convert viewport coordinates to board coordinates
	return courseBoard->deviceTransform(viewportTransform()).inverted().map(p);
}

// the following four functions are needed to handle both
// border presses and regular in-course presses

void KolfGame::mouseReleaseEvent(QMouseEvent * e)
{
	e->setAccepted(false);
	QGraphicsView::mouseReleaseEvent(e);
	if (e->isAccepted())
		return;

	QMouseEvent fixedEvent (QEvent::MouseButtonRelease, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMouseReleaseEvent(&fixedEvent);
	e->accept();
}

void KolfGame::mousePressEvent(QMouseEvent * e)
{
	e->setAccepted(false);
	QGraphicsView::mousePressEvent(e);
	if (e->isAccepted())
		return;

	QMouseEvent fixedEvent (QEvent::MouseButtonPress, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMousePressEvent(&fixedEvent);
	e->accept();
}

void KolfGame::mouseDoubleClickEvent(QMouseEvent * e)
{
	e->setAccepted(false);
	QGraphicsView::mouseDoubleClickEvent(e);
	if (e->isAccepted())
		return;

	QMouseEvent fixedEvent (QEvent::MouseButtonDblClick, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMouseDoubleClickEvent(&fixedEvent);
	e->accept();
}

void KolfGame::mouseMoveEvent(QMouseEvent * e)
{
	e->setAccepted(false);
	QGraphicsView::mouseMoveEvent(e);
	if (e->isAccepted())
		return;

	QMouseEvent fixedEvent (QEvent::MouseMove, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMouseMoveEvent(&fixedEvent);
	e->accept();
}

void KolfGame::handleMouseMoveEvent(QMouseEvent *e)
{
	if (!editing && !inPlay && putter && !m_ignoreEvents)
	{
		// mouse moving of putter
		updateMouse();
		e->accept();
	}
}

void KolfGame::updateMouse()
{
	// don't move putter if in advanced putting sequence
	if (!m_useMouse || ((stroking || putting) && m_useAdvancedPutting))
		return;

	const QPointF cursor = viewportToViewport(mapFromGlobal(QCursor::pos()));
	const QPointF ball((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	putter->setAngle(-Vector(cursor - ball).direction());
}

void KolfGame::handleMouseReleaseEvent(QMouseEvent *e)
{
	setCursor(Qt::ArrowCursor);

	if (editing)
	{
		emit newStatusText(QString());
	}

	if (m_ignoreEvents)
		return;

	if (!editing && m_useMouse)
	{
		if (!inPlay && e->button() == Qt::LeftButton)
			puttRelease();
		else if (e->button() == Qt::RightButton)
			toggleShowInfo();
	}

	setFocus();
}

void KolfGame::keyPressEvent(QKeyEvent *e)
{
	if (inPlay || editing || m_ignoreEvents)
		return;

	switch (e->key())
	{
		case Qt::Key_Up:
			if (!e->isAutoRepeat())
				toggleShowInfo();
			break;

		case Qt::Key_Escape:
			putting = false;
			stroking = false;
			finishStroking = false;
			strokeCircle->setVisible(false); 
			putterTimer->stop();
			putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
			break;

		case Qt::Key_Left:
		case Qt::Key_Right:
			// don't move putter if in advanced putting sequence
			if ((!stroking && !putting) || !m_useAdvancedPutting)
				putter->go(e->key() == Qt::Key_Left? D_Left : D_Right, e->modifiers() & Qt::ShiftModifier? Amount_More : e->modifiers() & Qt::ControlModifier? Amount_Less : Amount_Normal);
			break;

		case Qt::Key_Space: case Qt::Key_Down:
			puttPress();
			break;

		default:
			break;
	}
}

void KolfGame::toggleShowInfo()
{
	setShowInfo(!m_showInfo);
}

void KolfGame::updateShowInfo()
{
	setShowInfo(m_showInfo);
}

void KolfGame::setShowInfo(bool yes)
{
	m_showInfo = yes;
	QList<QGraphicsItem*> infoItems;
	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
			infoItems << citem->infoItems();
	}
	foreach (QGraphicsItem* qitem, infoItems)
		qitem->setVisible(m_showInfo);
}

void KolfGame::puttPress()
{
	// Advanced putting: 1st click start putting sequence, 2nd determine strength, 3rd determine precision

	if (!putting && !stroking && !inPlay)
	{
		puttCount = 0;
		puttReverse = false;
		putting = true;
		stroking = false;
		strength = 0;
		if (m_useAdvancedPutting)
		{
			strokeCircle->setValue(0); 
			int pw = (int)(putter->line().x2() - putter->line().x1());
			if (pw < 0) pw = -pw;
			int px = (int)putter->x() + pw / 2;
			int py = (int)putter->y();
			if (px > width / 2 && py < height / 2) 
				strokeCircle->setPos(px/2 - pw / 2 - 5 - strokeCircle->width()/2, py/2 + 5);
			else if (px > width / 2) 
				strokeCircle->setPos(px/2 - pw / 2 - 5 - strokeCircle->width()/2, py/2 - 5 - strokeCircle->height()/2);
			else if (py < height / 2) 
				strokeCircle->setPos(px/2 + pw / 2 + 5, py/2 + 5);
			else 
				strokeCircle->setPos(px/2 + pw / 2 + 5, py/2 - 5 - strokeCircle->height()/2);
			strokeCircle->setVisible(true); 
		}
		putterTimer->start(putterTimerMsec);
	}
	else if (m_useAdvancedPutting && putting && !editing)
	{
		putting = false;
		stroking = true;
		puttReverse = false;
		finishStroking = false;
	}
	else if (m_useAdvancedPutting && stroking)
	{
		finishStroking = true;
		putterTimeout();
	}
}

void KolfGame::keyReleaseEvent(QKeyEvent *e)
{
	if (e->isAutoRepeat() || m_ignoreEvents)
		return;

	if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Down)
		puttRelease();
	else if ((e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) && !(e->modifiers() & Qt::ControlModifier))
	{
		if (editing && selectedItem)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(selectedItem);
			if (!citem)
				return;
			QGraphicsItem *item = dynamic_cast<QGraphicsItem *>(citem);
			if (citem && !dynamic_cast<Ball*>(item))
			{
				lastDelId = citem->curId();

				m_topLevelQItems.removeAll(item);
				m_moveableQItems.removeAll(item);
				delete citem;
				setSelectedItem(0);

				setModified(true);
			}
		}
	}
	else if (e->key() == Qt::Key_I || e->key() == Qt::Key_Up)
		toggleShowInfo();
}

void KolfGame::resizeEvent( QResizeEvent* ev )
{
	int newW = ev->size().width();
	int newH = ev->size().height();
	int oldW = ev->oldSize().width();
	int oldH = ev->oldSize().height();

	if(oldW<=0 || oldH<=0) //this is the first draw so no point wasting resources resizing yet
		return;
	else if( (oldW==newW) && (oldH==newH) )
		return;

	int setSize = qMin(newW, newH);
	QGraphicsView::resize(setSize, setSize); //make sure new size is square
}

void KolfGame::puttRelease()
{
	if (!m_useAdvancedPutting && putting && !editing)
	{
		putting = false;
		stroking = true;
	}
}

void KolfGame::stoppedBall()
{
	if (!inPlay)
	{
		inPlay = true;
		dontAddStroke = true;
	}
}

void KolfGame::timeout()
{
	Ball *curBall = (*curPlayer).ball();

	// test if the ball is gone
	// in this case we want to stop the ball and
	// later undo the shot
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
                //QGV handles management of dirtied rects for us
		//course->update();

		if (!QRectF(QPointF(), courseBoard->logicalSize()).contains((*it).ball()->pos()))
		{
			(*it).ball()->setState(Stopped);

			// don't do it if he's past maxStrokes
			if ((*it).score(curHole) < holeInfo.maxStrokes() - 1 || !holeInfo.hasMaxStrokes())
			{
				loadStateList();
			}
			shotDone();

			return;
		}
	}

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		if ((*it).ball()->forceStillGoing() || ((*it).ball()->curState() == Rolling && Vector((*it).ball()->velocity()).magnitude() > 0 && (*it).ball()->isVisible()))
			return;

	int curState = curBall->curState();
	if (curState == Stopped && inPlay)
	{
		inPlay = false;
		QTimer::singleShot(0, this, &KolfGame::shotDone);
	}

	if (curState == Holed && inPlay)
	{
		emit inPlayEnd();

		int curScore = (*curPlayer).score(curHole);
		if (!dontAddStroke)
			curScore++;

		if (curScore == 1)
		{
			playSound(Sound::HoleINone);
		}
		else if (curScore <= holeInfo.par())
		{
			playSound(Sound::WooHoo);
		}

		(*curPlayer).ball()->setZValue((*curPlayer).ball()->zValue() + .1 - (.1)/(curScore));

		if (allPlayersDone())
		{
			inPlay = false;

			if (curHole > 0 && !dontAddStroke)
			{
				(*curPlayer).addStrokeToHole(curHole);
				emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));
			}
			QTimer::singleShot(600, this, &KolfGame::holeDone);
		}
		else
		{
			inPlay = false;
			QTimer::singleShot(0, this, &KolfGame::shotDone);
		}
	}
}

void KolfGame::fastTimeout()
{
	// do regular advance every other time
	if (regAdv)
		course->advance();
	regAdv = !regAdv;

	if (editing)
		return;

	// do Box2D advance
	//Because there are so much CanvasItems out there, there is currently no
	//easy and/or systematic approach to iterate over all of them, except for
	//using the b2Bodies available on the world.

	//prepare simulation
	for (b2Body* body = g_world->GetBodyList(); body; body = body->GetNext())
	{
		CanvasItem* citem = static_cast<CanvasItem*>(body->GetUserData());
		if (citem)
		{
			citem->startSimulation();
			//HACK: the following should not be necessary at this point
			QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(citem);
			if (qitem)
				citem->updateZ(qitem);
		}
	}
	//step world
	//NOTE: I previously set timeStep to 1.0 so that CItem's velocity()
	//corresponds to the position change per step. In this case, the
	//velocity would be scaled by Kolf::Box2DScaleFactor, which would result in
	//very small velocities (below Box2D's internal cutoff thresholds!) for
	//usual movements. Therefore, we apply the scaling to the timestep instead.
	const double timeStep = 1.0 * Kolf::Box2DScaleFactor;
	g_world->Step(timeStep, 10, 10); //parameters 2/3 = iteration counts (TODO: optimize)
	//conclude simulation
	for (b2Body* body = g_world->GetBodyList(); body; body = body->GetNext())
	{
		CanvasItem* citem = static_cast<CanvasItem*>(body->GetUserData());
		if (citem)
		{
			citem->endSimulation();
		}
	}
}

void KolfGame::ballMoved()
{
	if (putter->isVisible())
	{
		putter->setPos((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		updateMouse();
	}
}

void KolfGame::putterTimeout()
{
	if (inPlay || editing)
		return;

	if (m_useAdvancedPutting)
	{
		if (putting)
		{
			const qreal base = 2.0;

			if (puttReverse && strength <= 0)
			{
				// aborted
				putting = false;
				strokeCircle->setVisible(false); 
			}
			else if (strength > maxStrength || puttReverse)
			{
				// decreasing strength as we've reached the top
				puttReverse = true;
				strength -= pow(base, qreal(strength / maxStrength)) - 1.8;
				if ((int)strength < puttCount * 2)
				{
					puttCount--;
					if (puttCount >= 0)
						putter->go(Forwards);
				}
			}
			else
			{
				// make the increase at high strength faster
				strength += pow(base, strength / maxStrength) - .3;
				if ((int)strength > puttCount * 2)
				{
					putter->go(Backwards);
					puttCount++;
				}
			}
			// make the visible steps at high strength smaller
			strokeCircle->setValue(pow(strength / maxStrength, 0.8) * 360); 
		}
		else if (stroking)
		{
			double al = strokeCircle->value(); 
			if (al >= 45)
				al -= 0.2 + strength / 50 + al / 100;
			else
				al -= 0.2 + strength / 50;

			if (puttReverse)
			{
				// show the stroke
				puttCount--;
				if (puttCount >= 0)
					putter->go(Forwards);
				else
				{
					strokeCircle->setVisible(false);
					finishStroking = false;
					putterTimer->stop();
					putting = false;
					stroking = false;
					shotStart();
				}
			}
			else if (al < -45 || finishStroking)
			{
				strokeCircle->setValue(al); 
				int deg;
				// if > 45 or < -45 then bad stroke
				if (al > 45)
				{
					deg = putter->curDeg() - 45 + rand() % 90;
					strength -= qrand() % (int)strength;
				}
				else if (!finishStroking)
				{
					deg = putter->curDeg() - 45 + rand() % 90;
					strength -= qrand() % (int)strength;
				}
				else
					deg = putter->curDeg() + (int)(strokeCircle->value() / 3);

				if (deg < 0)
					deg += 360;
				else if (deg > 360)
					deg -= 360;

				putter->setDeg(deg);
				puttReverse = true;
			}
			else
			{
				strokeCircle->setValue(al);
				putterTimer->start(putterTimerMsec/10);
			}
		}
	}
	else
	{
		if (putting)
		{
			putter->go(Backwards);
			puttCount++;
			strength += 1.5;
			if (strength > maxStrength)
			{
				putting = false;
				stroking = true;
			}
		}
		else if (stroking)
		{
			if (putter->curLen() < (*curPlayer).ball()->height() + 2)
			{
				stroking = false;
				putterTimer->stop();
				putting = false;
				stroking = false;
				shotStart();
			}

			putter->go(Forwards);
			putterTimer->start(putterTimerMsec/10);
		}
	}
}

void KolfGame::autoSaveTimeout()
{
	// this should be a config option
	// until it is i'll disable it
	if (editing)
	{
		//save();
	}
}

void KolfGame::recreateStateList()
{
	savedState.clear();
	foreach (QGraphicsItem* item, m_topLevelQItems)
	{
		if (dynamic_cast<Ball*>(item)) continue; //see below
		CanvasItem* citem = dynamic_cast<CanvasItem*>(item);
		if (citem)
		{
			const QString key = makeStateGroup(citem->curId(), citem->name());
			savedState.insert(key, item->pos());
		}
	}

	ballStateList.clear();
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		ballStateList.append((*it).stateInfo(curHole));

	ballStateList.canUndo = true;
}

void KolfGame::undoShot()
{
	if (ballStateList.canUndo)
		loadStateList();
}

void KolfGame::loadStateList()
{
	foreach (QGraphicsItem* item, m_topLevelQItems)
	{
		if (dynamic_cast<Ball*>(item)) continue; //see below
		CanvasItem* citem = dynamic_cast<CanvasItem*>(item);
		if (citem)
		{
			const QString key = makeStateGroup(citem->curId(), citem->name());
			const QPointF currentPos = item->pos();
			const QPointF posDiff = savedState.value(key, currentPos) - currentPos;
			citem->moveBy(posDiff.x(), posDiff.y());
		}
	}

	for (BallStateList::Iterator it = ballStateList.begin(); it != ballStateList.end(); ++it)
	{
		BallStateInfo info = (*it);
		Player &player = (*(players->begin() + (info.id - 1) ));
		player.ball()->setPos(info.spot.x(), info.spot.y());
		player.ball()->setBeginningOfHole(info.beginningOfHole);
		if ((*curPlayer).id() == info.id)
			ballMoved();
		else
			player.ball()->setVisible(!info.beginningOfHole);
		player.setScoreForHole(info.score, curHole);
		player.ball()->setState(info.state);
		emit scoreChanged(info.id, curHole, info.score);
	}
}

void KolfGame::shotDone()
{
	inPlay = false;
	emit inPlayEnd();
	setFocus();

	Ball *ball = (*curPlayer).ball();

	if (!dontAddStroke && (*curPlayer).numHoles())
		(*curPlayer).addStrokeToHole(curHole);

	dontAddStroke = false;

	// do hack stuff, shouldn't be done here

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		if ((*it).ball()->addStroke())
		{
			for (int i = 1; i <= (*it).ball()->addStroke(); ++i)
				(*it).addStrokeToHole(curHole);

			// emit that we have a new stroke count
			emit scoreChanged((*it).id(), curHole, (*it).score(curHole));
		}
		(*it).ball()->setAddStroke(0);
	}

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		Ball *ball = (*it).ball();

		if (ball->curState() == Holed)
			continue;

		Vector oldVelocity;
		if (ball->placeOnGround(oldVelocity))
		{
			ball->setPlaceOnGround(false);

			QStringList options;
			const QString placeOutside = i18n("Drop Outside of Hazard");
			const QString rehit = i18n("Rehit From Last Location");
			options << placeOutside << rehit;
			const QString choice = KComboBoxDialog::getItem(i18n("What would you like to do for your next shot?"), i18n("%1 is in a Hazard", (*it).name()), options, placeOutside, QStringLiteral("hazardOptions"));

			if (choice == placeOutside)
			{
				(*it).ball()->setDoDetect(false);

				QPointF pos = ball->pos();
				//normalize old velocity
				const QPointF v = oldVelocity / oldVelocity.magnitude();

				while (1)
				{
					QList<QGraphicsItem *> list = ball->collidingItems();
					bool keepMoving = false;
					while (!list.isEmpty())
					{
						QGraphicsItem *item = list.takeFirst();
						if (item->data(0) == Rtti_DontPlaceOn)
							keepMoving = true;
					}
					if (!keepMoving)
						break;

					const qreal movePixel = 3.0;
					pos -= v * movePixel;
					ball->setPos(pos);
				}
			}
			else if (choice == rehit)
			{
				for (BallStateList::Iterator it = ballStateList.begin(); it != ballStateList.end(); ++it)
				{
					if ((*it).id == (*curPlayer).id())
					{
						if ((*it).beginningOfHole)
							ball->setPos(whiteBall->x(), whiteBall->y());
						else
							ball->setPos((*it).spot.x(), (*it).spot.y());

						break;
					}
				}
			}

			ball->setVisible(true);
			ball->setState(Stopped); 

			(*it).ball()->setDoDetect(true);
			ball->collisionDetect();
		}
	}

	// emit again
	emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));

	if(ball->curState() == Rolling) {
		inPlay = true; 
		return;
	}

	ball->setVelocity(Vector());

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		Ball *ball = (*it).ball();

		int curStrokes = (*it).score(curHole);
		if (curStrokes >= holeInfo.maxStrokes() && holeInfo.hasMaxStrokes())
		{
			ball->setState(Holed);
			ball->setVisible(false);

			// move to center in case he/she hit out
			ball->setPos(width / 2, height / 2);
			playerWhoMaxed = (*it).name();

			if (allPlayersDone())
			{
				startNextHole();
				QTimer::singleShot(100, this, &KolfGame::emitMax);
				return;
			}

			QTimer::singleShot(100, this, &KolfGame::emitMax);
		}
	}

	// change player to next player
	// skip player if he's Holed
	do
	{
		curPlayer++;
		if (curPlayer == players->end())
			curPlayer = players->begin();
	}
	while ((*curPlayer).ball()->curState() == Holed);

	emit newPlayersTurn(&(*curPlayer));

	(*curPlayer).ball()->setVisible(true);

	inPlay = false;
	(*curPlayer).ball()->collisionDetect();

	putter->setAngle((*curPlayer).ball());
	putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	updateMouse();
}

void KolfGame::emitMax()
{
	emit maxStrokesReached(playerWhoMaxed);
}

void KolfGame::startBall(const Vector &velocity)
{
	playSound(Sound::Hit);
	emit inPlayStart();
	putter->setVisible(false);

	(*curPlayer).ball()->setState(Rolling);
	(*curPlayer).ball()->setVelocity(velocity);
	(*curPlayer).ball()->shotStarted();

	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
			citem->shotStarted();
	}

	inPlay = true;
}

void KolfGame::shotStart()
{
	// ensure we never hit the ball back into the hole which
	// can cause hole skippage
	if ((*curPlayer).ball()->curState() == Holed)
		return;

	// save state
	recreateStateList();

	putter->saveAngle((*curPlayer).ball());
	strength /= 8;
	if (!strength)
		strength = 1;

	//kDebug(12007) << "Start started. BallX:" << (*curPlayer).ball()->x() << ", BallY:" << (*curPlayer).ball()->y() << ", Putter Angle:" << putter->curAngle() << ", Vector Strength: " << strength;

	(*curPlayer).ball()->collisionDetect();

	startBall(Vector::fromMagnitudeDirection(strength, -(putter->curAngle() + M_PI)));

	addHoleInfo(ballStateList);
}

void KolfGame::addHoleInfo(BallStateList &list)
{
	list.player = (*curPlayer).id();
	list.vector = (*curPlayer).ball()->velocity();
	list.hole = curHole;
}

void KolfGame::sayWhosGoing()
{
	if (players->count() >= 2)
	{
		KMessageBox::information(this, i18n("%1 will start off.", (*curPlayer).name()), i18n("New Hole"), QStringLiteral("newHole"));
	}
}

void KolfGame::holeDone()
{
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		(*it).ball()->setVisible(false);
	startNextHole();
	sayWhosGoing();
}

// this function is WAY too smart for it's own good
// ie, bad design :-(
void KolfGame::startNextHole()
{
	setFocus();

	bool reset = true;
	if (askSave(true))
	{
		if (allPlayersDone())
		{
			// we'll reload this hole, but not reset
			curHole--;
			reset = false;
		}
		else
			return;
	}
	else
		setModified(false);

	pause();

	dontAddStroke = false;

	inPlay = false;
	timer->stop();
	putter->resetAngles();

	int oldCurHole = curHole;
	curHole++;
	emit currentHole(curHole);

	if (reset)
	{
		whiteBall->setPos(width/2, height/2);
		holeInfo.borderWallsChanged(true);
	}

	int leastScore = INT_MAX;

	// to get the first player to go first on every hole,
	// don't do the score stuff below
	curPlayer = players->begin();

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		if (curHole > 1)
		{
			bool ahead = false;
			if ((*it).lastScore() != 0)
			{
				if ((*it).lastScore() < leastScore)
					ahead = true;
				else if ((*it).lastScore() == leastScore)
				{
					for (int i = curHole - 1; i > 0; --i)
					{
						while(i > (*it).scores().size())
							i--;

						const int thisScore = (*it).score(i);
						const int thatScore = (*curPlayer).score(i);
						if (thisScore < thatScore)
						{
							ahead = true;
							break;
						}
						else if (thisScore > thatScore)
							break;
					}
				}
			}

			if (ahead)
			{
				curPlayer = it;
				leastScore = (*it).lastScore();
			}
		}

		if (reset)
			(*it).ball()->setPos(width / 2, height / 2);
		else
			(*it).ball()->setPos(whiteBall->x(), whiteBall->y());

		(*it).ball()->setState(Stopped);

		// this gets set to false when the ball starts
		// to move by the Mr. Ball himself.
		(*it).ball()->setBeginningOfHole(true);
		if ((int)(*it).scores().count() < curHole)
			(*it).addHole();
		(*it).ball()->setVelocity(Vector());
		(*it).ball()->setVisible(false);
	}

	emit newPlayersTurn(&(*curPlayer));

	if (reset)
		openFile();

	inPlay = false;
	timer->start(timerMsec);

	if(size().width()!=400 || size().height()!=400) { //not default size, so resizing needed
		int setSize = qMin(size().width(), size().height());
		//resize needs to be called for setSize+1 first because otherwise it doesn't seem to get called (not sure why)
		QGraphicsView::resize(setSize+1, setSize+1);
		QGraphicsView::resize(setSize, setSize);
	}

	// if (false) { we're done with the round! }
	if (oldCurHole != curHole)
	{
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it) {
			(*it).ball()->setPlaceOnGround(false);
			while( (*it).numHoles() < (unsigned)curHole)
				(*it).addHole();
		}

		// here we have to make sure the scoreboard shows
		// all of the holes up until now;

		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			cfgGroup = KConfigGroup(cfg->group(QStringLiteral("%1-hole@-50,-50|0").arg(scoreboardHoles + 1)));
			emit newHole(cfgGroup.readEntry("par", 3));
		}

		resetHoleScores();
		updateShowInfo();

		// this is from shotDone()
		(*curPlayer).ball()->setVisible(true);
		putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		updateMouse();

		ballStateList.canUndo = false;

		(*curPlayer).ball()->collisionDetect();
	}

	unPause();
}

void KolfGame::showInfoDlg(bool addDontShowAgain)
{
	KMessageBox::information(parentWidget(),
			i18n("Course name: %1", holeInfo.name()) + QStringLiteral("\n")
			+ i18n("Created by %1", holeInfo.author()) + QStringLiteral("\n")
			+ i18np("%1 hole", "%1 holes", highestHole),
			i18n("Course Information"),
			addDontShowAgain? holeInfo.name() + QStringLiteral(" ") + holeInfo.author() : QString());
}

void KolfGame::openFile()
{
	QList<QGraphicsItem*> newTopLevelQItems;
	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		if (dynamic_cast<Ball*>(qitem))
		{
			//do not delete balls
			newTopLevelQItems << qitem;
			continue;
		}
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
		{
			delete citem;
		}
	}

	m_moveableQItems = m_topLevelQItems = newTopLevelQItems;
	selectedItem = 0;

	// will tell basic course info
	// we do this here for the hell of it.
	// there is no fake id, by the way,
	// because it's old and when i added ids i forgot to change it.
	cfgGroup = KConfigGroup(cfg->group(QStringLiteral("0-course@-50,-50")));
	holeInfo.setAuthor(cfgGroup.readEntry("author", holeInfo.author()));
	holeInfo.setName(cfgGroup.readEntry("Name", holeInfo.name()));
	holeInfo.setUntranslatedName(cfgGroup.readEntryUntranslated("Name", holeInfo.untranslatedName()));
	emit titleChanged(holeInfo.name());

	cfgGroup = KConfigGroup(KSharedConfig::openConfig(filename), QStringLiteral("%1-hole@-50,-50|0").arg(curHole));
	curPar = cfgGroup.readEntry("par", 3);
	holeInfo.setPar(curPar);
	holeInfo.borderWallsChanged(cfgGroup.readEntry("borderWalls", holeInfo.borderWalls()));
	holeInfo.setMaxStrokes(cfgGroup.readEntry("maxstrokes", 10));

	QStringList missingPlugins;

	// The "for" loop depends on the list of groups being in sorted order.
	QStringList groups = cfg->groupList();
	groups.sort();

	int numItems = 0;
	int _highestHole = 0;

	for (QStringList::const_iterator it = groups.constBegin(); it != groups.constEnd(); ++it)
	{
		// Format of group name is [<holeNum>-<name>@<x>,<y>|<id>]
		cfgGroup = KConfigGroup(cfg->group(*it));

		const int len = (*it).length();
		const int dashIndex = (*it).indexOf(QLatin1String("-"));
		const int holeNum = (*it).leftRef(dashIndex).toInt();
		if (holeNum > _highestHole)
			_highestHole = holeNum;

		const int atIndex = (*it).indexOf(QLatin1String("@"));
		const QString name = (*it).mid(dashIndex + 1, atIndex - (dashIndex + 1));

		if (holeNum != curHole)
		{
			// Break before reading all groups, if the highest hole
			// number is known and all items in curHole are done.
			if (numItems && !recalcHighestHole)
				break;
			continue;
		}
		numItems++;


		const int commaIndex = (*it).indexOf(QLatin1String(","));
		const int pipeIndex = (*it).indexOf(QLatin1String("|"));
		const int x = (*it).midRef(atIndex + 1, commaIndex - (atIndex + 1)).toInt();
		const int y = (*it).midRef(commaIndex + 1, pipeIndex - (commaIndex + 1)).toInt();

		// will tell where ball is
		if (name == QLatin1String("ball"))
		{
			for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
				(*it).ball()->setPos(x, y);
			whiteBall->setPos(x, y);
			continue;
		}

		const int id = (*it).rightRef(len - (pipeIndex + 1)).toInt();

		QGraphicsItem* newItem = m_factory.createInstance(name, courseBoard, g_world);
		if (newItem)
		{
			m_topLevelQItems << newItem;
			m_moveableQItems << newItem;
			CanvasItem *sceneItem = dynamic_cast<CanvasItem *>(newItem);

			if (!sceneItem)
				continue;

			sceneItem->setId(id);
			sceneItem->setGame(this);
			sceneItem->editModeChanged(editing);
			sceneItem->setName(name);
			m_moveableQItems.append(sceneItem->moveableItems());

			sceneItem->setPosition(QPointF(x, y));
			newItem->setVisible(true);

			// make things actually show
			cfgGroup = KConfigGroup(cfg->group(makeGroup(id, curHole, sceneItem->name(), x, y)));
			sceneItem->load(&cfgGroup);
		}
		else if (name != QLatin1String("hole") && !missingPlugins.contains(name))
			missingPlugins.append(name);

	}

	if (!missingPlugins.empty())
	{
		KMessageBox::informationList(this, QStringLiteral("<p>") + i18n("This hole uses the following plugins, which you do not have installed:") + QStringLiteral("</p>"), missingPlugins, QString(), QStringLiteral("%1 warning").arg(holeInfo.untranslatedName() + QString::number(curHole)));
	}

	lastDelId = -1;

	// if it's the first hole let's not
	if (!numItems && curHole > 1 && !addingNewHole && curHole >= _highestHole)
	{
		// we're done, let's quit
		curHole--;
		pause();
		emit holesDone();

		// tidy things up
		setBorderWalls(false);
		clearHole();
		setModified(false);
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->setVisible(false);

		return;
	}

	// do it down here; if !hasFinalLoad, do it up there!
	//QGraphicsItem *qsceneItem = 0;
	QList<QGraphicsItem *>::const_iterator qsceneItem;
	QList<CanvasItem *> todo;
	QList<QGraphicsItem *> qtodo;

	if (curHole > _highestHole)
		_highestHole = curHole;

	if (recalcHighestHole)
	{
		highestHole = _highestHole;
		recalcHighestHole = false;
		emit largestHole(highestHole);
	}

	if (curHole == 1 && !filename.isNull() && !infoShown)
	{
		// let's not now, because they see it when they choose course
		//showInfoDlg(true);
		infoShown = true;
	}

	setModified(false);
}

void KolfGame::addNewObject(const QString& identifier)
{
	QGraphicsItem *newItem = m_factory.createInstance(identifier, courseBoard, g_world);

	m_topLevelQItems << newItem;
	m_moveableQItems << newItem;
	if(!newItem->isVisible())
		newItem->setVisible(true);

	CanvasItem *sceneItem = dynamic_cast<CanvasItem *>(newItem);
	if (!sceneItem)
		return;

	// we need to find a number that isn't taken
	int i = lastDelId > 0? lastDelId : m_topLevelQItems.count() - 30;
	if (i <= 0)
		i = 0;

	for (;; ++i)
	{
		bool found = false;
		foreach (QGraphicsItem* qitem, m_topLevelQItems)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
			if (citem)
			{
				if (citem->curId() == i)
				{
					found = true;
					break;
				}
			}
		}


		if (!found)
			break;
	}
	sceneItem->setId(i);

	sceneItem->setGame(this);

	foreach (QGraphicsItem* qitem, sceneItem->infoItems())
		qitem->setVisible(m_showInfo);

	sceneItem->editModeChanged(editing);

	sceneItem->setName(identifier);
	m_moveableQItems.append(sceneItem->moveableItems());

	newItem->setPos(width/2 - 18, height / 2 - 18);
	sceneItem->moveBy(0, 0);
	sceneItem->setSize(newItem->boundingRect().size());

	setModified(true);
}

bool KolfGame::askSave(bool noMoreChances)
{
	if (!modified)
		// not cancel, don't save
		return false;

	int result = KMessageBox::warningYesNoCancel(this, i18n("There are unsaved changes to current hole. Save them?"), i18n("Unsaved Changes"), KStandardGuiItem::save(), noMoreChances? KStandardGuiItem::discard() : KGuiItem(i18n("Save &Later")), KStandardGuiItem::cancel(), noMoreChances? "DiscardAsk" : "SaveAsk");
	switch (result)
	{
		case KMessageBox::Yes:
			save();
			// fallthrough

		case KMessageBox::No:
			return false;
			break;

		case KMessageBox::Cancel:
			return true;
			break;

		default:
			break;
	}

	return false;
}

void KolfGame::addNewHole()
{
	if (askSave(true))
		return;

	// either it's already false
	// because it was saved by askSave(),
	// or the user pressed the 'discard' button
	setModified(false);

	// find highest hole num, and create new hole
	// now openFile makes highest hole for us

	addingNewHole = true;
	curHole = highestHole;
	recalcHighestHole = true;
	startNextHole();
	addingNewHole = false;
	emit currentHole(curHole);

	// make sure even the current player isn't showing
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		(*it).ball()->setVisible(false);

	whiteBall->setVisible(editing);
	putter->setVisible(!editing);
	inPlay = false;

	// add default objects
	foreach (const Kolf::ItemMetadata& metadata, m_factory.knownTypes())
		if (metadata.addOnNewHole)
			addNewObject(metadata.identifier);

	save();
}

// kantan deshou ;-)
void KolfGame::resetHole()
{
	if (askSave(true))
		return;
	setModified(false);
	curHole--;
	startNextHole();
	resetHoleScores();
}

void KolfGame::resetHoleScores()
{
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		(*it).resetScore(curHole);
		emit scoreChanged((*it).id(), curHole, 0);
	}
}

void KolfGame::clearHole()
{
	QList<QGraphicsItem*> newTopLevelQItems;
	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		if (dynamic_cast<Ball*>(qitem))
		{
			//do not delete balls
			newTopLevelQItems << qitem;
			continue;
		}
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
		{
			delete citem;
		}
	}

	m_moveableQItems = m_topLevelQItems = newTopLevelQItems;
	setSelectedItem(0);

	// add default objects
	foreach (const Kolf::ItemMetadata& metadata, m_factory.knownTypes())
		if (metadata.addOnNewHole)
			addNewObject(metadata.identifier);

	setModified(true);
}

void KolfGame::switchHole(int hole)
{
	if (inPlay)
		return;
	if (hole < 1 || hole > highestHole)
		return;

	bool wasEditing = editing;
	if (editing)
		toggleEditMode();

	if (askSave(true))
		return;
	setModified(false);

	curHole = hole;
	resetHole();

	if (wasEditing)
		toggleEditMode();
}

void KolfGame::switchHole(const QString &holestring)
{
	bool ok;
	int hole = holestring.toInt(&ok);
	if (!ok)
		return;
	switchHole(hole);
}

void KolfGame::nextHole()
{
	switchHole(curHole + 1);
}

void KolfGame::prevHole()
{
	switchHole(curHole - 1);
}

void KolfGame::firstHole()
{
	switchHole(1);
}

void KolfGame::lastHole()
{
	switchHole(highestHole);
}

void KolfGame::randHole()
{
	int newHole = 1 + (int)((double)KRandom::random() * ((double)(highestHole - 1) / (double)RAND_MAX));
	switchHole(newHole);
}

void KolfGame::save()
{
	if (filename.isEmpty())
	{
		QPointer<QFileDialog> fileSaveDialog = new QFileDialog(this);
		fileSaveDialog->setWindowTitle(i18nc("@title:window", "Pick Kolf Course to Save To"));
		fileSaveDialog->setMimeTypeFilters(QStringList(QStringLiteral("application/x-kourse")));
		fileSaveDialog->setAcceptMode(QFileDialog::AcceptSave);
		if (fileSaveDialog->exec() == QDialog::Accepted) {
			QUrl newfile = fileSaveDialog->selectedUrls().first();
			if (newfile.isEmpty()) {
				return;
			}
			else {
				setFilename(newfile.url());
			}
		}
		delete fileSaveDialog;
	}

	emit parChanged(curHole, holeInfo.par());
	emit titleChanged(holeInfo.name());

	const QStringList groups = cfg->groupList();

	// wipe out all groups from this hole
	for (QStringList::const_iterator it = groups.begin(); it != groups.end(); ++it)
	{
		int holeNum = (*it).leftRef((*it).indexOf(QLatin1String("-"))).toInt();
		if (holeNum == curHole)
			cfg->deleteGroup(*it);
	}
	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
		{
			cfgGroup = KConfigGroup(cfg->group(makeGroup(citem->curId(), curHole, citem->name(), (int)qitem->x(), (int)qitem->y())));
			citem->save(&cfgGroup);
		}
	}

	// save where ball starts (whiteBall tells all)
	cfgGroup = KConfigGroup(cfg->group(QStringLiteral("%1-ball@%2,%3").arg(curHole).arg((int)whiteBall->x()).arg((int)whiteBall->y())));
	cfgGroup.writeEntry("dummykey", true);

	cfgGroup = KConfigGroup(cfg->group(QStringLiteral("0-course@-50,-50")));
	cfgGroup.writeEntry("author", holeInfo.author());
	cfgGroup.writeEntry("Name", holeInfo.untranslatedName());

	// save hole info
	cfgGroup = KConfigGroup(cfg->group(QStringLiteral("%1-hole@-50,-50|0").arg(curHole)));
	cfgGroup.writeEntry("par", holeInfo.par());
	cfgGroup.writeEntry("maxstrokes", holeInfo.maxStrokes());
	cfgGroup.writeEntry("borderWalls", holeInfo.borderWalls());

	cfg->sync();

	setModified(false);
}

void KolfGame::toggleEditMode()
{
	// won't be editing anymore, and user wants to cancel, we return
	// this is pretty useless. when the person leaves the hole,
	// he gets asked again
	/*
	   if (editing && modified)
	   {
	   if (askSave(false))
	   {
	   emit checkEditing();
	   return;
	   }
	   }
	   */

	selectedItem = 0;

	editing = !editing;

	if (editing)
	{
		emit editingStarted();
		setSelectedItem(0);
	}
	else
	{
		emit editingEnded();
		setCursor(Qt::ArrowCursor);
	}

	// alert our items
	foreach (QGraphicsItem* qitem, m_topLevelQItems)
	{
		if (dynamic_cast<Ball*>(qitem)) continue;
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qitem);
		if (citem)
			citem->editModeChanged(editing);
	}

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		// curplayer shouldn't be hidden no matter what
		if ((*it).ball()->beginningOfHole() && it != curPlayer)
			(*it).ball()->setVisible(false);
		else
			(*it).ball()->setVisible(!editing);
	}

	whiteBall->setVisible(editing);
	whiteBall->editModeChanged(editing);

	// shouldn't see putter whilst editing
	putter->setVisible(!editing);

	if (editing)
		autoSaveTimer->start(autoSaveMsec);
	else
		autoSaveTimer->stop();

	inPlay = false;
}

void KolfGame::setSelectedItem(CanvasItem* citem)
{
	QGraphicsItem* qitem = dynamic_cast<QGraphicsItem*>(citem);
	selectedItem = qitem;
	emit newSelectedItem(qitem ? citem : &holeInfo);
	//deactivate all other overlays
	foreach (QGraphicsItem* otherQitem, m_topLevelQItems)
	{
		CanvasItem* otherCitem = dynamic_cast<CanvasItem*>(otherQitem);
		if (otherCitem && otherCitem != citem)
		{
			//false = do not create overlay if it does not exist yet
			Kolf::Overlay* otherOverlay = otherCitem->overlay(false);
			if (otherOverlay)
				otherOverlay->setState(Kolf::Overlay::Passive);
		}
	}
}

void HoleInfo::borderWallsChanged(bool yes)
{
	m_borderWalls = yes;
	game->setBorderWalls(yes);
}

bool KolfGame::allPlayersDone()
{
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		if ((*it).ball()->curState() != Holed)
			return false;

	return true;
}

void KolfGame::setBorderWalls(bool showing)
{
	foreach (Kolf::Wall* wall, borderWalls)
		wall->setVisible(showing);
}

void KolfGame::setUseAdvancedPutting(bool yes)
{
	m_useAdvancedPutting = yes;

	// increase maxStrength in advanced putting mode
	if (yes)
		maxStrength = 65;
	else
		maxStrength = 55;
}

void KolfGame::setShowGuideLine(bool yes)
{
	putter->setShowGuideLine(yes);
}

void KolfGame::setSound(bool yes)
{
	m_sound = yes;
}

void KolfGame::courseInfo(CourseInfo &info, const QString& filename)
{
	KConfig config(filename);
	KConfigGroup configGroup (config.group(QStringLiteral("0-course@-50,-50")));
	info.author = configGroup.readEntry("author", info.author);
	info.name = configGroup.readEntry("Name", configGroup.readEntry("name", info.name));
	info.untranslatedName = configGroup.readEntryUntranslated("Name", configGroup.readEntryUntranslated("name", info.name));

	unsigned int hole = 1;
	unsigned int par= 0;
	while (1)
	{
		QString group = QStringLiteral("%1-hole@-50,-50|0").arg(hole);
		if (!config.hasGroup(group))
		{
			hole--;
			break;
		}

		configGroup = KConfigGroup(config.group(group));
		par += configGroup.readEntry("par", 3);

		hole++;
	}

	info.par = par;
	info.holes = hole;
}

void KolfGame::scoresFromSaved(KConfig *config, PlayerList &players)
{
	KConfigGroup configGroup(config->group(QStringLiteral("0 Saved Game")));
	int numPlayers = configGroup.readEntry("Players", 0);
	if (numPlayers <= 0)
		return;

	for (int i = 1; i <= numPlayers; ++i)
	{
		// this is same as in kolf.cpp, but we use saved game values
		configGroup = KConfigGroup(config->group(QString::number(i)));
		players.append(Player());
		players.last().ball()->setColor(configGroup.readEntry("Color", "#ffffff"));
		players.last().setName(configGroup.readEntry("Name"));
		players.last().setId(i);

		const QStringList scores(configGroup.readEntry("Scores",QStringList()));
		QList<int> intscores;
		for (QStringList::const_iterator it = scores.begin(); it != scores.end(); ++it)
			intscores.append((*it).toInt());

		players.last().setScores(intscores);
	}
}

void KolfGame::saveScores(KConfig *config)
{
	// wipe out old player info
	const QStringList groups = config->groupList();
	for (QStringList::const_iterator it = groups.begin(); it != groups.end(); ++it)
	{
		// this deletes all int groups, ie, the player info groups
		bool ok = false;
		(*it).toInt(&ok);
		if (ok)
			config->deleteGroup(*it);
	}

	KConfigGroup configGroup(config->group(QStringLiteral("0 Saved Game")));
	configGroup.writeEntry("Players", players->count());
	configGroup.writeEntry("Course", filename);
	configGroup.writeEntry("Current Hole", curHole);

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		KConfigGroup configGroup(config->group(QString::number((*it).id())));
		configGroup.writeEntry("Name", (*it).name());
		configGroup.writeEntry("Color", (*it).ball()->color().name());

		QStringList scores;
		QList<int> intscores = (*it).scores();
		for (QList<int>::Iterator it = intscores.begin(); it != intscores.end(); ++it)
			scores.append(QString::number(*it));

		configGroup.writeEntry("Scores", scores);
	}
}

CourseInfo::CourseInfo()
	: name(i18n("Course Name")), author(i18n("Course Author")), holes(0), par(0)
{
}


