#warning port to the new sound system once it exists
/*
#include <arts/kmedia2.h>
#include <arts/kplayobject.h>
#include <arts/kplayobjectfactory.h>
*/

#include <kapplication.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kpixmapeffect.h>
#include <kprinter.h>
#include <kstandarddirs.h>

#include <qbrush.h>
#include <q3canvas.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmap.h>
#include <qpainter.h>
#include <q3paintdevicemetrics.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qpoint.h>
#include <q3pointarray.h>
#include <qrect.h>
#include <q3simplerichtext.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <q3valuelist.h>

//Added by qt3to4:
#include <QMouseEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <Q3PtrList>
#include <QHBoxLayout>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <krandom.h>

#include "kcomboboxdialog.h"
#include "kvolumecontrol.h"
#include "vector.h"
#include "game.h"


inline QString makeGroup(int id, int hole, QString name, int x, int y)
{
	return QString("%1-%2@%3,%4|%5").arg(hole).arg(name).arg(x).arg(y).arg(id);
}

inline QString makeStateGroup(int id, const QString &name)
{
	return QString("%1|%2").arg(name).arg(id);
}

/////////////////////////

RectPoint::RectPoint(QColor color, RectItem *rect, Q3Canvas *canvas)
	: Q3CanvasEllipse(canvas)
{
	setZ(9999);
	setSize(10, 10);
	this->rect = rect;
	setBrush(QBrush(color));
	setSizeFactor(1.0);
	dontmove = false;
}

void RectPoint::moveBy(double dx, double dy)
{
	Q3CanvasEllipse::moveBy(dx, dy);

	if (dontmove)
	{
		dontmove = false;
		return;
	}

	Q3CanvasItem *qitem = dynamic_cast<Q3CanvasItem *>(rect);
	if (!qitem)
		return;

	int nw = ( int )( m_sizeFactor * fabs(x() - qitem->x()) );
	int nh = ( int )( m_sizeFactor * fabs(y() - qitem->y()) );
	if (nw <= 0 || nh <= 0)
		return;

	rect->newSize( nw, nh);
}

Config *RectPoint::config(QWidget *parent)
{
	CanvasItem *citem = dynamic_cast<CanvasItem *>(rect);
	if (citem)
		return citem->config(parent);
	else
		return CanvasItem::config(parent);
}

/////////////////////////

Arrow::Arrow(Q3Canvas *canvas)
	: Q3CanvasLine(canvas)
{
	line1 = new Q3CanvasLine(canvas);
	line2 = new Q3CanvasLine(canvas);

	m_angle = 0;
	m_length = 20;
	m_reversed = false;

	setPen(QPen(Qt::black));

	updateSelf();
	setVisible(false);
}

void Arrow::setPen(QPen p)
{
	Q3CanvasLine::setPen(p);
	line1->setPen(p);
	line2->setPen(p);
}

void Arrow::setZ(double newz)
{
	Q3CanvasLine::setZ(newz);
	line1->setZ(newz);
	line2->setZ(newz);
}

void Arrow::setVisible(bool yes)
{
	Q3CanvasLine::setVisible(yes);
	line1->setVisible(yes);
	line2->setVisible(yes);
}

void Arrow::moveBy(double dx, double dy)
{
	Q3CanvasLine::moveBy(dx, dy);
	line1->moveBy(dx, dy);
	line2->moveBy(dx, dy);
}

void Arrow::aboutToDie()
{
	delete line1;
	delete line2;
}

void Arrow::updateSelf()
{
	QPoint start = startPoint();
	QPoint end(int( m_length * cos(m_angle) ), int( m_length * sin(m_angle)) );

	if (m_reversed)
	{
		QPoint tmp(start);
		start = end;
		end = tmp;
	}

	setPoints(start.x(), start.y(), end.x(), end.y());

	const double lineLen = m_length / 2;

	const double angle1 = m_angle - M_PI / 2 - 1;
	line1->move(end.x() + x(), end.y() + y());
	start = end;
	end = QPoint(int( lineLen * cos(angle1) ), int( lineLen * sin(angle1) ) );
	line1->setPoints(0, 0, end.x(), end.y());

	const double angle2 = m_angle + M_PI / 2 + 1;
	line2->move(start.x() + x(), start.y() + y());
	end = QPoint(lineLen * cos(angle2), lineLen * sin(angle2));
	line2->setPoints(0, 0, end.x(), end.y());
}

/////////////////////////

BridgeConfig::BridgeConfig(Bridge *bridge, QWidget *parent)
	: Config(parent)
{
	this->bridge = bridge;

	m_vlayout = new QVBoxLayout(this);
        m_vlayout->setMargin( marginHint() );
        m_vlayout->setSpacing( spacingHint() );
	QGridLayout *layout = new QGridLayout( );
        m_vlayout->addItem( layout );
        layout->setSpacing( spacingHint());
	layout->addWidget(new QLabel(i18n("Walls on:"), this), 0, 0);
	top = new QCheckBox(i18n("&Top"), this);
	layout->addWidget(top, 0, 1);
	connect(top, SIGNAL(toggled(bool)), this, SLOT(topWallChanged(bool)));
	top->setChecked(bridge->topWallVisible());
	bot = new QCheckBox(i18n("&Bottom"), this);
	layout->addWidget(bot, 1, 1);
	connect(bot, SIGNAL(toggled(bool)), this, SLOT(botWallChanged(bool)));
	bot->setChecked(bridge->botWallVisible());
	left = new QCheckBox(i18n("&Left"), this);
	layout->addWidget(left, 1, 0);
	connect(left, SIGNAL(toggled(bool)), this, SLOT(leftWallChanged(bool)));
	left->setChecked(bridge->leftWallVisible());
	right = new QCheckBox(i18n("&Right"), this);
	layout->addWidget(right, 1, 2);
	connect(right, SIGNAL(toggled(bool)), this, SLOT(rightWallChanged(bool)));
	right->setChecked(bridge->rightWallVisible());
}

void BridgeConfig::topWallChanged(bool yes)
{
	bridge->setTopWallVisible(yes);
	changed();
}

void BridgeConfig::botWallChanged(bool yes)
{
	bridge->setBotWallVisible(yes);
	changed();
}

void BridgeConfig::leftWallChanged(bool yes)
{
	bridge->setLeftWallVisible(yes);
	changed();
}

void BridgeConfig::rightWallChanged(bool yes)
{
	bridge->setRightWallVisible(yes);
	changed();
}

/////////////////////////

Bridge::Bridge(QRect rect, Q3Canvas *canvas)
	: Q3CanvasRectangle(rect, canvas)
{
	QColor color("#92772D");
	setBrush(QBrush(color));
	setPen(Qt::NoPen);
	setZ(998);

	topWall = new Wall(canvas);
	topWall->setAlwaysShow(true);
	botWall = new Wall(canvas);
	botWall->setAlwaysShow(true);
	leftWall = new Wall(canvas);
	leftWall->setAlwaysShow(true);
	rightWall = new Wall(canvas);
	rightWall->setAlwaysShow(true);

	setWallZ(z() + 0.01);
	setWallColor(color);

	topWall->setVisible(false);
	botWall->setVisible(false);
	leftWall->setVisible(false);
	rightWall->setVisible(false);

	point = new RectPoint(color, this, canvas);
	editModeChanged(false);

	newSize(width(), height());
}

bool Bridge::collision(Ball *ball, long int /*id*/)
{
	ball->setFrictionMultiplier(.63);
	return false;
}

void Bridge::setWallZ(double newz)
{
	topWall->setZ(newz);
	botWall->setZ(newz);
	leftWall->setZ(newz);
	rightWall->setZ(newz);
}

void Bridge::setGame(KolfGame *game)
{
	CanvasItem::setGame(game);
	topWall->setGame(game);
	botWall->setGame(game);
	leftWall->setGame(game);
	rightWall->setGame(game);
}

void Bridge::setWallColor(QColor color)
{
	topWall->setPen(QPen(color.dark(), 3));
	botWall->setPen(topWall->pen());
	leftWall->setPen(topWall->pen());
	rightWall->setPen(topWall->pen());
}

void Bridge::aboutToDie()
{
	delete point;
	topWall->aboutToDie();
	delete topWall;
	botWall->aboutToDie();
	delete botWall;
	leftWall->aboutToDie();
	delete leftWall;
	rightWall->aboutToDie();
	delete rightWall;
}

void Bridge::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Bridge::moveBy(double dx, double dy)
{
	Q3CanvasRectangle::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width(), y() + height());

	topWall->move(x(), y());
	botWall->move(x(), y() - 1);
	leftWall->move(x(), y());
	rightWall->move(x(), y());

	Q3CanvasItemList list = collisions(true);
	for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem)
			citem->updateZ();
	}
}

void Bridge::load(KConfig *cfg)
{
	doLoad(cfg);
}

void Bridge::doLoad(KConfig *cfg)
{
	newSize(cfg->readEntry("width", width()), cfg->readEntry("height", height()));
	setTopWallVisible(cfg->readEntry("topWallVisible", topWallVisible()));
	setBotWallVisible(cfg->readEntry("botWallVisible", botWallVisible()));
	setLeftWallVisible(cfg->readEntry("leftWallVisible", leftWallVisible()));
	setRightWallVisible(cfg->readEntry("rightWallVisible", rightWallVisible()));
}

void Bridge::save(KConfig *cfg)
{
	doSave(cfg);
}

void Bridge::doSave(KConfig *cfg)
{
	cfg->writeEntry("width", width());
	cfg->writeEntry("height", height());
	cfg->writeEntry("topWallVisible", topWallVisible());
	cfg->writeEntry("botWallVisible", botWallVisible());
	cfg->writeEntry("leftWallVisible", leftWallVisible());
	cfg->writeEntry("rightWallVisible", rightWallVisible());
}

Q3PtrList<Q3CanvasItem> Bridge::moveableItems() const
{
	Q3PtrList<Q3CanvasItem> ret;
	ret.append(point);
	return ret;
}

void Bridge::newSize(int width, int height)
{
	setSize(width, height);
}

void Bridge::setSize(int width, int height)
{
	Q3CanvasRectangle::setSize(width, height);

	topWall->setPoints(0, 0, width, 0);
	botWall->setPoints(0, height, width, height);
	leftWall->setPoints(0, 0, 0, height);
	rightWall->setPoints(width, 0, width, height);

	moveBy(0, 0);
}

/////////////////////////

WindmillConfig::WindmillConfig(Windmill *windmill, QWidget *parent)
	: BridgeConfig(windmill, parent)
{
	this->windmill = windmill;
	m_vlayout->addStretch();

	QCheckBox *check = new QCheckBox(i18n("Windmill on bottom"), this);
	check->setChecked(windmill->bottom());
	connect(check, SIGNAL(toggled(bool)), this, SLOT(endChanged(bool)));
	m_vlayout->addWidget(check);

	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        m_vlayout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange( 1, 10 );
        slider->setPageStep( 1 );
        slider->setValue( windmill->curSpeed() );
	hlayout->addWidget(slider);
	hlayout->addWidget(new QLabel(i18n("Fast"), this));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(speedChanged(int)));

	endChanged(check->isChecked());
}

void WindmillConfig::speedChanged(int news)
{
	windmill->setSpeed(news);
	changed();
}

void WindmillConfig::endChanged(bool bottom)
{
	windmill->setBottom(bottom);
	changed();

	bot->setEnabled(!bottom);
	if (startedUp)
	{
		bot->setChecked(!bottom);
		botWallChanged(bot->isChecked());
	}
	top->setEnabled(bottom);
	if (startedUp)
	{
		top->setChecked(bottom);
		topWallChanged(top->isChecked());
	}
}

/////////////////////////

Windmill::Windmill(QRect rect, Q3Canvas *canvas)
	: Bridge(rect, canvas), speedfactor(16), m_bottom(true)
{
	guard = new WindmillGuard(canvas);
	guard->setPen(QPen(Qt::black, 5));
	guard->setVisible(true);
	guard->setAlwaysShow(true);
	setSpeed(5);
	guard->setZ(wallZ() + .1);

	left = new Wall(canvas);
	left->setPen(wallPen());
	left->setAlwaysShow(true);
	right = new Wall(canvas);
	right->setPen(wallPen());
	right->setAlwaysShow(true);
	left->setZ(wallZ());
	right->setZ(wallZ());
	left->setVisible(true);
	right->setVisible(true);

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(true);
	setRightWallVisible(true);

	newSize(width(), height());
	moveBy(0, 0);
}

void Windmill::aboutToDie()
{
	Bridge::aboutToDie();
	guard->aboutToDie();
	delete guard;
	left->aboutToDie();
	delete left;
	right->aboutToDie();
	delete right;
}

void Windmill::setSpeed(int news)
{
	if (news < 0)
		return;
	speed = news;
	guard->setXVelocity(((double)news / (double)3) * (guard->xVelocity() > 0? 1 : -1));
}

void Windmill::setGame(KolfGame *game)
{
	Bridge::setGame(game);
	guard->setGame(game);
	left->setGame(game);
	right->setGame(game);
}

void Windmill::save(KConfig *cfg)
{
	cfg->writeEntry("speed", speed);
	cfg->writeEntry("bottom", m_bottom);

	doSave(cfg);
}

void Windmill::load(KConfig *cfg)
{
	setSpeed(cfg->readEntry("speed", -1));

	doLoad(cfg);

	left->editModeChanged(false);
	right->editModeChanged(false);
	guard->editModeChanged(false);

	setBottom(cfg->readEntry("bottom", true));
}

void Windmill::moveBy(double dx, double dy)
{
	Bridge::moveBy(dx, dy);

	left->move(x(), y());
	right->move(x(), y());

	guard->moveBy(dx, dy);
	guard->setBetween(x(), x() + width());

	update();
}

void Windmill::setSize(int width, int height)
{
	newSize(width, height);
}

void Windmill::setBottom(bool yes)
{
	m_bottom = yes;
	newSize(width(), height());
}

void Windmill::newSize(int width, int height)
{
	Bridge::newSize(width, height);

	const int indent = width / 4;

	double indentY = m_bottom? height : 0;
	left->setPoints(0, indentY, indent, indentY);
	right->setPoints(width - indent, indentY, width, indentY);

	guard->setBetween(x(), x() + width);
	double guardY = m_bottom? height + 4 : -4;
	guard->setPoints(0, guardY, (double)indent / (double)1.07 - 2, guardY);
}

/////////////////////////

void WindmillGuard::advance(int phase)
{
	Wall::advance(phase);

	if (phase == 1)
	{
		if (x() + startPoint().x() <= min)
			setXVelocity(fabs(xVelocity()));
		else if (x() + endPoint().x() >= max)
			setXVelocity(-fabs(xVelocity()));
	}
}

/////////////////////////

Sign::Sign(Q3Canvas *canvas)
	: Bridge(QRect(0, 0, 110, 40), canvas)
{
	setZ(998.8);
	m_text = m_untranslatedText = i18n("New Text");
	setBrush(QBrush(Qt::white));
	setWallColor(Qt::black);
	setWallZ(z() + .01);

	setTopWallVisible(true);
	setBotWallVisible(true);
	setLeftWallVisible(true);
	setRightWallVisible(true);
}

void Sign::load(KConfig *cfg)
{
	m_text = cfg->readEntry("Comment", m_text);
	m_untranslatedText = cfg->readEntryUntranslated("Comment", m_untranslatedText);

	doLoad(cfg);
}

void Sign::save(KConfig *cfg)
{
	cfg->writeEntry("Comment", m_untranslatedText);

	doSave(cfg);
}

void Sign::setText(const QString &text)
{
	m_text = text;
	m_untranslatedText = text;

	update();
}

void Sign::draw(QPainter &painter)
{
	Bridge::draw(painter);

	painter.setPen(QPen(Qt::black, 1));
	Q3SimpleRichText txt(m_text, kapp->font());
	const int indent = wallPen().width() + 3;
	txt.setWidth(width() - 2*indent);
	QColorGroup colorGroup;
	colorGroup.setColor(QPalette::Foreground, Qt::black);
	colorGroup.setColor(QPalette::Text, Qt::black);
	colorGroup.setColor(QPalette::Background, Qt::black);
	colorGroup.setColor(QPalette::Base, Qt::black);
	txt.draw(&painter, x() + indent, y(), QRect(x() + indent, y(), width() - indent, height() - indent), colorGroup);
}

/////////////////////////

SignConfig::SignConfig(Sign *sign, QWidget *parent)
	: BridgeConfig(sign, parent)
{
	this->sign = sign;
	m_vlayout->addStretch();
	m_vlayout->addWidget(new QLabel(i18n("Sign HTML:"), this));
	KLineEdit *name = new KLineEdit(sign->text(), this);
	m_vlayout->addWidget(name);
	connect(name, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));
}

void SignConfig::textChanged(const QString &text)
{
	sign->setText(text);
	changed();
}

/////////////////////////

EllipseConfig::EllipseConfig(Ellipse *ellipse, QWidget *parent)
	: Config(parent), slow1(0), fast1(0), slow2(0), fast2(0), slider1(0), slider2(0)
{
	this->ellipse = ellipse;

	m_vlayout = new QVBoxLayout(this);
        m_vlayout->setMargin( marginHint() );
        m_vlayout->setSpacing( spacingHint() );

	QCheckBox *check = new QCheckBox(i18n("Enable show/hide"), this);
	m_vlayout->addWidget(check);
	connect(check, SIGNAL(toggled(bool)), this, SLOT(check1Changed(bool)));
	check->setChecked(ellipse->changeEnabled());

	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        m_vlayout->addLayout( hlayout );
	slow1 = new QLabel(i18n("Slow"), this);
	hlayout->addWidget(slow1);
	slider1 = new QSlider(Qt::Horizontal, this);
        slider1->setRange( 1, 100 );
        slider1->setPageStep( 5 );
        slider1->setValue( 100 - ellipse->changeEvery() );
	hlayout->addWidget(slider1);
	fast1 = new QLabel(i18n("Fast"), this);
	hlayout->addWidget(fast1);

	connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(value1Changed(int)));

	check1Changed(ellipse->changeEnabled());

	// TODO add slider2 and friends and make it possible for ellipses to grow and contract

	m_vlayout->addStretch();
}

void EllipseConfig::value1Changed(int news)
{
	ellipse->setChangeEvery(100 - news);
	changed();
}

void EllipseConfig::value2Changed(int /*news*/)
{
	changed();
}

void EllipseConfig::check1Changed(bool on)
{
	ellipse->setChangeEnabled(on);
	if (slider1)
		slider1->setEnabled(on);
	if (slow1)
		slow1->setEnabled(on);
	if (fast1)
		fast1->setEnabled(on);

	changed();
}

void EllipseConfig::check2Changed(bool on)
{
	//ellipse->setChangeEnabled(on);
	if (slider2)
		slider2->setEnabled(on);
	if (slow2)
		slow2->setEnabled(on);
	if (fast2)
		fast2->setEnabled(on);

	changed();
}

/////////////////////////

Ellipse::Ellipse(Q3Canvas *canvas)
	: Q3CanvasEllipse(canvas)
{
	savingDone();
	setChangeEnabled(false);
	setChangeEvery(50);
	count = 0;
	setVisible(true);

	point = new RectPoint(Qt::black, this, canvas);
	point->setSizeFactor(2.0);
}

void Ellipse::aboutToDie()
{
	delete point;
}

void Ellipse::setChangeEnabled(bool changeEnabled)
{
	m_changeEnabled = changeEnabled;
	setAnimated(m_changeEnabled);

	if (!m_changeEnabled)
		setVisible(true);
}

Q3PtrList<Q3CanvasItem> Ellipse::moveableItems() const
{
	Q3PtrList<Q3CanvasItem> ret;
	ret.append(point);
	return ret;
}

void Ellipse::newSize(int width, int height)
{
	Q3CanvasEllipse::setSize(width, height);
}

void Ellipse::moveBy(double dx, double dy)
{
	Q3CanvasEllipse::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width() / 2, y() + height() / 2);
}

void Ellipse::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Ellipse::advance(int phase)
{
	Q3CanvasEllipse::advance(phase);

	if (phase == 1 && m_changeEnabled && !dontHide)
	{
		if (count > (m_changeEvery + 10) * 1.8)
			count = 0;
		if (count == 0)
			setVisible(!isVisible());

		count++;
	}
}

void Ellipse::load(KConfig *cfg)
{
	setChangeEnabled(cfg->readEntry("changeEnabled", changeEnabled()));
	setChangeEvery(cfg->readEntry("changeEvery", changeEvery()));
	double newWidth = width(), newHeight = height();
	newWidth = cfg->readEntry("width", newWidth);
	newHeight = cfg->readEntry("height", newHeight);
	newSize(newWidth, newHeight);
}

void Ellipse::save(KConfig *cfg)
{
	cfg->writeEntry("changeEvery", changeEvery());
	cfg->writeEntry("changeEnabled", changeEnabled());
	cfg->writeEntry("width", width());
	cfg->writeEntry("height", height());
}

Config *Ellipse::config(QWidget *parent)
{
	return new EllipseConfig(this, parent);
}

void Ellipse::aboutToSave()
{
	setVisible(true);
	dontHide = true;
}

void Ellipse::savingDone()
{
	dontHide = false;
}

/////////////////////////

Puddle::Puddle(Q3Canvas *canvas)
	: Ellipse(canvas)
{
	setSize(45, 30);

	QBrush brush;
	QPixmap pic;

	if (!QPixmapCache::find("puddle", pic))
	{
		pic.load(locate("appdata", "pics/puddle.png"));
		QPixmapCache::insert("puddle", pic);
	}

	brush.setTexture(pic);
	setBrush(brush);

	KPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setTexture(pointPic);
	point->setBrush(brush);

	setZ(-25);
}

bool Puddle::collision(Ball *ball, long int /*id*/)
{
	if (ball->isVisible())
	{
		Q3CanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
		i.setVisible(true);

		// is center of ball in?
		if (i.collidesWith(this)/* && ball->curVector().magnitude() < 4*/)
		{
			playSound("puddle");
			ball->setAddStroke(ball->addStroke() + 1);
			ball->setPlaceOnGround(true);
			ball->setVisible(false);
			ball->setState(Stopped);
			ball->setVelocity(0, 0);
			if (game && game->curBall() == ball)
				game->stoppedBall();
		}
		else
			return true;
	}

	return false;
}

/////////////////////////

Sand::Sand(Q3Canvas *canvas)
	: Ellipse(canvas)
{
	setSize(45, 40);

	QBrush brush;
	QPixmap pic;

	if (!QPixmapCache::find("sand", pic))
	{
		pic.load(locate("appdata", "pics/sand.png"));
		QPixmapCache::insert("sand", pic);
	}

	brush.setTexture(pic);
	setBrush(brush);

	KPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setTexture(pointPic);
	point->setBrush(brush);

	setZ(-26);
}

bool Sand::collision(Ball *ball, long int /*id*/)
{
	Q3CanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
	i.setVisible(true);

	// is center of ball in?
	if (i.collidesWith(this)/* && ball->curVector().magnitude() < 4*/)
	{
		if (ball->curVector().magnitude() > 0)
			ball->setFrictionMultiplier(7);
		else
		{
			ball->setVelocity(0, 0);
			ball->setState(Stopped);
		}
	}

	return true;
}

/////////////////////////

Putter::Putter(Q3Canvas *canvas)
	: Q3CanvasLine(canvas)
{
	m_showGuideLine = true;
	oneDegree = M_PI / 180;
	len = 9;
	angle = 0;

	guideLine = new Q3CanvasLine(canvas);
	guideLine->setPen(QPen(Qt::white, 1, Qt::DotLine));
	guideLine->setZ(998.8);

	setPen(QPen(Qt::black, 4));
	putterWidth = 11;
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
	Q3CanvasLine::moveBy(dx, dy);
	guideLine->move(x(), y());
}

void Putter::setShowGuideLine(bool yes)
{
	m_showGuideLine = yes;
	setVisible(isVisible());
}

void Putter::setVisible(bool yes)
{
	Q3CanvasLine::setVisible(yes);
	guideLine->setVisible(m_showGuideLine? yes : false);
}

void Putter::setOrigin(int _x, int _y)
{
	setVisible(true);
	move(_x, _y);
	len = 9;
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
			len -= 1;
			guideLine->setVisible(false);
			break;
		case Backwards:
			len += 1;
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
	midPoint.setX(cos(angle) * len);
	midPoint.setY(-sin(angle) * len);

	QPoint start;
	QPoint end;

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

 	guideLine->setPoints(midPoint.x(), midPoint.y(), -cos(angle) * len * 4, sin(angle) * len * 4);

	setPoints(start.x(), start.y(), end.x(), end.y());
}

/////////////////////////

Bumper::Bumper(Q3Canvas *canvas)
	: Q3CanvasEllipse(20, 20, canvas)
{
	setZ(-25);

	firstColor = QColor("#E74804");
	secondColor = firstColor.light();

	count = 0;
	setBrush(firstColor);
	setAnimated(false);

	inside = new Inside(this, canvas);
	inside->setBrush(firstColor.light(109));
	inside->setSize(width() / 2.6, height() / 2.6);
	inside->show();
}

void Bumper::aboutToDie()
{
	delete inside;
}

void Bumper::moveBy(double dx, double dy)
{
	Q3CanvasEllipse::moveBy(dx, dy);
	//const double insideLen = (double)(width() - inside->width()) / 2.0;
	inside->move(x(), y());
}

void Bumper::editModeChanged(bool changed)
{
	inside->setVisible(!changed);
}

void Bumper::advance(int phase)
{
	Q3CanvasEllipse::advance(phase);

	if (phase == 1)
	{
		count++;
		if (count > 2)
		{
			count = 0;
			setBrush(firstColor);
			update();
			setAnimated(false);
		}
	}
}

bool Bumper::collision(Ball *ball, long int /*id*/)
{
	setBrush(secondColor);

	double speed = 1.8 + ball->curVector().magnitude() * .9;
	if (speed > 8)
		speed = 8;

	const QPoint start(x(), y());
	const QPoint end(ball->x(), ball->y());

	Vector betweenVector(start, end);
	betweenVector.setMagnitude(speed);

	// add some randomness so we don't go indefinetely
	betweenVector.setDirection(betweenVector.direction() + deg2rad((KRandom::random() % 3) - 1));

	ball->setVector(betweenVector);
	// for some reason, x is always switched...
	ball->setXVelocity(-ball->xVelocity());
	ball->setState(Rolling);

	setAnimated(true);

	return true;
}

/////////////////////////

Hole::Hole(QColor color, Q3Canvas *canvas)
	: Q3CanvasEllipse(15, 15, canvas)
{
	setZ(998.1);
	setPen(QPen(Qt::black));
	setBrush(color);
}

bool Hole::collision(Ball *ball, long int /*id*/)
{
	bool wasCenter = false;

	switch (result(QPoint(ball->x(), ball->y()), ball->curVector().magnitude(), &wasCenter))
	{
		case Result_Holed:
			place(ball, wasCenter);
			return false;

		default:
		break;
	}

	return true;
}

HoleResult Hole::result(QPoint p, double s, bool * /*wasCenter*/)
{
	const double longestRadius = width() > height()? width() : height();
	if (s > longestRadius / 5.0)
		return Result_Miss;

	Q3CanvasRectangle i(QRect(p, QSize(1, 1)), canvas());
	i.setVisible(true);

	// is center of ball in cup?
	if (i.collidesWith(this))
	{
		return Result_Holed;
	}
	else
		return Result_Miss;
}

/////////////////////////

Cup::Cup(Q3Canvas *canvas)
	: Hole(QColor("#808080"), canvas)
{
	if (!QPixmapCache::find("cup", pixmap))
	{
		pixmap.load(locate("appdata", "pics/cup.png"));
		QPixmapCache::insert("cup", pixmap);
	}
}

void Cup::draw(QPainter &p)
{
	p.drawPixmap(QPoint(x() - width() / 2, y() - height() / 2), pixmap);
}

bool Cup::place(Ball *ball, bool /*wasCenter*/)
{
	ball->setState(Holed);
	playSound("holed");

	// the picture's center is a little different
	ball->move(x() - 1, y());
	ball->setVelocity(0, 0);
	if (game && game->curBall() == ball)
		game->stoppedBall();
	return true;
}

void Cup::save(KConfig *cfg)
{
	cfg->writeEntry("dummykey", true);
}

/////////////////////////

BlackHole::BlackHole(Q3Canvas *canvas)
	: Hole(Qt::black, canvas), exitDeg(0)
{
	infoLine = 0;
	m_minSpeed = 3.0;
	m_maxSpeed = 5.0;
	runs = 0;

	const QColor myColor((QRgb)(KRandom::random() % 0x01000000));

	outside = new Q3CanvasEllipse(canvas);
	outside->setZ(z() - .001);

	outside->setBrush(QBrush(myColor));
	setBrush(Qt::black);

	exitItem = new BlackHoleExit(this, canvas);
	exitItem->setPen(QPen(myColor, 6));
	exitItem->setX(300);
	exitItem->setY(100);

	setSize(width(), width() / .8);
	const float factor = 1.3;
	outside->setSize(width() * factor, height() * factor);
	outside->setVisible(true);

	moveBy(0, 0);

	finishMe();
}

void BlackHole::showInfo()
{
	delete infoLine;
	infoLine = new Q3CanvasLine(canvas());
	infoLine->setVisible(true);
	infoLine->setPen(QPen(exitItem->pen().color(), 2));
	infoLine->setZ(10000);
	infoLine->setPoints(x(), y(), exitItem->x(), exitItem->y());

	exitItem->showInfo();
}

void BlackHole::hideInfo()
{
	delete infoLine;
	infoLine = 0;

	exitItem->hideInfo();
}

void BlackHole::aboutToDie()
{
	Hole::aboutToDie();
	delete outside;
	exitItem->aboutToDie();
	delete exitItem;
}

void BlackHole::updateInfo()
{
	if (infoLine)
	{
		infoLine->setVisible(true);
		infoLine->setPoints(x(), y(), exitItem->x(), exitItem->y());
		exitItem->showInfo();
	}
}

void BlackHole::moveBy(double dx, double dy)
{
	Q3CanvasEllipse::moveBy(dx, dy);
	outside->move(x(), y());
	updateInfo();
}

void BlackHole::setExitDeg(int newdeg)
{
	exitDeg = newdeg;
	if (game && game->isEditing() && game->curSelectedItem() == exitItem)
		game->updateHighlighter();

	exitItem->updateArrowAngle();
	finishMe();
}

Q3PtrList<Q3CanvasItem> BlackHole::moveableItems() const
{
	Q3PtrList<Q3CanvasItem> ret;
	ret.append(exitItem);
	return ret;
}

BlackHoleTimer::BlackHoleTimer(Ball *ball, double speed, int msec)
	: m_speed(speed), m_ball(ball)
{
	QTimer::singleShot(msec, this, SLOT(mySlot()));
	QTimer::singleShot(msec / 2, this, SLOT(myMidSlot()));
}

void BlackHoleTimer::mySlot()
{
	emit eject(m_ball, m_speed);
	delete this;
}

void BlackHoleTimer::myMidSlot()
{
	emit halfway();
}

bool BlackHole::place(Ball *ball, bool /*wasCenter*/)
{
	// most number is 10
	if (runs > 10 && game && game->isInPlay())
		return false;

	playSound("blackholeputin");

	const double diff = (m_maxSpeed - m_minSpeed);
	const double speed = m_minSpeed + ball->curVector().magnitude() * (diff / 3.75);

	ball->setVelocity(0, 0);
	ball->setState(Stopped);
	ball->setVisible(false);
	ball->setForceStillGoing(true);

	double magnitude = Vector(QPoint(x(), y()), QPoint(exitItem->x(), exitItem->y())).magnitude();
	BlackHoleTimer *timer = new BlackHoleTimer(ball, speed, magnitude * 2.5 - speed * 35 + 500);

	connect(timer, SIGNAL(eject(Ball *, double)), this, SLOT(eject(Ball *, double)));
	connect(timer, SIGNAL(halfway()), this, SLOT(halfway()));

	playSound("blackhole");
	return false;
}

void BlackHole::eject(Ball *ball, double speed)
{
	ball->move(exitItem->x(), exitItem->y());

	Vector v;
	v.setMagnitude(10);
	v.setDirection(deg2rad(exitDeg));
	ball->setVector(v);

	// advance ball 10
	ball->doAdvance();

	v.setMagnitude(speed);
	ball->setVector(v);

	ball->setForceStillGoing(false);
	ball->setVisible(true);
	ball->setState(Rolling);

	runs++;

	playSound("blackholeeject");
}

void BlackHole::halfway()
{
	playSound("blackhole");
}

void BlackHole::load(KConfig *cfg)
{
	QPoint exit = cfg->readPointEntry("exit", &exit);
	exitItem->setX(exit.x());
	exitItem->setY(exit.y());
	exitDeg = cfg->readEntry("exitDeg", exitDeg);
	m_minSpeed = cfg->readDoubleNumEntry("minspeed", m_minSpeed);
	m_maxSpeed = cfg->readDoubleNumEntry("maxspeed", m_maxSpeed);
	exitItem->updateArrowAngle();
	exitItem->updateArrowLength();

	finishMe();
}

void BlackHole::finishMe()
{
	double radians = deg2rad(exitDeg);
	QPoint midPoint(0, 0);
	QPoint start;
	QPoint end;
	const int width = 15;

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
		end.setY(midPoint.y() - width);
		end.setX(midPoint.x());
	}

	exitItem->setPoints(start.x(), start.y(), end.x(), end.y());
	exitItem->setVisible(true);
}

void BlackHole::save(KConfig *cfg)
{
	cfg->writeEntry("exit", QPoint(exitItem->x(), exitItem->y()));
	cfg->writeEntry("exitDeg", exitDeg);
	cfg->writeEntry("minspeed", m_minSpeed);
	cfg->writeEntry("maxspeed", m_maxSpeed);
}

/////////////////////////

BlackHoleExit::BlackHoleExit(BlackHole *blackHole, Q3Canvas *canvas)
	: Q3CanvasLine(canvas)
{
	this->blackHole = blackHole;
	arrow = new Arrow(canvas);
	setZ(blackHole->z());
	arrow->setZ(z() - .00001);
	updateArrowLength();
	arrow->setVisible(false);
}

void BlackHoleExit::aboutToDie()
{
	arrow->aboutToDie();
	delete arrow;
}

void BlackHoleExit::moveBy(double dx, double dy)
{
	Q3CanvasLine::moveBy(dx, dy);
	arrow->move(x(), y());
	blackHole->updateInfo();
}

void BlackHoleExit::setPen(QPen p)
{
	Q3CanvasLine::setPen(p);
	arrow->setPen(QPen(p.color(), 1));
}

void BlackHoleExit::updateArrowAngle()
{
	// arrows work in a different angle system
	arrow->setAngle(-deg2rad(blackHole->curExitDeg()));
	arrow->updateSelf();
}

void BlackHoleExit::updateArrowLength()
{
	arrow->setLength(10.0 + 5.0 * (double)(blackHole->minSpeed() + blackHole->maxSpeed()) / 2.0);
	arrow->updateSelf();
}

void BlackHoleExit::editModeChanged(bool editing)
{
	if (editing)
		showInfo();
	else
		hideInfo();
}

void BlackHoleExit::showInfo()
{
	arrow->setVisible(true);
}

void BlackHoleExit::hideInfo()
{
	arrow->setVisible(false);
}

Config *BlackHoleExit::config(QWidget *parent)
{
	return blackHole->config(parent);
}

/////////////////////////

BlackHoleConfig::BlackHoleConfig(BlackHole *blackHole, QWidget *parent)
	: Config(parent)
{
	this->blackHole = blackHole;
	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );
	layout->addWidget(new QLabel(i18n("Exiting ball angle:"), this));
	QSpinBox *deg = new QSpinBox(this);
        deg->setRange( 0, 359 );
        deg->setSingleStep( 10 );
	deg->setSuffix(QString(" ") + i18n("degrees"));
	deg->setValue(blackHole->curExitDeg());
	deg->setWrapping(true);
	layout->addWidget(deg);
	connect(deg, SIGNAL(valueChanged(int)), this, SLOT(degChanged(int)));

	layout->addStretch();

	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Minimum exit speed:"), this));
	KDoubleNumInput *min = new KDoubleNumInput(this);
	min->setRange(0, 8, 1, true);
	hlayout->addWidget(min);
	connect(min, SIGNAL(valueChanged(double)), this, SLOT(minChanged(double)));
	min->setValue(blackHole->minSpeed());

	hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Maximum:"), this));
	KDoubleNumInput *max = new KDoubleNumInput(this);
	max->setRange(1, 10, 1, true);
	hlayout->addWidget(max);
	connect(max, SIGNAL(valueChanged(double)), this, SLOT(maxChanged(double)));
	max->setValue(blackHole->maxSpeed());
}

void BlackHoleConfig::degChanged(int newdeg)
{
	blackHole->setExitDeg(newdeg);
	changed();
}

void BlackHoleConfig::minChanged(double news)
{
	blackHole->setMinSpeed(news);
	changed();
}

void BlackHoleConfig::maxChanged(double news)
{
	blackHole->setMaxSpeed(news);
	changed();
}

/////////////////////////

WallPoint::WallPoint(bool start, Wall *wall, Q3Canvas *canvas)
	: Q3CanvasEllipse(canvas)
{
	this->wall = wall;
	this->start = start;
	alwaysShow = false;
	editing = false;
	visible = true;
	lastId = INT_MAX - 10;
	dontmove = false;

	move(0, 0);
	QPoint p;
	if (start)
		p = wall->startPoint();
	else
		p = wall->endPoint();
	setX(p.x());
	setY(p.y());
}

void WallPoint::clean()
{
	int oldWidth = width();
	setSize(7, 7);
	update();

	Q3CanvasItem *onPoint = 0;
	Q3CanvasItemList l = collisions(true);
	for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
		if ((*it)->rtti() == rtti())
			onPoint = (*it);

	if (onPoint)
		move(onPoint->x(), onPoint->y());

	setSize(oldWidth, oldWidth);
}

void WallPoint::moveBy(double dx, double dy)
{
	Q3CanvasEllipse::moveBy(dx, dy);
	if (!editing)
		updateVisible();

	if (dontmove)
	{
		dontmove = false;
		return;
	}

	if (!wall)
		return;

	if (start)
	{
		wall->setPoints(x(), y(), wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
	}
	else
	{
		wall->setPoints(wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y(), x(), y());
	}
	wall->move(0, 0);
}

void WallPoint::updateVisible()
{
	if (!wall->isVisible())
	{
		visible = false;
		return;
	}

	if (alwaysShow)
		visible = true;
	else
	{
		visible = true;
		Q3CanvasItemList l = collisions(true);
		for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
			if ((*it)->rtti() == rtti())
				visible = false;
	}
}

void WallPoint::editModeChanged(bool changed)
{
	editing = changed;
	setVisible(true);
	if (!editing)
		updateVisible();
}

bool WallPoint::collision(Ball *ball, long int id)
{
	if (ball->curVector().magnitude() <= 0)
		return false;

	long int tempLastId = lastId;
	lastId = id;
	Q3CanvasItemList l = collisions(true);
	for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		if ((*it)->rtti() == rtti())
		{
			WallPoint *point = (WallPoint *)(*it);
			point->lastId = id;
		}
	}

	//kDebug(12007) << "WallPoint::collision id: " << id << ", tempLastId: " << tempLastId << endl;
	Vector ballVector(ball->curVector());

	//kDebug(12007) << "Wall::collision ball speed: " << ball->curVector().magnitude() << endl;
	int allowableDifference = 1;
	if (ballVector.magnitude() < .30)
		allowableDifference = 8;
	else if (ballVector.magnitude() < .50)
		allowableDifference = 6;
	else if (ballVector.magnitude() < .65)
		allowableDifference = 4;
	else if (ballVector.magnitude() < .95)
		allowableDifference = 2;

	if (abs(id - tempLastId) <= allowableDifference)
	{
		//kDebug(12007) << "WallPoint::collision - SKIP\n";
	}
	else
	{
	bool weirdBounce = visible;

	QPoint relStart(start? wall->startPoint() : wall->endPoint());
	QPoint relEnd(start? wall->endPoint() : wall->startPoint());
	Vector wallVector(relStart, relEnd);
	wallVector.setDirection(-wallVector.direction());

	// find the angle between vectors, between 0 and PI
	{
		double difference = fabs(wallVector.direction() - ballVector.direction());
		while (difference > 2 * M_PI)
			difference -= 2 * M_PI;

		if (difference < M_PI / 2 || difference > 3 * M_PI / 2)
			weirdBounce = false;
	}

	playSound("wall", ball->curVector().magnitude() / 10.0);

	ballVector /= wall->dampening;
	const double ballAngle = ballVector.direction();

	double wallAngle = wallVector.direction();

	// opposite bounce, because we're the endpoint
	if (weirdBounce)
		wallAngle += M_PI / 2;

	const double collisionAngle = ballAngle - wallAngle;
	const double leavingAngle = wallAngle - collisionAngle;

	ballVector.setDirection(leavingAngle);
	ball->setVector(ballVector);
	wall->lastId = id;

	//kDebug(12007) << "WallPoint::collision - NOT skip, weirdBounce is " << weirdBounce << endl;
	} // end if that skips

	wall->lastId = id;
	return false;
}

/////////////////////////

Wall::Wall(Q3Canvas *canvas)
	: Q3CanvasLine(canvas)
{
	editing = false;
	lastId = INT_MAX - 10;

	dampening = 1.2;

	startItem = 0;
	endItem = 0;

	moveBy(0, 0);
	setZ(50);

	startItem = new WallPoint(true, this, canvas);
	endItem = new WallPoint(false, this, canvas);
	startItem->setVisible(true);
	endItem->setVisible(true);
	setPen(QPen(Qt::darkRed, 3));

	setPoints(-15, 10, 15, -5);

	moveBy(0, 0);

	editModeChanged(false);
}

void Wall::selectedItem(Q3CanvasItem *item)
{
	if (item->rtti() == Rtti_WallPoint)
	{
		WallPoint *wallPoint = dynamic_cast<WallPoint *>(item);
		if (wallPoint) {
			setPoints(startPoint().x(), startPoint().y(), wallPoint->x() - x(), wallPoint->y() - y());
		}
	}
}

void Wall::clean()
{
	startItem->clean();
	endItem->clean();
}

void Wall::setAlwaysShow(bool yes)
{
	startItem->setAlwaysShow(yes);
	endItem->setAlwaysShow(yes);
}

void Wall::setVisible(bool yes)
{
	Q3CanvasLine::setVisible(yes);

	startItem->setVisible(yes);
	endItem->setVisible(yes);
	startItem->updateVisible();
	endItem->updateVisible();
}

void Wall::setZ(double newz)
{
	Q3CanvasLine::setZ(newz);
	if (startItem)
		startItem->setZ(newz + .002);
	if (endItem)
		endItem->setZ(newz + .001);
}

void Wall::setPen(QPen p)
{
	Q3CanvasLine::setPen(p);

	if (startItem)
		startItem->setBrush(QBrush(p.color()));
	if (endItem)
		endItem->setBrush(QBrush(p.color()));
}

void Wall::aboutToDie()
{
	delete startItem;
	delete endItem;
}

void Wall::setGame(KolfGame *game)
{
	CanvasItem::setGame(game);
	startItem->setGame(game);
	endItem->setGame(game);
}

Q3PtrList<Q3CanvasItem> Wall::moveableItems() const
{
	Q3PtrList<Q3CanvasItem> ret;
	ret.append(startItem);
	ret.append(endItem);
	return ret;
}

void Wall::moveBy(double dx, double dy)
{
	Q3CanvasLine::moveBy(dx, dy);

	if (!startItem || !endItem)
		return;

	startItem->dontMove();
	endItem->dontMove();
	startItem->move(startPoint().x() + x(), startPoint().y() + y());
	endItem->move(endPoint().x() + x(), endPoint().y() + y());
}

void Wall::setVelocity(double vx, double vy)
{
	Q3CanvasLine::setVelocity(vx, vy);
	/*
	startItem->setVelocity(vx, vy);
	endItem->setVelocity(vx, vy);
	*/
}

Q3PointArray Wall::areaPoints() const
{
	// editing we want full width for easy moving
	if (editing)
		return Q3CanvasLine::areaPoints();

	// lessen width, for QCanvasLine::areaPoints() likes
	// to make lines _very_ fat :(
	// from qcanvas.cpp, only the stuff for a line width of 1 taken

	// it's all squished because I don't want my
	// line counts to count code I didn't write!
	Q3PointArray p(4); const int xi = int(x()); const int yi = int(y()); const QPoint start = startPoint(); const QPoint end = endPoint(); const int x1 = start.x(); const int x2 = end.x(); const int y1 = start.y(); const int y2 = end.y(); const int dx = QABS(x1-x2); const int dy = QABS(y1-y2); if ( dx > dy ) { p[0] = QPoint(x1+xi,y1+yi-1); p[1] = QPoint(x2+xi,y2+yi-1); p[2] = QPoint(x2+xi,y2+yi+1); p[3] = QPoint(x1+xi,y1+yi+1); } else { p[0] = QPoint(x1+xi-1,y1+yi); p[1] = QPoint(x2+xi-1,y2+yi); p[2] = QPoint(x2+xi+1,y2+yi); p[3] = QPoint(x1+xi+1,y1+yi); } return p;
}

void Wall::editModeChanged(bool changed)
{
	// make big for debugging?
	const bool debugPoints = false;

	editing = changed;

	startItem->setZ(z() + .002);
	endItem->setZ(z() + .001);
	startItem->editModeChanged(editing);
	endItem->editModeChanged(editing);

	int neww = 0;
	if (changed || debugPoints)
		neww = 10;
	else
		neww = pen().width();

	startItem->setSize(neww, neww);
	endItem->setSize(neww, neww);

	moveBy(0, 0);
}

bool Wall::collision(Ball *ball, long int id)
{
	if (ball->curVector().magnitude() <= 0)
		return false;

	long int tempLastId = lastId;
	lastId = id;
	startItem->lastId = id;
	endItem->lastId = id;

	//kDebug(12007) << "Wall::collision id: " << id << ", tempLastId: " << tempLastId << endl;
	Vector ballVector(ball->curVector());

	//kDebug(12007) << "Wall::collision ball speed: " << ball->curVector().magnitude() << endl;
	int allowableDifference = 1;
	if (ballVector.magnitude() < .30)
		allowableDifference = 8;
	else if (ballVector.magnitude() < .50)
		allowableDifference = 6;
	else if (ballVector.magnitude() < .75)
		allowableDifference = 4;
	else if (ballVector.magnitude() < .95)
		allowableDifference = 2;
	//kDebug(12007) << "Wall::collision allowableDifference is " << allowableDifference << endl;
	if (abs(id - tempLastId) <= allowableDifference)
	{
		//kDebug(12007) << "Wall::collision - SKIP\n";
		return false;
	}

	playSound("wall", ball->curVector().magnitude() / 10.0);

	ballVector /= dampening;
	const double ballAngle = ballVector.direction();

	const double wallAngle = -Vector(startPoint(), endPoint()).direction();
	const double collisionAngle = ballAngle - wallAngle;
	const double leavingAngle = wallAngle - collisionAngle;

	ballVector.setDirection(leavingAngle);
	ball->setVector(ballVector);

	//kDebug(12007) << "Wall::collision - NOT skip\n";
	return false;
}

void Wall::load(KConfig *cfg)
{
	QPoint start(startPoint());
	start = cfg->readPointEntry("startPoint", &start);
	QPoint end(endPoint());
	end = cfg->readPointEntry("endPoint", &end);

	setPoints(start.x(), start.y(), end.x(), end.y());

	moveBy(0, 0);
	startItem->move(start.x(), start.y());
	endItem->move(end.x(), end.y());
}

void Wall::save(KConfig *cfg)
{
	cfg->writeEntry("startPoint", QPoint(startItem->x(), startItem->y()));
	cfg->writeEntry("endPoint", QPoint(endItem->x(), endItem->y()));
}

/////////////////////////

HoleConfig::HoleConfig(HoleInfo *holeInfo, QWidget *parent)
	: Config(parent)
{
	this->holeInfo = holeInfo;

	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );

	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Course name: "), this));
	KLineEdit *nameEdit = new KLineEdit(holeInfo->untranslatedName(), this);
	hlayout->addWidget(nameEdit);
	connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged(const QString &)));

	hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Course author: "), this));
	KLineEdit *authorEdit = new KLineEdit(holeInfo->author(), this);
	hlayout->addWidget(authorEdit);
	connect(authorEdit, SIGNAL(textChanged(const QString &)), this, SLOT(authorChanged(const QString &)));

	layout->addStretch();

	hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Par:"), this));
	QSpinBox *par = new QSpinBox(this);
        par->setRange( 1, 15 );
        par->setSingleStep( 1 );
	par->setValue(holeInfo->par());
	hlayout->addWidget(par);
	connect(par, SIGNAL(valueChanged(int)), this, SLOT(parChanged(int)));
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
	connect(maxstrokes, SIGNAL(valueChanged(int)), this, SLOT(maxStrokesChanged(int)));

	QCheckBox *check = new QCheckBox(i18n("Show border walls"), this);
	check->setChecked(holeInfo->borderWalls());
	layout->addWidget(check);
	connect(check, SIGNAL(toggled(bool)), this, SLOT(borderWallsChanged(bool)));
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

StrokeCircle::StrokeCircle(Q3Canvas *canvas)
	: Q3CanvasItem(canvas)
{
	dvalue = 0;
	dmax = 360;
	iwidth = 100;
	iheight = 100;
	ithickness = 8;
	setZ(10000);
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

bool StrokeCircle::collidesWith(const Q3CanvasItem*) const { return false; }

bool StrokeCircle::collidesWith(const Q3CanvasSprite*, const Q3CanvasPolygonalItem*, const Q3CanvasRectangle*, const Q3CanvasEllipse*, const Q3CanvasText*) const { return false; }

QRect StrokeCircle::boundingRect() const { return QRect(x(), y(), iwidth, iheight); }

void StrokeCircle::setMaxValue(double m)
{
	dmax = m;
	if (dvalue > dmax)
		dvalue = dmax;

	update();
}
void StrokeCircle::setSize(int w, int h)
{
	if (w > 0)
		iwidth = w;
	if (h > 0)
		iheight = h;

	update();
}
void StrokeCircle::setThickness(int t)
{
	if (t > 0)
		ithickness = t;

	update();
}

int StrokeCircle::thickness() const
{
	return ithickness;
}

int StrokeCircle::width() const
{
	return iwidth;
}

int StrokeCircle::height() const
{
	return iheight;
}

void StrokeCircle::draw(QPainter &p)
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

	p.setBrush(QBrush(Qt::black, Qt::NoBrush));
	p.setPen(QPen(Qt::white, ithickness / 2));
	p.drawEllipse(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness);
	p.setPen(QPen(QColor((int)(0xff * dvalue) / dmax, 0, 0xff - (int)(0xff * dvalue) / dmax), ithickness));
	p.drawArc(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness, deg, length);

	p.setPen(QPen(Qt::white, 1));
	p.drawEllipse(x(), y(), iwidth, iheight);
	p.drawEllipse(x() + ithickness, y() + ithickness, iwidth - ithickness * 2, iheight - ithickness * 2);
	p.setPen(QPen(Qt::white, 3));
	p.drawLine(x() + iwidth / 2, y() + iheight - ithickness * 1.5, x() + iwidth / 2, y() + iheight);
	p.drawLine(x() + iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 + iheight / 20, x() + iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 - iheight / 20);
	p.drawLine(x() + iwidth - iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 + iheight / 20, x() + iwidth - iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 - iheight / 20);
}

/////////////////////////////////////////

KolfGame::KolfGame(ObjectList *obj, PlayerList *players, QString filename, QWidget *parent, const char *name )
	: Q3CanvasView(parent, name)
{
	// for mouse control
	setMouseTracking(true);
	viewport()->setMouseTracking(true);
	setFrameShape(NoFrame);

	regAdv = false;
	curHole = 0; // will get ++'d
	cfg = 0;
	setFilename(filename);
	this->players = players;
	this->obj = obj;
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
	fastAdvancedExist = false;
	soundDir = locate("appdata", "sounds/");
	dontAddStroke = false;
	addingNewHole = false;
	scoreboardHoles = 0;
	infoShown = false;
	m_useMouse = true;
	m_useAdvancedPutting = false;
	m_useAdvancedPutting = true;
	m_sound = true;
	m_ignoreEvents = false;
	soundedOnce = false;
#warning port to the new sound system once it exists
//	oldPlayObjects.setAutoDelete(true);
	highestHole = 0;
	recalcHighestHole = false;

	holeInfo.setGame(this);
	holeInfo.setAuthor(i18n("Course Author"));
	holeInfo.setName(i18n("Course Name"));
	holeInfo.setUntranslatedName(i18n("Course Name"));
	holeInfo.setMaxStrokes(10);
	holeInfo.borderWallsChanged(true);

	// width and height are the width and height of the canvas
	// in easy storage
	width = 400;
	height = 400;
	grass = QColor("#35760D");

	margin = 10;

	setFocusPolicy(Qt::StrongFocus);
	setFixedSize(width + 2 * margin, height + 2 * margin);

	setMargins(margin, margin, margin, margin);

	course = new Q3Canvas(this);
	course->setBackgroundColor(Qt::white);
	course->resize(width, height);

	QPixmap pic;
	if (!QPixmapCache::find("grass", pic))
	{
		pic.load(locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", pic);
	}
	course->setBackgroundPixmap(pic);

	setCanvas(course);
	move(0, 0);
	adjustSize();

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		(*it).ball()->setCanvas(course);

	// highlighter shows current item
	highlighter = new Q3CanvasRectangle(course);
	highlighter->setPen(QPen(Qt::yellow, 1));
	highlighter->setBrush(QBrush(Qt::NoBrush));
	highlighter->setVisible(false);
	highlighter->setZ(10000);

	// shows some info about hole
	infoText = new Q3CanvasText(course);
	infoText->setText("");
	infoText->setColor(Qt::white);
	QFont font = kapp->font();
	font.setPixelSize(12);
	infoText->move(15, width/2);
	infoText->setZ(10001);
	infoText->setFont(font);
	infoText->setVisible(false);

	// create the advanced putting indicator
	strokeCircle = new StrokeCircle(course);
	strokeCircle->move(width - 90, height - 90);
	strokeCircle->setSize(80, 80);
	strokeCircle->setThickness(8);
	strokeCircle->setVisible(false);
	strokeCircle->setValue(0);
	strokeCircle->setMaxValue(360);

	// whiteBall marks the spot of the whole whilst editing
	whiteBall = new Ball(course);
	whiteBall->setGame(this);
	whiteBall->setColor(Qt::white);
	whiteBall->setVisible(false);
	whiteBall->setDoDetect(false);

	int highestLog = 0;

	// if players have scores from loaded game, move to last hole
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		if ((int)(*it).scores().count() > highestLog)
			highestLog = (*it).scores().count();

		(*it).ball()->setGame(this);
		(*it).ball()->setAnimated(true);
	}

	// here only for saved games
	if (highestLog)
		curHole = highestLog;

	putter = new Putter(course);

	// border walls:

	// horiz
	addBorderWall(QPoint(margin, margin), QPoint(width - margin, margin));
	addBorderWall(QPoint(margin, height - margin - 1), QPoint(width - margin, height - margin - 1));

	// vert
	addBorderWall(QPoint(margin, margin), QPoint(margin, height - margin));
	addBorderWall(QPoint(width - margin - 1, margin), QPoint(width - margin - 1, height - margin));

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timerMsec = 300;

	fastTimer = new QTimer(this);
	connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimeout()));
	fastTimerMsec = 11;

	autoSaveTimer = new QTimer(this);
	connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSaveTimeout()));
	autoSaveMsec = 5 * 1000 * 60; // 5 min autosave

	// setUseAdvancedPutting() sets maxStrength!
	setUseAdvancedPutting(false);

	putting = false;
	putterTimer = new QTimer(this);
	connect(putterTimer, SIGNAL(timeout()), this, SLOT(putterTimeout()));
	putterTimerMsec = 20;
}

void KolfGame::startFirstHole(int hole)
{
	if (curHole > 0) // if there was saved game, sync scoreboard
	                 // with number of holes
	{
		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			cfg->setGroup(QString("%1-hole@-50,-50|0").arg(scoreboardHoles + 1));
			emit newHole(cfg->readEntry("par", 3));
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
	cfg = new KConfig(filename, false, false);
}

KolfGame::~KolfGame()
{
#warning port to the new sound system once it exists
//	oldPlayObjects.clear();
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

void KolfGame::addBorderWall(QPoint start, QPoint end)
{
	Wall *wall = new Wall(course);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->setVisible(true);
	wall->setGame(this);
	wall->setZ(998.7);
	borderWalls.append(wall);
}

void KolfGame::updateHighlighter()
{
	if (!selectedItem)
		return;
	QRect rect = selectedItem->boundingRect();
	highlighter->move(rect.x() + 1, rect.y() + 1);
	highlighter->setSize(rect.width(), rect.height());
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
		if (inPlay)
			return;

		storedMousePos = e->pos();

		Q3CanvasItemList list = course->collisions(e->pos());
		if (list.first() == highlighter)
			list.pop_front();

		moving = false;
		highlighter->setVisible(false);
		selectedItem = 0;
		movingItem = 0;

		if (list.count() < 1)
		{
			emit newSelectedItem(&holeInfo);
			return;
		}
		// only items we keep track of
		if ((!(items.containsRef(list.first()) || list.first() == whiteBall || extraMoveable.containsRef(list.first()))))
		{
			emit newSelectedItem(&holeInfo);
			return;
		}

		CanvasItem *citem = dynamic_cast<CanvasItem *>(list.first());
		if (!citem || !citem->moveable())
		{
			emit newSelectedItem(&holeInfo);
			return;
		}

		switch (e->button())
		{
			// select AND move now :)
			case Qt::LeftButton:
			{
				selectedItem = list.first();
				movingItem = selectedItem;
				moving = true;

				if (citem->cornerResize())
					setCursor(KCursor::sizeFDiagCursor());
				else
					setCursor(KCursor::sizeAllCursor());

				emit newSelectedItem(citem);
				highlighter->setVisible(true);
				QRect rect = selectedItem->boundingRect();
				highlighter->move(rect.x() + 1, rect.y() + 1);
				highlighter->setSize(rect.width(), rect.height());
			}
			break;

			default:
			break;
		}
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
	// for some reason viewportToContents doesn't work right
	return p - QPoint(margin, margin);
}

// the following four functions are needed to handle both
// border presses and regular in-course presses

void KolfGame::mouseReleaseEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonRelease, viewportToViewport(viewportToContents(e->pos())), e->button(), e->state());
	handleMouseReleaseEvent(&fixedEvent);
}

void KolfGame::mousePressEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonPress, viewportToViewport(viewportToContents(e->pos())), e->button(), e->state());
	handleMousePressEvent(&fixedEvent);
}

void KolfGame::mouseDoubleClickEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonDblClick, viewportToViewport(viewportToContents(e->pos())), e->button(), e->state());
	handleMouseDoubleClickEvent(&fixedEvent);
}

void KolfGame::mouseMoveEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseMove, viewportToViewport(viewportToContents(e->pos())), e->button(), e->state());
	handleMouseMoveEvent(&fixedEvent);
}

void KolfGame::handleMouseMoveEvent(QMouseEvent *e)
{
	if (inPlay || !putter || m_ignoreEvents)
		return;

	QPoint mouse = e->pos();

	// mouse moving of putter
	if (!editing)
	{
		updateMouse();
		return;
	}

	if (!moving)
	{
		// lets change the cursor to a hand
		// if we're hovering over something

		Q3CanvasItemList list = course->collisions(e->pos());
		if (list.count() > 0)
			setCursor(KCursor::handCursor());
		else
			setCursor(KCursor::arrowCursor());
		return;
	}

	int moveX = storedMousePos.x() - mouse.x();
	int moveY = storedMousePos.y() - mouse.y();

	// moving counts as modifying
	if (moveX || moveY)
		setModified(true);

	highlighter->moveBy(-(double)moveX, -(double)moveY);
	movingItem->moveBy(-(double)moveX, -(double)moveY);
	QRect brect = movingItem->boundingRect();
	emit newStatusText(QString("%1x%2").arg(brect.x()).arg(brect.y()));
	storedMousePos = mouse;
}

void KolfGame::updateMouse()
{
	// don't move putter if in advanced putting sequence
	if (!m_useMouse || ((stroking || putting) && m_useAdvancedPutting))
		return;

	const QPoint cursor = viewportToViewport(viewportToContents(mapFromGlobal(QCursor::pos())));
	const QPoint ball((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	putter->setAngle(-Vector(cursor, ball).direction());
}

void KolfGame::handleMouseReleaseEvent(QMouseEvent *e)
{
	setCursor(KCursor::arrowCursor());

	if (editing)
	{
		emit newStatusText(QString::null);
		moving = false;
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
				putter->go(e->key() == Qt::Key_Left? D_Left : D_Right, e->state() & Qt::ShiftModifier? Amount_More : e->state() & Qt::ControlModifier? Amount_Less : Amount_Normal);
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

	if (m_showInfo)
	{
		Q3CanvasItem *item = 0;
		for (item = items.first(); item; item = items.next())
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
			if (citem)
				citem->showInfo();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->showInfo();

		showInfo();
	}
	else
	{
		Q3CanvasItem *item = 0;
		for (item = items.first(); item; item = items.next())
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
			if (citem)
				citem->hideInfo();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->hideInfo();

		hideInfo();
	}
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
			int pw = putter->endPoint().x() - putter->startPoint().x();
			if (pw < 0) pw = -pw;
			int px = (int)putter->x() + pw / 2;
			int py = (int)putter->y();
			if (px > width / 2 && py < height / 2)
				strokeCircle->move(px - pw / 2 - 10 - strokeCircle->width(), py + 10);
			else if (px > width / 2)
				strokeCircle->move(px - pw / 2 - 10 - strokeCircle->width(), py - 10 - strokeCircle->height());
			else if (py < height / 2)
				strokeCircle->move(px + pw / 2 + 10, py + 10);
			else
				strokeCircle->move(px + pw / 2 + 10, py - 10 - strokeCircle->height());
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
	else if ((e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) && !(e->state() & Qt::ControlModifier))
	{
		if (editing && !moving && selectedItem)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(selectedItem);
			if (!citem)
				return;
			citem = citem->itemToDelete();
			if (!citem)
				return;
			Q3CanvasItem *item = dynamic_cast<Q3CanvasItem *>(citem);
			if (citem && citem->deleteable())
			{
				lastDelId = citem->curId();

				highlighter->setVisible(false);
				items.removeRef(item);
				citem->hideInfo();
				citem->aboutToDelete();
				citem->aboutToDie();
				delete citem;
				selectedItem = 0;
				emit newSelectedItem(&holeInfo);

				setModified(true);
			}
		}
	}
	else if (e->key() == Qt::Key_I || e->key() == Qt::Key_Up)
		toggleShowInfo();
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
		if (!course->rect().contains(QPoint((*it).ball()->x(), (*it).ball()->y())))
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
		if ((*it).ball()->forceStillGoing() || ((*it).ball()->curState() == Rolling && (*it).ball()->curVector().magnitude() > 0 && (*it).ball()->isVisible()))
			return;

	int curState = curBall->curState();
	if (curState == Stopped && inPlay)
	{
		inPlay = false;
		QTimer::singleShot(0, this, SLOT(shotDone()));
	}

	if (curState == Holed && inPlay)
	{
		emit inPlayEnd();
		emit playerHoled(&(*curPlayer));

		int curScore = (*curPlayer).score(curHole);
		if (!dontAddStroke)
			curScore++;

		if (curScore == 1)
		{
			playSound("holeinone");
		}
		else if (curScore <= holeInfo.par())
		{
			// I don't have a sound!!
			// *sob*
			// playSound("woohoo");
		}

		(*curPlayer).ball()->setZ((*curPlayer).ball()->z() + .1 - (.1)/(curScore));

		if (allPlayersDone())
		{
			inPlay = false;

			if (curHole > 0 && !dontAddStroke)
			{
				(*curPlayer).addStrokeToHole(curHole);
				emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));
			}
			QTimer::singleShot(600, this, SLOT(holeDone()));
		}
		else
		{
			inPlay = false;
			QTimer::singleShot(0, this, SLOT(shotDone()));
		}
	}
}

void KolfGame::fastTimeout()
{
	// do regular advance every other time
	if (regAdv)
		course->advance();
	regAdv = !regAdv;

	if (!editing)
	{
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->doAdvance();

		if (fastAdvancedExist)
		{
			CanvasItem *citem = 0;
			for (citem = fastAdvancers.first(); citem; citem = fastAdvancers.next())
				citem->doAdvance();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->fastAdvanceDone();

		if (fastAdvancedExist)
		{
			CanvasItem *citem = 0;
			for (citem = fastAdvancers.first(); citem; citem = fastAdvancers.next())
				citem->fastAdvanceDone();
		}
	}
}

void KolfGame::ballMoved()
{
	if (putter->isVisible())
	{
		putter->move((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
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
			const float base = 2.0;

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
				strength -= pow(base, strength / maxStrength) - 1.8;
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
					strength -= rand() % (int)strength;
				}
				else if (!finishStroking)
				{
					deg = putter->curDeg() - 45 + rand() % 90;
					strength -= rand() % (int)strength;
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
	stateDB.clear();

	Q3CanvasItem *item = 0;

	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			stateDB.setName(makeStateGroup(citem->curId(), citem->name()));
			citem->saveState(&stateDB);
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
	Q3CanvasItem *item = 0;

	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			stateDB.setName(makeStateGroup(citem->curId(), citem->name()));
			citem->loadState(&stateDB);
		}
	}

	for (BallStateList::Iterator it = ballStateList.begin(); it != ballStateList.end(); ++it)
	{
		BallStateInfo info = (*it);
		Player &player = (*players->at(info.id - 1));
		player.ball()->move(info.spot.x(), info.spot.y());
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
	double oldx = ball->x(), oldy = ball->y();

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

		Vector v;
		if (ball->placeOnGround(v))
		{
			ball->setPlaceOnGround(false);

			QStringList options;
			const QString placeOutside = i18n("Drop Outside of Hazard");
			const QString rehit = i18n("Rehit From Last Location");
			options << placeOutside << rehit;
			const QString choice = KComboBoxDialog::getItem(i18n("What would you like to do for your next shot?"), i18n("%1 is in a Hazard", (*it).name()), options, placeOutside, "hazardOptions");

			if (choice == placeOutside)
			{
				(*it).ball()->setDoDetect(false);

				double x = ball->x(), y = ball->y();

				while (1)
				{
					Q3CanvasItemList list = ball->collisions(true);
					bool keepMoving = false;
					while (!list.isEmpty())
					{
						Q3CanvasItem *item = list.first();
						if (item->rtti() == Rtti_DontPlaceOn)
							keepMoving = true;

						list.pop_front();
					}
					if (!keepMoving)
						break;

					const float movePixel = 3.0;
					x -= cos(v.direction()) * movePixel;
					y += sin(v.direction()) * movePixel;

					ball->move(x, y);
				}

				// move another two pixels away
				x -= cos(v.direction()) * 2;
				y += sin(v.direction()) * 2;
			}
			else if (choice == rehit)
			{
				for (BallStateList::Iterator it = ballStateList.begin(); it != ballStateList.end(); ++it)
				{
					if ((*it).id == (*curPlayer).id())
					{
						if ((*it).beginningOfHole)
							ball->move(whiteBall->x(), whiteBall->y());
						else
							ball->move((*it).spot.x(), (*it).spot.y());

						break;
					}
				}
			}

			ball->setVisible(true);
			ball->setState(Stopped);

			(*it).ball()->setDoDetect(true);
			ball->collisionDetect(oldx, oldy);
		}
	}

	// emit again
	emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));

	ball->setVelocity(0, 0);

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		Ball *ball = (*it).ball();

		int curStrokes = (*it).score(curHole);
		if (curStrokes >= holeInfo.maxStrokes() && holeInfo.hasMaxStrokes())
		{
			ball->setState(Holed);
			ball->setVisible(false);

			// move to center in case he/she hit out
			ball->move(width / 2, height / 2);
			playerWhoMaxed = (*it).name();

			if (allPlayersDone())
			{
				startNextHole();
				QTimer::singleShot(100, this, SLOT(emitMax()));
				return;
			}

			QTimer::singleShot(100, this, SLOT(emitMax()));
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

	putter->setAngle((*curPlayer).ball());
	putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	updateMouse();

	inPlay = false;
	(*curPlayer).ball()->collisionDetect(oldx, oldy);
}

void KolfGame::emitMax()
{
	emit maxStrokesReached(playerWhoMaxed);
}

void KolfGame::startBall(const Vector &vector)
{
	playSound("hit");

	emit inPlayStart();
	putter->setVisible(false);

	(*curPlayer).ball()->setState(Rolling);
	(*curPlayer).ball()->setVector(vector);

	Q3CanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
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

	startBall(Vector(strength, putter->curAngle() + M_PI));

	addHoleInfo(ballStateList);
}

void KolfGame::addHoleInfo(BallStateList &list)
{
	list.player = (*curPlayer).id();
	list.vector = (*curPlayer).ball()->curVector();
	list.hole = curHole;
}

void KolfGame::sayWhosGoing()
{
	if (players->count() >= 2)
	{
		KMessageBox::information(this, i18n("%1 will start off.", (*curPlayer).name()), i18n("New Hole"), "newHole");
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
		whiteBall->move(width/2, height/2);
		holeInfo.borderWallsChanged(true);
	}

	int leastScore = INT_MAX;

	// to get the first player to go first on every hole,
	// don't do the score stuff below
	curPlayer = players->begin();
	double oldx=(*curPlayer).ball()->x(), oldy=(*curPlayer).ball()->y();

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
			(*it).ball()->move(width / 2, height / 2);
		else
			(*it).ball()->move(whiteBall->x(), whiteBall->y());

		(*it).ball()->setState(Stopped);

		// this gets set to false when the ball starts
		// to move by the Mr. Ball himself.
		(*it).ball()->setBeginningOfHole(true);
		if ((int)(*it).scores().count() < curHole)
			(*it).addHole();
		(*it).ball()->setVelocity(0, 0);
		(*it).ball()->setVisible(false);
	}

	emit newPlayersTurn(&(*curPlayer));

	if (reset)
		openFile();

	inPlay = false;
	timer->start(timerMsec);

	// if (false) { we're done with the round! }
	if (oldCurHole != curHole)
	{
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->setPlaceOnGround(false);

		// here we have to make sure the scoreboard shows
		// all of the holes up until now;

		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			cfg->setGroup(QString("%1-hole@-50,-50|0").arg(scoreboardHoles + 1));
			emit newHole(cfg->readEntry("par", 3));
		}

		resetHoleScores();
		updateShowInfo();

		// this is from shotDone()
		(*curPlayer).ball()->setVisible(true);
		putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		updateMouse();

		ballStateList.canUndo = false;

		(*curPlayer).ball()->collisionDetect(oldx, oldy);
	}

	unPause();
}

void KolfGame::showInfo()
{
	QString text = i18n("Hole %1: par %2, maximum %3 strokes", curHole, holeInfo.par(), holeInfo.maxStrokes());
	infoText->move((width - QFontMetrics(infoText->font()).width(text)) / 2, infoText->y());
	infoText->setText(text);
	// I hate this text! Let's not show it
	//infoText->setVisible(true);

	emit newStatusText(text);
}

void KolfGame::showInfoDlg(bool addDontShowAgain)
{
	KMessageBox::information(parentWidget(),
	i18n("Course name: %1", holeInfo.name()) + QString("\n")
	+ i18n("Created by %1", holeInfo.author()) + QString("\n")
	+ i18n("%1 holes", highestHole),
	i18n("Course Information"),
	addDontShowAgain? holeInfo.name() + QString(" ") + holeInfo.author() : QString::null);
}

void KolfGame::hideInfo()
{
	infoText->setText("");
	infoText->setVisible(false);

	emit newStatusText(QString::null);
}

void KolfGame::openFile()
{
	Object *curObj = 0;

	Q3CanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			// sometimes info is still showing
			citem->hideInfo();
			citem->aboutToDie();
		}
	}

	items.setAutoDelete(true);
	items.clear();
	items.setAutoDelete(false);

	extraMoveable.setAutoDelete(false);
	extraMoveable.clear();
	fastAdvancers.setAutoDelete(false);
	fastAdvancers.clear();
	selectedItem = 0;

	// will tell basic course info
	// we do this here for the hell of it.
	// there is no fake id, by the way,
	// because it's old and when i added ids i forgot to change it.
	cfg->setGroup("0-course@-50,-50");
	holeInfo.setAuthor(cfg->readEntry("author", holeInfo.author()));
	holeInfo.setName(cfg->readEntry("Name", holeInfo.name()));
	holeInfo.setUntranslatedName(cfg->readEntryUntranslated("Name", holeInfo.untranslatedName()));
	emit titleChanged(holeInfo.name());

	cfg->setGroup(QString("%1-hole@-50,-50|0").arg(curHole));
	curPar = cfg->readEntry("par", 3);
	holeInfo.setPar(curPar);
	holeInfo.borderWallsChanged(cfg->readEntry("borderWalls", holeInfo.borderWalls()));
	holeInfo.setMaxStrokes(cfg->readEntry("maxstrokes", 10));
	bool hasFinalLoad = cfg->readEntry("hasFinalLoad", true);

	QStringList missingPlugins;
	QStringList groups = cfg->groupList();

	int numItems = 0;
	int _highestHole = 0;

	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		// [<holeNum>-<name>@<x>,<y>|<id>]
		cfg->setGroup(*it);

		const int len = (*it).length();
		const int dashIndex = (*it).indexOf("-");
		const int holeNum = (*it).left(dashIndex).toInt();
		if (holeNum > _highestHole)
			_highestHole = holeNum;

		const int atIndex = (*it).indexOf("@");
		const QString name = (*it).mid(dashIndex + 1, atIndex - (dashIndex + 1));

		if (holeNum != curHole)
		{
			// if we've had one, break, cause list is sorted
			// erps, no, cause we need to know highest hole!
			if (numItems && !recalcHighestHole)
					break;
			continue;
		}
		numItems++;


		const int commaIndex = (*it).indexOf(",");
		const int pipeIndex = (*it).indexOf("|");
		const int x = (*it).mid(atIndex + 1, commaIndex - (atIndex + 1)).toInt();
		const int y = (*it).mid(commaIndex + 1, pipeIndex - (commaIndex + 1)).toInt();

		// will tell where ball is
		if (name == "ball")
		{
			for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
				(*it).ball()->move(x, y);
			whiteBall->move(x, y);
			continue;
		}

		const int id = (*it).right(len - (pipeIndex + 1)).toInt();

		bool loaded = false;

		for (curObj = obj->first(); curObj; curObj = obj->next())
		{
			if (name != curObj->_name())
				continue;

			Q3CanvasItem *newItem = curObj->newObject(course);
			items.append(newItem);

			CanvasItem *canvasItem = dynamic_cast<CanvasItem *>(newItem);
			if (!canvasItem)
				continue;

			canvasItem->setId(id);
			canvasItem->setGame(this);
			canvasItem->editModeChanged(editing);
			canvasItem->setName(curObj->_name());
			addItemsToMoveableList(canvasItem->moveableItems());
			if (canvasItem->fastAdvance())
				addItemToFastAdvancersList(canvasItem);

			newItem->move(x, y);
			canvasItem->firstMove(x, y);

			newItem->setVisible(true);

			// make things actually show
			if (!hasFinalLoad)
			{
				cfg->setGroup(makeGroup(id, curHole, canvasItem->name(), x, y));
				canvasItem->load(cfg);
				course->update();
			}

			// we don't allow multiple items for the same thing in
			// the file!

			loaded = true;
			break;
		}

		if (!loaded && name != "hole" && missingPlugins.contains(name) <= 0)
			missingPlugins.append(name);
	}

	if (!missingPlugins.empty())
	{
		KMessageBox::informationList(this, QString("<p>&lt;http://katzbrown.com/kolf/Plugins/&gt;</p><p>") + i18n("This hole uses the following plugins, which you do not have installed:") + QString("</p>"), missingPlugins, QString::null, QString("%1 warning").arg(holeInfo.untranslatedName() + QString::number(curHole)));
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
	Q3CanvasItem *qcanvasItem = 0;
	Q3PtrList<CanvasItem> todo;
	Q3PtrList<Q3CanvasItem> qtodo;
	if (hasFinalLoad)
	{
		for (qcanvasItem = items.first(); qcanvasItem; qcanvasItem = items.next())
		{
			CanvasItem *item = dynamic_cast<CanvasItem *>(qcanvasItem);
			if (item)
			{
				if (item->loadLast())
				{
					qtodo.append(qcanvasItem);
					todo.append(item);
				}
				else
				{
					QString group = makeGroup(item->curId(), curHole, item->name(), (int)qcanvasItem->x(), (int)qcanvasItem->y());
					cfg->setGroup(group);
					item->load(cfg);
				}
			}
		}

		CanvasItem *citem = 0;
		qcanvasItem = qtodo.first();
		for (citem = todo.first(); citem; citem = todo.next())
		{
			cfg->setGroup(makeGroup(citem->curId(), curHole, citem->name(), (int)qcanvasItem->x(), (int)qcanvasItem->y()));
			citem->load(cfg);

			qcanvasItem = qtodo.next();
		}
	}

	for (qcanvasItem = items.first(); qcanvasItem; qcanvasItem = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qcanvasItem);
		if (citem)
			citem->updateZ();
	}

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

void KolfGame::addItemsToMoveableList(Q3PtrList<Q3CanvasItem> list)
{
	Q3CanvasItem *item = 0;
	for (item = list.first(); item; item = list.next())
		extraMoveable.append(item);
}

void KolfGame::addItemToFastAdvancersList(CanvasItem *item)
{
	fastAdvancers.append(item);
	fastAdvancedExist = fastAdvancers.count() > 0;
}

void KolfGame::addNewObject(Object *newObj)
{
	Q3CanvasItem *newItem = newObj->newObject(course);
	items.append(newItem);
	newItem->setVisible(true);

	CanvasItem *canvasItem = dynamic_cast<CanvasItem *>(newItem);
	if (!canvasItem)
		return;

	// we need to find a number that isn't taken
	int i = lastDelId > 0? lastDelId : items.count() - 30;
	if (i <= 0)
		i = 0;

	for (;; ++i)
	{
		bool found = false;
		Q3CanvasItem *item = 0;
		for (item = items.first(); item; item = items.next())
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
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
	canvasItem->setId(i);

	canvasItem->setGame(this);

	if (m_showInfo)
		canvasItem->showInfo();
	else
		canvasItem->hideInfo();

	canvasItem->editModeChanged(editing);

	canvasItem->setName(newObj->_name());
	addItemsToMoveableList(canvasItem->moveableItems());

	if (canvasItem->fastAdvance())
		addItemToFastAdvancersList(canvasItem);

	newItem->move(width/2 - 18, height / 2 - 18);

	if (selectedItem)
		canvasItem->selectedItem(selectedItem);

	setModified(true);
}

bool KolfGame::askSave(bool noMoreChances)
{
	if (!modified)
		// not cancel, don't save
		return false;

	int result = KMessageBox::warningYesNoCancel(this, i18n("There are unsaved changes to current hole. Save them?"), i18n("Unsaved Changes"), KStdGuiItem::save(), noMoreChances? KStdGuiItem::discard() : i18n("Save &Later"), noMoreChances? "DiscardAsk" : "SaveAsk");
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
	highlighter->setVisible(false);
	putter->setVisible(!editing);
	inPlay = false;

	// add default objects
	Object *curObj = 0;
	for (curObj = obj->first(); curObj; curObj = obj->next())
		if (curObj->addOnNewHole())
			addNewObject(curObj);

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
	Q3CanvasItem *qcanvasItem = 0;
	for (qcanvasItem = items.first(); qcanvasItem; qcanvasItem = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qcanvasItem);
		if (citem)
			citem->aboutToDie();
	}
	items.setAutoDelete(true);
	items.clear();
	items.setAutoDelete(false);
	emit newSelectedItem(&holeInfo);

	// add default objects
	Object *curObj = 0;
	for (curObj = obj->first(); curObj; curObj = obj->next())
		if (curObj->addOnNewHole())
			addNewObject(curObj);

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
	if (filename.isNull())
	{
		QString newfilename = KFileDialog::getSaveFileName(":kourses", "application/x-kourse", this, i18n("Pick Kolf Course to Save To"));
		if (newfilename.isNull())
			return;

		setFilename(newfilename);
	}

	emit parChanged(curHole, holeInfo.par());
	emit titleChanged(holeInfo.name());

	// we use this bool for optimization
	// in openFile().
	bool hasFinalLoad = false;
	fastAdvancedExist = false;

	Q3CanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			citem->aboutToSave();
			if (citem->loadLast())
				hasFinalLoad = true;
		}
	}

	QStringList groups = cfg->groupList();

	// wipe out all groups from this hole
	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		int holeNum = (*it).left((*it).indexOf("-")).toInt();
		if (holeNum == curHole)
			cfg->deleteGroup(*it);
	}
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
		{
			citem->clean();

			cfg->setGroup(makeGroup(citem->curId(), curHole, citem->name(), (int)item->x(), (int)item->y()));
			citem->save(cfg);
		}
	}

	// save where ball starts (whiteBall tells all)
	cfg->setGroup(QString("%1-ball@%2,%3").arg(curHole).arg((int)whiteBall->x()).arg((int)whiteBall->y()));
	cfg->writeEntry("dummykey", true);

	cfg->setGroup("0-course@-50,-50");
	cfg->writeEntry("author", holeInfo.author());
	cfg->writeEntry("Name", holeInfo.untranslatedName());

	// save hole info
	cfg->setGroup(QString("%1-hole@-50,-50|0").arg(curHole));
	cfg->writeEntry("par", holeInfo.par());
	cfg->writeEntry("maxstrokes", holeInfo.maxStrokes());
	cfg->writeEntry("borderWalls", holeInfo.borderWalls());
	cfg->writeEntry("hasFinalLoad", hasFinalLoad);

	cfg->sync();

	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->savingDone();
	}

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

	moving = false;
	selectedItem = 0;

	editing = !editing;

	if (editing)
	{
		emit editingStarted();
		emit newSelectedItem(&holeInfo);
	}
	else
	{
		emit editingEnded();
		setCursor(KCursor::arrowCursor());
	}

	// alert our items
	Q3CanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
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
	highlighter->setVisible(false);

	// shouldn't see putter whilst editing
	putter->setVisible(!editing);

	if (editing)
		autoSaveTimer->start(autoSaveMsec);
	else
		autoSaveTimer->stop();

	inPlay = false;
}

void KolfGame::playSound(QString /*file*/, double /*vol*/)
{
#warning port to the new sound system when it exists
/*	if (m_sound)
	{
		KPlayObject *oldPlayObject = 0;
		for (oldPlayObject = oldPlayObjects.first(); oldPlayObject; oldPlayObject = oldPlayObjects.next())
		{
			if (oldPlayObject && oldPlayObject->state() != Arts::posPlaying)
			{
				oldPlayObjects.remove();

				// because we will go to next() next time
				// and after remove current item is one after
				// removed item
				(void) oldPlayObjects.prev();
			}
		}

		file = soundDir + file + QString::fromLatin1(".wav");

		// not needed when all of the files are in the distribution
		//if (!QFile::exists(file))
			//return;
		KPlayObjectFactory factory(artsServer.server());
		KPlayObject *playObject = factory.createPlayObject(KUrl(file), true);

		if (playObject && !playObject->isNull())
		{
			if (vol > 1)
				vol = 1;
			else if (vol <= .01)
			{
				delete playObject;
				return;
			}

			if (vol < .99)
			{
				//new KVolumeControl(vol, artsServer.server(), playObject);
			}

			playObject->play();
			oldPlayObjects.append(playObject);
		}
	}*/
}

void HoleInfo::borderWallsChanged(bool yes)
{
	m_borderWalls = yes;
	game->setBorderWalls(yes);
}

void KolfGame::print(KPrinter &pr)
{
	QPainter p(&pr);

	Q3PaintDeviceMetrics metrics(&pr);

	// translate to center
	p.translate(metrics.width() / 2 - course->rect().width() / 2, metrics.height() / 2 - course->rect().height() / 2);

	QPixmap pix(width, height);
	QPainter pixp(&pix);
	course->drawArea(course->rect(), &pixp);
	p.drawPixmap(0, 0, pix);

	p.setPen(QPen(Qt::black, 2));
	p.drawRect(course->rect());

	p.resetMatrix();

	if (pr.option("kde-kolf-title") == "true")
	{
		QString text = i18n("%1 - Hole %2; by %3", holeInfo.name(), curHole, holeInfo.author());
		QFont font(kapp->font());
		font.setPointSize(18);
		QRect rect = QFontMetrics(font).boundingRect(text);
		p.setFont(font);

		p.drawText(metrics.width() / 2 - rect.width() / 2, metrics.height() / 2 - course->rect().height() / 2 -20 - rect.height(), text);
	}
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
	Wall *wall = 0;
	for (wall = borderWalls.first(); wall; wall = borderWalls.next())
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
	KConfig cfg(filename);
	cfg.setGroup("0-course@-50,-50");
	info.author = cfg.readEntry("author", info.author);
	info.name = cfg.readEntry("Name", cfg.readEntry("name", info.name));
	info.untranslatedName = cfg.readEntryUntranslated("Name", cfg.readEntryUntranslated("name", info.name));

	unsigned int hole = 1;
	unsigned int par= 0;
	while (1)
	{
		QString group = QString("%1-hole@-50,-50|0").arg(hole);
		if (!cfg.hasGroup(group))
		{
			hole--;
			break;
		}

		cfg.setGroup(group);
		par += cfg.readEntry("par", 3);

		hole++;
	}

	info.par = par;
	info.holes = hole;
}

void KolfGame::scoresFromSaved(KConfig *config, PlayerList &players)
{
	config->setGroup("0 Saved Game");
	int numPlayers = config->readEntry("Players", 0);
	if (numPlayers <= 0)
		return;

	for (int i = 1; i <= numPlayers; ++i)
	{
		// this is same as in kolf.cpp, but we use saved game values
		config->setGroup(QString::number(i));
		players.append(Player());
		players.last().ball()->setColor(config->readEntry("Color", "#ffffff"));
		players.last().setName(config->readEntry("Name"));
		players.last().setId(i);

		QStringList scores(config->readEntry("Scores",QStringList()));
		Q3ValueList<int> intscores;
		for (QStringList::Iterator it = scores.begin(); it != scores.end(); ++it)
			intscores.append((*it).toInt());

		players.last().setScores(intscores);
	}
}

void KolfGame::saveScores(KConfig *config)
{
	// wipe out old player info
	QStringList groups = config->groupList();
	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		// this deletes all int groups, ie, the player info groups
		bool ok = false;
		(*it).toInt(&ok);
		if (ok)
			config->deleteGroup(*it);
	}

	config->setGroup("0 Saved Game");
	config->writeEntry("Players", players->count());
	config->writeEntry("Course", filename);
	config->writeEntry("Current Hole", curHole);

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		config->setGroup(QString::number((*it).id()));
		config->writeEntry("Name", (*it).name());
		config->writeEntry("Color", (*it).ball()->color().name());

		QStringList scores;
		Q3ValueList<int> intscores = (*it).scores();
		for (Q3ValueList<int>::Iterator it = intscores.begin(); it != intscores.end(); ++it)
			scores.append(QString::number(*it));

		config->writeEntry("Scores", scores);
	}
}

CourseInfo::CourseInfo()
: name(i18n("Course Name")), author(i18n("Course Author")), holes(0), par(0)
{
}

#include "game.moc"
