#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kpopupmenu.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <qbitmap.h>
#include <qbrush.h>
#include <qcanvas.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmap.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
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
#include <qvaluelist.h>

#include "game.h"

// from Qt... you gotta love the humor-- i present the following uncut:
const double PI = 3.14159265358979323846;   // pi // one more useful comment

inline double deg2rad(double theDouble)
{
	return (((2L * PI) / 360L) * theDouble);
}

inline double rad2deg(double theDouble)
{
	return ((360L / (2L * PI)) * theDouble);
}

/////////////////////////

int Config::spacingHint()
{
	// the configs need to be squashed IMHO.
	return KDialog::spacingHint() / 2;
}

int Config::marginHint()
{
	return KDialog::marginHint();
}

void Config::changed()
{
	if (startedUp)
		emit modified();
}

/////////////////////////

QCanvasItem *CanvasItem::onVStrut()
{
	QCanvasItem *qthis = dynamic_cast<QCanvasItem *>(this);
	if (!qthis)
		return 0;
	QCanvasItemList l = qthis->collisions(true);
	l.sort();
	bool aboveVStrut = false;
	CanvasItem *item = 0;
	QCanvasItem *qitem = 0;
	for (QCanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		item = dynamic_cast<CanvasItem *>(*it);
		qitem = *it;
		if (item)
		{
			if (item->vStrut())
			{
				//kdDebug() << "above vstrut\n";
				aboveVStrut = true;
				break;
			}
		}
	}

	return aboveVStrut? qitem : 0;
}

/////////////////////////

DefaultConfig::DefaultConfig(QWidget *parent)
	: Config(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addStretch();
	hlayout->addWidget(new QLabel(i18n("No configuration options"), this));
	hlayout->addStretch();
}

/////////////////////////

RectPoint::RectPoint(QColor color, QCanvasRectangle *rect, QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	setZ(9999);
	setSize(10, 10);
	this->rect = rect;
	setBrush(QBrush(color));
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

	double nw = fabs(x() - rect->x());
	double nh = fabs(y() - rect->y());
	if (nw <= 0 || nh <= 0)
		return;
	RectItem *ritem = dynamic_cast<RectItem *>(rect);
	ritem->newSize(nw, nh);

	update();
}

/////////////////////////

Slope::Slope(QRect rect, QCanvas *canvas)
	: QCanvasRectangle(rect, canvas), grade(4), showGrade(false), reversed(false), color(QColor("#307002"))

{
	stuckOnGround = false;
	setPen(NoPen);
	gradientKeys[KPixmapEffect::VerticalGradient] = "Vertical";
	gradientKeys[KPixmapEffect::HorizontalGradient] = "Horizontal";
	gradientKeys[KPixmapEffect::DiagonalGradient] = "Diagonal";
	gradientKeys[KPixmapEffect::CrossDiagonalGradient] = "Opposite Diagonal";
	gradientKeys[KPixmapEffect::EllipticGradient] = "Elliptic";
	gradientI18nKeys[KPixmapEffect::VerticalGradient] = i18n("Vertical");
	gradientI18nKeys[KPixmapEffect::HorizontalGradient] = i18n("Horizontal");
	gradientI18nKeys[KPixmapEffect::DiagonalGradient] = i18n("Diagonal");
	gradientI18nKeys[KPixmapEffect::CrossDiagonalGradient] = i18n("Opposite Diagonal");
	gradientI18nKeys[KPixmapEffect::EllipticGradient] = i18n("Elliptic");
	setZ(-50);

	point = new RectPoint(color.light(), this, canvas);
	editModeChanged(false);

	// this does updatePixmap
	setGradient("Vertical");
}

void Slope::showInfo()
{
	setGradeVisible(true);
}

void Slope::hideInfo()
{
	setGradeVisible(false);
}

void Slope::aboutToDie()
{
	delete point;
}

QPtrList<QCanvasItem> Slope::moveableItems()
{
	//kdDebug() << "moveableitems\n";
	QPtrList<QCanvasItem> ret;
	ret.append(point);
	return ret;
}

void Slope::setSize(int width, int height)
{
	newSize(width, height);
}

void Slope::newSize(int width, int height)
{
	QCanvasRectangle::setSize(width, height);

	updateZ();
}

void Slope::moveBy(double dx, double dy)
{
	QCanvasRectangle::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width(), y() + height());

	updateZ();

	update();
}

void Slope::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Slope::updateZ()
{
	//const bool diag = (type == KPixmapEffect::DiagonalGradient || type == KPixmapEffect::CrossDiagonalGradient);
	int area = (height() * width());
	//if (diag)
		//area /= 2;
	const int defaultz = -50;

	double newZ = 0;

	QCanvasItem *qitem = 0;
	if (!stuckOnGround)
		qitem = onVStrut();
	if (qitem)
	{
		QCanvasRectangle *rect = dynamic_cast<QCanvasRectangle *>(qitem);
		if (rect)
			if (area > (rect->width() * rect->height()))
				newZ = defaultz;
			else
				newZ = qitem->z();
		else
			newZ = qitem->z();
	}
	else
		newZ = defaultz;

	setZ(((double)1 / (area == 0? 1 : area))  + newZ);
}

void Slope::load(KSimpleConfig *cfg, int hole)
{
	//kdDebug() << "Slope::load()\n";

	cfg->setGroup(makeGroup(hole, "slope", x(), y()));

	stuckOnGround = cfg->readBoolEntry("stuckOnGround", stuckOnGround);
	grade = cfg->readNumEntry("grade", grade);
	reversed = cfg->readBoolEntry("reversed", reversed);
	setSize(cfg->readNumEntry("width", width()), cfg->readNumEntry("height", height()));
	QString gradientType = cfg->readEntry("gradient", gradientKeys[type]);
	setGradient(gradientType);
}

void Slope::save(KSimpleConfig *cfg, int hole)
{
	//kdDebug() << "Slope::save()\n";

	cfg->setGroup(makeGroup(hole, "slope", x(), y()));
	//kdDebug() << "on group: " << cfg->group() << endl;

	cfg->writeEntry("reversed", reversed);
	cfg->writeEntry("width", width());
	cfg->writeEntry("height", height());
	cfg->writeEntry("gradient", gradientKeys[type]);
	cfg->writeEntry("grade", grade);
	cfg->writeEntry("stuckOnGround", stuckOnGround);
}

void Slope::draw(QPainter &painter)
{
	if (pixmap.width() != width() || pixmap.height() != height())
		updatePixmap();
	//kdDebug() << "Slope::draw\n";
	painter.drawPixmap((int)x(), (int)y(), pixmap);

	if (showGrade)
	{
		painter.setPen(white);
		QFont font(kapp->font());
		font.setPixelSize(18); 
		painter.setFont(font);
		QFontMetrics metrics(painter.fontMetrics());

		QString num(QString::number(grade));

		// this makes it about in the center
		int xavg = 0, yavg = 0;
		QPointArray r = areaPoints();
		for (unsigned int i = 0; i < r.size(); ++i)
		{
			xavg += r[i].x();
			yavg += r[i].y();
		}
		xavg /= r.size();
		yavg /= r.size();

		//kdDebug() << "xavg is " << xavg << endl;
		//kdDebug() << "yavg is " << yavg << endl;

		const int tx = xavg - metrics.width(num) / 2;
		const int ty = yavg + metrics.boundingRect(num).height() / 2;

		painter.drawText(tx, ty, num);
	}
}

QPointArray Slope::areaPoints() const
{
	//kdDebug() << "Slope::areaPoints\n";
	switch (type)
	{
		case KPixmapEffect::CrossDiagonalGradient:
		{
			QPointArray ret(3);
			ret[0] = QPoint((int)x(), (int)y());
			ret[1] = QPoint((int)x() + width(), (int)y() + height());
			ret[2] = reversed? QPoint((int)x() + width(), y()) : QPoint((int)x(), (int)y() + height());

			return ret;
		}

		case KPixmapEffect::DiagonalGradient:
		{
			QPointArray ret(3);
			ret[0] = QPoint((int)x() + width(), (int)y());
			ret[1] = QPoint((int)x(), (int)y() + height());
			ret[2] = !reversed? QPoint((int)x() + width(), y() + height()) : QPoint((int)x(), (int)y());

			return ret;
		}

		case KPixmapEffect::EllipticGradient:
		{
    		QPointArray ret;
			ret.makeEllipse((int)x(), (int)y(), width(), height());
    		return ret;
		}

		default:
			return QCanvasRectangle::areaPoints();
	}
}

void Slope::collision(Ball *ball, long int /*id*/)
{
	//kdDebug() << "Slope::collision\n";
	double vx = ball->xVelocity();
	double vy = ball->yVelocity();
	//if (vy == 0 && vx == 0)
		//return;
	const double addto = 0.013 * grade;

	const bool diag = type == KPixmapEffect::DiagonalGradient || type == KPixmapEffect::CrossDiagonalGradient;
	const bool circle = type == KPixmapEffect::EllipticGradient;
	double slopeAngle = 0;
	if (diag)
		slopeAngle = atan((double)width() / (double)height());
	else if (circle)
	{
		const QPoint start(x() + (int)width() / 2.0, y() + (int)height() / 2.0);
		const QPoint end((int)ball->x(), (int)ball->y());
		const double yDiff = (double)(end.x() - start.x());
		const double xDiff = (double)(start.y() - end.y());
		kdDebug() << "yDiff: " << yDiff << endl;
		kdDebug() << "xDiff: " << xDiff << endl;

		if (yDiff == 0 && xDiff == 0)
			return;

		if (yDiff == 0)
		{
			// horizontal
			slopeAngle = start.x() < end.x()? 0 : PI;
		}
		else
		{
			slopeAngle = atan(yDiff / xDiff);
			kdDebug() << "before slopeAngle: " << rad2deg(slopeAngle) << endl;

			//if (slopeAngle < 0)
			if (end.y() < start.y())
			{
				slopeAngle *= -1;
				kdDebug() << "neg slopeAngle: " << rad2deg(slopeAngle) << endl;
				slopeAngle = (PI / 2) - slopeAngle;
				kdDebug() << "pi / 2 -  slopeAngle: " << rad2deg(slopeAngle) << endl;
			}
			else
			{
				//slopeAngle += PI / 2;
				slopeAngle *= -1;
				slopeAngle = (-PI / 2) - slopeAngle;
				kdDebug() << "neg slopeAngle: " << rad2deg(slopeAngle) << endl;
			}

		}
		
		//slopeAngle += PI;
	}

	kdDebug() << "slopeAngle: " << rad2deg(slopeAngle) << endl;

	switch (type)
	{
		case KPixmapEffect::HorizontalGradient:
			reversed? vx += addto : vx -= addto;
		break;

		case KPixmapEffect::VerticalGradient:
			reversed? vy += addto : vy -= addto;
		break;

		case KPixmapEffect::DiagonalGradient:
		case KPixmapEffect::EllipticGradient:
			reversed? vx += cos(slopeAngle) * addto : vx -= cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		case KPixmapEffect::CrossDiagonalGradient:
			reversed? vx -= cos(slopeAngle) * addto : vx += cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		default:
		break;
	}

	if (vx / ball->xVelocity() < 0 && vy / ball->yVelocity() < 0)
	{
		vx = vy = 0;
		ball->setState(Stopped);
	}

	ball->setVelocity(vx, vy);
}

void Slope::setGradient(QString text)
{
	for (QMap<KPixmapEffect::GradientType, QString>::Iterator it = gradientI18nKeys.begin(); it != gradientI18nKeys.end(); ++it)
	{
		if (it.data() == text)
		{
			setType(it.key());
			break;
		}
	}
}

void Slope::updatePixmap()
{
	QPixmap qpixmap(width(), height());
	pixmap = qpixmap;
	const bool diag = type == KPixmapEffect::DiagonalGradient || type == KPixmapEffect::CrossDiagonalGradient;
	const bool circle = type == KPixmapEffect::EllipticGradient;

	// these are approximate for i guess what would theoretically
	// be purrfect
	// but I've not the time to get it purrfect
	// i wish i did -- anybody wanna fix it? :-)
	QColor darkColor = color.dark(100 + grade * 10);
	QColor lightColor = diag || circle? color.light(110 + (diag? 5 : 2) * grade) : color;

	(void) KPixmapEffect::gradient(pixmap, reversed? darkColor : lightColor, reversed? lightColor : darkColor, type);

	if (diag || circle)
	{
		// make cleared bitmap
		QBitmap bitmap(pixmap.width(), pixmap.height(), true);;
		QPainter bpainter;
		bpainter.begin(&bitmap);
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

void Bridge::collision(Ball *ball, long int /*id*/)
{
	ball->setFrictionMultiplier(.63);
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
	topWall->setPen(QPen(color.dark(), 2));
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

QPointArray Bridge::areaPoints() const
{
	// this is what qcanvas.cpp does, except here i also subtract indent
	/*
    QPointArray pa(4);
    int pw = (pen().width()+1)/2;
    if ( pw < 1 ) pw = 1;
    if ( pen() == NoPen ) pw = 0;
	const short indent = 4;
    pa[0] = QPoint(((int)x()-pw) + indent, ((int)y()-pw) + indent);
    pa[1] = pa[0] + QPoint((width()+pw*2) - indent,0);
    pa[2] = pa[1] + QPoint(0,(height()+pw*2) - 2*indent);
    pa[3] = pa[0] + QPoint(0,(height()+pw*2) - 2*indent);
    return pa;
	*/
	return QCanvasRectangle::areaPoints();
}

void Bridge::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Bridge::moveBy(double dx, double dy)
{
	//kdDebug() << "bridge::moveBy()\n";
	QCanvasRectangle::moveBy(dx, dy);

	point->dontMove();
	point->move(x() + width(), y() + height());

	topWall->move(x(), y());
	botWall->move(x(), y());
	leftWall->move(x(), y());
	rightWall->move(x(), y());

	QCanvasItemList list = collisions(true);
	for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
	{
		//kdDebug() << "on an item\n";
		CanvasItem *citem = dynamic_cast<CanvasItem *>(*it);
		if (citem)
			citem->updateZ();
	}
}

void Bridge::setVelocity(double vx, double vy)
{
	QCanvasRectangle::setVelocity(vx, vy);
	/*
	topWall->setVelocity(vx, vy);
	botWall->setVelocity(vx, vy);
	leftWall->setVelocity(vx, vy);
	rightWall->setVelocity(vx, vy);
	*/
}

void Bridge::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "bridge", x(), y()));

	doLoad(cfg, hole);
}

void Bridge::doLoad(KSimpleConfig *cfg, int /*hole*/)
{
	newSize(cfg->readNumEntry("width", width()), cfg->readNumEntry("height", height()));
	setTopWallVisible(cfg->readBoolEntry("topWallVisible", topWallVisible()));
	setBotWallVisible(cfg->readBoolEntry("botWallVisible", botWallVisible()));
	setLeftWallVisible(cfg->readBoolEntry("leftWallVisible", leftWallVisible()));
	setRightWallVisible(cfg->readBoolEntry("rightWallVisible", rightWallVisible()));
	update();
}

void Bridge::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "bridge", x(), y()));

	doSave(cfg, hole);
}

void Bridge::doSave(KSimpleConfig *cfg, int /*hole*/)
{
	cfg->writeEntry("width", width());
	cfg->writeEntry("height", height());
	cfg->writeEntry("topWallVisible", topWallVisible());
	cfg->writeEntry("botWallVisible", botWallVisible());
	cfg->writeEntry("leftWallVisible", leftWallVisible());
	cfg->writeEntry("rightWallVisible", rightWallVisible());
}

QPtrList<QCanvasItem> Bridge::moveableItems()
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

/////////////////////////

Floater::Floater(QRect rect, QCanvas *canvas)
	: Bridge(rect, canvas), speedfactor(16)
{
	//kdDebug() << "Floater::Floater\n";
	setEnabled(true);
	noUpdateZ = false;
	haventMoved = true;
	wall = new FloaterGuide(this, canvas);
	wall->setPoints(100, 100, 200, 200);
	lastStart = wall->startPoint();
	lastEnd = wall->endPoint();
	lastWall = QPoint(wall->x(), wall->y());
	move(wall->endPoint().x(), wall->endPoint().y());

	setTopWallVisible(false);
	setBotWallVisible(false);
	setLeftWallVisible(false);
	setRightWallVisible(false);

	newSize(width(), height());
	moveBy(0, 0);
	setSpeed(5);

	editModeChanged(false);
}

void Floater::setGame(KolfGame *game)
{
	Bridge::setGame(game);

	wall->setGame(game);
}

void Floater::editModeChanged(bool changed)
{
	if (changed)
	{
		wall->editModeChanged(true);
	}
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
		QPoint start = wall->startPoint(), end = wall->endPoint();
		const double wally = wall->y();
		const double wallx = wall->x();

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

		if (lastStart != wall->startPoint() || lastEnd != wall->endPoint() || lastWall != QPoint(wallx, wally))
		{
			move(wall->endPoint().x() + wallx, wall->endPoint().y() + wally);
			setSpeed(speed);
			//kdDebug() << "moving back to beginning\n";
		}
		/*
		else if (start.x() == end.x())
			// vertical
		{
			//kdDebug() << "vertical\n";
			if (yVelocity() < 0)
			{
				if (y() < start.y() + wally)
				{
					//kdDebug() << "top\n";
					setSpeed(speed);
					setVelocity(-xVelocity(), -yVelocity());
				}
			}
			else if (y() > end.y() + wally)
			{
				//kdDebug() << "bottom\n";
				setSpeed(speed);
				setVelocity(-xVelocity(), -yVelocity());
			}
		}
		*/
		else if (start.y() == end.y())
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
			//kdDebug() << "first\n";
			setSpeed(speed);
			setVelocity(-xVelocity(), -yVelocity());
		}
		else if (x() > end.x() + wallx && (yVelocity() < 0? y() < end.y() + wally : y() > end.y() + wally))
		{
			//kdDebug() << "second\n";
			setSpeed(speed);
		}

		lastStart = wall->startPoint();
		lastEnd = wall->endPoint();
		lastWall = QPoint(wallx, wally);

		//kdDebug() << "wall is visible: " << wall->isVisible() << endl;
	}
}

QPtrList<QCanvasItem> Floater::moveableItems()
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
	//kdDebug() << "setSpeed(" << news << ")" << endl;
	if (news < 0)
		return;

	if (news != 0)
		speed = news;

	if (!wall)
		return;

	/*
	QRect rect(wall->startPoint(), wall->endPoint());
	rect = rect.normalize();
	wall->setPoints(rect.x(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height());
	if (wall->endPoint().y() < wall->startPoint().y() && wall->endPoint().x() < wall->startPoint().x())
		wall->setPoints(wall->endPoint().x(), wall->endPoint().y(), wall->startPoint().x(), wall->startPoint().y());
	*/
	
	const double rise = wall->startPoint().y() - wall->endPoint().y();
	const double run = wall->startPoint().x() - wall->endPoint().x();
	double wallAngle = atan(rise / run);
	const double factor = (double)speed / (double)3.5;
	
	setVelocity(-cos(wallAngle) * factor, -sin(wallAngle) * factor);
	//kdDebug() << "velocities now " << xVelocity() << ", " << yVelocity() << endl;
}

void Floater::aboutToSave()
{
	setSpeed(0);
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
			if (item)
				item->updateZ();

		if ((*it)->z() >= z())
		{
			if (item)
			{
				if (item->canBeMovedByOthers())
				{
					if (collidesWith(*it))
					{
						if ((*it)->rtti() == Rtti_Ball)
						{
							((Ball *)(*it))->setMoved(true);
							if (game)
								if (game->hasFocus() && !game->isEditing())
									if (game->curBall() == (Ball *)(*it))
										//QCursor::setPos(QCursor::pos().x() + dx, QCursor::pos().y() + dy);
										game->updateMouse();
						}

						//kdDebug() << "moving an item\n";
						if ((*it)->rtti() != Rtti_Putter)
							(*it)->moveBy(dx, dy);
					}
				}
			}
		}
	}

	point->dontMove();
	point->move(x() + width(), y() + height());

	topWall->move(x(), y());
	botWall->move(x(), y());
	leftWall->move(x(), y());
	rightWall->move(x(), y());

	// this call must come after we have tested for collisions, otherwise we skip them when saving!
	// that's a bad thing
	QCanvasRectangle::moveBy(dx, dy);
}

void Floater::save(KSimpleConfig *cfg, int hole)
{
	aboutToSave();
	//kdDebug() << "floater::save\n";
	//kdDebug() << "at: " << x() << ", " << y() << endl;
	cfg->setGroup(makeGroup(hole, "floater", x(), y()));
	cfg->writeEntry("speed", speed);
	cfg->writeEntry("startPoint", QPoint(wall->startPoint().x() + wall->x(), wall->startPoint().y() + wall->y()));
	cfg->writeEntry("endPoint", QPoint(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y()));

	doSave(cfg, hole);
}

void Floater::load(KSimpleConfig *cfg, int hole)
{
	//kdDebug() << "floater::load, hole is " << hole << endl;
	move(firstPoint.x(), firstPoint.y());
	//kdDebug() << "at: " << x() << ", " << y() << endl;
	cfg->setGroup(makeGroup(hole, "floater", x(), y()));

	QPoint start(wall->startPoint());
	start = cfg->readPointEntry("startPoint", &start);
	QPoint end(wall->endPoint());
	end = cfg->readPointEntry("endPoint", &end);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->move(0, 0);

	setSpeed(cfg->readNumEntry("speed", -1));

	doLoad(cfg, hole);

	move(wall->endPoint().x() + wall->x(), wall->endPoint().y() + wall->y());
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
	QHBoxLayout *hlayout = new QHBoxLayout(m_vlayout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(1, 20, 2, floater->curSpeed(), Qt::Horizontal, this);
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
	QHBoxLayout *hlayout = new QHBoxLayout(m_vlayout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Slow"), this));
	QSlider *slider = new QSlider(1, 10, 1, windmill->curSpeed(), Qt::Horizontal, this);
	hlayout->addWidget(slider);
	hlayout->addWidget(new QLabel(i18n("Fast"), this));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(speedChanged(int)));

	bot->setEnabled(false);
}

void WindmillConfig::speedChanged(int news)
{
	windmill->setSpeed(news);
	changed();
}

/////////////////////////

Windmill::Windmill(QRect rect, QCanvas *canvas)
	: Bridge(rect, canvas), speedfactor(16)
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
	//kdDebug() << "setSpeed(" << news << ")" << endl;
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

void Windmill::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "windmill", x(), y()));
	cfg->writeEntry("speed", speed);

	doSave(cfg, hole);
}

void Windmill::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "windmill", x(), y()));
	setSpeed(cfg->readNumEntry("speed", -1));

	doLoad(cfg, hole);

	left->editModeChanged(false);
	right->editModeChanged(false);
	guard->editModeChanged(false);
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

void Windmill::setVelocity(double vx, double vy)
{
	Bridge::setVelocity(vx, vy);
	/*
	guard->setVelocity(guard->xVelocity() + vx, guard->yVelocity() + vy);
	left->setVelocity(vx, vy);
	right->setVelocity(vx, vy);
	*/
}

void Windmill::setSize(int width, int height)
{
	newSize(width, height);
}

void Windmill::newSize(int width, int height)
{
	Bridge::newSize(width, height);
	const int indent = width / 4;
	left->setPoints(0, height, indent, height);
	right->setPoints(width - indent, height, width, height);

	guard->setBetween(x(), x() + width);
	guard->setPoints(0, height + 4, (double)indent / (double)1.07 - 2, height + 4);
}

/////////////////////////

void WindmillGuard::advance(int phase)
{
	Wall::advance(phase);

	if (phase == 1)
	{
		//kdDebug() << "my start x: " << x() + startPoint().x() << ", min: " << min << endl;
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

void Sign::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "sign", x(), y()));

	m_text = cfg->readEntry("text", m_text);

	doLoad(cfg, hole);
}

void Sign::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "sign", x(), y()));

	cfg->writeEntry("text", m_text);

	doSave(cfg, hole);
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
	//painter.drawText(, y() + height() / 2 + 3, m_text);
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

SlopeConfig::SlopeConfig(Slope *slope, QWidget *parent)
	: Config(parent)
{
	this->slope = slope;
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	KComboBox *gradient = new KComboBox(this);
	QStringList items;
	QString curText;
	for (QMap<KPixmapEffect::GradientType, QString>::Iterator it = slope->gradientI18nKeys.begin(); it != slope->gradientI18nKeys.end(); ++it)
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

	QCheckBox *reversed = new QCheckBox(i18n("Reverse Direction"), this);
	reversed->setChecked(slope->isReversed());
	layout->addWidget(reversed);
	connect(reversed, SIGNAL(toggled(bool)), this, SLOT(setReversed(bool)));

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Shallow"), this));
	QSlider *grade = new QSlider(1, 8, 1, slope->curGrade(), Qt::Horizontal, this);
	hlayout->addWidget(grade);
	hlayout->addWidget(new QLabel(i18n("Steep"), this));
	connect(grade, SIGNAL(valueChanged(int)), this, SLOT(gradeChanged(int)));

	QCheckBox *stuck = new QCheckBox(i18n("Permanently On Ground"), this);
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

void SlopeConfig::gradeChanged(int newgrade)
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

	QCheckBox *check = new QCheckBox(i18n("Enable Show/Hide"), this);
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
}

void Ellipse::advance(int phase)
{
	QCanvasEllipse::advance(phase);

	//kdDebug() << "m_changeEnabled is " << m_changeEnabled << endl;
	if (phase == 1 && m_changeEnabled && !dontHide)
	{
		//kdDebug() << "in advance\n";
		if (count > (m_changeEvery + 10) * 1.8)
			count = 0;
		if (count == 0)
			setVisible(!isVisible());

		count++;
	}
}

void Ellipse::doLoad(KSimpleConfig *cfg, int /*hole*/)
{
	setChangeEnabled(cfg->readBoolEntry("changeEnabled", changeEnabled()));
	setChangeEvery(cfg->readNumEntry("changeEvery", changeEvery()));
}

void Ellipse::doSave(KSimpleConfig *cfg, int /*hole*/)
{
	cfg->writeEntry("changeEvery", changeEvery());
	cfg->writeEntry("changeEnabled", changeEnabled());
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

	setBrush(QColor("#5498FF"));
	setZ(-25);
	setPen(blue);
}

void Puddle::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "puddle", x(), y()));

	doSave(cfg, hole);
}

void Puddle::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "puddle", x(), y()));

	doLoad(cfg, hole);
}

void Puddle::collision(Ball *ball, long int /*id*/)
{
	QCanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
	i.setVisible(true);

	// is center of ball in?
	if (i.collidesWith(this)/* && ball->curSpeed() < 4*/)
	{
		playSound("puddle");
		ball->setAddStroke(ball->addStroke() + 1);
		ball->setPlaceOnGround(true, ball->xVelocity(), ball->yVelocity());
		//ball->setVelocity(0, 0);
		ball->setVisible(false);
		ball->setState(Stopped);
	}
}

/////////////////////////
// sand is similiar to puddle

Sand::Sand(QCanvas *canvas)
	: Ellipse(canvas)
{
	setSize(45, 40);

	setBrush(QColor("#c9c933"));
	setZ(-26);
	setPen(yellow);
}

void Sand::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "sand", x(), y()));

	doSave(cfg, hole);
}

void Sand::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "sand", x(), y()));

	doLoad(cfg, hole);
}

void Sand::collision(Ball *ball, long int /*id*/)
{
	QCanvasRectangle i(QRect(ball->x(), ball->y(), 1, 1), canvas());
	i.setVisible(true);

	// is center of ball in?
	if (i.collidesWith(this)/* && ball->curSpeed() < 4*/)
	{
		ball->setFrictionMultiplier(7);
	}
}

/////////////////////////

Putter::Putter(QCanvas *canvas)
	: QCanvasLine(canvas)
{
	guideLine = new QCanvasLine(canvas);
	guideLine->setPen(QPen(white, 1));
	guideLine->setZ(998.8);

	setPen(QPen(black, 4));
	putterWidth = 8;
	maxDeg = 360;

	hideInfo();

	// this also sets Z
	resetDegrees();
}

void Putter::showInfo()
{
	guideLine->setVisible(true);
}

void Putter::hideInfo()
{
	guideLine->setVisible(false);
}

void Putter::moveBy(double dx, double dy)
{
	//kdDebug() << "putter::moveBy(" << dx << ", " << dy << endl;
	QCanvasLine::moveBy(dx, dy);
	guideLine->move(x(), y());
}

void Putter::setVisible(bool yes)
{
	QCanvasLine::setVisible(yes);
	guideLine->setVisible(false);
}

void Putter::setOrigin(int _x, int _y)
{
	setVisible(true);
	//if ((int)x() == _x && (int)y() == _y)
		//return;
	move(_x, _y);
	//kdDebug() << "setOrigin\n";
	len = 9;
	finishMe();
}

void Putter::setDegrees(Ball *ball)
{
	deg = degMap.contains(ball)? degMap[ball] : 0;
	//kdDebug() << "contains? " << degMap.contains(ball) << endl;
	finishMe();
}

void Putter::go(Direction d, bool more)
{
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
			deg += more? 6 : 3;
			if (deg > maxDeg)
				deg -= maxDeg;
			break;
		case D_Right:
			deg -= more? 6 : 3;
			if (deg < 0)
				deg = maxDeg - abs(deg);
			break;
	}

	finishMe();
}

void Putter::finishMe()
{
	const double radians = deg2rad(deg);
	midPoint.setX(cos(radians)*len);
	midPoint.setY(-sin(radians)*len);

	QPoint start;
	QPoint end;

	if (midPoint.y() || !midPoint.x())
	{
		start.setX(midPoint.x() - putterWidth*sin(radians));
		start.setY(midPoint.y() - putterWidth*cos(radians));
		end.setX(midPoint.x() + putterWidth*sin(radians));
		end.setY(midPoint.y() + putterWidth*cos(radians));
	}
	else
	{
		start.setX(midPoint.x());
		start.setY(midPoint.y() + putterWidth);
		end.setY(midPoint.y() - putterWidth);
		end.setX(midPoint.x());
	}

	guideLine->setPoints(midPoint.x(), midPoint.y(), -midPoint.x() * 2, -midPoint.y() * 2);

	setPoints(start.x(), start.y(), end.x(), end.y());

	update();
}

/////////////////////////

Ball::Ball(QCanvas *canvas)
	: QCanvasEllipse(canvas)
{
	setBeginningOfHole(false);
	setBlowUp(false);
	setBrush(black);
	setPen(black);
	resetSize();
	//kdDebug() << "ball ctor\n";
	setVelocity(0, 0);
	collisionId = 0;
	m_addStroke = false;
	m_placeOnGround = false;

	// this sets z
	setState(Stopped);
}

void Ball::setState(BallState newState)
{
	state = newState;
	if (state == Stopped)
		setZ(1000);
	else
		setBeginningOfHole(false);
}

void Ball::advance(int phase)
{
	//QCanvasEllipse::advance(phase);

	if (phase == 1 && m_blowUp)
	{
		if (blowUpCount >= 50)
		{
			setAddStroke(addStroke() + 1);
			setBlowUp(false);
			resetSize();
			return;
		}

		const double diff = 8;
		double randnum = kapp->random();
		const double width = 6 + randnum * (diff / RAND_MAX);
		randnum = kapp->random();
		const double height = 6 + randnum * (diff / RAND_MAX);
		setSize(width, height);
		blowUpCount++;
	}
}

void Ball::friction()
{
	if (state == Stopped || state == Holed || !isVisible()) { setVelocity(0, 0); return; }
	double vx = xVelocity();
	double vy = yVelocity();
	double ballAngle = atan(vx / vy);
	if (vy < 0)
		ballAngle -= PI;
	ballAngle = PI/2 - ballAngle;
	vx -= cos(ballAngle) * .033 * frictionMultiplier;
	vy -= sin(ballAngle) * .033 * frictionMultiplier;
	if (vx / xVelocity() < 0)
	{
		vx = vy = 0;
		state = Stopped;
	}
	setVelocity(vx, vy);

	frictionMultiplier = 1;
}

void Ball::collisionDetect()
{
	if (state == Stopped)
		return;

	if (collisionId >= INT_MAX - 1)
		collisionId = 0;
	else
		collisionId++;

	QCanvasItemList list = collisions(true);
	if (list.isEmpty())
		return;

	// please don't ask why QCanvas doesn't actually sort its list
	// it just doesn't.
	list.sort();

	QCanvasItem *item = 0;

	//kdDebug() << "---" << endl;
	for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
	{
		item = *it;

		//kdDebug() << "z: " << item->z() << endl;

		if (item->rtti() == Rtti_NoCollision || item->rtti() == Rtti_Putter)
			continue;
		if (!collidesWith(item))
			continue;

		if (item->rtti() == rtti())
		{
			if (curSpeed() > 2.7)
			{
				// it's one of our own kind, a ball, and we're hitting it
				// sorta hard
				//kdDebug() << "gonna blow up\n";
				Ball *oball = dynamic_cast<Ball *>(item);
				if (/*oball->curState() != Stopped && */oball->curState() != Holed)
					oball->setBlowUp(true);
				continue;
			}
		}

		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->collision(this, collisionId);
		break;
	}
}

BallState Ball::currentState()
{
	return state;
}

/////////////////////////

Hole::Hole(QColor color, QCanvas *canvas)
	: QCanvasEllipse(15, 15, canvas)
{
	setZ(998.1);
	setPen(black);
	setBrush(color);
}

void Hole::collision(Ball *ball, long int /*id*/)
{
	bool wasCenter = false;

	switch (result(QPoint(ball->x(), ball->y()), ball->curSpeed(), &wasCenter))
	{
		case Result_Holed:
			if (place(ball, wasCenter))
				ball->setState(Holed);
			return;

		default:
		break;
	}
}

HoleResult Hole::result(QPoint p, double s, bool * /*wasCenter*/)
{
	//kdDebug() << "Hole::result\n";
	if (s > 3.0)
	{
		//kdDebug() << "too fast!\n";
		return Result_Miss;
	}

	QCanvasRectangle i(QRect(p, QSize(1, 1)), canvas());
	i.setVisible(true);

	// is center of ball in cup?
	if (i.collidesWith(this))
	{
		//kdDebug() << "in circle\n";
		return Result_Holed;
	}
	else
		return Result_Miss;
}

/////////////////////////

bool Cup::place(Ball *ball, bool /*wasCenter*/)
{
	//kdDebug() << "Cup::place()\n";
	ball->move(x(), y());
	ball->setVelocity(0, 0);
	return true;
}

void Cup::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "cup", x(), y()));
	cfg->writeEntry("dummykey", true);
}

/////////////////////////

BlackHole::BlackHole(QCanvas *canvas)
	: Hole(black, canvas), exitDeg(0)
{
	m_minSpeed = 3;
	m_maxSpeed = 5;

	exitItem = new BlackHoleExit(this, canvas);
	exitItem->setPen(QPen(darkGray, 6));
	exitItem->setX(300);
	exitItem->setY(100);

	infoLine = 0;

	setSize(width(), width() / .70);

	finishMe();
}

void BlackHole::showInfo()
{
	delete infoLine;
	infoLine = new QCanvasLine(canvas());
	infoLine->setVisible(true);
	infoLine->setPen(QPen(white, 2));
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
	delete exitItem;
}

QPtrList<QCanvasItem> BlackHole::moveableItems()
{
	QPtrList<QCanvasItem> ret;
	ret.append(exitItem);
	return ret;
}

bool BlackHole::place(Ball *ball, bool /*wasCenter*/)
{
	playSound("blackhole");

	double diff = (m_maxSpeed - m_minSpeed);
	double strength = m_minSpeed + ball->curSpeed() * (diff / 3.0);
	//kdDebug() << "strength is " << strength << endl;

	ball->move(exitItem->x(), exitItem->y());
	int deg = -exitDeg;
	ball->setVelocity(cos(deg2rad(deg))*strength, sin(deg2rad(deg))*strength);
	ball->setState(Rolling);

	return false;
}

void BlackHole::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "blackhole", x(), y()));
	QPoint exit = cfg->readPointEntry("exit", &exit);
	exitItem->setX(exit.x());
	exitItem->setY(exit.y());
	exitDeg = cfg->readNumEntry("exitDeg", exitDeg);
	m_minSpeed = cfg->readNumEntry("minspeed", m_minSpeed);
	m_maxSpeed = cfg->readNumEntry("maxspeed", m_maxSpeed);

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

	//kdDebug() << "moving points to " << start.x() << ", " << start.y() << " and " << end.x() << ", " << end.y() << endl;
	exitItem->setPoints(start.x(), start.y(), end.x(), end.y());
	exitItem->setVisible(true);
}

void BlackHole::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "blackhole", x(), y()));
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
	infoLine = 0;
	setZ(blackHole->z());
}

Config *BlackHoleExit::config(QWidget *parent)
{
	return blackHole->config(parent);
}

void BlackHoleExit::showInfo()
{
	/*
	QPoint start = startPoint();
	start.setX((int)start.x() + (int)x());
	start.setY((int)start.y() + (int)y());
	QPoint end = endPoint();
	end.setX((int)end.x() + (int)x());
	end.setY((int)end.y() + (int)y());
	double wallAngle = deg2rad(-blackHole->curExitDeg() + 90);
	//kdDebug() << "wallAngle is " << wallAngle << endl;
	const double width = sqrt(((start.y() - end.y()) * (start.y() - end.y())) + ((start.x() - end.x()) * (start.x() - end.x())));
	const QPoint midPoint(start.x() + ((double)(cos(wallAngle) * width) / 2), start.y() + (double)(sin(wallAngle) * width) / 2);
	const int angle = deg2rad(blackHole->curExitDeg());
	//kdDebug() << "angle is " << angle << endl;
	const int guideLength = 40;
	const QPoint endPoint(midPoint.x() + cos(angle) * guideLength, midPoint.y() - sin(angle) * guideLength);

	delete infoLine;
	infoLine = new QCanvasLine(canvas());
	infoLine->setPen(QPen(white, 1));
	//kdDebug() << "start is " << start.x() << ", " << start.y() << endl;
	//kdDebug() << "midPoint is " << midPoint.x() << ", " << midPoint.y() << endl;
	//kdDebug() << "endPoint is " << endPoint.x() << ", " << endPoint.y() << endl;
	infoLine->setPoints(midPoint.x(), midPoint.y(), endPoint.x(), endPoint.y());
	infoLine->setVisible(true);
	*/
}

void BlackHoleExit::hideInfo()
{
	/*
	delete infoLine;
	infoLine = 0;
	*/
}

/////////////////////////

BlackHoleConfig::BlackHoleConfig(BlackHole *blackHole, QWidget *parent)
	: Config(parent)
{
	this->blackHole = blackHole;
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	layout->addWidget(new QLabel(i18n("Degree measure of exiting ball:"), this));
	QSpinBox *deg = new QSpinBox(0, 359, 10, this);
	deg->setValue(blackHole->curExitDeg());
	deg->setWrapping(true);
	layout->addWidget(deg);
	connect(deg, SIGNAL(valueChanged(int)), this, SLOT(degChanged(int)));

	layout->addStretch();

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Minimum exit speed"), this));
	QSpinBox *min = new QSpinBox(0, 12, 1, this);
	hlayout->addWidget(min);
	connect(min, SIGNAL(valueChanged(int)), this, SLOT(minChanged(int)));
	min->setValue(blackHole->minSpeed());

	hlayout = new QHBoxLayout(layout, spacingHint());
	hlayout->addWidget(new QLabel(i18n("Maximum"), this));
	QSpinBox *max = new QSpinBox(1, 12, 1, this);
	hlayout->addWidget(max);
	connect(max, SIGNAL(valueChanged(int)), this, SLOT(maxChanged(int)));
	max->setValue(blackHole->maxSpeed());
}

void BlackHoleConfig::degChanged(int newdeg)
{
	blackHole->setExitDeg(newdeg);
	changed();
}

void BlackHoleConfig::minChanged(int news)
{
	blackHole->setMinSpeed(news);
	changed();
}

void BlackHoleConfig::maxChanged(int news)
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
	this->alwaysShow = false;
	this->editing = false;
	this->visible = true;
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

void WallPoint::moveBy(double dx, double dy)
{
	//kdDebug() << "WallPoint::moveBy(" << dx << ", " << dy << ")\n";
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
	//kdDebug() << "WallPoint::updateVisible\n";
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
	//kdDebug() << "WallPoint::editModeChanged " << changed << endl;
	editing = changed;
	setVisible(true);
	if (!editing)
		updateVisible();
}

void WallPoint::collision(Ball *ball, long int id)
{
	//kdDebug() << "WallPoint::collision\n";
	
	//kdDebug() << "lastId is " << lastId << ", this id is " << id << endl;
	if (abs(id - lastId) < 2)
	{
		//kdDebug() << "collisionIds too similiar\n";
		lastId = id;
		return;
	}

	playSound("wall");

	const QPoint start = wall->startPoint();
	const QPoint end = wall->endPoint();

	const double wallSlope = (double)(-(start.x() - end.x()))/(double)(end.y() - start.y());
	double vx = ball->xVelocity();
	double vy = ball->yVelocity();
	const double wallAngle = atan(wallSlope);
	const double ballSlope = -(double)vy/(double)vx;
	double ballAngle = atan(ballSlope);
	if (vx < 0)  
		ballAngle += PI;

	//kdDebug() << "----\nstart: " << this->start << endl;
	//kdDebug() << "ballAngle: " << rad2deg(ballAngle) << endl;
	//kdDebug() << "wallAngle: " << rad2deg(wallAngle) << endl;

	// visible just means if we should bounce opposite way
	bool weirdbounce = visible;

	double relWallAngle = wallAngle + PI / 2;

	// wierd neg. slope angle
	if (relWallAngle > PI / 2)
	{
		//kdDebug() << "neg slope\n";
		relWallAngle -= PI / 2;
		relWallAngle = PI / 2 - relWallAngle;
		relWallAngle *= -1;
	}

	//kdDebug() << "relWallAngle: " << rad2deg(relWallAngle) << endl;

	// forget that we are using english, i switched
	// start and end i think
	// but it works
	bool isStart = this->start;
	if (start.x() < end.x())
		isStart = !isStart;

	//const double angle = PI / 3;
	const double angle = PI / 2;

	// if it's going 'backwards', don't bounce opposite way
	if (isStart)
	{
		//kdDebug() << "isStart true\n";
		if (ballAngle > relWallAngle - angle && ballAngle < relWallAngle + angle)
			weirdbounce = false;
	}
	else
	{
		//kdDebug() << "isStart false\n";
		if (ballAngle > relWallAngle + angle || ballAngle < relWallAngle - angle)
			weirdbounce = false;
	}

	//kdDebug() << "weirdbounce: " << weirdbounce << endl;

	if (weirdbounce)
	{
		//kdDebug() << "weird col\n";
		lastId = id;

		const double dampening = wall->dampening();

		if (start.y() == end.y())
		{
			//kdDebug() << "horizontal\n";
			vx *= -1;
			vy /= dampening;
			vx /= dampening;
		}
		else if (start.x() == end.x())
		{
			//kdDebug() << "vertical\n";
			vy *= -1;
			vy /= dampening;
			vx /= dampening;
		}
		else
		{
			const double speed = ball->curSpeed() / dampening;

			const double collisionAngle = ballAngle - wallAngle;
			const double leavingAngle = PI - collisionAngle + wallAngle;

			vx = -cos(leavingAngle)*speed;
			vy = sin(leavingAngle)*speed;
		}

		//kdDebug() << "new velocities: vx = " << vx << ", vy = " << vy << endl;
		//kdDebug() << "--------------\n";
		ball->setVelocity(vx, vy);
	}
	else
	{
		//kdDebug() << "passing on to wall\n";
		wall->collision(ball, id);
	}
}

/////////////////////////

Wall::Wall(QCanvas *canvas)
	: QCanvasLine(canvas)
{
	editing = false;
	
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
	move(0, 0);

	moveBy(0, 0);

	editModeChanged(false);
}

void Wall::finalLoad()
{
	startItem->updateVisible();
	endItem->updateVisible();
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

QPtrList<QCanvasItem> Wall::moveableItems()
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
	//kdDebug() << "Wall::editModeChanged\n";
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

	//kdDebug() << "neww = " << neww << endl;

	startItem->setSize(neww, neww);
	endItem->setSize(neww, neww);

	moveBy(0, 0);
}

void Wall::collision(Ball *ball, long int id)
{
	//kdDebug() << "lastId is " << lastId << ", this id is " << id << endl;
	if (abs(id - lastId) < 2)
	{
		//kdDebug() << "collisionIds too similiar\n";
		lastId = id;
		return;
	}

	playSound("wall");

	const QPoint start = startPoint();
	const QPoint end = endPoint();

	lastId = id;

	double vx = ball->xVelocity();
	double vy = ball->yVelocity();
	double _dampening = dampening();

	if (start.y() == end.y())
		// horizontal
	{
		//kdDebug() << "horizontal\n";
		vy *= -1;
		vy /= _dampening;
		vx /= _dampening;
	}
	else if (start.x() == end.x())
		// vertical
	{
		//kdDebug() << "vertical\n";
		vx *= -1;
		vy /= _dampening;
		vx /= _dampening;
	}
	else
	{
		const double speed = (double)sqrt(vx * vx + vy * vy) / _dampening;
		const double ballSlope = -(double)vy / (double)vx;
		const double wallSlope = (double)(start.y() - end.y()) / (double)(end.x() - start.x());
		double ballAngle = atan(ballSlope);
		if (vx < 0)  
			ballAngle += PI;
		const double wallAngle = atan(wallSlope);

		const double collisionAngle = ballAngle - wallAngle;
		const double leavingAngle = PI - collisionAngle + wallAngle;
		
		vx = -cos(leavingAngle)*speed;
		vy = sin(leavingAngle)*speed;
	}
				   
	//kdDebug() << "new velocities: vx = " << vx << ", vy = " << vy << endl;
	//kdDebug() << "--------------\n";
	ball->setVelocity(vx, vy);
}

void Wall::load(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "wall", x(), y()));

	QPoint start(startPoint());
	start = cfg->readPointEntry("startPoint", &start);
	QPoint end(endPoint());
	end = cfg->readPointEntry("endPoint", &end);

	setPoints(start.x(), start.y(), end.x(), end.y());

	moveBy(0, 0);
	startItem->move(start.x(), start.y());
	endItem->move(end.x(), end.y());
}

void Wall::save(KSimpleConfig *cfg, int hole)
{
	cfg->setGroup(makeGroup(hole, "wall", x(), y()));

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
	KLineEdit *nameEdit = new KLineEdit(holeInfo->name(), this);
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
	QSpinBox *par = new QSpinBox(2, 15, 1, this);
	par->setValue(holeInfo->par());
	hlayout->addWidget(par);
	connect(par, SIGNAL(valueChanged(int)), this, SLOT(parChanged(int)));
	hlayout->addStretch();

	hlayout->addWidget(new QLabel(i18n("Max. strokes"), this));
	QSpinBox *maxstrokes = new QSpinBox(4, 30, 1, this);
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

KolfGame::KolfGame(PlayerList *players, QString filename, QWidget *parent, const char *name )
	: QCanvasView(parent, name)
{
	// for mouse control
	QScrollView::viewport()->setMouseTracking(true);

	curHole = 0; // will get ++'d 
	cfg = 0;
	setFilename(filename);
	this->players = players;
	curPlayer = players->end();
	curPlayer--; // will get ++'d to end and sent back
	             // to beginning 
	modified = false;
	inPlay = false;
	putting = false;
	stroking = false;
	editing = false;
	m_serverRunning = false;
	m_soundError = true;
	soundDir = locate("appdata", "sounds/");
	addingNewHole = false;
	scoreboardHoles = 0;
	infoShown = false;
	m_useMouse = true;
	highestHole = 0;

	holeInfo.setGame(this);
	holeInfo.setAuthor(i18n("Course Author"));
	holeInfo.setName(i18n("Course Name"));
	holeInfo.setMaxStrokes(10);
	holeInfo.borderWallsChanged(true);

	initSoundServer();

	width = 400;
	height = 400;
	grass = QColor("#35760D");

	setFixedSize(width + 5, height + 5);
	setFocusPolicy(QWidget::StrongFocus);

	course = new QCanvas(this);
	course->resize(width, height);
	course->setBackgroundColor(grass);
	setCanvas(course);
	move(0, 0);
	adjustSize();

	obj.setAutoDelete(true);
	obj.append(new CupObj());
	obj.append(new SlopeObj());
	obj.append(new WallObj());
	obj.append(new PuddleObj());
	obj.append(new SandObj());
	obj.append(new WindmillObj());
	obj.append(new BlackHoleObj());
	obj.append(new FloaterObj());
	obj.append(new BridgeObj());
	obj.append(new SignObj());

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

	// whiteBall marks the spot of the whole whilst editing
	whiteBall = new Ball(course);
	whiteBall->setColor(white);
	whiteBall->setVisible(false);

	putter = new Putter(course);

	// border walls:
	//{
		int margin = 10;
		// horiz
		addBorderWall(QPoint(margin, margin), QPoint(width - margin, margin));
		addBorderWall(QPoint(margin, height - margin - 1), QPoint(width - margin, height - margin - 1));

		// vert
		addBorderWall(QPoint(margin, margin), QPoint(margin, height - margin));
		addBorderWall(QPoint(width - margin - 1, margin), QPoint(width - margin - 1, height - margin));
	//}

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timerMsec = 200;

	fastTimer = new QTimer(this);
	connect(fastTimer, SIGNAL(timeout()), this, SLOT(fastTimeout()));
	//fastTimerMsec = 11;
	fastTimerMsec = 11;

	frictionTimer = new QTimer(this);
	connect(frictionTimer, SIGNAL(timeout()), this, SLOT(frictionTimeout()));
	frictionTimerMsec = 22;

	autoSaveTimer = new QTimer(this);
	connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSaveTimeout()));
	autoSaveMsec = 5 * 1000 * 60; // 5 min autosave

	maxStrength = 55;
	putting = false;
	putterTimer = new QTimer(this);
	connect(putterTimer, SIGNAL(timeout()), this, SLOT(putterTimeout()));
	putterTimerMsec = 20;

	// this increments curHole, etc
	recalcHighestHole = true;
	holeDone();
	paused = true;
	unPause();
}

void KolfGame::setFilename(const QString &filename)
{
	this->filename = filename;
	delete cfg;
	cfg = new KSimpleConfig(filename);
}

KolfGame::~KolfGame()
{
	delete cfg;
}

void KolfGame::pause()
{
	if (paused == true)
	{
		// play along with people who call pause() again, instead of unPause()
		unPause();
		return;
	}

	paused = true;
	timer->stop();
	fastTimer->stop();
	frictionTimer->stop();
	putterTimer->stop();
}

void KolfGame::unPause()
{
	if (!paused)
		return;

	paused = false;

	timer->start(timerMsec);
	fastTimer->start(fastTimerMsec);
	frictionTimer->start(frictionTimerMsec);

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

void KolfGame::contentsMousePressEvent(QMouseEvent *e)
{
	if (inPlay)
		return;

	if (!editing)
	{
		if (m_useMouse)
		{
			if (e->button() == LeftButton)
				puttPress();
			else if (e->button() == RightButton)
				showInfoPress();
		}
		return;
	}

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
		//kdDebug() << "returning\n";
		emit newSelectedItem(&holeInfo);
		return;
	}
	// only items we keep track of
	if ((!(items.containsRef(list.first()) || list.first() == whiteBall || extraMoveable.containsRef(list.first()))))
	{
		//kdDebug() << "returning\n";
		emit newSelectedItem(&holeInfo);
		return;
	}

	CanvasItem *citem = dynamic_cast<CanvasItem *>(list.first());
	if (!citem || !citem->moveable())
	{
		//kdDebug() << "returning\n";
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

	setFocus();
}

void KolfGame::contentsMouseMoveEvent(QMouseEvent *e)
{
	if (inPlay || !putter)
		return;

	QPoint mouse = e->pos();

	// mouse moving of putter
	if (!editing)
	{
		updateMouse();
		return;
	}

	//kdDebug() << "moving: " << moving << endl;
	if (!moving)
		return;

	int moveX = storedMousePos.x() - mouse.x();
	int moveY = storedMousePos.y() - mouse.y();

	// moving counts as modifying
	if (moveX || moveY)
		modified = true;

	movingItem->moveBy(-(double)moveX, -(double)moveY);
	highlighter->moveBy(-(double)moveX, -(double)moveY);
	storedMousePos = mouse;
}

void KolfGame::updateMouse()
{
	if (!m_useMouse)
		return;

	double newAngle = 0;
	QPoint mouse(viewportToContents(mapFromGlobal(QCursor::pos())));
	const double yDiff = (double)(putter->y() - mouse.y());
	const double xDiff = (double)(mouse.x() - putter->x());
	//kdDebug() << "yDiff: " << yDiff << ", xDiff: " << xDiff << endl;
	if (yDiff == 0)
	{
		//kdDebug() << "horizontal\n";
		// horizontal
		if (putter->x() < mouse.x())
			newAngle = 0;
		else
			newAngle = PI;
	}
	else if (xDiff == 0)
	{
		//kdDebug() << "vertical\n";
		// vertical
		if (putter->y() < mouse.y())
			newAngle = (3 * PI) / 2;
		else
			newAngle = PI / 2;
	}
	else
	{
		const double slope = yDiff / xDiff;
		const double angle = atan(slope);
		//kdDebug() << "angle: " << rad2deg(angle) << endl;
		newAngle = angle;

		if (mouse.x() < putter->x())
			newAngle += PI;
	}

	//kdDebug() << "newAngle: " << rad2deg(newAngle) << endl;

	const int degrees = (int)rad2deg(newAngle);
	//kdDebug() << "degrees: " << degrees << endl;
	putter->setDeg(degrees);
}

void KolfGame::contentsMouseReleaseEvent(QMouseEvent *e)
{
	moving = false;

	if (inPlay)
		return;

	if (!editing)
	{
		if (m_useMouse)
		{
			if (e->button() == LeftButton)
				puttRelease();
			else if (e->button() == RightButton)
				showInfoRelease();
		}
	}

	setFocus();
}

void KolfGame::keyPressEvent(QKeyEvent *e)
{
	if (inPlay || editing)
	{
		//kdDebug() << "inPlay or editing so i'm returning\n";
		return;
	}
	switch (e->key())
	{
		case Key_I: case Key_Up:
			if (!e->isAutoRepeat())
				showInfoPress();
		break;

		case Key_Escape:
			putting = false;
			stroking = false;
			putterTimer->stop();
			putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		break;

		case Key_Left:
			putter->go(D_Left, e->state() & ShiftButton);
		break;

		case Key_Right:
			putter->go(D_Right, e->state() & ShiftButton);
		break;

		case Key_Space: case Key_Down:
			puttPress();
		break;

		default:
		break;
	}
}

void KolfGame::showInfoPress()
{
	QCanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->showInfo();
	}
	showInfo();
	putter->showInfo();
}

void KolfGame::showInfoRelease()
{
	QCanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->hideInfo();
	}
	hideInfoText();
	putter->hideInfo();
}

void KolfGame::puttPress()
{
	if (!putting && !stroking && !inPlay)
	{
		puttCount = 0;
		putting = true;
		stroking = false;
		strength = 0;
		putterTimer->start(putterTimerMsec);
	}
}

void KolfGame::keyReleaseEvent(QKeyEvent *e)
{
	//kdDebug() << "keyrelease event\n";
	if (e->isAutoRepeat())
		return;

	//kdDebug() << "mada koko ni aru\n";
	if (e->key() == Key_Space || e->key() == Key_Down)
		puttRelease();
	else if ((e->key() == Key_Backspace || e->key() == Key_Delete) && !(e->state() & ControlButton))
	{
		//kdDebug() << "a delete key\n";
		if (editing && !moving && selectedItem)
		{
			CanvasItem *citem = dynamic_cast<CanvasItem *>(selectedItem);
			if (citem)
			{
				//kdDebug() << "citem is true\n";
				if (citem->deleteable())
				{
					//kdDebug() << "deleteable\n";
					highlighter->setVisible(false);
					items.remove(selectedItem);
					citem->aboutToDelete();
					citem->aboutToDie();
					delete selectedItem;
					selectedItem = 0;
					emit newSelectedItem(&holeInfo);

					modified = true;
				}
			}
		}
	}
	else if (e->key() == Key_I || e->key() == Key_Up)
		showInfoRelease();
}

void KolfGame::puttRelease()
{
	if (putting && !editing)
	{
		putting = false;
		stroking = true;
	}
}

void KolfGame::timeout()
{
	Ball *curBall = (*curPlayer).ball();

	int curState = curBall->curState();
	if (curState == Stopped && inPlay)
	{
		inPlay = false;

		emit inPlayEnd();

		QTimer::singleShot(500, this, SLOT(shotDone()));
	}

	if (curState == Holed && inPlay)
	{
		emit inPlayEnd();
		emit playerHoled(&(*curPlayer));

		int curScore = (*curPlayer).score(curHole) + 1;
		if (curScore == 1)
			playSound("holeinone");
		else if (curScore <= holeInfo.par())
			playSound("woohoo");

		(*curPlayer).ball()->setZ((*curPlayer).ball()->z() + .1 - (.1)/(curScore));
		//kdDebug() << "z now is " << (*curPlayer).ball()->z()<< endl;

		if (allPlayersDone())
		{
			for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
				(*it).ball()->setVisible(false);
			inPlay = false;

			if (curHole > 0)
			{
				(*curPlayer).addStrokeToHole(curHole);
				emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));
			}
			QTimer::singleShot(500, this, SLOT(holeDone()));
		}
		else
		{
			inPlay = false;
			QTimer::singleShot(500, this, SLOT(shotDone()));
		}
	}
}

void KolfGame::fastTimeout()
{
	if (inPlay && !editing)
		(*curPlayer).ball()->collisionDetect();

	//course->advance();
	(*curPlayer).ball()->doAdvance();

	if (!inPlay)
	{
		if ((*curPlayer).ball()->moved())
		{
			if (putter->isVisible())
			{
				putter->move((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
				(*curPlayer).ball()->setMoved(false);
			}
		}
	}
}

void KolfGame::frictionTimeout()
{
	if (inPlay && !editing)
		(*curPlayer).ball()->friction();
	course->advance();
}

void KolfGame::putterTimeout()
{
	if (inPlay || editing)
		return;

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

void KolfGame::autoSaveTimeout()
{
	if (editing)
		save();
}

void KolfGame::shotDone()
{
	setFocus();

	//kdDebug() << "curHole is " << curHole << endl;

	Ball *ball = (*curPlayer).ball();

	if ((*curPlayer).numHoles())
		(*curPlayer).addStrokeToHole(curHole);

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

	/*
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
	{
		if ((*it).ball()->blowUp())
		{
			(*it).addStrokeToHole(curHole);
			emit scoreChanged((*it).id(), curHole, (*it).score(curHole));
		}
		(*it).ball()->setBlowUp(false);
	}
	*/

	double vx = 0, vy = 0;
	if (ball->placeOnGround(vx, vy))
	{
		double x = ball->x(), y = ball->y();

		double ballAngle = atan(vx / vy);
		//kdDebug() << "ballAngle is " << ballAngle << endl;
		if (vy < 0)
			ballAngle -= PI;
		ballAngle = PI/2 - ballAngle;
		
		//kdDebug() << "ballAngle is " << ballAngle << endl;
		while (1)
		{
			QCanvasItemList list = ball->collisions(true);
			bool keepMoving = false;
			while (!list.isEmpty())
			{
				QCanvasItem *item = list.first();
				if (item->rtti() == Rtti_DontPlaceOn)
				{
					//kdDebug() << "still water\n";
					keepMoving = true;
					break;
				}

				list.pop_front();
			}
			if (!keepMoving)
				break;

			x -= cos(ballAngle) * 4;
			y -= sin(ballAngle) * 4;

			ball->move(x, y);

			// for debugging
			/*
			ball->setVisible(true);
			qapp->processEvents();
			//kdDebug() << "(" << x << ", " << y << ")" << endl;
			sleep(1);
			*/
		}

		//kdDebug() << "placing on " << p.x() << ", " << p.y() << endl;
		ball->move(ball->x(), ball->y());
	}

	// off by default
	ball->setPlaceOnGround(false, 0, 0);
	// end hacky stuff
	
	// emit again
	emit scoreChanged((*curPlayer).id(), curHole, (*curPlayer).score(curHole));
	
	ball->setVelocity(0, 0);

	int curStrokes = (*curPlayer).score(curHole);
	//kdDebug() << "curStrokes is " << curStrokes << endl;
	if (curStrokes >= holeInfo.maxStrokes())
	{
		emit maxStrokesReached();
		ball->setState(Holed);

		if (allPlayersDone())
		{
			holeDone();
			return;
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

	putter->setDegrees((*curPlayer).ball());
	putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
	updateMouse();

	inPlay = false;
	(*curPlayer).ball()->setVelocity(0, 0);
}

void KolfGame::shotStart()
{
	emit inPlayStart();

	putter->saveDegrees((*curPlayer).ball());
	strength /= 8;
	putter->setVisible(false);
	int deg = putter->curDeg();
	//kdDebug() << "deg is " << deg << endl;
	double vx = 0, vy = 0;

	vx = -cos(deg2rad(deg))*strength;
	vy = sin(deg2rad(deg))*strength;

	//kdDebug() << "calculated new speed is " << sqrt(vx * vx + vy * vy) << endl;

	(*curPlayer).ball()->setVelocity(vx, vy);
	(*curPlayer).ball()->setState(Rolling);

	inPlay = true;
}

void KolfGame::holeDone()
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

	//kdDebug() << "KolfGame::holeDone()\n";

	inPlay = false;
	timer->stop();
	putter->resetDegrees();

	int oldCurHole = curHole;
	curHole++;
	//kdDebug() << "curHole ++'d to " << curHole << endl;

	if (reset)
	{
		whiteBall->move(width/2, height/2);
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
			//kdDebug() << "lastScore: " << (*it).lastScore() << ", leastScore: " << leastScore << endl;
			if ((*it).lastScore() < leastScore && (*it).lastScore() != 0)
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
		(*it).addHole();
		(*it).ball()->setVelocity(0, 0);
		(*it).ball()->setMoved(false);
		(*it).ball()->setVisible(false);
	}

	emit newPlayersTurn(&(*curPlayer));
	//kdDebug() << (*curPlayer).name() << endl;
	//kdDebug() << (*curPlayer).ball()->color().name() << endl;
	
	if (reset)
		openFile();

	inPlay = false;
	timer->start(timerMsec);

	if (oldCurHole != curHole)
	{
		// here we have to make sure the scoreboard shows
		// all of the holes up until now;
		//KSimpleConfig cfg(filename);

		//kdDebug() << "curHole is " << curHole << endl;

		for (; scoreboardHoles < curHole; ++scoreboardHoles)
		{
			//kdDebug() << "scoreboardHoles is " << scoreboardHoles << endl;
			cfg->setGroup(QString("%1-hole@-50,-50|0").arg(scoreboardHoles + 1));
			emit newHole(cfg->readNumEntry("par", 3));
		}

		resetHoleScores();

		(*curPlayer).ball()->setVisible(true);
		putter->setOrigin((*curPlayer).ball()->x(), (*curPlayer).ball()->y());
		updateMouse();
	}
	// else we're done
}

void KolfGame::showInfo()
{
	QString text = i18n("Hole %1: par %1, maximum number of strokes is %2.").arg(curHole).arg(holeInfo.par()).arg(holeInfo.maxStrokes());
	infoText->move((width - QFontMetrics(infoText->font()).width(text)) / 2, infoText->y());
	infoText->setText(text);
	infoText->setVisible(true);
}

void KolfGame::showInfoDlg(bool addDontShowAgain)
{
	//kdDebug() << "show info dlg\n";
	KMessageBox::information(parentWidget(), i18n("Course name: %1").arg(holeInfo.name()) + QString("\n") + i18n("Created by %1").arg(holeInfo.author()), i18n("Course Information"), addDontShowAgain? holeInfo.name() + QString(" ") + holeInfo.author() : QString::null);
}

void KolfGame::hideInfoText()
{
	infoText->setText("");
	infoText->setVisible(false);
}

void KolfGame::openFile()
{
	//kdDebug() << "KolfGame::openFile(), hole is " << curHole << endl;
	pause();

	Object *curObj = 0;

	QPtrList<QCanvasItem> olditems(items);
	items.setAutoDelete(false);
	items.clear();
	extraMoveable.setAutoDelete(false);
	extraMoveable.clear();

	//KSimpleConfig cfg(filename);

	QStringList groups = cfg->groupList();

	int numItems = 0;
	curPar = 3;
	holeInfo.setPar(curPar);
	// if you want the default max strokes to always be 10....
	//holeInfo.setMaxStrokes(10);
	int _highestHole = 0;

	/****
	TODO : add a progress bar based on groups.size()
	oh.. but... that's always the same for any size hole
	bah humbug, how to tell?!
	****/

	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
	{
		//kdDebug() << "GROUP: " << *it << endl;
		// [<holeNum>-<name>@<x>,<y>|<id>]
		
		cfg->setGroup(*it);

		int len = (*it).length();
		int dashIndex = (*it).find("-");
		int holeNum = (*it).left(dashIndex).toInt();
		if (holeNum > _highestHole)
			_highestHole = holeNum;

		int atIndex = (*it).find("@");
		QString name = (*it).mid(dashIndex + 1, atIndex - (dashIndex + 1));
		
		//kdDebug() << "name is " << name << endl;

		// will tell basic course info
		if (name == "course")
		{
			holeInfo.setAuthor(cfg->readEntry("author", holeInfo.author()));
			holeInfo.setName(cfg->readEntry("name", holeInfo.name()));
			holeInfo.borderWallsChanged(cfg->readBoolEntry("borderWalls", holeInfo.borderWalls()));
			continue;
		}

		if (holeNum != curHole)
		{
			// if we've had one, break, cause list is sorted
			// erps, no, cause we need to know highest hole!
			if (numItems && !recalcHighestHole)
					break;
			continue;
		}
		numItems++;
	

		int commaIndex = (*it).find(",");
		int pipeIndex = (*it).find("|");
		int x = (*it).mid(atIndex + 1, commaIndex - (atIndex + 1)).toInt();
		int y = (*it).mid(commaIndex + 1, pipeIndex - (commaIndex + 1)).toInt();

		// will tell where ball is
		if (name == "ball")
		{
			for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
				(*it).ball()->move(x, y);
			whiteBall->move(x, y);
			continue;
		}
		else
		// stores par
		if (name == "hole")
		{
			curPar = cfg->readNumEntry("par", curPar);
			holeInfo.setPar(curPar);
			holeInfo.setMaxStrokes(cfg->readNumEntry("maxstrokes", holeInfo.maxStrokes()));
			continue;
		}

		int id = (*it).right(len - (pipeIndex + 1)).toInt();

		//kdDebug() << "after parsing key: hole = " << holeNum << ", name = " << name << ", x = " << x << ", y = " << y << ", id = " << id << endl;

		for (curObj = obj.first(); curObj; curObj = obj.next())
		{
			if (name != curObj->_name())
				continue;

			//kdDebug() << "Object: " << curObj->_name() << endl;

			QCanvasItem *newItem = curObj->newObject(course);
			//kdDebug() << "item created\n";
			items.append(newItem);
			newItem->setVisible(true);

			CanvasItem *canvasItem = dynamic_cast<CanvasItem *>(newItem);
			if (!canvasItem)
				continue;
			canvasItem->setId(id);
			canvasItem->setGame(this);
			canvasItem->editModeChanged(editing);
			addItemsToMoveableList(canvasItem->moveableItems());

			newItem->move(x, y);
			canvasItem->firstMove(x, y);

			// make things actually show
			// kapp->processEvents();
			// turns out that that fscks everything up
			
			// we don't allow multiple items for the same thing in
			// the file!
			break;
		}
	}

	// if it's the first hole let's not
	if (!numItems && curHole > 1 && !addingNewHole && !(curHole < _highestHole))
	{
		// we're done, let's quit
		curHole--;
		pause();
		emit holesDone();
		return;
	}

	QCanvasItem *item = 0;
	for (item = olditems.first(); item; item = olditems.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->aboutToDie();
	}

	olditems.setAutoDelete(true);
	olditems.clear();

	QCanvasItem *qcanvasItem = 0;
	QPtrList<CanvasItem> todo;
	for (qcanvasItem = items.first(); qcanvasItem; qcanvasItem = items.next())
	{
		CanvasItem *item = dynamic_cast<CanvasItem *>(qcanvasItem);
		if (item)
		{
			if (item->loadLast())
				todo.append(item);
			else
				item->load(cfg, curHole);
		}
	}
	CanvasItem *citem = 0;
	for (citem = todo.first(); citem; citem = todo.next())
		citem->load(cfg, curHole);
	for (qcanvasItem = items.first(); qcanvasItem; qcanvasItem = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(qcanvasItem);
		if (citem)
			citem->updateZ();
	}

	if (curHole == 1 && !filename.isNull() && !infoShown)
	{
		showInfoDlg(true);
		infoShown = true;
	}

	if (curHole > highestHole)
		highestHole = curHole;

	//kdDebug() << "openfile finishing; highestHole is " << highestHole << endl;

	if (recalcHighestHole)
	{
		highestHole = _highestHole;
		recalcHighestHole = false;
	}
	unPause();
}

void KolfGame::addItemsToMoveableList(QPtrList<QCanvasItem> list)
{
	QCanvasItem *item = 0;
	for (item = list.first(); item; item = list.next())
		extraMoveable.append(item);
}

void KolfGame::addNewObject(Object *newObj)
{
	QCanvasItem *newItem = newObj->newObject(course);
	items.append(newItem);
	newItem->setVisible(true);

	CanvasItem *canvasItem = dynamic_cast<CanvasItem *>(newItem);
	if (!canvasItem)
		return;
	canvasItem->setId(items.count() + 2);
	canvasItem->setGame(this);
	canvasItem->editModeChanged(editing);
	addItemsToMoveableList(canvasItem->moveableItems());

	newItem->move(width/2, height/2);
}

bool KolfGame::askSave(bool noMoreChances)
{
	if (!modified)
		// not cancel, don't save
		return false;

	int result = KMessageBox::warningYesNoCancel(this, i18n("There are unsaved changes to current hole. Save them?"), i18n("Unsaved changes"), i18n("&Save"), noMoreChances? i18n("&Discard") : i18n("Save &Later"), noMoreChances? "DiscardAsk" : "SaveAsk", true);
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

	//KSimpleConfig cfg(filename);
	QStringList groups = cfg->groupList();

	addingNewHole = true;
	curHole = highestHole;
	//kdDebug() << "highestHole is " << highestHole << endl;
	recalcHighestHole = true;
	holeDone();
	emit largestHole(curHole);
	addingNewHole = false;

	// make sure even the current player isn't showing
	for (PlayerList::Iterator it = players->begin(); it != players->end(); ++it)
		(*it).ball()->setVisible(false);

	whiteBall->setVisible(editing);
	highlighter->setVisible(false);
	putter->setVisible(!editing);
	inPlay = false;

	// obviously, we've modified this course by adding a new hole!
	modified = true;
}

// kantan deshou ;-)
void KolfGame::resetHole()
{
	if (askSave(true))
		return;
	modified = false;
	curHole--;
	holeDone();
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
	//kdDebug() << "clearHole()\n";
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

	modified = true;
}

void KolfGame::switchHole(int hole)
{
	//kdDebug() << "switchHole\n";
	if (editing || inPlay)
		return;
	if (hole < 1 || hole > highestHole)
		return;

	curHole = hole;
	resetHole();
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
	//kdDebug() << "randHole\n";
	int newHole = 1 + (int)((double)kapp->random() * ((double)(highestHole - 1) / (double)RAND_MAX));
	//kdDebug() << "newHole: " << newHole << endl;
	switchHole(newHole);
}

void KolfGame::save()
{
	if (filename.isNull())
	{
		QString newfilename = KFileDialog::getSaveFileName(QString::null, "*.kolf", this, i18n("Pick Kolf Course to Save To"));
		if (newfilename.isNull())
			return;

		filename = newfilename;
	}

	QCanvasItem *item = 0;
	for (item = items.first(); item; item = items.next())
	{
		CanvasItem *citem = dynamic_cast<CanvasItem *>(item);
		if (citem)
			citem->aboutToSave();
	}

	//KSimpleConfig cfg(filename);
	//kdDebug() << "KolfGame::save " << filename << "\n";
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
			citem->save(cfg, curHole);
	}

	// save where ball starts (whiteBall tells all)
	cfg->setGroup(QString("%1-ball@%2,%3").arg(curHole).arg((int)whiteBall->x()).arg((int)whiteBall->y()));
	cfg->writeEntry("dummykey", true);

	cfg->setGroup("0-course@-50,-50");
	cfg->writeEntry("author", holeInfo.author());
	cfg->writeEntry("name", holeInfo.name());
	cfg->writeEntry("borderWalls", holeInfo.borderWalls());

	// save hole info
	cfg->setGroup(QString("%1-hole@-50,-50|0").arg(curHole));
	cfg->writeEntry("par", holeInfo.par());
	cfg->writeEntry("maxstrokes", holeInfo.maxStrokes());

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
	if (editing && modified)
	{
		if (askSave(false))
		{
			emit checkEditing();
			return;
		}
	}

	moving = false;

	//kdDebug() << "toggling\n";
	editing = !editing;

	if (editing)
	{
		emit editingStarted();
		emit newSelectedItem(&holeInfo);
	}
	else
		emit editingEnded();

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

// this is essentially out of kdegames/kbattleship
void KolfGame::initSoundServer()
{
	soundserver = Arts::Reference("global:Arts_SimpleSoundServer");
	playObjectFactory = Arts::Reference("global:Arts_PlayObjectFactory");
	if(soundserver.isNull())
	{
		KMessageBox::error(this, i18n("Couldn't connect to aRts Soundserver. Sound deactivated."));
		playObjectFactory = Arts::PlayObjectFactory::null();
		soundserver = Arts::SimpleSoundServer::null();
		m_serverRunning = false;
		m_soundError = true;
	}
	else
	{
		QString soundDirCheck = locate("appdata", "sounds/");
		QString oneSoundCheck = locate("appdata", "sounds/wall.wav");
		if(!soundDirCheck.isEmpty() && !oneSoundCheck.isEmpty())
		{
			m_serverRunning = true;
			m_soundError = false;
		}
		else
		{
			KMessageBox::error(this, i18n("You don't have Kolf sounds installed. Sound deactivated."));
			playObjectFactory = Arts::PlayObjectFactory::null();
			soundserver = Arts::SimpleSoundServer::null();
			m_serverRunning = false;
			m_soundError = true;
		}
	}
}

void KolfGame::playSound(QString file)
{
	if (m_serverRunning && !m_soundError)
	{
		QString playFile = soundDir + file + QString::fromLatin1(".wav");

		playObject = playObjectFactory.createPlayObject(playFile.latin1());
		if(!playObject.isNull())
			playObject.play();
	}
}

void CanvasItem::playSound(QString file)
{
	if (game)
		game->playSound(file);
}

void HoleInfo::borderWallsChanged(bool yes)
{
	m_borderWalls = yes;
	game->setBorderWalls(yes);
}

void KolfGame::print(QPainter &p)
{
	// this is pretty ugly/broken
	// but it's better than nothing.
	QString text("%1 - Hole %2; by %3");
	text = text.arg(holeInfo.name()).arg(curHole).arg(holeInfo.author());
	p.drawText(0, 20, text);
	const double scale = .7;
	p.scale(scale, scale);
	p.setWindow(QRect(20, -45, width, height));
	QRect r(0, 0, width, height);

	course->setBackgroundColor(white);
	course->drawArea(r, &p);
	course->setBackgroundColor(grass);
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

#include "game.moc"
