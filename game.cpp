#include "game.h"
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
#include <phonon/audioplayer.h>

#include <QGraphicsView>
#include <QResizeEvent>
#include <QCheckBox>
#include <QPixmapCache>
#include <QPixmap>
#include <QCursor>
#include <qimage.h>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QToolTip>
#include <QStyleOptionGraphicsItem>

#include <QMouseEvent>
#include <QKeyEvent>
#include <QLayout>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <krandom.h>

#include "kcomboboxdialog.h"
#include "kvolumecontrol.h"
#include "vector.h"

inline QString makeGroup(int id, int hole, QString name, int x, int y)
{
	return QString("%1-%2@%3,%4|%5").arg(hole).arg(name).arg(x).arg(y).arg(id);
}

inline QString makeStateGroup(int id, const QString &name)
{
	return QString("%1|%2").arg(name).arg(id);
}

/////////////////////////

RectPoint::RectPoint(QColor color, RectItem *rect, QGraphicsItem * parent, QGraphicsScene *scene)
: QGraphicsEllipseItem(parent, scene)
{
	setZValue(9999);
	setSize(10, 10);
	this->rect = rect;
	setBrush(QBrush(color));
	setSizeFactor(1.0);
	dontmove = false;
}

void RectPoint::moveBy(double dx, double dy)
{
	QGraphicsEllipseItem::moveBy(dx, dy);

	if (dontmove)
	{
		dontmove = false;
		return;
	}

	QGraphicsItem *qitem = dynamic_cast<QGraphicsItem *>(rect);
	if (!qitem)
		return;

	int nw = ( int )( m_sizeFactor * fabs(x() - qitem->x()) );
	int nh = ( int )( m_sizeFactor * fabs(y() - qitem->y()) );
	if (nw <= 0 || nh <= 0)
		return;

	rect->newSize(nw, nh);
}

Config *RectPoint::config(QWidget *parent)
{
	CanvasItem *citem = dynamic_cast<CanvasItem *>(rect);
	if (citem)
		return citem->config(parent);
	else
		return CanvasItem::config(parent);
}

void RectPoint::setSize(double width, double height)
{
	setRect(x(), y(), width, height);
}


/////////////////////////

Arrow::Arrow(QGraphicsItem * parent, QGraphicsScene *scene)
: QGraphicsLineItem(parent, scene)
{
	line1 = new QGraphicsLineItem(this, scene);
	line2 = new QGraphicsLineItem(this, scene);

	m_angle = 0;
	m_length = 20;
	m_reversed = false;

	setPen(QPen(Qt::black));

	updateSelf();
	setVisible(false);
}

void Arrow::setPen(QPen p)
{
	QGraphicsLineItem::setPen(p);
	line1->setPen(p);
	line2->setPen(p);
}

void Arrow::setZValue(double newz)
{
	QGraphicsLineItem::setZValue(newz);
	line1->setZValue(newz);
	line2->setZValue(newz);
}

void Arrow::setVisible(bool yes)
{
	QGraphicsLineItem::setVisible(yes);
	line1->setVisible(yes);
	line2->setVisible(yes);
}

void Arrow::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
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
	QPointF start( line().x1(), line().y1() );
	QPointF end(( m_length * cos(m_angle) ), ( m_length * sin(m_angle)) );

	if (m_reversed)
	{
		QPointF tmp(start);
		start = end;
		end = tmp;
	}

	setLine(start.x(), start.y(), end.x(), end.y());

	const double lineLen = m_length / 2;

	const double angle1 = m_angle - M_PI / 2 - 1;
	line1->setPos(end.x() + x(), end.y() + y());
	start = end;
	end = QPoint(int( lineLen * cos(angle1) ), int( lineLen * sin(angle1) ) );
	line1->setLine(0, 0, end.x(), end.y());

	const double angle2 = m_angle + M_PI / 2 + 1;
	line2->setPos(start.x() + x(), start.y() + y());
	end = QPointF(lineLen * cos(angle2), lineLen * sin(angle2));
	line2->setLine(0, 0, end.x(), end.y());
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

Bridge::Bridge(QRect rect, QGraphicsItem *parent, QGraphicsScene *scene, QString type)
: QGraphicsRectItem(rect, parent, scene)
{
	this->type = type;
	QColor color("#92772D");
	setBrush(Qt::NoBrush);
        setPen(Qt::NoPen);
	setZValue(998);
	
	//not using antialiasing because it looks too blurry here
	topWall = new Wall(parent, scene, false);
	topWall->setAlwaysShow(true);
	botWall = new Wall(parent, scene, false);
	botWall->setAlwaysShow(true);
	leftWall = new Wall(parent, scene, false);
	leftWall->setAlwaysShow(true);
	rightWall = new Wall(parent, scene, false);
	rightWall->setAlwaysShow(true);

	setWallZ(zValue() + 0.01);
	setWallColor(color);

	topWall->setVisible(false);
	botWall->setVisible(false);
	leftWall->setVisible(false);
	rightWall->setVisible(false);

	pixmapInitialised=false;

	point = new RectPoint(color, this, parent, scene);
	editModeChanged(false);

	newSize(width(), height());
}

void Bridge::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) 
{
	if(pixmapInitialised == false) {
		if(game == 0)
			return;
		else {
			pixmap=game->renderer->renderSvg(type, (int)rect().width(), (int)rect().height(), 0);
			pixmapInitialised=true;
		}
	}
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

void Bridge::resize(double resizeFactor)
{
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	setRect(0, 0, baseWidth*resizeFactor, baseHeight*resizeFactor);
	pixmap=game->renderer->renderSvg(type, (int)rect().width(), (int)rect().height(), 0);
	botWall->setPos(baseBotWallX*resizeFactor, baseBotWallY*resizeFactor);
	botWall->resize(resizeFactor);
	topWall->setPos(baseTopWallX*resizeFactor, baseTopWallY*resizeFactor);
	topWall->resize(resizeFactor);
	leftWall->setPos(baseLeftWallX*resizeFactor, baseLeftWallY*resizeFactor);
	leftWall->resize(resizeFactor);
	rightWall->setPos(baseRightWallX*resizeFactor, baseRightWallY*resizeFactor);
	rightWall->resize(resizeFactor);
}

bool Bridge::collision(Ball *ball, long int /*id*/)
{
	ball->setFrictionMultiplier(.63);
	return false;
}

void Bridge::setWallZ(double newz)
{
	topWall->setZValue(newz);
	botWall->setZValue(newz);
	leftWall->setZValue(newz);
	rightWall->setZValue(newz);
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
	QGraphicsRectItem::moveBy(dx, dy);

	point->dontMove();
	point->setPos(x() + width(), y() + height());

	topWall->setPos(x(), y());
	botWall->setPos(x(), y() - 1);
	leftWall->setPos(x(), y());
	rightWall->setPos(x(), y());

	QList<QGraphicsItem *> list = collidingItems();
	for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem)
			citem->updateZ();
	}
}

void Bridge::load(KConfigGroup *cfgGroup)
{
	doLoad(cfgGroup);
}

void Bridge::doLoad(KConfigGroup *cfgGroup)
{
	newSize(cfgGroup->readEntry("width", width()), cfgGroup->readEntry("height", height()));
	setTopWallVisible(cfgGroup->readEntry("topWallVisible", topWallVisible()));
	setBotWallVisible(cfgGroup->readEntry("botWallVisible", botWallVisible()));
	setLeftWallVisible(cfgGroup->readEntry("leftWallVisible", leftWallVisible()));
	setRightWallVisible(cfgGroup->readEntry("rightWallVisible", rightWallVisible()));

	baseX = x();
	baseY = y();
	baseTopWallX = topWall->x();
	baseTopWallY = topWall->y();
	baseBotWallX = botWall->x();
	baseBotWallY = botWall->y();
	baseLeftWallX = leftWall->x();
	baseLeftWallY = leftWall->y();
	baseRightWallX = rightWall->x();
	baseRightWallY = rightWall->y();
}

void Bridge::save(KConfigGroup *cfgGroup)
{
	doSave(cfgGroup);
}

void Bridge::doSave(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("width", width());
	cfgGroup->writeEntry("height", height());
	cfgGroup->writeEntry("topWallVisible", topWallVisible());
	cfgGroup->writeEntry("botWallVisible", botWallVisible());
	cfgGroup->writeEntry("leftWallVisible", leftWallVisible());
	cfgGroup->writeEntry("rightWallVisible", rightWallVisible());
}

QList<QGraphicsItem *> Bridge::moveableItems() const
{
	QList<QGraphicsItem *> ret;
	ret.append(point);
	return ret;
}

void Bridge::newSize(double width, double height)
{
	Bridge::setSize(width, height);
}

void Bridge::setSize(double width, double height)
{
	setRect(rect().x(), rect().y(), width, height);

	topWall->setPoints(0, 0, width, 0);
	botWall->setPoints(0, height, width, height);
	leftWall->setPoints(0, 0, 0, height);
	rightWall->setPoints(width, 0, width, height);

	if(game != 0)
		pixmap=game->renderer->renderSvg(type, (int)width, (int)height, 0);
	baseWidth = width;
	baseHeight = height;

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
	slider->setValue( (int)windmill->curSpeed() );
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

Windmill::Windmill(QRect rect, QGraphicsItem * parent, QGraphicsScene *scene)
: Bridge(rect, parent, scene, "windmill"), speedfactor(16), m_bottom(true)
{
	baseGuardSpeed = 5;
	guard = new WindmillGuard(0, scene);
	guard->setPen(QPen(Qt::black, 5));
	guard->setBasePenWidth(5);
	guard->setVisible(true);
	guard->setAlwaysShow(true);
	setSpeed(baseGuardSpeed);
	guard->setZValue(wallZ() + .1);

	//not using antialiasing because it looks too blurry here
	left = new Wall(0, scene, false);
	left->setPen(wallPen());
	left->setAlwaysShow(true);
	right = new Wall(0, scene, false);
	right->setPen(wallPen());
	right->setAlwaysShow(true);
	left->setZValue(wallZ());
	right->setZValue(wallZ());
	left->setVisible(true);
	right->setVisible(true);

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(true);
	setRightWallVisible(true);

	newSize(width(), height());
	moveBy(0, 0);
}

void Windmill::resize(double resizeFactor)
{
	Bridge::resize(resizeFactor);
	guard->setBetween(baseGuardMin*resizeFactor, baseGuardMax*resizeFactor);
	guard->QGraphicsLineItem::setPos(baseGuardX*resizeFactor, baseGuardY*resizeFactor);
	guard->resize(resizeFactor);
	setSpeed(baseGuardSpeed*resizeFactor);
	left->QGraphicsLineItem::setPos(baseLeftX*resizeFactor, baseLeftY*resizeFactor);
	left->resize(resizeFactor);
	right->QGraphicsLineItem::setPos(baseRightX*resizeFactor, baseRightY*resizeFactor);
	right->resize(resizeFactor);
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

void Windmill::setSpeed(double news)
{
	if (news < 0)
		return;
	speed = news;
	guard->setXVelocity((news/3) * (guard->getXVelocity() > 0? 1 : -1));
}

void Windmill::setGame(KolfGame *game)
{
	Bridge::setGame(game);
	guard->setGame(game);
	left->setGame(game);
	right->setGame(game);
}

void Windmill::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("speed", speed);
	cfgGroup->writeEntry("bottom", m_bottom);

	doSave(cfgGroup);
}

void Windmill::load(KConfigGroup *cfgGroup)
{
	setSpeed(cfgGroup->readEntry("speed", -1));

	doLoad(cfgGroup);

	left->editModeChanged(false);
	right->editModeChanged(false);
	guard->editModeChanged(false);

	setBottom(cfgGroup->readEntry("bottom", true));

	baseGuardMin = guard->getMin();
	baseGuardMax = guard->getMax();
	baseGuardX = guard->x();
	baseGuardY = guard->y();
	baseLeftX = left->x();
	baseLeftY = left->y();
	baseRightX = right->x();
	baseRightY = right->y();
}

void Windmill::moveBy(double dx, double dy)
{
	Bridge::moveBy(dx, dy);

	left->setPos(x(), y());
	right->setPos(x(), y());

	//guard->moveBy(dx, dy);
	guard->setPos(x(), y());
	guard->setBetween(x(), x() + width());
}

void Windmill::setSize(double width, double height)
{
	newSize(width, height);
}

void Windmill::setBottom(bool yes)
{
	m_bottom = yes;
	newSize(width(), height());
}

void Windmill::newSize(double width, double height)
{
	Bridge::newSize(width, height);

	const double indent = width / 4;

	double indentY = m_bottom? height : 0;
	left->setPoints(0, indentY, indent, indentY);
	right->setPoints(width - indent, indentY, width, indentY);

	guard->setBetween(x(), x() + width);
	double guardY = m_bottom? height + 4 : -4;
	guard->setPoints(0, guardY, (double)indent / (double)1.07 - 2, guardY);
	//guard->setPoints(x()+0, y()+guardY, x()+(double)indent / (double)1.07 - 2, y()+guardY);
}

/////////////////////////

void WindmillGuard::advance(int phase)
{
	if (phase == 1)
	{
		Wall::doAdvance();
		if (x() + line().x1() <= min)
			setXVelocity(fabs(getXVelocity()));
		else if (x() + line().x2() >= max)
			setXVelocity(-fabs(getXVelocity()));
	}
}

/////////////////////////

Sign::Sign(QGraphicsItem * parent, QGraphicsScene *scene)
: Bridge(QRect(0, 0, 110, 40), parent, scene, "sign")
{
	setZValue(998.8);
	m_text = m_untranslatedText = i18n("New Text");
	setBrush(QBrush(Qt::white));
	setWallColor(Qt::black);
	setWallZ(zValue() + .01);
	baseFontPixelSize = fontPixelSize = 12;

	setTopWallVisible(true);
	setBotWallVisible(true);
	setLeftWallVisible(true);
	setRightWallVisible(true);
}

void Sign::resize(double resizeFactor)
{
	fontPixelSize = baseFontPixelSize*resizeFactor;
	Bridge::resize(resizeFactor);
}

void Sign::load(KConfigGroup *cfgGroup)
{
	m_text = cfgGroup->readEntry("Comment", m_text);
	m_untranslatedText = cfgGroup->readEntryUntranslated("Comment", m_untranslatedText);

	doLoad(cfgGroup);
}

void Sign::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("Comment", m_untranslatedText);

	doSave(cfgGroup);
}

void Sign::setText(const QString &text)
{
	m_text = text;
	m_untranslatedText = text;
}

void Sign::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	const QStyleOptionGraphicsItem * style = new QStyleOptionGraphicsItem();
	Bridge::paint(painter, style);

	painter->setPen(QPen(Qt::black, 1));
	QGraphicsTextItem txt;
	txt.setFont(kapp->font());
	QFont font = kapp->font();
	font.setPixelSize((int)fontPixelSize);
	txt.setFont(font);
	txt.setHtml(m_text);
	const int indent = wallPen().width() + 13;
	txt.setTextWidth(width() - 2*indent); 
	txt.paint(painter, style, 0);
	//txt.paint(painter, x() + indent, y(), QRect(x() + indent, y(), width() - indent, height() - indent), colorGroup); 
	// minor problem, can't find how to set the start the rect for html text, so the next is right next to the egde of the text box
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
EllipseConfig::EllipseConfig(KolfEllipse *_ellipse, QWidget *parent)
: Config(parent), slow1(0), fast1(0), slow2(0), fast2(0), slider1(0), slider2(0)
{
	this->ellipse = _ellipse;

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

KolfEllipse::KolfEllipse(QGraphicsItem *parent, QGraphicsScene *scene, QString type)
: QGraphicsEllipseItem(parent, scene)
{
	this->type = type;
	savingDone();
	setChangeEnabled(false);
	setChangeEvery(50);
	count = 0;
	setVisible(true);
	setPen(QPen(Qt::NoPen));

	point = new RectPoint(Qt::black, this, parent, scene);
	point->setSizeFactor(2.0);
}

void KolfEllipse::firstMove(int x, int y)
{
	baseX = (double)x;
	baseY = (double)y;
}

void KolfEllipse::aboutToDie()
{
	delete point;
}

void KolfEllipse::setChangeEnabled(bool changeEnabled)
{
	m_changeEnabled = changeEnabled;
	setAnimated(m_changeEnabled);

	if (!m_changeEnabled)
		setVisible(true);
}

QList<QGraphicsItem *> KolfEllipse::moveableItems() const
{
	QList<QGraphicsItem *> ret;
	ret.append(point);
	return ret;
}

void KolfEllipse::resize(double resizeFactor)
{
	setRect(baseWidth*resizeFactor*-0.5, baseHeight*resizeFactor*-0.5, baseWidth*resizeFactor, baseHeight*resizeFactor);
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	moveBy(0, 0); 
	pixmap=game->renderer->renderSvg(type, (int)rect().width(), (int)rect().height(), 0);
}

void KolfEllipse::newSize(double width, double height)
{
	setSize(width, height);
}


void KolfEllipse::setSize(double width, double height)
{
	setRect(width*-0.5, height*-0.5, width, height);
	if(game != 0)
		pixmap=game->renderer->renderSvg(type, (int)width, (int)height, 0);
	baseWidth = width;
	baseHeight = height;
}

void KolfEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/ ) 
{
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

void KolfEllipse::moveBy(double dx, double dy)
{
	QGraphicsEllipseItem::moveBy(dx, dy);

	point->dontMove();
	point->setPos(x() + width()/2, y() + height()/2);
}

void KolfEllipse::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void KolfEllipse::advance(int phase)
{
	QGraphicsEllipseItem::advance(phase);

	if (phase == 1 && m_changeEnabled && !dontHide)
	{
		if (count > (m_changeEvery + 10) * 1.8)
			count = 0;
		if (count == 0)
			setVisible(!isVisible());

		count++;
	}
}

void KolfEllipse::load(KConfigGroup *cfgGroup)
{
	setChangeEnabled(cfgGroup->readEntry("changeEnabled", changeEnabled()));
	setChangeEvery(cfgGroup->readEntry("changeEvery", changeEvery()));
	double newWidth = width(), newHeight = height();
	newWidth = cfgGroup->readEntry("width", newWidth);
	newHeight = cfgGroup->readEntry("height", newHeight);
	setSize(newWidth, newHeight);
	moveBy(0, 0); 
} 

void KolfEllipse::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("changeEvery", changeEvery());
	cfgGroup->writeEntry("changeEnabled", changeEnabled());
	cfgGroup->writeEntry("width", width());
	cfgGroup->writeEntry("height", height());
}

Config *KolfEllipse::config(QWidget *parent)
{
	return new EllipseConfig(this, parent);
}

void KolfEllipse::aboutToSave()
{
	setVisible(true);
	dontHide = true;
}

void KolfEllipse::savingDone()
{
	dontHide = false;
}

/////////////////////////

Puddle::Puddle(QGraphicsItem * parent, QGraphicsScene *scene)
: KolfEllipse(parent, scene, "puddle")
{
	setData(0, Rtti_DontPlaceOn);
	setSize(45, 30);

	QBrush brush;
	QPixmap pic;

	if (!QPixmapCache::find("puddle", pic))
	{
		pic.load(KStandardDirs::locate("appdata", "pics/puddle.png"));
		QPixmapCache::insert("puddle", pic);
	}

	brush.setTexture(pic);
	setBrush(brush);

	QPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setTexture(pointPic);
	point->setBrush(QBrush(QColor("blue")));

	setZValue(-25);
}

bool Puddle::collision(Ball *ball, long int /*id*/)
{
	if (ball->isVisible())
	{
		QGraphicsRectItem i(QRectF(ball->x(), ball->y(), 1, 1), 0, 0);
		i.setVisible(true);

		// is center of ball in?
		if (i.collidesWithItem(this)/* && ball->curVector().magnitude() < 4*/)
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

Sand::Sand(QGraphicsItem * parent, QGraphicsScene *scene)
: KolfEllipse(parent, scene, "sand")
{
	setSize(45, 40);

	QBrush brush;
	QPixmap pic;

	if (!QPixmapCache::find("sand", pic))
	{
		pic.load(KStandardDirs::locate("appdata", "pics/sand.png"));
		QPixmapCache::insert("sand", pic);
	}

	brush.setTexture(pic);
	setBrush(brush);

	QPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setTexture(pointPic);
	point->setBrush(brush);

	setZValue(-26);
}

bool Sand::collision(Ball *ball, long int /*id*/)
{
	QGraphicsRectItem i(QRectF(ball->x(), ball->y(), 1, 1), 0, 0);
	i.setVisible(true);

	// is center of ball in?
	if (i.collidesWithItem(this)/* && ball->curVector().magnitude() < 4*/)
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

Putter::Putter(QGraphicsScene *scene)
: QGraphicsLineItem(0, scene)
{
	setData(0, Rtti_Putter);
	m_showGuideLine = true;
	oneDegree = M_PI / 180;
	resizeFactor = 1;
	baseGuideLineLength = guideLineLength = 9;
	baseGuideLineThickness = 1;
	basePutterThickness = 4;
	basePutterWidth = putterWidth = 11;
	angle = 0;

	guideLine = new QGraphicsLineItem(this, scene);
	guideLine->setPen(QPen(Qt::white, baseGuideLineThickness, Qt::DotLine));
	guideLine->setZValue(998.8);

	setPen(QPen(Qt::black, basePutterThickness));
	maxAngle = 2 * M_PI;

	hideInfo();

	// this also sets Z
	resetAngles();
}

void Putter::resize(double resizeFactor)
{
	this->resizeFactor = resizeFactor;
	guideLineLength = baseGuideLineLength * resizeFactor;
	guideLine->setPen(QPen(Qt::white, baseGuideLineThickness*resizeFactor, Qt::DotLine));
	putterWidth = basePutterWidth * resizeFactor;
	setPen(QPen(Qt::black, basePutterThickness*resizeFactor));
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
	guideLineLength = baseGuideLineLength * resizeFactor;
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
			guideLineLength -= 1*resizeFactor;
			guideLine->setVisible(false);
			break;
		case Backwards:
			guideLineLength += 1*resizeFactor;
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

Bumper::Bumper(QGraphicsItem * parent, QGraphicsScene *scene)
: QGraphicsEllipseItem(parent, scene)
{
	baseDiameter=20;
	setRect(-0.5*baseDiameter, -0.5*baseDiameter, baseDiameter, baseDiameter);
	setZValue(-25);
	pixmapInitialised=false;

	count = 0;
	setAnimated(false);
}

void Bumper::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) 
{
	if(pixmapInitialised == false) {
		if(game == 0)
			return;
		else {
			//ensure the bumper_on pixmap is in the cache so it will be immediately available when required
			if(!QPixmapCache::find("bumper_on"))
				pixmap=game->renderer->renderSvg("bumper_on", (int)rect().width(), (int)rect().height(), 0);
			pixmap=game->renderer->renderSvg("bumper_off", (int)rect().width(), (int)rect().height(), 0);
			pixmapInitialised=true;
		}
	}
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

void Bumper::moveBy(double x, double y)
{
        QGraphicsEllipseItem::moveBy(x, y);
}

void Bumper::firstMove(int x, int y)
{
	baseX = (double)x;
	baseY = (double)y;
}

void Bumper::resize(double resizeFactor)
{
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	setRect(-0.5*baseDiameter*resizeFactor, -0.5*baseDiameter*resizeFactor, baseDiameter*resizeFactor, baseDiameter*resizeFactor);
	pixmapInitialised=false; //do I need this?
	pixmap=game->renderer->renderSvg("bumper_off", (int)rect().width(), (int)rect().height(), 0);
}

void Bumper::advance(int phase)
{
	if(!animated)
		return;

	QGraphicsEllipseItem::advance(phase);

	if (phase == 1)
	{
		count++;
		if (count > 2)
		{
			count = 0;
			pixmap=game->renderer->renderSvg("bumper_off", (int)rect().width(), (int)rect().height(), 0);
			update(); 
			setAnimated(false);
		}
	}
}

bool Bumper::collision(Ball *ball, long int /*id*/)
{
	pixmap=game->renderer->renderSvg("bumper_on", (int)rect().width(), (int)rect().width(), 0);
	update();

	double speed = 1.8 + ball->curVector().magnitude() * .9;
	if (speed > 8)
		speed = 8;

	const QPointF start(x(), y());
	const QPointF end(ball->x(), ball->y());

	Vector betweenVector(start, end);
	betweenVector.setMagnitude(speed);

	// add some randomness so we don't go indefinetely
	betweenVector.setDirection(betweenVector.direction() + deg2rad((KRandom::random() % 3) - 1));

	ball->setVector(betweenVector);
	// for some reason, x is always switched...
	ball->setXVelocity(-ball->getXVelocity());
	ball->setState(Rolling);

	setAnimated(true);

	return true;
}

/////////////////////////

Cup::Cup(QGraphicsItem * parent, QGraphicsScene * scene)
	: QGraphicsEllipseItem(parent, scene)
{
	baseDiameter = 16;
	setRect(-0.5*baseDiameter, -0.5*baseDiameter, baseDiameter, baseDiameter);
	pixmapInitialised=false;

	setZValue(998.1);
}

void Cup::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) 
{
	if(pixmapInitialised == false) {
		if(game == 0)
			return;
		else {
			pixmap=game->renderer->renderSvg("cup", (int)rect().width(), (int)rect().height(), 0);
			pixmapInitialised=true;
		}
	}
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

bool Cup::place(Ball *ball, bool /*wasCenter*/)
{
	ball->setState(Holed);
	playSound("holed");

	ball->setResizedPos(x(), y());
	ball->setVelocity(0, 0);
	return true;
}

void Cup::moveBy(double x, double y)
{
        QGraphicsEllipseItem::moveBy(x, y);
}

void Cup::firstMove(int x, int y)
{
	baseX = (double)x;
	baseY = (double)y;
}

void Cup::resize(double resizeFactor)
{
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	setRect(-0.5*baseDiameter*resizeFactor, -0.5*baseDiameter*resizeFactor, baseDiameter*resizeFactor, baseDiameter*resizeFactor);
	pixmap=game->renderer->renderSvg("cup", (int)rect().width(), (int)rect().height(), 0);
}

void Cup::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("dummykey", true);
}

void Cup::saveState(StateDB *db)
{
	db->setPoint(QPointF(x(), y()));
}

void Cup::loadState(StateDB *db)
{
	const QPointF moveTo = db->point();
	setPos(moveTo.x(), moveTo.y());
}

bool Cup::collision(Ball *ball, long int /*id*/)
{
	bool wasCenter = false;

	switch (result(QPointF(ball->x() + ball->width()/2, ball->y() + ball->height()/2), ball->curVector().magnitude(), &wasCenter))
	{
		case Result_Holed:
			place(ball, wasCenter);
			return false;

		default:
			break;
	}

	return true;
}

HoleResult Cup::result(QPointF p, double speed, bool * /*wasCenter*/)
{
	if (speed > 3.75)
		return Result_Miss;

	QPointF holeCentre(x() + boundingRect().width()/2, y() + boundingRect().height()/2);
	double distanceSquared = (holeCentre.x() - p.x())*(holeCentre.x() - p.x()) + (holeCentre.y() - p.y())*(holeCentre.y() - p.y());

	if(distanceSquared < (boundingRect().width()/2)*(boundingRect().width()/2))
		return Result_Holed;
	else
		return Result_Miss;
}

/////////////////////////

BlackHole::BlackHole(QGraphicsItem * parent, QGraphicsScene *scene)
	: QGraphicsEllipseItem(-8, -9, 16, 18, parent, scene), exitDeg(0)
{
	setZValue(998.1);

	infoLine = 0;
	m_minSpeed = 3.0;
	m_maxSpeed = 5.0;
	runs = 0;
	baseInfoLineThickness = 2;
	baseExitLineWidth = 15;
	baseWidth = rect().width();
	baseHeight = rect().height();

	const QColor myColor((QRgb)(KRandom::random() % 0x01000000));
	QPen pen(myColor);
        setPen(Qt::NoPen);
        setBrush(myColor);

	exitItem = new BlackHoleExit(this, 0, scene);
	exitItem->setPen(QPen(myColor, 6));
	exitItem->setPos(300, 100);

	setSize(baseWidth, baseHeight);
	pixmapInitialised=false;

	moveBy(0, 0); 

	finishMe();
}

void BlackHole::paint(QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget *widget) 
{
	if(pixmapInitialised == false) {
		if(game == 0)
			return;
		else {
			pixmap=game->renderer->renderSvg("black_hole", (int)rect().width(), (int)rect().height(), 0);
			pixmapInitialised=true;
		}
	}
	QGraphicsEllipseItem::paint(painter, option, widget);
	painter->drawPixmap((int)rect().x(), (int)rect().y(), pixmap);  
}

void BlackHole::showInfo()
{
	delete infoLine;
	infoLine = new AntialisedLine(0, scene());
	infoLine->setVisible(true);
	infoLine->setPen(QPen(exitItem->pen().color(), baseInfoLineThickness));
	infoLine->setZValue(10000);
	infoLine->setLine(x(), y(), exitItem->x(), exitItem->y());

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
	//Hole::aboutToDie();
	exitItem->aboutToDie();
	delete exitItem;
}

void BlackHole::resize(double resizeFactor)
{
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	setRect(-0.5*baseWidth*resizeFactor, -0.5*baseHeight*resizeFactor, baseWidth*resizeFactor, baseHeight*resizeFactor);
	pixmap=game->renderer->renderSvg("black_hole", (int)(baseWidth*resizeFactor), (int)(baseHeight*resizeFactor), 0);
	exitItem->setPos(baseExitX*resizeFactor, baseExitY*resizeFactor);
	finishMe(baseExitLineWidth*resizeFactor);
	if(infoLine) {
		infoLine->setPen(QPen(exitItem->pen().color(), baseInfoLineThickness*resizeFactor));
		infoLine->setLine(x(), y(), exitItem->x(), exitItem->y());
	}
	exitItem->setArrowPen(QPen(exitItem->pen().color(), exitItem->getBaseArrowPenThickness()*resizeFactor));
	exitItem->updateArrowLength(resizeFactor);
}

void BlackHole::updateInfo()
{
	if (infoLine)
	{
		infoLine->setVisible(true);
		infoLine->setLine(x(), y(), exitItem->x(), exitItem->y());
		exitItem->showInfo();
	}
}

void BlackHole::moveBy(double dx, double dy)
{
	QGraphicsEllipseItem::moveBy(dx, dy);
	exitItem->moveBy(dx, dy);
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

QList<QGraphicsItem *> BlackHole::moveableItems() const
{
	QList<QGraphicsItem *> ret;
	ret.append(exitItem);
	return ret;
}

bool BlackHole::collision(Ball *ball, long int /*id*/)
{
	bool wasCenter = false;

	switch (result(QPointF(ball->x(), ball->y()), ball->curVector().magnitude(), &wasCenter))
	{
		case Result_Holed:
			place(ball, wasCenter);
			return false;

		default:
			break;
	}

	return true;
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

	double magnitude = Vector(QPointF(x(), y()), QPointF(exitItem->x(), exitItem->y())).magnitude();
	BlackHoleTimer *timer = new BlackHoleTimer(ball, speed, (int)(magnitude * 2.5 - speed * 35 + 500));

	connect(timer, SIGNAL(eject(Ball *, double)), this, SLOT(eject(Ball *, double)));
	connect(timer, SIGNAL(halfway()), this, SLOT(halfway()));

	playSound("blackhole");
	return false;
}

void BlackHole::eject(Ball *ball, double speed)
{
	ball->setResizedPos(exitItem->x(), exitItem->y());

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

void BlackHole::load(KConfigGroup *cfgGroup)
{
	QPoint exit = cfgGroup->readEntry("exit", exit);
	exitItem->setPos(exit.x(), exit.y());
	exitDeg = cfgGroup->readEntry("exitDeg", exitDeg);
	m_minSpeed = cfgGroup->readEntry("minspeed", m_minSpeed);
	m_maxSpeed = cfgGroup->readEntry("maxspeed", m_maxSpeed);
	exitItem->updateArrowAngle();
	exitItem->updateArrowLength();

	baseX = x();
	baseY = y();
	baseExitX = exit.x();
	baseExitY = exit.y();

	finishMe();
}

void BlackHole::finishMe(double width)
{
	if(width==0) //default value
		width=baseExitLineWidth;

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

void BlackHole::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("exit", QPointF(exitItem->x(), exitItem->y()));
	cfgGroup->writeEntry("exitDeg", exitDeg);
	cfgGroup->writeEntry("minspeed", m_minSpeed);
	cfgGroup->writeEntry("maxspeed", m_maxSpeed);
}

HoleResult BlackHole::result(QPointF p, double s, bool * /*wasCenter*/)
{
	const double longestRadius = width() > height()? width() : height();
	if (s > longestRadius / 5.0)
		return Result_Miss;

	QGraphicsRectItem i(QRectF(p, QSize(1, 1)), 0, 0);
	i.setVisible(true);

	// is center of ball in cup?
	if (i.collidesWithItem(this))
	{
		return Result_Holed;
	}
	else
		return Result_Miss;
}

/////////////////////////

BlackHoleExit::BlackHoleExit(BlackHole *blackHole, QGraphicsItem * parent, QGraphicsScene *scene)
: AntialisedLine(parent, scene)
{
	setData(0, Rtti_NoCollision);
	this->blackHole = blackHole;
	arrow = new Arrow(this, scene);
	setZValue(blackHole->zValue());
	arrow->setZValue(zValue() - .00001);
	updateArrowLength();
	arrow->setVisible(false);
	baseArrowPenThickness = 1;
}

void BlackHoleExit::aboutToDie()
{
	arrow->aboutToDie();
	delete arrow;
}

void BlackHoleExit::moveBy(double dx, double dy)
{
	QGraphicsLineItem::moveBy(dx, dy);
	blackHole->updateInfo();
}

void BlackHoleExit::setPen(QPen p)
{
	QGraphicsLineItem::setPen(p);
	arrow->setPen(QPen(p.color(), baseArrowPenThickness));
}

void BlackHoleExit::updateArrowAngle()
{
	// arrows work in a different angle system
	arrow->setAngle(-deg2rad(blackHole->curExitDeg()));
	arrow->updateSelf();
}

void BlackHoleExit::updateArrowLength(double resizeFactor)
{
	arrow->setLength(resizeFactor * (10.0 + 5.0 * (double)(blackHole->minSpeed() + blackHole->maxSpeed()) / 2.0));
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

AntialisedLine::AntialisedLine(QGraphicsItem *parent, QGraphicsScene *scene)
	: QGraphicsLineItem(parent, scene)
{ 
	;
}

void AntialisedLine::paint(QPainter *p, const QStyleOptionGraphicsItem *style, QWidget *widget)
{
	p->setRenderHint(QPainter::Antialiasing, true);
	QGraphicsLineItem::paint(p, style, widget);
}

/////////////////////////

WallPoint::WallPoint(bool start, Wall *wall, QGraphicsItem * parent, QGraphicsScene *scene)
: QGraphicsEllipseItem(parent, scene)
{
	setData(0, Rtti_WallPoint);
	this->wall = wall;
	this->start = start;
	alwaysShow = false;
	editing = false;
	visible = false;
	lastId = INT_MAX - 10;
	dontmove = false;

	setPos(0, 0);
	QPointF p;
	if (start)
		p = wall->startPointF();
	else
		p = wall->endPointF();
	setPos(p.x(), p.y());
	setPen(QPen(Qt::NoPen));
}

void WallPoint::clean()
{
	double oldWidth = width();
	setSize(7, 7);

	QGraphicsItem *onPoint = 0;
	QList<QGraphicsItem *> l = collidingItems();
	for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
		if ((*it)->data(0) == data(0))
			onPoint = (*it);

	if (onPoint)
		setPos(onPoint->x(), onPoint->y());

	setSize(oldWidth, oldWidth);
}

void WallPoint::moveBy(double dx, double dy)
{
	QGraphicsEllipseItem::moveBy(dx, dy);
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
		wall->setLine(x(), y(), wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
	}
	else
	{
		wall->setLine(wall->startPointF().x() + wall->x(), wall->startPointF().y() + wall->y(), x(), y());
	}
	wall->setPos(0, 0);
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
		QList<QGraphicsItem *> l = collidingItems();
		for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
			if ((*it)->data(0) == data(0))
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
	QList<QGraphicsItem *> l = collidingItems();
	for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
	{
		if ((*it)->data(0) == data(0))
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

Wall::Wall( QGraphicsItem *parent, QGraphicsScene *scene, bool antialiasing)
: QGraphicsLineItem(parent, scene)
{
	basePenWidth = 3;
	this->antialiasing = antialiasing;
	setData(0, Rtti_Wall);
	editing = false;
	lastId = INT_MAX - 10;

	dampening = 1.2;

	startItem = 0;
	endItem = 0;

	moveBy(0, 0);
	setZValue(50);

	startItem = new WallPoint(true, this, parent, scene);
	endItem = new WallPoint(false, this, parent, scene);
	startItem->setVisible(true);
	endItem->setVisible(true);
	setPen(QPen(Qt::darkRed, basePenWidth));

	setLine(-15, 10, 15, -5);

	moveBy(0, 0);

	editModeChanged(false);
}

void Wall::paint(QPainter *p, const QStyleOptionGraphicsItem *style, QWidget *widget)
{
	if(antialiasing)
		p->setRenderHint(QPainter::Antialiasing, true);
	QGraphicsLineItem::paint(p, style, widget);
}

void Wall::resize(double resizeFactor)
{
	QGraphicsLineItem::setLine(baseX1*resizeFactor, baseY1*resizeFactor, baseX2*resizeFactor, baseY2*resizeFactor);
	startItem->dontMove();
	endItem->dontMove();
	startItem->setPos(startPointF().x() + x(), startPointF().y() + y());
	endItem->setPos(endPointF().x() + x(), endPointF().y() + y());

	QPen newPen = pen();
	newPen.setWidthF(basePenWidth*resizeFactor);
	setPen(newPen);
}

void Wall::setLine(const QLineF & line)
{
	setLine(line.x1(), line.y1(), line.x2(), line.y2());
}

void Wall::setLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
	baseX1 = x1;
	baseY1 = y1;
	baseX2 = x2;
	baseY2 = y2;
	QGraphicsLineItem::setLine(x1, y1, x2, y2);
}

void Wall::selectedItem(QGraphicsItem *item)
{
	if (item->data(0) == Rtti_WallPoint)
	{
		WallPoint *wallPoint = dynamic_cast<WallPoint *>(item);
		if (wallPoint) {
			setLine(startPointF().x(), startPointF().y(), wallPoint->x() - x(), wallPoint->y() - y());
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
	QGraphicsLineItem::setVisible(yes);

	startItem->setVisible(yes);
	endItem->setVisible(yes);
	startItem->updateVisible();
	endItem->updateVisible();
}

void Wall::setZValue(double newz)
{
	QGraphicsLineItem::setZValue(newz);
	if (startItem)
		startItem->setZValue(newz + .002);
	if (endItem)
		endItem->setZValue(newz + .001);
}

void Wall::setPen(QPen p)
{
	QGraphicsLineItem::setPen(p);

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

QList<QGraphicsItem *> Wall::moveableItems() const
{
	QList<QGraphicsItem *> ret;
	ret.append(startItem);
	ret.append(endItem);
	return ret;
}

void Wall::moveBy(double dx, double dy)
{
	setPos(x() + dx, y() + dy);
	return;
}

void Wall::setPos(double x, double y)
{
	QGraphicsLineItem::setPos(x, y);

	if (!startItem || !endItem)
		return;

	startItem->dontMove();
	endItem->dontMove();
	startItem->setPos(startPointF().x() + x, startPointF().y() + y);
	endItem->setPos(endPointF().x() + x, endPointF().y() + y);
}

void Wall::setVelocity(double vx, double vy)
{
	CanvasItem::setVelocity(vx, vy);
}

void Wall::editModeChanged(bool changed)
{
	// make big for debugging?
	const bool debugPoints = false;

	editing = changed;

	startItem->setZValue(zValue() + .002);
	endItem->setZValue(zValue() + .001);
	startItem->editModeChanged(editing);
	endItem->editModeChanged(editing);

	double neww = 0;
	if (changed || debugPoints)
		neww = 10;
	else
		neww = pen().width();

	startItem->setRect(-0.5*neww, -0.5*neww, neww, neww);
	endItem->setRect(-0.5*neww, -0.5*neww, neww, neww);

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

void Wall::load(KConfigGroup *cfgGroup)
{
	QPoint start(startPoint());
	start = cfgGroup->readEntry("startPoint", start);
	QPoint end(endPoint());
	end = cfgGroup->readEntry("endPoint", end);

	setLine(start.x(), start.y(), end.x(), end.y());

	moveBy(0, 0);
	startItem->setPos(start.x(), start.y());
	endItem->setPos(end.x(), end.y());
}

void Wall::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("startPoint", QPoint((int)startItem->x(), (int)startItem->y()));
	cfgGroup->writeEntry("endPoint", QPoint((int)endItem->x(), (int)endItem->y()));
}

void Wall::doAdvance()
{
	moveBy(getXVelocity(), getYVelocity());
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

StrokeCircle::StrokeCircle(QGraphicsItem *parent, QGraphicsScene *scene)
: QGraphicsItem(parent, scene)
{
	dvalue = 0;
	dmax = 360;
	iwidth = 100;
	iheight = 100;
	ithickness = 8;
	setZValue(10000);
}

void StrokeCircle::setValue(double v)
{
	dvalue = v;
	if (dvalue > dmax)
		dvalue = dmax;
}

double StrokeCircle::value()
{
	return dvalue;
}

bool StrokeCircle::collidesWithItem(const QGraphicsItem*) const { return false; }

QRectF StrokeCircle::boundingRect() const { return QRectF(x(), y(), iwidth, iheight); }

void StrokeCircle::setMaxValue(double m)
{
	dmax = m;
	if (dvalue > dmax)
		dvalue = dmax;
}
void StrokeCircle::setSize(int w, int h)
{
	if (w > 0)
		iwidth = w;
	if (h > 0)
		iheight = h;
}
void StrokeCircle::setThickness(int t)
{
	if (t > 0)
		ithickness = t;
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

KolfGame::KolfGame(ObjectList *obj, PlayerList *players, QString filename, QWidget *parent)
: QGraphicsView(parent)
{
	// for mouse control
	setMouseTracking(true);
	viewport()->setMouseTracking(true);
	setFrameShape(NoFrame);

	regAdv = false;
	curHole = 0; // will get ++'d
	cfg = 0;
	cfgGroup = 0;
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
	soundDir = KStandardDirs::locate("appdata", "sounds/");
	dontAddStroke = false;
	addingNewHole = false;
	scoreboardHoles = 0;
	infoShown = false;
	m_useMouse = true;
	m_useAdvancedPutting = false;
	m_useAdvancedPutting = true;
	m_sound = true;
	m_ignoreEvents = false;
	highestHole = 0;
	recalcHighestHole = false;
	
	renderer = new KolfSvgRenderer( KStandardDirs::locate("appdata", "pics/default_theme.svgz") );

#ifdef SOUND
	m_player = new Phonon::AudioPlayer( Phonon::GameCategory, this );
#endif

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
	grass = QColor("#35760D");

	margin = 10;

	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(width, height);
	QSizePolicy sizePolicy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setSizePolicy(sizePolicy);

	setContentsMargins(margin, margin, margin, margin);

	course = new QGraphicsScene(this);
	course->setBackgroundBrush(Qt::white);
	course->setSceneRect(0, 0, width, height);

	QPixmap pic;
	pic = renderer->renderSvg("grass", width, height, 0);
	course->setBackgroundBrush(QBrush(pic));

	setScene(course);
	adjustSize();

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		course->addItem((*it).ball());

	// highlighter shows current item when editing
	highlighter = new QGraphicsRectItem(0, course);
	highlighter->setPen(QPen(Qt::yellow, 1));
	highlighter->setBrush(QBrush(Qt::NoBrush));
	highlighter->setVisible(false);
	highlighter->setZValue(10000);

	QFont font = kapp->font();
	font.setPixelSize(12);

	// create the advanced putting indicator
	strokeCircle = new StrokeCircle(0, course);
	strokeCircle->setPos(width - 90, height - 90);
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
			cfgGroup = new KConfigGroup(cfg->group(QString("%1-hole@-50,-50|0").arg(scoreboardHoles + 1)));
			emit newHole(cfgGroup->readEntry("par", 3));
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
	delete cfg;
#ifdef SOUND
	delete m_player;
#endif
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
	Wall *wall = new Wall(0, course);
	wall->setLine(start.x(), start.y(), end.x(), end.y());
	wall->setVisible(true);
	wall->setGame(this);
	wall->setZValue(998.7);
	borderWalls.append(wall);
}

void KolfGame::updateHighlighter()
{
	if (!selectedItem)
		return;
	QRectF rect = selectedItem->boundingRect();
	highlighter->setPos(0, 0);
	highlighter->setRect(rect.x() + selectedItem->x() + 1, rect.y() + selectedItem->y() + 1, rect.width(), rect.height());
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

		QList<QGraphicsItem *> list = course->items(e->pos());
		if(list.count() > 0)
			if (list.first() == highlighter)
				list.pop_front();

		moving = false;
		highlighter->setVisible(false);
		selectedItem = 0;
		movingItem = 0;
		movingCanvasItem = 0;

		if (list.count() < 1)
		{
			emit newSelectedItem(&holeInfo);
			return;
		}
		// only items we keep track of
		if ((!(items.count(list.first()) || list.first() == whiteBall || extraMoveable.count(list.first()))))
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
					movingCanvasItem = dynamic_cast<CanvasItem *>(movingItem);
					moving = true;

					if (citem->cornerResize())
						setCursor(KCursor::sizeFDiagCursor());
					else
						setCursor(KCursor::sizeAllCursor());

					emit newSelectedItem(citem);
					highlighter->setVisible(true);
					QRectF rect = selectedItem->boundingRect();
					highlighter->setPos(0, 0);
					highlighter->setRect(rect.x() + selectedItem->x() + 1, rect.y() + selectedItem->y() + 1, rect.width(), rect.height());
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
	return p;// - QPoint(margin, margin);
}

// the following four functions are needed to handle both
// border presses and regular in-course presses

void KolfGame::mouseReleaseEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonRelease, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMouseReleaseEvent(&fixedEvent);
}

void KolfGame::mousePressEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonPress, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMousePressEvent(&fixedEvent);
}

void KolfGame::mouseDoubleClickEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseButtonDblClick, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
	handleMouseDoubleClickEvent(&fixedEvent);
}

void KolfGame::mouseMoveEvent(QMouseEvent * e)
{
	QMouseEvent fixedEvent (QEvent::MouseMove, viewportToViewport(e->pos()), e->button(), e->buttons(), e->modifiers());
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

		QList<QGraphicsItem *> list = course->items(e->pos());
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
	movingCanvasItem->moveBy(-(double)moveX, -(double)moveY);
	QRectF brect = movingItem->boundingRect();
	emit newStatusText(QString("%1x%2").arg(brect.x()).arg(brect.y()));
	storedMousePos = mouse;
}

void KolfGame::updateMouse()
{
	// don't move putter if in advanced putting sequence
	if (!m_useMouse || ((stroking || putting) && m_useAdvancedPutting))
		return;

	const QPointF cursor = viewportToViewport(mapFromGlobal(QCursor::pos()));
	const QPointF ball((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
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

	if (m_showInfo)
	{
		QList<QGraphicsItem *>::const_iterator item;
		for (item = items.constBegin(); item != items.constEnd(); ++item)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
			if (citem)
				citem->showInfo();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->showInfo();
	}
	else
	{
		QList<QGraphicsItem *>::const_iterator item;
		for (item = items.constBegin(); item != items.constEnd(); ++item)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
			if (citem)
				citem->hideInfo();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->hideInfo();
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
		if (editing && !moving && selectedItem)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(selectedItem);
			if (!citem)
				return;
			citem = citem->itemToDelete();
			if (!citem)
				return;
			QGraphicsItem *item = dynamic_cast<QGraphicsItem *>(citem);
			if (citem && citem->deleteable())
			{
				lastDelId = citem->curId();

				highlighter->setVisible(false);
				items.removeAll(item);
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

	int newSize = qMin(newW, newH);
	double resizeFactor = (double)newSize/400.0;
	QGraphicsView::resize(newSize, newSize); //make sure new size is square

	if(newSize!=400) //not default size, so resizing is needed
		resizeAllItems(resizeFactor);
}

void KolfGame::resizeAllItems(double resizeFactor, bool resizeBorderWalls)
{
	//resizeFactor is the number to multiply default sizes and positions by to get their resized value (i.e. if it is 1 then use default size, if it is >1 then everything needs to be bigger, and if it is <1 then everything needs to be smaller)
	course->setSceneRect(0, 0, 400*resizeFactor, 400*resizeFactor);

	QPixmap pic = renderer->renderSvg("grass", (int)(width*resizeFactor), (int)(height*resizeFactor), 0);
	course->setBackgroundBrush(QBrush(pic));

	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
		if (citem) 
			citem->resize(resizeFactor);
	}

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		(*it).ball()->resize(resizeFactor);
	
	putter->setPos((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	putter->resize(resizeFactor);

	QList<Wall *>::const_iterator wall;
	if(resizeBorderWalls) {
		for (wall = borderWalls.constBegin(); wall != borderWalls.constEnd(); ++wall)
			(*wall)->resize(resizeFactor);
	}
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

		if (!course->sceneRect().contains(QPointF((*it).ball()->x(), ((*it).ball()->y()))))
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

		(*curPlayer).ball()->setZValue((*curPlayer).ball()->zValue() + .1 - (.1)/(curScore));

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
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it) {
			(*it).ball()->doAdvance();
		}

		if (fastAdvancedExist)
		{
			QList<CanvasItem *>::const_iterator citem;
			for (citem = fastAdvancers.constBegin(); citem != fastAdvancers.constEnd(); ++citem)
				(*citem)->doAdvance();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->fastAdvanceDone();

		if (fastAdvancedExist)
		{
			QList<CanvasItem *>::const_iterator citem;
			for (citem = fastAdvancers.constBegin(); citem != fastAdvancers.constEnd(); ++citem)
				(*citem)->fastAdvanceDone();
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

	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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
	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
		if (citem)
		{
			stateDB.setName(makeStateGroup(citem->curId(), citem->name()));
			citem->loadState(&stateDB);
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
					QList<QGraphicsItem *> list = ball->collidingItems();
					bool keepMoving = false;
					while (!list.isEmpty())
					{
						QGraphicsItem *item = list.first();
						if (item->data(0) == Rtti_DontPlaceOn)
							keepMoving = true;

						list.pop_front();
					}
					if (!keepMoving)
						break;

					const float movePixel = 3.0;
					x -= cos(v.direction()) * movePixel;
					y += sin(v.direction()) * movePixel;

					ball->setResizedPos(x, y);
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
			ball->collisionDetect(oldx, oldy);
		}
	}

	// emit again
	emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));

	if(ball->curState() == Rolling) {
		inPlay = true; 
		return;
	}

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
			ball->setPos(width / 2, height / 2);
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

	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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

	(*curPlayer).ball()->collisionDetect((*curPlayer).ball()->x(), (*curPlayer).ball()->y());

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
		whiteBall->setPos(width/2, height/2);
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
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it) {
			(*it).ball()->setPlaceOnGround(false);
			while( (*it).numHoles() < (unsigned)curHole)
				(*it).addHole();
		}

		// here we have to make sure the scoreboard shows
		// all of the holes up until now;

		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			cfgGroup = new KConfigGroup(cfg->group(QString("%1-hole@-50,-50|0").arg(scoreboardHoles + 1)));
			emit newHole(cfgGroup->readEntry("par", 3));
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

	if(size().width()!=400 || size().height()!=400) { //not default size, so resizing needed
		int newSize = qMin(size().width(), size().height());
		//resize needs to be called for newSize+1 first because otherwise it doesn't seem to get called (not sure why) 
		QGraphicsView::resize(newSize+1, newSize+1);
		QGraphicsView::resize(newSize, newSize);
	}

	unPause();
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

void KolfGame::openFile()
{
	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
		if (citem)
		{
			// sometimes info is still showing
			citem->hideInfo();
			citem->aboutToDie();
		}
	}

	while (!items.isEmpty())
		delete items.takeFirst();

	extraMoveable.clear();
	fastAdvancers.clear();
	selectedItem = 0;

	// will tell basic course info
	// we do this here for the hell of it.
	// there is no fake id, by the way,
	// because it's old and when i added ids i forgot to change it.
	cfgGroup = new KConfigGroup(cfg->group(QString("0-course@-50,-50")));
	holeInfo.setAuthor(cfgGroup->readEntry("author", holeInfo.author()));
	holeInfo.setName(cfgGroup->readEntry("Name", holeInfo.name()));
	holeInfo.setUntranslatedName(cfgGroup->readEntryUntranslated("Name", holeInfo.untranslatedName()));
	emit titleChanged(holeInfo.name());

	cfgGroup = new KConfigGroup(KSharedConfig::openConfig(filename), QString("%1-hole@-50,-50|0").arg(curHole));
	curPar = cfgGroup->readEntry("par", 3);
	holeInfo.setPar(curPar);
	holeInfo.borderWallsChanged(cfgGroup->readEntry("borderWalls", holeInfo.borderWalls()));
	holeInfo.setMaxStrokes(cfgGroup->readEntry("maxstrokes", 10));
	bool hasFinalLoad = cfgGroup->readEntry("hasFinalLoad", true);

	QStringList missingPlugins;
	QStringList groups = cfg->groupList();

	int numItems = 0;
	int _highestHole = 0;

	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		// [<holeNum>-<name>@<x>,<y>|<id>]
		cfgGroup = new KConfigGroup(cfg->group(*it));

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
				(*it).ball()->setPos(x, y);
			whiteBall->setPos(x, y);
			continue;
		}

		const int id = (*it).right(len - (pipeIndex + 1)).toInt();

		bool loaded = false;

		QList<Object *>::const_iterator curObj;
		for (curObj = obj->constBegin(); curObj != obj->constEnd(); ++curObj)
		{
			if (name != (*curObj)->_name())
				continue;

			QGraphicsItem *newItem; 
			newItem = (*curObj)->newObject(0, course);

			items.append(newItem);
			CanvasItem *sceneItem = dynamic_cast<CanvasItem *>(newItem);

			if (!sceneItem)
				continue;

			sceneItem->setId(id);
			sceneItem->setGame(this);
			sceneItem->editModeChanged(editing);
			sceneItem->setName((*curObj)->_name());
			addItemsToMoveableList(sceneItem->moveableItems());
			if (sceneItem->fastAdvance())
				addItemToFastAdvancersList(sceneItem);

			newItem->setPos(x, y); 

			sceneItem->firstMove(x, y);
			newItem->setVisible(true);

			// make things actually show
			if (!hasFinalLoad)
			{
				cfgGroup = new KConfigGroup(cfg->group(makeGroup(id, curHole, sceneItem->name(), x, y)));
				sceneItem->load(cfgGroup);
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
	//QGraphicsItem *qsceneItem = 0;
	QList<QGraphicsItem *>::const_iterator qsceneItem;
	QList<CanvasItem *> todo;
	QList<QGraphicsItem *> qtodo;
	if (hasFinalLoad)
	{
		for (qsceneItem = items.constBegin(); qsceneItem != items.constEnd(); ++qsceneItem)
		{
			CanvasItem *item = dynamic_cast<CanvasItem *>(*qsceneItem);
			if (item)
			{
				if (item->loadLast())
				{
					qtodo.append(*qsceneItem);
					todo.append(item);
				}
				else
				{
					QString group = makeGroup(item->curId(), curHole, item->name(), (int)(*qsceneItem)->x(), (int)(*qsceneItem)->y());
					cfgGroup = new KConfigGroup(cfg->group(group));
					item->load(cfgGroup);
				}
			}
		}

		QList<CanvasItem *>::const_iterator citem;
		qsceneItem = qtodo.constBegin();
		for (citem = todo.constBegin(); citem != todo.constEnd(); ++citem)
		{
			cfgGroup = new KConfigGroup(cfg->group(makeGroup((*citem)->curId(), curHole, (*citem)->name(), (int)(*qsceneItem)->x(), (int)(*qsceneItem)->y())));
			(*citem)->load(cfgGroup);

			qsceneItem++;
		}
	}

	for (qsceneItem = items.constBegin(); qsceneItem != items.constEnd(); ++qsceneItem)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*qsceneItem);
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

void KolfGame::addItemsToMoveableList(QList<QGraphicsItem *> list)
{
	QList<QGraphicsItem *>::const_iterator item;
	for (item = list.constBegin(); item != list.constEnd(); ++item)
		extraMoveable.append(*item);
}

void KolfGame::addItemToFastAdvancersList(CanvasItem *item)
{
	fastAdvancers.append(item);
	fastAdvancedExist = fastAdvancers.count() > 0;
}

void KolfGame::addNewObject(Object *newObj)
{
	QGraphicsItem *newItem;
	newItem = newObj->newObject(0, course);

	items.append(newItem);
	if(!newItem->isVisible())
		newItem->setVisible(true);

	CanvasItem *sceneItem = dynamic_cast<CanvasItem *>(newItem);
	if (!sceneItem)
		return;

	// we need to find a number that isn't taken
	int i = lastDelId > 0? lastDelId : items.count() - 30;
	if (i <= 0)
		i = 0;

	for (;; ++i)
	{
		bool found = false;
		QList<QGraphicsItem *>::const_iterator item;
		for (item = items.constBegin(); item != items.constEnd(); ++item)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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

	if (m_showInfo)
		sceneItem->showInfo();
	else
		sceneItem->hideInfo();

	sceneItem->editModeChanged(editing);

	sceneItem->setName(newObj->_name());
	addItemsToMoveableList(sceneItem->moveableItems());

	if (sceneItem->fastAdvance())
		addItemToFastAdvancersList(sceneItem);

	newItem->setPos(width/2 - 18, height / 2 - 18);
	sceneItem->firstMove(width/2 - 18, height/2 - 18); //do I need this?
	sceneItem->moveBy(0, 0);
	sceneItem->setSize(newItem->boundingRect().width(), newItem->boundingRect().height());

	if (selectedItem)
		sceneItem->selectedItem(selectedItem);

	setModified(true);
}

bool KolfGame::askSave(bool noMoreChances)
{
	if (!modified)
		// not cancel, don't save
		return false;

	int result = KMessageBox::warningYesNoCancel(this, i18n("There are unsaved changes to current hole. Save them?"), i18n("Unsaved Changes"), KStandardGuiItem::save(), noMoreChances? KStandardGuiItem::discard() : KGuiItem(i18n("Save &Later")), noMoreChances? "DiscardAsk" : "SaveAsk");
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
	QList<Object *>::const_iterator curObj;
	for (curObj = obj->constBegin(); curObj != obj->constEnd(); ++curObj)
		if ((*curObj)->addOnNewHole())
			addNewObject(*curObj);

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
	QList<QGraphicsItem *>::const_iterator qsceneItem;
	for (qsceneItem = items.constBegin(); qsceneItem != items.constEnd(); ++qsceneItem)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*qsceneItem);
		if (citem)
			citem->aboutToDie();
	}

	while (!items.isEmpty())
		delete items.takeFirst();

	emit newSelectedItem(&holeInfo);

	// add default objects
	QList<Object *>::const_iterator curObj;
	for (curObj = obj->constBegin(); curObj != obj->constEnd(); ++curObj)
		if ((*curObj)->addOnNewHole())
			addNewObject(*curObj);

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
		QString newfilename = KFileDialog::getSaveFileName(KUrl("kfiledialog:///kourses"), 
				"application/x-kourse", this, i18n("Pick Kolf Course to Save To"));
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

	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
		if (citem)
		{
			citem->clean();

			cfgGroup = new KConfigGroup(cfg->group(makeGroup(citem->curId(), curHole, citem->name(), (int)(*item)->x(), (int)(*item)->y())));
			citem->save(cfgGroup);
		}
	}

	// save where ball starts (whiteBall tells all)
	cfgGroup = new KConfigGroup(cfg->group(QString("%1-ball@%2,%3").arg(curHole).arg((int)whiteBall->x()).arg((int)whiteBall->y())));
	cfgGroup->writeEntry("dummykey", true);

	cfgGroup = new KConfigGroup(cfg->group(QString("0-course@-50,-50")));
	cfgGroup->writeEntry("author", holeInfo.author());
	cfgGroup->writeEntry("Name", holeInfo.untranslatedName());

	// save hole info
	cfgGroup = new KConfigGroup(cfg->group(QString("%1-hole@-50,-50|0").arg(curHole)));
	cfgGroup->writeEntry("par", holeInfo.par());
	cfgGroup->writeEntry("maxstrokes", holeInfo.maxStrokes());
	cfgGroup->writeEntry("borderWalls", holeInfo.borderWalls());
	cfgGroup->writeEntry("hasFinalLoad", hasFinalLoad);

	cfg->sync();

	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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
	QList<QGraphicsItem *>::const_iterator item;
	for (item = items.constBegin(); item != items.constEnd(); ++item)
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*item);
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

void KolfGame::playSound(const QString& file, float vol)
{
	if (m_sound)
	{
		QString resFile = soundDir + file + QString::fromLatin1(".wav");

		// not needed when all of the files are in the distribution
		//if (!QFile::exists(resFile))
		//return;
		if (vol > 1)
			vol = 1;
#ifdef SOUND
		m_player->play(KUrl::fromPath(resFile));
#endif
	}
}

void HoleInfo::borderWallsChanged(bool yes)
{
	m_borderWalls = yes;
	game->setBorderWalls(yes);
}

void KolfGame::print(KPrinter &pr) //note: this is currently broken, see comment below
{
	kDebug(12007) << "Printing Currently broken" << endl;
	QPainter p(&pr);

	// translate to center
	p.translate(pr.width() / 2 - course->sceneRect().width() / 2, pr.height() / 2 - course->sceneRect().height() / 2);

	QPixmap pix(width, height);
	QPainter pixp(&pix);
	//course->drawArea(course->sceneRect(), &pixp); //not sure how to fix this line to work with QGV, so just commenting for now. This will break printing
	p.drawPixmap(0, 0, pix);

	p.setPen(QPen(Qt::black, 2));
	p.drawRect(course->sceneRect());

	p.resetMatrix();

	if (pr.option("kde-kolf-title") == "true")
	{
		QString text = i18n("%1 - Hole %2; by %3", holeInfo.name(), curHole, holeInfo.author());
		QFont font(kapp->font());
		font.setPointSize(18);
		QRect rect = QFontMetrics(font).boundingRect(text);
		p.setFont(font);

		p.drawText(QPointF(pr.width() / 2 - rect.width() / 2, pr.height() / 2 - course->sceneRect().height() / 2 -20 - rect.height()), text);
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
	QList<Wall *>::const_iterator wall;
	for (wall = borderWalls.constBegin(); wall != borderWalls.constEnd(); ++wall)
		(*wall)->setVisible(showing);
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
	KConfigGroup configGroup (config.group(QString("0-course@-50,-50")));
	info.author = configGroup.readEntry("author", info.author);
	info.name = configGroup.readEntry("Name", configGroup.readEntry("name", info.name));
	info.untranslatedName = configGroup.readEntryUntranslated("Name", configGroup.readEntryUntranslated("name", info.name));

	unsigned int hole = 1;
	unsigned int par= 0;
	while (1)
	{
		QString group = QString("%1-hole@-50,-50|0").arg(hole);
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
	KConfigGroup *configGroup  = new KConfigGroup(config->group(QString("0 Saved Game")));
	int numPlayers = configGroup->readEntry("Players", 0);
	if (numPlayers <= 0)
		return;

	for (int i = 1; i <= numPlayers; ++i)
	{
		// this is same as in kolf.cpp, but we use saved game values
		configGroup = new KConfigGroup(config->group(QString::number(i)));
		players.append(Player());
		players.last().ball()->setColor(configGroup->readEntry("Color", "#ffffff"));
		players.last().setName(configGroup->readEntry("Name"));
		players.last().setId(i);

		QStringList scores(configGroup->readEntry("Scores",QStringList()));
		QList<int> intscores;
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

	KConfigGroup *configGroup  = new KConfigGroup(config->group(QString("0 Saved Game")));
	configGroup->writeEntry("Players", players->count());
	configGroup->writeEntry("Course", filename);
	configGroup->writeEntry("Current Hole", curHole);

	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		configGroup  = new KConfigGroup(config->group(QString::number((*it).id())));
		configGroup->writeEntry("Name", (*it).name());
		configGroup->writeEntry("Color", (*it).ball()->color().name());

		QStringList scores;
		QList<int> intscores = (*it).scores();
		for (QList<int>::Iterator it = intscores.begin(); it != intscores.end(); ++it)
			scores.append(QString::number(*it));

		configGroup->writeEntry("Scores", scores);
	}
}

CourseInfo::CourseInfo()
	: name(i18n("Course Name")), author(i18n("Course Author")), holes(0), par(0)
{
}

#include "game.moc"
