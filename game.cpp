#include <arts/kmedia2.h>
#include <arts/kplayobject.h>
#include <arts/kplayobjectfactory.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kimageeffect.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kpixmapeffect.h>
#include <kprinter.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include <qbitmap.h>
#include <qbrush.h>
#include <qcanvas.h>
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
#include <qpaintdevicemetrics.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qpoint.h>
#include <qpointarray.h>
#include <qrect.h>
#include <qsimplerichtext.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qvaluelist.h>
#include <qwhatsthis.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "kcomboboxdialog.h"
#include "kvolumecontrol.h"
#include "rtti.h"
#include "object.h"
#include "config.h"
#include "canvasitem.h"
#include "ball.h"
#include "statedb.h"
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

RectPoint::RectPoint(QColor color, RectItem *rect, QCanvas *canvas)
	: QCanvasEllipse(canvas)
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
	QCanvasEllipse::moveBy(dx, dy);

	if (dontmove)
	{
		dontmove = false;
		return;
	}

	QCanvasItem *qitem = dynamic_cast<QCanvasItem *>(rect);
	if (!qitem)
		return;

	double nw = m_sizeFactor * fabs(x() - qitem->x());
	double nh = m_sizeFactor * fabs(y() - qitem->y());
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

/////////////////////////

Arrow::Arrow(QCanvas *canvas)
	: QCanvasLine(canvas)
{
	line1 = new QCanvasLine(canvas);
	line2 = new QCanvasLine(canvas);

	m_angle = 0;
	m_length = 20;
	m_reversed = false;

	setPen(black);

	updateSelf();
	setVisible(false);
}

void Arrow::setPen(QPen p)
{
	QCanvasLine::setPen(p);
	line1->setPen(p);
	line2->setPen(p);
}

void Arrow::setZ(double newz)
{
	QCanvasLine::setZ(newz);
	line1->setZ(newz);
	line2->setZ(newz);
}

void Arrow::setVisible(bool yes)
{
	QCanvasLine::setVisible(yes);
	line1->setVisible(yes);
	line2->setVisible(yes);
}

void Arrow::moveBy(double dx, double dy)
{
	QCanvasLine::moveBy(dx, dy);
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
	QPoint end(m_length * cos(m_angle), m_length * sin(m_angle));

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
	end = QPoint(lineLen * cos(angle1), lineLen * sin(angle1));
	line1->setPoints(0, 0, end.x(), end.y());

	const double angle2 = m_angle + M_PI / 2 + 1;
	line2->move(start.x() + x(), start.y() + y());
	end = QPoint(lineLen * cos(angle2), lineLen * sin(angle2));
	line2->setPoints(0, 0, end.x(), end.y());
}

/////////////////////////

Slope::Slope(QRect rect, QCanvas *canvas)
	: QCanvasRectangle(rect, canvas), type(KImageEffect::VerticalGradient), grade(4), reversed(false), color(QColor("#327501"))
{
	stuckOnGround = false;
	showingInfo = false;

	gradientKeys[KImageEffect::VerticalGradient] = "Vertical";
	gradientKeys[KImageEffect::HorizontalGradient] = "Horizontal";
	gradientKeys[KImageEffect::DiagonalGradient] = "Diagonal";
	gradientKeys[KImageEffect::CrossDiagonalGradient] = "Opposite Diagonal";
	gradientKeys[KImageEffect::EllipticGradient] = "Elliptic";

	gradientI18nKeys[KImageEffect::VerticalGradient] = i18n("Vertical");
	gradientI18nKeys[KImageEffect::HorizontalGradient] = i18n("Horizontal");
	gradientI18nKeys[KImageEffect::DiagonalGradient] = i18n("Diagonal");
	gradientI18nKeys[KImageEffect::CrossDiagonalGradient] = i18n("Opposite Diagonal");
	gradientI18nKeys[KImageEffect::EllipticGradient] = i18n("Circular");

	setZ(-50);

	if (!QPixmapCache::find("grass", grass))
	{
		grass.load(locate("appdata", "pics/grass.png"));
		QPixmapCache::insert("grass", grass);
	}

	point = new RectPoint(color.light(), this, canvas);

	QFont font(kapp->font());
	font.setPixelSize(18);
	text = new QCanvasText(canvas);
	text->setZ(99999.99);
	text->setFont(font);
	text->setColor(white);

	editModeChanged(false);
	hideInfo();

	// this does updatePixmap
	setGradient("Vertical");
}

bool Slope::terrainCollisions() const
{
	// having circles be different is evil
	//return type == KImageEffect::EllipticGradient;
	return false;
}

void Slope::showInfo()
{
	showingInfo = true;
	Arrow *arrow = 0;
	for (arrow = arrows.first(); arrow; arrow = arrows.next())
	{
		arrow->setZ(z() + .01);
		arrow->setVisible(true);
	}
	text->setVisible(true);
}

void Slope::hideInfo()
{
	showingInfo = false;
	Arrow *arrow = 0;
	for (arrow = arrows.first(); arrow; arrow = arrows.next())
		arrow->setVisible(false);
	text->setVisible(false);
}

void Slope::aboutToDie()
{
	delete point;
	clearArrows();
	delete text;
}

void Slope::clearArrows()
{
	Arrow *arrow = 0;
	for (arrow = arrows.first(); arrow; arrow = arrows.next())
	{
		arrow->setVisible(false);
		arrow->aboutToDie();
	}
	arrows.setAutoDelete(true);
	arrows.clear();
	arrows.setAutoDelete(false);
}

QPtrList<QCanvasItem> Slope::moveableItems() const
{
	QPtrList<QCanvasItem> ret;
	ret.append(point);
	return ret;
}

void Slope::setGrade(double newGrade)
{
	if (newGrade >= 0 && newGrade < 11)
	{
		grade = newGrade;
		updatePixmap();
	}
}

void Slope::setSize(int width, int height)
{
	newSize(width, height);
}

void Slope::newSize(int width, int height)
{
	if (type == KImageEffect::EllipticGradient)
	{
		QCanvasRectangle::setSize(width, width);
		// move point back to good spot
		moveBy(0, 0);

		if (game && game->isEditing())
			game->updateHighlighter();
	}
	else
		QCanvasRectangle::setSize(width, height);

	updatePixmap();
	updateZ();
}

void Slope::moveBy(double dx, double dy)
{
	QCanvasRectangle::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width(), y() + height());

	moveArrow();
	updateZ();
}

void Slope::moveArrow()
{
	int xavg = 0, yavg = 0;
	QPointArray r = areaPoints();
	for (unsigned int i = 0; i < r.size(); ++i)
	{
		xavg += r[i].x();
		yavg += r[i].y();
	}
	xavg /= r.size();
	yavg /= r.size();

	Arrow *arrow = 0;
	for (arrow = arrows.first(); arrow; arrow = arrows.next())
		arrow->move((double)xavg, (double)yavg);
	
	if (showingInfo)
		showInfo();
	else
		hideInfo();

	text->move((double)xavg - text->boundingRect().width() / 2, (double)yavg - text->boundingRect().height() / 2);
}

void Slope::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Slope::updateZ(QCanvasRectangle *vStrut)
{
	const int area = (height() * width());
	const int defaultz = -50;

	double newZ = 0;

	QCanvasRectangle *rect = 0;
	if (!stuckOnGround)
		rect = vStrut? vStrut : onVStrut();

	if (rect)
	{
		if (area > (rect->width() * rect->height()))
			newZ = defaultz;
		else
			newZ = rect->z();
	}
	else
		newZ = defaultz;

	setZ(((double)1 / (area == 0? 1 : area)) + newZ);
}

void Slope::load(KConfig *cfg)
{
	stuckOnGround = cfg->readBoolEntry("stuckOnGround", stuckOnGround);
	grade = cfg->readDoubleNumEntry("grade", grade);
	reversed = cfg->readBoolEntry("reversed", reversed);

	// bypass updatePixmap which newSize normally does
	QCanvasRectangle::setSize(cfg->readNumEntry("width", width()), cfg->readNumEntry("height", height()));
	updateZ();

	QString gradientType = cfg->readEntry("gradient", gradientKeys[type]);
	setGradient(gradientType);
}

void Slope::save(KConfig *cfg)
{
	cfg->writeEntry("reversed", reversed);
	cfg->writeEntry("width", width());
	cfg->writeEntry("height", height());
	cfg->writeEntry("gradient", gradientKeys[type]);
	cfg->writeEntry("grade", grade);
	cfg->writeEntry("stuckOnGround", stuckOnGround);
}

void Slope::draw(QPainter &painter)
{
	painter.drawPixmap(x(), y(), pixmap);
}

QPointArray Slope::areaPoints() const
{
	switch (type)
	{
		case KImageEffect::CrossDiagonalGradient:
		{
			QPointArray ret(3);
			ret[0] = QPoint((int)x(), (int)y());
			ret[1] = QPoint((int)x() + width(), (int)y() + height());
			ret[2] = reversed? QPoint((int)x() + width(), y()) : QPoint((int)x(), (int)y() + height());

			return ret;
		}

		case KImageEffect::DiagonalGradient:
		{
			QPointArray ret(3);
			ret[0] = QPoint((int)x() + width(), (int)y());
			ret[1] = QPoint((int)x(), (int)y() + height());
			ret[2] = !reversed? QPoint((int)x() + width(), y() + height()) : QPoint((int)x(), (int)y());

			return ret;
		}

		case KImageEffect::EllipticGradient:
		{
			QPointArray ret;
			ret.makeEllipse((int)x(), (int)y(), width(), height());
			return ret;
		}

		default:
			return QCanvasRectangle::areaPoints();
	}
}

bool Slope::collision(Ball *ball, long int /*id*/)
{
	if (grade <= 0)
		return true;

	double vx = ball->xVelocity();
	double vy = ball->yVelocity();
	double addto = 0.013 * grade;

	const bool diag = type == KImageEffect::DiagonalGradient || type == KImageEffect::CrossDiagonalGradient;
	const bool circle = type == KImageEffect::EllipticGradient;

	double slopeAngle = 0;

	if (diag)
		slopeAngle = atan((double)width() / (double)height());
	else if (circle)
	{
		const QPoint start(x() + (int)width() / 2.0, y() + (int)height() / 2.0);
		const QPoint end((int)ball->x(), (int)ball->y());

		Vector betweenVector(start, end);
		const double factor = betweenVector.magnitude() / ((double)width() / 2.0);
		slopeAngle = betweenVector.direction();

		// this little bit by Daniel
		addto *= factor * M_PI / 2;
		addto = sin(addto);
	}

	switch (type)
	{
		case KImageEffect::HorizontalGradient:
			reversed? vx += addto : vx -= addto;
		break;

		case KImageEffect::VerticalGradient:
			reversed? vy += addto : vy -= addto;
		break;

		case KImageEffect::DiagonalGradient:
		case KImageEffect::EllipticGradient:
			reversed? vx += cos(slopeAngle) * addto : vx -= cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		case KImageEffect::CrossDiagonalGradient:
			reversed? vx -= cos(slopeAngle) * addto : vx += cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		default:
		break;
	}

	ball->setVelocity(vx, vy);
	ball->setState(Rolling);

	return true;
}

void Slope::setGradient(QString text)
{
	for (QMap<KImageEffect::GradientType, QString>::Iterator it = gradientKeys.begin(); it != gradientKeys.end(); ++it)
	{
		if (it.data() == text)
		{
			setType(it.key());
			return;
		}
	}

	// extra forgiveness ;-) (note it's i18n keys)
	for (QMap<KImageEffect::GradientType, QString>::Iterator it = gradientI18nKeys.begin(); it != gradientI18nKeys.end(); ++it)
	{
		if (it.data() == text)
		{
			setType(it.key());
			return;
		}
	}
}

void Slope::setType(KImageEffect::GradientType type)
{
	this->type = type;

	if (type == KImageEffect::EllipticGradient)
	{
		// calls updatePixmap
		newSize(width(), height());
	}
	else
		updatePixmap();
}

void Slope::updatePixmap()
{
	// make a gradient, make grass that's bright or dim
	// merge into this->pixmap. This is drawn in draw()

	// we update the arrows in this function
	clearArrows();

	const bool diag = type == KImageEffect::DiagonalGradient || type == KImageEffect::CrossDiagonalGradient;
	const bool circle = type == KImageEffect::EllipticGradient;

	const QColor darkColor = color.dark(100 + grade * (circle? 20 : 10));
	const QColor lightColor = diag || circle? color.light(110 + (diag? 5 : .5) * grade) : color;
	// hack only for circles
	const bool _reversed = circle? !reversed : reversed;
	QImage gradientImage = KImageEffect::gradient(QSize(width(), height()), _reversed? darkColor : lightColor, _reversed? lightColor : darkColor, type);

	QPixmap qpixmap(width(), height());
	QPainter p(&qpixmap);
	p.drawTiledPixmap(QRect(0, 0, width(), height()), grass);
	p.end();

	const double length = sqrt(width() * width() + height() * height()) / 4;

	if (circle)
	{
		const QColor otherLightColor = color.light(110 + 15 * grade);
		const QColor otherDarkColor = darkColor.dark(110 + 20 * grade);
		QImage otherGradientImage = KImageEffect::gradient(QSize(width(), height()), reversed? otherDarkColor : otherLightColor, reversed? otherLightColor : otherDarkColor, KImageEffect::DiagonalGradient);

		QImage grassImage(qpixmap.convertToImage());

		QImage finalGradientImage = KImageEffect::blend(otherGradientImage, gradientImage, .60);
		pixmap.convertFromImage(KImageEffect::blend(grassImage, finalGradientImage, .40));

		// make arrows
		double angle = 0;
		for (int i = 0; i < 4; ++i)
		{
			angle += M_PI / 2;
			Arrow *arrow = new Arrow(canvas());
			arrow->setLength(length);
			arrow->setAngle(angle);
			arrow->setReversed(reversed);
			arrow->updateSelf();
			arrows.append(arrow);
		}
	}
	else
	{
		Arrow *arrow = new Arrow(canvas());

		float ratio = 0;
		float factor = 1;

		double angle = 0;

		switch (type)
		{
			case KImageEffect::HorizontalGradient:
				angle = 0;
				factor = .32;
				break;

			case KImageEffect::VerticalGradient:
				angle = M_PI / 2;
				factor = .32;
				break;

			case KImageEffect::DiagonalGradient:
				angle = atan((double)width() / (double)height());

				factor = .45;
				break;

			case KImageEffect::CrossDiagonalGradient:
				angle = atan((double)width() / (double)height());
				angle = M_PI - angle;

				factor = .05;
				break;

			default:
				break;
		}

		float factorPart = factor * 2;
		// gradePart is out of 1
		float gradePart = grade / 8.0;

		ratio = factorPart * gradePart;

		// reverse the reversed ones
		if (reversed)
			ratio *= -1;
		else
			angle += M_PI;

		KPixmap kpixmap = qpixmap;
		(void) KPixmapEffect::intensity(kpixmap, ratio);

		QImage grassImage(kpixmap.convertToImage());

		// okay, now we have a grass image that's
		// appropriately lit, and a gradient;
		// lets blend..
		pixmap.convertFromImage(KImageEffect::blend(gradientImage, grassImage, .42));
		arrow->setAngle(angle);
		arrow->setLength(length);
		arrow->updateSelf();

		arrows.append(arrow);
	}

	text->setText(QString::number(grade));

	if (diag || circle)
	{
		// make cleared bitmap
		QBitmap bitmap(pixmap.width(), pixmap.height(), true);
		QPainter bpainter(&bitmap);
		bpainter.setBrush(color1);
		QPointArray r = areaPoints();

		// shift all the points
		for (unsigned int i = 0; i < r.count(); ++i)
		{
			QPoint &p = r[i];
			p.setX(p.x() - x());
			p.setY(p.y() - y());
		}
		bpainter.drawPolygon(r);

		// mask is drawn
		pixmap.setMask(bitmap);
	}

	moveArrow();
	update();
}

/////////////////////////

BridgeConfig::BridgeConfig(Bridge *bridge, QWidget *parent)
	: Config(parent)
{
	this->bridge = bridge;

	m_vlayout = new QVBoxLayout(this, marginHint(), spacingHint());
	QGridLayout *layout = new QGridLayout(m_vlayout, 2, 3, spacingHint());
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
}

/////////////////////////

Bridge::Bridge(QRect rect, QCanvas *canvas)
	: QCanvasRectangle(rect, canvas)
{
	QColor color("#92772D");
	setBrush(QBrush(color));
	setPen(NoPen);
	setZ(998);

	topWall = new Wall(canvas);
	topWall->setAlwaysShow(true);
	botWall = new Wall(canvas);
	botWall->setAlwaysShow(true);
	leftWall = new Wall(canvas);
	leftWall->setAlwaysShow(true);
	rightWall = new Wall(canvas);
	rightWall->setAlwaysShow(true);

	setWallZ(998.1);
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
	QCanvasRectangle::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width(), y() + height());

	topWall->move(x(), y());
	botWall->move(x(), y() - 1);
	leftWall->move(x(), y());
	rightWall->move(x(), y());

	QCanvasItemList list = collisions(true);
	for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
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
	newSize(cfg->readNumEntry("width", width()), cfg->readNumEntry("height", height()));
	setTopWallVisible(cfg->readBoolEntry("topWallVisible", topWallVisible()));
	setBotWallVisible(cfg->readBoolEntry("botWallVisible", botWallVisible()));
	setLeftWallVisible(cfg->readBoolEntry("leftWallVisible", leftWallVisible()));
	setRightWallVisible(cfg->readBoolEntry("rightWallVisible", rightWallVisible()));
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

QPtrList<QCanvasItem> Bridge::moveableItems() const
{
	QPtrList<QCanvasItem> ret;
	ret.append(point);
	return ret;
}

void Bridge::newSize(int width, int height)
{
	setSize(width, height);
}

void Bridge::setSize(int width, int height)
{
	QCanvasRectangle::setSize(width, height);

	topWall->setPoints(0, 0, width, 0);
	botWall->setPoints(0, height, width, height);
	leftWall->setPoints(0, 0, 0, height);
	rightWall->setPoints(width, 0, width, height);

	moveBy(0, 0);
}

/////////////////////////

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

void FloaterGuide::setPoints(int xa, int ya, int xb, int yb)
{
	if (fabs(xa - xb) > 0 && fabs(ya - yb) > 0)
	{
		Wall::setPoints(xa, ya, xb, yb);
		if (floater)
			floater->reset();
	}
}

Config *FloaterGuide::config(QWidget *parent)
{
	return floater->config(parent);
}

/////////////////////////

Floater::Floater(QRect rect, QCanvas *canvas)
	: Bridge(rect, canvas), speedfactor(16)
{
	wall = 0;
	setEnabled(true);
	noUpdateZ = false;
	haventMoved = true;
	wall = new FloaterGuide(this, canvas);
	wall->setPoints(100, 100, 200, 200);
	wall->setPen(QPen(wall->pen().color().light(), wall->pen().width() - 1));
	move(wall->endPoint().x(), wall->endPoint().y());

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(false);
	setRightWallVisible(false);

	newSize(width(), height());
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
	if (!isEnabled())
		return;

	Bridge::advance(phase);

	if (phase == 1 && (xVelocity() || yVelocity()))
	{
		const double wally = wall->y();
		const double wallx = wall->x();

		if (start.y() == end.y())
			// horizontal
		{
			if (xVelocity() > 0)
			{
				if (x() > end.x() + wallx)
					setSpeed(speed);
			}
			else if (x() < start.x() + wallx)
			{
				setSpeed(speed);
				setVelocity(-xVelocity(), -yVelocity());
			}
		}
		else if (x() < start.x() + wallx && (yVelocity() > 0? y() > start.y() + wally : y() < start.y() + wally))
		{
			setSpeed(speed);
			setVelocity(-xVelocity(), -yVelocity());
		}
		else if (x() > end.x() + wallx && (yVelocity() < 0? y() < end.y() + wally : y() > end.y() + wally))
		{
			setSpeed(speed);
		}
	}
}

void Floater::reset()
{
	start = wall->startPoint();
	end = wall->endPoint();

	if (start.y() < end.y())
	{
		int old = start.y();
		start.setY(end.y());
		end.setY(old);
	}

	if (end.x() < start.x())
	{
		int old = start.x();
		start.setX(end.x());
		end.setX(old);
	}

	move(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
	setSpeed(speed);
}

QPtrList<QCanvasItem> Floater::moveableItems() const
{
	QPtrList<QCanvasItem> ret(wall->moveableItems());
	ret.append(wall);
	ret.append(point);
	return ret;
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
		setVelocity(0, 0);
		return;
	}


	const double rise = wall->startPoint().y() - wall->endPoint().y();
	const double run = wall->startPoint().x() - wall->endPoint().x();
	double wallAngle = atan(rise / run);
	const double factor = (double)speed / 3.5;

	setVelocity(-cos(wallAngle) * factor, -sin(wallAngle) * factor);
}

void Floater::aboutToSave()
{
	setVelocity(0, 0);
	noUpdateZ = true;
	move(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
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

	QCanvasItemList l = collisions(false);
	for (QCanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		CanvasItem *item = dynamic_cast<CanvasItem *>(*it);

		if (!noUpdateZ)
			if (item && item->canBeMovedByOthers())
				item->updateZ(this);

		if ((*it)->z() >= z())
		{
			if (item && item->canBeMovedByOthers() && collidesWith(*it))
			{
				if ((*it)->rtti() == Rtti_Ball)
				{
					//((Ball *)(*it))->setState(Rolling);
					(*it)->moveBy(dx, dy);
					if (game && game->hasFocus() && !game->isEditing() && game->curBall() == (Ball *)(*it))
							game->ballMoved();
				}
				else if ((*it)->rtti() != Rtti_Putter)
					(*it)->moveBy(dx, dy);
			}
		}
	}

	point->dontMove();
	point->move(x() + width(), y() + height());

	// this call must come after we have tested for collisions, otherwise we skip them when saving!
	// that's a bad thing
	QCanvasRectangle::moveBy(dx, dy);

	// because we don't do Bridge::moveBy();
	topWall->move(x(), y());
	botWall->move(x(), y() - 1);
	leftWall->move(x(), y());
	rightWall->move(x(), y());

	if (game && game->isEditing())
		game->updateHighlighter();
}

void Floater::saveState(StateDB *db)
{
	db->setPoint(QPoint(x(), y()));
}

void Floater::loadState(StateDB *db)
{
	const QPoint moveTo = db->point();
	move(moveTo.x(), moveTo.y());
}

void Floater::save(KConfig *cfg)
{
	cfg->writeEntry("speed", speed);
	cfg->writeEntry("startPoint", QPoint(wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y()));
	cfg->writeEntry("endPoint", QPoint(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y()));

	doSave(cfg);
}

void Floater::load(KConfig *cfg)
{
	move(firstPoint.x(), firstPoint.y());

	QPoint start(wall->startPoint());
	start = cfg->readPointEntry("startPoint", &start);
	QPoint end(wall->endPoint());
	end = cfg->readPointEntry("endPoint", &end);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->move(0, 0);

	setSpeed(cfg->readNumEntry("speed", -1));

	doLoad(cfg);

	reset();
}

void Floater::firstMove(int x, int y)
{
	firstPoint = QPoint(x, y);
}

/////////////////////////

FloaterConfig::FloaterConfig(Floater *floater, QWidget *parent)
	: BridgeConfig(floater, parent)
{
	this->floater = floater;
	m_vlayout->addStretch();

	m_vlayout->addWidget(new QLabel(i18n("Moving speed"), this));
	QHBoxLayout *hlayout = new QHBoxLayout(m_vlayout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(0, 20, 2, floater->curSpeed(), Qt::Horizontal, this);
	hlayout->addWidget(slider);
	hlayout->addWidget(new QLabel(i18n("Fast"), this));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(speedChanged(int)));
}

void FloaterConfig::speedChanged(int news)
{
	floater->setSpeed(news);
	changed();
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

	QHBoxLayout *hlayout = new QHBoxLayout(m_vlayout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(1, 10, 1, windmill->curSpeed(), Qt::Horizontal, this);
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

Windmill::Windmill(QRect rect, QCanvas *canvas)
	: Bridge(rect, canvas), speedfactor(16), m_bottom(true)
{
	guard = new WindmillGuard(canvas);
	guard->setPen(QPen(black, 5));
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
	setSpeed(cfg->readNumEntry("speed", -1));

	doLoad(cfg);

	left->editModeChanged(false);
	right->editModeChanged(false);
	guard->editModeChanged(false);

	setBottom(cfg->readBoolEntry("bottom", true));
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

Sign::Sign(QCanvas *canvas)
	: Bridge(QRect(0, 0, 110, 40), canvas)
{
	setZ(998.8);
	m_text = i18n("New Text");
	setBrush(QBrush(white));
	setWallColor(black);
	setWallZ(z() + .01);

	setTopWallVisible(true);
	setBotWallVisible(true);
	setLeftWallVisible(true);
	setRightWallVisible(true);
}

void Sign::load(KConfig *cfg)
{
	// Comment so it gets translated
	// text was the old name, we should keep bc
	m_text = cfg->readEntry("Comment", cfg->readEntry("text", m_text));

	doLoad(cfg);
}

void Sign::save(KConfig *cfg)
{
	cfg->writeEntry("Comment", m_text);

	doSave(cfg);
}

void Sign::draw(QPainter &painter)
{
	Bridge::draw(painter);

	painter.setPen(QPen(black, 1));
	QSimpleRichText txt(m_text, kapp->font());
	const int indent = wallPen().width() + 3;
	txt.setWidth(width() - 2*indent);
	QColorGroup colorGroup;
	colorGroup.setColor(QColorGroup::Foreground, black);
	colorGroup.setColor(QColorGroup::Text, black);
	colorGroup.setColor(QColorGroup::Background, black);
	colorGroup.setColor(QColorGroup::Base, black);
	txt.draw(&painter, x() + indent, y(), QRect(x() + indent, y(), width() - indent, height() - indent), colorGroup);
}

/////////////////////////

SignConfig::SignConfig(Sign *sign, QWidget *parent)
	: BridgeConfig(sign, parent)
{
	this->sign = sign;
	m_vlayout->addStretch();
	m_vlayout->addWidget(new QLabel(i18n("Sign HTML"), this));
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

SlopeConfig::SlopeConfig(Slope *slope, QWidget *parent)
	: Config(parent)
{
	this->slope = slope;
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	KComboBox *gradient = new KComboBox(this);
	QStringList items;
	QString curText;
	for (QMap<KImageEffect::GradientType, QString>::Iterator it = slope->gradientI18nKeys.begin(); it != slope->gradientI18nKeys.end(); ++it)
	{
		if (it.key() == slope->curType())
			curText = it.data();
		items.append(it.data());
	}
	gradient->insertStringList(items);
	gradient->setCurrentText(curText);
	layout->addWidget(gradient);
	connect(gradient, SIGNAL(activated(const QString &)), this, SLOT(setGradient(const QString &)));

	layout->addStretch();

	QCheckBox *reversed = new QCheckBox(i18n("Reverse direction"), this);
	reversed->setChecked(slope->isReversed());
	layout->addWidget(reversed);
	connect(reversed, SIGNAL(toggled(bool)), this, SLOT(setReversed(bool)));

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Grade"), this));
	KDoubleNumInput *grade = new KDoubleNumInput(this);
	grade->setRange(0, 8, 1, true);
	grade->setValue(slope->curGrade());
	hlayout->addWidget(grade);
	connect(grade, SIGNAL(valueChanged(double)), this, SLOT(gradeChanged(double)));

	QCheckBox *stuck = new QCheckBox(i18n("Unmoveable"), this);
	QWhatsThis::add(stuck, i18n("Whether or not this slope can be moved by other objects, like floaters."));
	stuck->setChecked(slope->isStuckOnGround());
	layout->addWidget(stuck);
	connect(stuck, SIGNAL(toggled(bool)), this, SLOT(setStuckOnGround(bool)));
}

void SlopeConfig::setGradient(const QString &text)
{
	slope->setGradient(text);
	changed();
}

void SlopeConfig::setReversed(bool yes)
{
	slope->setReversed(yes);
	changed();
}

void SlopeConfig::setStuckOnGround(bool yes)
{
	slope->setStuckOnGround(yes);
	changed();
}

void SlopeConfig::gradeChanged(double newgrade)
{
	slope->setGrade(newgrade);
	changed();
}

/////////////////////////

EllipseConfig::EllipseConfig(Ellipse *ellipse, QWidget *parent)
	: Config(parent), slow1(0), fast1(0), slow2(0), fast2(0), slider1(0), slider2(0)
{
	this->ellipse = ellipse;

	m_vlayout = new QVBoxLayout(this, marginHint(), spacingHint());

	QCheckBox *check = new QCheckBox(i18n("Enable show/hide"), this);
	m_vlayout->addWidget(check);
	connect(check, SIGNAL(toggled(bool)), this, SLOT(check1Changed(bool)));
	check->setChecked(ellipse->changeEnabled());

	QHBoxLayout *hlayout = new QHBoxLayout(m_vlayout, spacingHint());
	slow1 = new QLabel(i18n("Slow"), this);
	hlayout->addWidget(slow1);
	slider1 = new QSlider(1, 100, 5, 100 - ellipse->changeEvery(), Qt::Horizontal, this);
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

void EllipseConfig::value2Changed(int news)
{
	ellipse->setChangeEvery(100 - news);
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

Ellipse::Ellipse(QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	savingDone();
	setChangeEnabled(false);
	setChangeEvery(50);
	count = 0;
	setVisible(true);

	point = new RectPoint(black, this, canvas);
	point->setSizeFactor(2.0);
}

void Ellipse::aboutToDie()
{
	delete point;
}

QPtrList<QCanvasItem> Ellipse::moveableItems() const
{
	QPtrList<QCanvasItem> ret;
	ret.append(point);
	return ret;
}

void Ellipse::newSize(int width, int height)
{
	QCanvasEllipse::setSize(width, height);
}

void Ellipse::moveBy(double dx, double dy)
{
	QCanvasEllipse::moveBy(dx, dy);

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
	QCanvasEllipse::advance(phase);

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
	setChangeEnabled(cfg->readBoolEntry("changeEnabled", changeEnabled()));
	setChangeEvery(cfg->readNumEntry("changeEvery", changeEvery()));
	double newWidth = width(), newHeight = height();
	newWidth = cfg->readNumEntry("width", newWidth);
	newHeight = cfg->readNumEntry("height", newHeight);
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

Puddle::Puddle(QCanvas *canvas)
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

	brush.setPixmap(pic);
	setBrush(brush);

	KPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setPixmap(pointPic);
	point->setBrush(brush);

	setZ(-25);
}

bool Puddle::collision(Ball *ball, long int /*id*/)
{
	if (ball->isVisible())
	{
		QCanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
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
	}

	return false;
}

/////////////////////////

Sand::Sand(QCanvas *canvas)
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

	brush.setPixmap(pic);
	setBrush(brush);

	KPixmap pointPic(pic);
	KPixmapEffect::intensity(pointPic, .45);
	brush.setPixmap(pointPic);
	point->setBrush(brush);

	setZ(-26);
}

bool Sand::collision(Ball *ball, long int /*id*/)
{
	QCanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
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

Putter::Putter(QCanvas *canvas)
	: QCanvasLine(canvas)
{
	m_showGuideLine = true;
	oneDegree = M_PI / 180;
	len = 9;
	angle = 0;

	guideLine = new QCanvasLine(canvas);
	guideLine->setPen(QPen(white, 1, QPen::DotLine));
	guideLine->setZ(998.8);

	setPen(QPen(black, 4));
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
	QCanvasLine::moveBy(dx, dy);
	guideLine->move(x(), y());
}

void Putter::setShowGuideLine(bool yes)
{
	m_showGuideLine = yes;
	setVisible(isVisible());
}

void Putter::setVisible(bool yes)
{
	QCanvasLine::setVisible(yes);
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

Bumper::Bumper(QCanvas *canvas)
	: QCanvasEllipse(20, 20, canvas)
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
	QCanvasEllipse::moveBy(dx, dy);
	//const double insideLen = (double)(width() - inside->width()) / 2.0;
	inside->move(x(), y());
}

void Bumper::editModeChanged(bool changed)
{
	inside->setVisible(!changed);
}

void Bumper::advance(int phase)
{
	QCanvasEllipse::advance(phase);

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
	betweenVector.setDirection(betweenVector.direction() + deg2rad((kapp->random() % 3) - 1));

	ball->setVector(betweenVector);
	// for some reason, x is always switched...
	ball->setXVelocity(-ball->xVelocity());
	ball->setState(Rolling);

	setAnimated(true);

	return true;
}

/////////////////////////

Hole::Hole(QColor color, QCanvas *canvas)
	: QCanvasEllipse(15, 15, canvas)
{
	setZ(998.1);
	setPen(black);
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

	QCanvasRectangle i(QRect(p, QSize(1, 1)), canvas());
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

Cup::Cup(QCanvas *canvas)
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

BlackHole::BlackHole(QCanvas *canvas)
	: Hole(black, canvas), exitDeg(0)
{
	infoLine = 0;
	m_minSpeed = 3.0;
	m_maxSpeed = 5.0;
	runs = 0;

	const QColor myColor((QRgb)(kapp->random() % 0x01000000));

	outside = new QCanvasEllipse(canvas);
	outside->setZ(z() - .001);

	outside->setBrush(QBrush(myColor));
	setBrush(black);

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
	infoLine = new QCanvasLine(canvas());
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
	QCanvasEllipse::moveBy(dx, dy);
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

QPtrList<QCanvasItem> BlackHole::moveableItems() const
{
	QPtrList<QCanvasItem> ret;
	ret.append(exitItem);
	return ret;
}

BlackHoleTimer::BlackHoleTimer(Ball *ball, double speed, int msec)
{
	this->ball = ball;
	this->speed = speed;
	QTimer::singleShot(msec, this, SLOT(mySlot()));
}

void BlackHoleTimer::mySlot()
{
	emit eject(ball, speed);
	delete this;
}

bool BlackHole::place(Ball *ball, bool /*wasCenter*/)
{
	// most number is 10
	if (runs > 10 && game && game->isInPlay())
		return false;

	const double diff = (m_maxSpeed - m_minSpeed);
	const double speed = m_minSpeed + ball->curVector().magnitude() * (diff / 3.75);

	ball->setVelocity(0, 0);
	ball->setState(Stopped);
	ball->setVisible(false);
	ball->setForceStillGoing(true);
	
	BlackHoleTimer *timer = new BlackHoleTimer(ball, speed, Vector(QPoint(x(), y()), QPoint(exitItem->x(), exitItem->y())).magnitude() * 2.5 - speed * 35);

	connect(timer, SIGNAL(eject(Ball *, double)), this, SLOT(eject(Ball *, double)));

	playSound("blackhole");
	return false;
}

void BlackHole::eject(Ball *ball, double speed)
{
	Vector v;
	v.setMagnitude(speed);
	v.setDirection(deg2rad(exitDeg));

	ball->setForceStillGoing(false);
	ball->move(exitItem->x(), exitItem->y());
	ball->setVisible(true);
	ball->setVector(v);
	ball->setState(Rolling);

	runs++;
}

void BlackHole::load(KConfig *cfg)
{
	QPoint exit = cfg->readPointEntry("exit", &exit);
	exitItem->setX(exit.x());
	exitItem->setY(exit.y());
	exitDeg = cfg->readNumEntry("exitDeg", exitDeg);
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

BlackHoleExit::BlackHoleExit(BlackHole *blackHole, QCanvas *canvas)
	: QCanvasLine(canvas)
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
	QCanvasLine::moveBy(dx, dy);
	arrow->move(x(), y());
	blackHole->updateInfo();
}

void BlackHoleExit::setPen(QPen p)
{
	QCanvasLine::setPen(p);
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
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	layout->addWidget(new QLabel(i18n("Exiting ball angle:"), this));
	QSpinBox *deg = new QSpinBox(0, 359, 10, this);
	deg->setSuffix(QString(" ") + i18n("degrees"));
	deg->setValue(blackHole->curExitDeg());
	deg->setWrapping(true);
	layout->addWidget(deg);
	connect(deg, SIGNAL(valueChanged(int)), this, SLOT(degChanged(int)));

	layout->addStretch();

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Minimum exit speed"), this));
	KDoubleNumInput *min = new KDoubleNumInput(this);
	min->setRange(0, 8, 1, true);
	hlayout->addWidget(min);
	connect(min, SIGNAL(valueChanged(double)), this, SLOT(minChanged(double)));
	min->setValue(blackHole->minSpeed());

	hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Maximum"), this));
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

WallPoint::WallPoint(bool start, Wall *wall, QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	this->wall = wall;
	this->start = start;
	alwaysShow = false;
	editing = false;
	visible = true;
	lastId = 0;
	dontmove = false;

	move(0, 0);
	QPoint p;
	if (start)
		p = wall->startPoint();
	else
		p = wall->endPoint();
	setX(p.x());
	setY(p.y());
	setZ(wall->z() + 1);
}

void WallPoint::clean()
{
	int oldWidth = width();
	setSize(7, 7);
	update();

	QCanvasItem *onPoint = 0;
	QCanvasItemList l = collisions(true);
	for (QCanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
		if ((*it)->rtti() == rtti())
			onPoint = (*it);

	if (onPoint)
		move(onPoint->x(), onPoint->y());

	setSize(oldWidth, oldWidth);
}

void WallPoint::moveBy(double dx, double dy)
{
	QCanvasEllipse::moveBy(dx, dy);
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
		QCanvasItemList l = collisions(true);
		for (QCanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
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

	if (abs(id - lastId) < 2)
	{
		lastId = id;
		return false;
	}

	lastId = id;

	bool weirdBounce = visible;

	QPoint relStart(start? wall->startPoint() : wall->endPoint());
	QPoint relEnd(start? wall->endPoint() : wall->startPoint());
	Vector wallVector(relStart, relEnd);
	wallVector.setDirection(-wallVector.direction());

	Vector ballVector(ball->curVector());

	// find the angle between vectors, between 0 and PI
	{
		double difference = fabs(wallVector.direction() - ballVector.direction());
		while (difference > 2 * M_PI)
			difference -= 2 * M_PI;

		if (difference < M_PI / 2 || difference > 3 * M_PI / 2)
			weirdBounce = false;
	}

	// find difference between wall vector and ball vector

	if (weirdBounce)
	{
		playSound("wall", ball->curVector().magnitude() / 10.0);

		ballVector /= wall->dampening;
		const double ballAngle = ballVector.direction();

		// opposite bounce, because we're the endpoint
		const double wallAngle = wallVector.direction() + M_PI / 2;

		const double collisionAngle = ballAngle - wallAngle;
		const double leavingAngle = wallAngle - collisionAngle;

		ballVector.setDirection(leavingAngle);
		ball->setVector(ballVector);

		return false;
	}
	else
		return wall->collision(ball, id);
}

/////////////////////////

Wall::Wall(QCanvas *canvas)
	: QCanvasLine(canvas)
{
	editing = false;
	lastId = 0;

	dampening = 1.2;

	startItem = 0;
	endItem = 0;

	moveBy(0, 0);
	setPoints(-15, 10, 15, -5);
	setZ(50);

	startItem = new WallPoint(true, this, canvas);
	endItem = new WallPoint(false, this, canvas);
	startItem->setVisible(true);
	endItem->setVisible(true);
	setPen(QPen(darkRed, 3));

	setPoints(-15, 10, 15, -5);

	moveBy(0, 0);

	editModeChanged(false);
}

void Wall::selectedItem(QCanvasItem *item)
{
	if (item->rtti() == Rtti_WallPoint)
	{
		WallPoint *wallPoint = dynamic_cast<WallPoint *>(item);
		setPoints(startPoint().x(), startPoint().y(), wallPoint->x() - x(), wallPoint->y() - y());
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
	QCanvasLine::setVisible(yes);

	startItem->setVisible(yes);
	endItem->setVisible(yes);
	startItem->updateVisible();
	endItem->updateVisible();
}

void Wall::setZ(double newz)
{
	QCanvasLine::setZ(newz);
	if (startItem)
		startItem->setZ(newz + 1);
	if (endItem)
		endItem->setZ(newz + 1);
}

void Wall::setPen(QPen p)
{
	QCanvasLine::setPen(p);

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

QPtrList<QCanvasItem> Wall::moveableItems() const
{
	QPtrList<QCanvasItem> ret;
	ret.append(startItem);
	ret.append(endItem);
	return ret;
}

void Wall::moveBy(double dx, double dy)
{
	QCanvasLine::moveBy(dx, dy);

	if ((!startItem || !endItem))
		return;

	startItem->dontMove();
	endItem->dontMove();
	startItem->move(startPoint().x() + x(), startPoint().y() + y());
	endItem->move(endPoint().x() + x(), endPoint().y() + y());
}

void Wall::setVelocity(double vx, double vy)
{
	QCanvasLine::setVelocity(vx, vy);
	/*
	startItem->setVelocity(vx, vy);
	endItem->setVelocity(vx, vy);
	*/
}

QPointArray Wall::areaPoints() const
{
	// editing we want full width for easy moving
	if (editing)
		return QCanvasLine::areaPoints();

	// lessen width, for QCanvasLine::areaPoints() likes
	// to make lines _very_ fat :(
	// from qcanvas.cpp, only the stuff for a line width of 1 taken

	// it's all squished because I don't want my
	// line counts to count code I didn't write!
	QPointArray p(4); const int xi = int(x()); const int yi = int(y()); const QPoint start = startPoint(); const QPoint end = endPoint(); const int x1 = start.x(); const int x2 = end.x(); const int y1 = start.y(); const int y2 = end.y(); const int dx = QABS(x1-x2); const int dy = QABS(y1-y2); if ( dx > dy ) { p[0] = QPoint(x1+xi,y1+yi-1); p[1] = QPoint(x2+xi,y2+yi-1); p[2] = QPoint(x2+xi,y2+yi+1); p[3] = QPoint(x1+xi,y1+yi+1); } else { p[0] = QPoint(x1+xi-1,y1+yi); p[1] = QPoint(x2+xi-1,y2+yi); p[2] = QPoint(x2+xi+1,y2+yi); p[3] = QPoint(x1+xi+1,y1+yi); } return p;
}

void Wall::editModeChanged(bool changed)
{
	editing = changed;

	startItem->setZ(z() + 1);
	endItem->setZ(z() + 1);
	startItem->editModeChanged(editing);
	endItem->editModeChanged(editing);

	int neww = 0;
	if (changed)
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

	if (abs(id - lastId) < 2)
	{
		lastId = id;
		return false;
	}

	lastId = id;
	playSound("wall", ball->curVector().magnitude() / 10.0);

	Vector ballVector(ball->curVector());
	ballVector /= dampening;
	const double ballAngle = ballVector.direction();

	const double wallAngle = -Vector(startPoint(), endPoint()).direction();
	const double collisionAngle = ballAngle - wallAngle;
	const double leavingAngle = wallAngle - collisionAngle;

	ballVector.setDirection(leavingAngle);
	ball->setVector(ballVector);

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

	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Course name: "), this));
	KLineEdit *nameEdit = new KLineEdit(holeInfo->untranslatedName(), this);
	hlayout->addWidget(nameEdit);
	connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged(const QString &)));

	hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Course author: "), this));
	KLineEdit *authorEdit = new KLineEdit(holeInfo->author(), this);
	hlayout->addWidget(authorEdit);
	connect(authorEdit, SIGNAL(textChanged(const QString &)), this, SLOT(authorChanged(const QString &)));

	layout->addStretch();

	hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Par"), this));
	QSpinBox *par = new QSpinBox(1, 15, 1, this);
	par->setValue(holeInfo->par());
	hlayout->addWidget(par);
	connect(par, SIGNAL(valueChanged(int)), this, SLOT(parChanged(int)));
	hlayout->addStretch();

	hlayout->addWidget(new QLabel(i18n("Maximum"), this));
	QSpinBox *maxstrokes = new QSpinBox(holeInfo->lowestMaxStrokes(), 30, 1, this);
	QWhatsThis::add(maxstrokes, i18n("Maximum number of strokes player can take on this hole."));
	QToolTip::add(maxstrokes, i18n("Maximum number of strokes"));
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

StrokeCircle::StrokeCircle(QCanvas *canvas)
	: QCanvasItem(canvas)
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

bool StrokeCircle::collidesWith(const QCanvasItem*) const { return false; }

bool StrokeCircle::collidesWith(const QCanvasSprite*, const QCanvasPolygonalItem*, const QCanvasRectangle*, const QCanvasEllipse*, const QCanvasText*) const { return false; }

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

	p.setBrush(QBrush(black, Qt::NoBrush));
	p.setPen(QPen(white, ithickness / 2));
	p.drawEllipse(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness);
	p.setPen(QPen(QColor((int)(0xff * dvalue) / dmax, 0, 0xff - (int)(0xff * dvalue) / dmax), ithickness));
	p.drawArc(x() + ithickness / 2, y() + ithickness / 2, iwidth - ithickness, iheight - ithickness, deg, length);

	p.setPen(QPen(white, 1));
	p.drawEllipse(x(), y(), iwidth, iheight);
	p.drawEllipse(x() + ithickness, y() + ithickness, iwidth - ithickness * 2, iheight - ithickness * 2);
	p.setPen(QPen(white, 3));
	p.drawLine(x() + iwidth / 2, y() + iheight - ithickness * 1.5, x() + iwidth / 2, y() + iheight);
	p.drawLine(x() + iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 + iheight / 20, x() + iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 - iheight / 20);
	p.drawLine(x() + iwidth - iwidth / 4 + iwidth / 20, y() + iheight - iheight / 4 + iheight / 20, x() + iwidth - iwidth / 4 - iwidth / 20, y() + iheight - iheight / 4 - iheight / 20);
}

/////////////////////////////////////////

KolfGame::KolfGame(ObjectList *obj, PlayerList *players, QString filename, QWidget *parent, const char *name )
	: QCanvasView(parent, name)
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
	oldPlayObjects.setAutoDelete(true);
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

	setFocusPolicy(QWidget::StrongFocus);
	setFixedSize(width + 2 * margin, height + 2 * margin);

	setMargins(margin, margin, margin, margin);

	course = new QCanvas(this);
	course->setBackgroundColor(white);
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
	highlighter = new QCanvasRectangle(course);
	highlighter->setPen(QPen(yellow, 1));
	highlighter->setBrush(QBrush(NoBrush));
	highlighter->setVisible(false);
	highlighter->setZ(10000);

	// shows some info about hole
	infoText = new QCanvasText(course);
	infoText->setText("");
	infoText->setColor(white);
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
	whiteBall->setColor(white);
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
			emit newHole(cfg->readNumEntry("par", 3));
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
	oldPlayObjects.clear();
	delete cfg;
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
	if (inPlay || m_ignoreEvents)
		return;

	if (!editing)
	{
		if (m_useMouse)
		{
			if (e->button() == LeftButton)
				puttPress();
			else if (e->button() == RightButton)
				toggleShowInfo();
		}
	}
	else
	{
		storedMousePos = e->pos();

		QCanvasItemList list = course->collisions(e->pos());
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
			case LeftButton:
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

		QCanvasItemList list = course->collisions(e->pos());
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
		modified = true;

	highlighter->moveBy(-(double)moveX, -(double)moveY);
	movingItem->moveBy(-(double)moveX, -(double)moveY);
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
	moving = false;

	if (inPlay || m_ignoreEvents)
		return;

	if (!editing)
	{
		if (m_useMouse)
		{
			if (e->button() == LeftButton)
				puttRelease();
			else if (e->button() == RightButton)
				toggleShowInfo();
		}
	}

	setFocus();
}

void KolfGame::keyPressEvent(QKeyEvent *e)
{
	if (inPlay || editing || m_ignoreEvents)
		return;

	switch (e->key())
	{
		case Key_Up:
			if (!e->isAutoRepeat())
				toggleShowInfo();
		break;

		case Key_Escape:
			putting = false;
			stroking = false;
			finishStroking = false;
			strokeCircle->setVisible(false);
			putterTimer->stop();
			putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		break;

		case Key_Left:
		case Key_Right:
			// don't move putter if in advanced putting sequence
			if ((!stroking && !putting) || !m_useAdvancedPutting)
				putter->go(e->key() == Key_Left? D_Left : D_Right, e->state() & ShiftButton? Amount_More : e->state() & ControlButton? Amount_Less : Amount_Normal);
		break;

		case Key_Space: case Key_Down:
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
		QCanvasItem *item = 0;
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
		QCanvasItem *item = 0;
		for (item = items.first(); item; item = items.next())
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
			if (citem)
				citem->hideInfo();
		}

		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->hideInfo();

		hideInfoText();
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

	if (e->key() == Key_Space || e->key() == Key_Down)
		puttRelease();
	else if ((e->key() == Key_Backspace || e->key() == Key_Delete) && !(e->state() & ControlButton))
	{
		if (editing && !moving && selectedItem)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(selectedItem);
			if (!citem)
				return;
			citem = citem->itemToDelete();
			if (!citem)
				return;
			QCanvasItem *item = dynamic_cast<QCanvasItem *>(citem);
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

				modified = true;
			}
		}
	}
	else if (e->key() == Key_I || e->key() == Key_Up)
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
			if ((*it).score(curHole) < holeInfo.maxStrokes() || !holeInfo.hasMaxStrokes())
			{
				shotDone();
				loadStateList();
				
				// increment curPlayer; he did take a shot, after all
				(*curPlayer).addStrokeToHole(curHole);
				emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));
			}
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
		QTimer::singleShot(500, this, SLOT(shotDone()));
	}

	if (curState == Holed && inPlay)
	{
		emit inPlayEnd();
		emit playerHoled(&(*curPlayer));

		int curScore = (*curPlayer).score(curHole);
		if (!dontAddStroke)
			curScore++;

		if (curScore == 1)
			playSound("holeinone");
		else if (curScore <= holeInfo.par())
			playSound("woohoo");

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
			QTimer::singleShot(40, this, SLOT(shotDone()));
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
				putterTimer->changeInterval(putterTimerMsec/10);
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
			putterTimer->changeInterval(putterTimerMsec/10);
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

	QCanvasItem *item = 0;

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
	QCanvasItem *item = 0;

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

	if (!dontAddStroke)
		if ((*curPlayer).numHoles())
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
			const QString placeOutside = i18n("Drop outside of hazard");
			const QString rehit = i18n("Rehit from last location");
			options << placeOutside << rehit;
			const QString choice = KComboBoxDialog::getItem(i18n("What would you like to do for your next shot?"), i18n("%1 is in a Hazard").arg((*it).name()), options, placeOutside, "hazardOptions");

			if (choice == placeOutside)
			{
				(*it).ball()->setDoDetect(false);

				double x = ball->x(), y = ball->y();

				while (1)
				{
					QCanvasItemList list = ball->collisions(true);
					bool keepMoving = false;
					while (!list.isEmpty())
					{
						QCanvasItem *item = list.first();
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
			emit maxStrokesReached((*it).name());
			ball->setState(Holed);
			ball->setVisible(false);

			if (allPlayersDone())
			{
				startNextHole();
				return;
			}
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

void KolfGame::startBall(const Vector &vector)
{
	emit inPlayStart();
	putter->setVisible(false);

	(*curPlayer).ball()->setState(Rolling);
	(*curPlayer).ball()->setVector(vector);

	QCanvasItem *item = 0;
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
		KMessageBox::information(this, i18n("%1 will start off.").arg((*curPlayer).name()), i18n("New Hole"), "newHole");
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
		modified = false;

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
			emit newHole(cfg->readNumEntry("par", 3));
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
	QString text = i18n("Hole %1: par %2, maximum number of strokes is %3.").arg(curHole).arg(holeInfo.par()).arg(holeInfo.maxStrokes());
	infoText->move((width - QFontMetrics(infoText->font()).width(text)) / 2, infoText->y());
	infoText->setText(text);
	// I hate this text! Let's not show it
	//infoText->setVisible(true);
}

void KolfGame::showInfoDlg(bool addDontShowAgain)
{
	KMessageBox::information(parentWidget(),
	i18n("Course name: %1").arg(holeInfo.name()) + QString("\n")
	+ i18n("Created by %1").arg(holeInfo.author()) + QString("\n")
	+ i18n("%1 holes").arg(highestHole),
	i18n("Course Information"),
	addDontShowAgain? holeInfo.name() + QString(" ") + holeInfo.author() : QString::null);
}

void KolfGame::hideInfoText()
{
	infoText->setText("");
	infoText->setVisible(false);
}

void KolfGame::openFile()
{
	Object *curObj = 0;

	QCanvasItem *item = 0;
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
	curPar = cfg->readNumEntry("par", 3);
	holeInfo.setPar(curPar);
	holeInfo.borderWallsChanged(cfg->readBoolEntry("borderWalls", holeInfo.borderWalls()));
	holeInfo.setMaxStrokes(cfg->readNumEntry("maxstrokes", 10));
	bool hasFinalLoad = cfg->readBoolEntry("hasFinalLoad", true);

	QStringList warned;
	QStringList groups = cfg->groupList();

	int numItems = 0;
	int _highestHole = 0;

	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		// [<holeNum>-<name>@<x>,<y>|<id>]
		cfg->setGroup(*it);

		const int len = (*it).length();
		const int dashIndex = (*it).find("-");
		const int holeNum = (*it).left(dashIndex).toInt();
		if (holeNum > _highestHole)
			_highestHole = holeNum;

		const int atIndex = (*it).find("@");
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


		const int commaIndex = (*it).find(",");
		const int pipeIndex = (*it).find("|");
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

			QCanvasItem *newItem = curObj->newObject(course);
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

		if (!loaded && name != "hole" && warned.contains(name) <= 0)
		{
			KMessageBox::sorry(this, i18n("To fully experience this hole, you'll need to install the %1 plugin.").arg(QString("\"%1\"").arg(name)));
			warned.append(name);
		}
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
		modified = false;
		for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
			(*it).ball()->setVisible(false);

		return;
	}

	// do it down here; if !hasFinalLoad, do it up there!
	QCanvasItem *qcanvasItem = 0;
	QPtrList<CanvasItem> todo;
	QPtrList<QCanvasItem> qtodo;
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

	modified = false;
}

void KolfGame::addItemsToMoveableList(QPtrList<QCanvasItem> list)
{
	QCanvasItem *item = 0;
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
	QCanvasItem *newItem = newObj->newObject(course);
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
		QCanvasItem *item = 0;
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

	modified = true;
}

bool KolfGame::askSave(bool noMoreChances)
{
	if (!modified)
		// not cancel, don't save
		return false;

	int result = KMessageBox::warningYesNoCancel(this, i18n("There are unsaved changes to current hole. Save them?"), i18n("Unsaved Changes"), i18n("&Save"), noMoreChances? i18n("&Discard") : i18n("Save &Later"), noMoreChances? "DiscardAsk" : "SaveAsk", true);
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
	modified = false;

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
	modified = false;
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
	QCanvasItem *qcanvasItem = 0;
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

	modified = true;
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
	modified = false;

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
	int newHole = 1 + (int)((double)kapp->random() * ((double)(highestHole - 1) / (double)RAND_MAX));
	switchHole(newHole);
}

void KolfGame::save()
{
	if (filename.isNull())
	{
		QString newfilename = KFileDialog::getSaveFileName(QString::null, "application/x-kourse", this, i18n("Pick Kolf Course to Save To"));
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

	QCanvasItem *item = 0;
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
		int holeNum = (*it).left((*it).find("-")).toInt();
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
	cfg->writeEntry("Name", holeInfo.name());

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

	modified = false;
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
	QCanvasItem *item = 0;
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

void KolfGame::playSound(QString file, double vol)
{
	if (m_sound)
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

		KURL url(soundDir + file + QString::fromLatin1(".wav"));
		KPlayObjectFactory factory(artsServer.server());
		KPlayObject *playObject = factory.createPlayObject(url, true);

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
	}
}

void HoleInfo::borderWallsChanged(bool yes)
{
	m_borderWalls = yes;
	game->setBorderWalls(yes);
}

void KolfGame::print(KPrinter &pr)
{
	QPainter p(&pr);

	QPaintDeviceMetrics metrics(&pr);

	// translate to center
	p.translate(metrics.width() / 2 - course->rect().width() / 2, metrics.height() / 2 - course->rect().height() / 2);

	QPixmap pix(width, height);
	QPainter pixp(&pix);
	course->drawArea(course->rect(), &pixp);
	p.drawPixmap(0, 0, pix);

	p.setPen(QPen(black, 2));
	p.drawRect(course->rect());

	p.resetXForm();

	if (pr.option("kde-kolf-title") == "true")
	{
		QString text("%1 - Hole %2; by %3");
		text = text.arg(holeInfo.name()).arg(curHole).arg(holeInfo.author());
		QFont font(kapp->font());
		font.setPointSize(18);
		QRect rect = QFontMetrics(font).boundingRect(text);
		p.setFont(font);

		p.drawText(metrics.width() / 2 - rect.width() / 2, metrics.height() / 2 - course->rect().height() / 2 -20 - rect.height(), text);
	}
}

bool KolfGame::allPlayersDone()
{
	bool b = true;
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		if ((*it).ball()->curState() != Holed)
			b = false;

	return b;
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
		par += cfg.readNumEntry("par", 3);

		hole++;
	}

	info.par = par;
	info.holes = hole;
}

void KolfGame::scoresFromSaved(KConfig *config, PlayerList &players)
{
	config->setGroup("0 Saved Game");
	int numPlayers = config->readNumEntry("Players", 0);
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

		QStringList scores(config->readListEntry("Scores"));
		QValueList<int> intscores;
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
		QValueList<int> intscores = (*it).scores();
		for (QValueList<int>::Iterator it = intscores.begin(); it != intscores.end(); ++it)
			scores.append(QString::number(*it));

		config->writeEntry("Scores", scores);
	}
}

CourseInfo::CourseInfo()
: name(i18n("Course Name")), author(i18n("Course Author")), holes(0), par(0)
{
}

#include "game.moc"
