#include <qbitmap.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qimage.h>
#include <qpixmapcache.h>
#include <qwhatsthis.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <knuminput.h>
#include <kpixmapeffect.h>
#include <kstandarddirs.h>

#include "slope.h"

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
	// we are a terrain collision
	return true;
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
		return false;

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

	// do NOT do terrain collisions
	return false;
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
	hlayout->addWidget(new QLabel(i18n("Grade:"), this));
	KDoubleNumInput *grade = new KDoubleNumInput(this);
	grade->setRange(0, 8, 1, true);
	grade->setValue(slope->curGrade());
	hlayout->addWidget(grade);
	connect(grade, SIGNAL(valueChanged(double)), this, SLOT(gradeChanged(double)));

	QCheckBox *stuck = new QCheckBox(i18n("Unmovable"), this);
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

#include "slope.moc"
