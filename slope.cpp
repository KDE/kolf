/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

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

#include "slope.h"

#include <qbitmap.h>
#include <QCheckBox>
#include <QLabel>
#include <qimage.h>
#include <qpixmapcache.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QApplication>

#include <kcombobox.h>
#include <kconfig.h>
#include <knuminput.h>
#include <kstandarddirs.h>

Slope::Slope(const QRect &rect, QGraphicsItem * parent, QGraphicsScene *scene)
	: QGraphicsRectItem(rect, parent, scene), type(Vertical), grade(4), reversed(false), color(QColor("#327501")) 
{
	setData(0, 1031);
	stuckOnGround = false;
	showingInfo = false;
	baseArrowPenThickness = arrowPenThickness = 1;

	gradientKeys[Vertical] = "Vertical";
	gradientKeys[Horizontal] = "Horizontal";
	gradientKeys[Diagonal] = "Diagonal";
	gradientKeys[CrossDiagonal] = "Opposite Diagonal";
	gradientKeys[Elliptic] = "Elliptic";

	gradientI18nKeys[Vertical] = i18n("Vertical");
	gradientI18nKeys[Horizontal] = i18n("Horizontal");
	gradientI18nKeys[Diagonal] = i18n("Diagonal");
	gradientI18nKeys[CrossDiagonal] = i18n("Opposite Diagonal");
	gradientI18nKeys[Elliptic] = i18n("Circular");

	setZValue(-50);

	point = new RectPoint(color.light(), this, parent, scene);

	QFont font(QApplication::font());
	baseFontPixelSize = 18;
	font.setPixelSize(baseFontPixelSize);
	text = new QGraphicsSimpleTextItem(0, scene);
	text->setZValue(99999.99);
	text->setFont(font);
	text->setBrush(Qt::white);

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
	QList<Arrow *>::const_iterator arrow;
	for (arrow = arrows.constBegin(); arrow != arrows.constEnd(); ++arrow)
	{
		(*arrow)->setZValue(zValue() + .01);
		(*arrow)->setVisible(true);
	}
	text->setVisible(true);
}

void Slope::hideInfo()
{
	showingInfo = false;
	QList<Arrow *>::const_iterator arrow;
	for (arrow = arrows.constBegin(); arrow != arrows.constEnd(); ++arrow)
		(*arrow)->setVisible(false);
	text->setVisible(false);
}

void Slope::resize(double resizeFactor)
{
	this->resizeFactor = resizeFactor;
	QFont font = text->font();
	font.setPixelSize((int)(baseFontPixelSize*resizeFactor));
	text->setFont(font);
	arrowPenThickness = baseArrowPenThickness*resizeFactor;
	setPos(baseX*resizeFactor, baseY*resizeFactor);
	setRect(0, 0, baseWidth*resizeFactor, baseHeight*resizeFactor);
	updatePixmap();
}

void Slope::firstMove(int x, int y)
{
	baseX = (double)x;
	baseY = (double)y;
}

void Slope::aboutToDie()
{
	delete point;
	clearArrows();
	delete text;
}

void Slope::clearArrows()
{
	QList<Arrow *>::const_iterator arrow;
	for (arrow = arrows.constBegin(); arrow != arrows.constEnd(); ++arrow)
	{
		(*arrow)->setVisible(false);
		(*arrow)->aboutToDie();
	}
        while (!arrows.isEmpty())
            delete arrows.takeFirst();
}

QList<QGraphicsItem *> Slope::moveableItems() const
{
	QList<QGraphicsItem *> ret;
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

void Slope::setSize(double width, double height)
{
	newSize(width, height);
}

void Slope::newSize(double width, double height)
{
	if (type == Elliptic)
	{
		QGraphicsRectItem::setRect(rect().x(), rect().y(), width, width);
		// move point back to good spot
		moveBy(0, 0);

		if (game && game->isEditing())
			game->updateHighlighter();
	}
	else
		QGraphicsRectItem::setRect(rect().x(), rect().y(), width, height);

	updatePixmap();
	updateZ();
}

void Slope::moveBy(double dx, double dy)
{
	QGraphicsRectItem::moveBy(dx, dy);

	point->dontMove();
	point->setPos(x() + width(), y() + height());

	moveArrow();
	updateZ();
}

void Slope::moveArrow()
{
	double xavg, yavg;

	if(type == Diagonal) {
		if(reversed) {
			xavg = boundingRect().width()*1/4 + x();
			yavg = boundingRect().height()*1/4 + y();
		}
		else {
			xavg = boundingRect().width()*3/4 + x();
			yavg = boundingRect().height()*3/4 + y();
		}
	}
	else if(type == CrossDiagonal) {
		if(reversed) {
			xavg = boundingRect().width()*3/4 + x();
			yavg = boundingRect().height()*1/4 + y();
		}
		else {
			xavg = boundingRect().width()*1/4 + x();
			yavg = boundingRect().height()*3/4 + y();
		}
	}
	else {
		xavg = boundingRect().width()/2 + x();
		yavg = boundingRect().height()/2 + y();
	}

	QList<Arrow *>::const_iterator arrow;
	for (arrow = arrows.constBegin(); arrow != arrows.constEnd(); ++arrow)
		(*arrow)->setPos(xavg, yavg);

	if (showingInfo)
		showInfo();
	else
		hideInfo();

	text->setPos(xavg - text->boundingRect().width() / 2, yavg - text->boundingRect().height() / 2);
}

void Slope::editModeChanged(bool changed)
{
	point->setVisible(changed);
	moveBy(0, 0);
}

void Slope::updateZ(QGraphicsRectItem *vStrut)
{
	const double area = (height() * width());
	const int defaultz = -50;

	double newZ = 0;

	QGraphicsRectItem *rect = 0;
	if (!stuckOnGround)
		rect = vStrut? vStrut : onVStrut();

	if (rect)
	{
		if (area > (rect->rect().width() * rect->rect().height()))
			newZ = defaultz;
		else
			newZ = rect->zValue();
	}
	else
		newZ = defaultz;

	setZValue(((double)1 / (area == 0? 1 : area)) + newZ);
}

void Slope::load(KConfigGroup *cfgGroup)
{
	stuckOnGround = cfgGroup->readEntry("stuckOnGround", stuckOnGround);
	grade = cfgGroup->readEntry("grade", grade);
	reversed = cfgGroup->readEntry("reversed", reversed);

	// bypass updatePixmap which newSize normally does
	QGraphicsRectItem::setRect(rect().x(), rect().y(), cfgGroup->readEntry("width", width()), cfgGroup->readEntry("height", height()));
	baseWidth = rect().width();
	baseHeight = rect().height();
	updateZ();

	QString gradientType = cfgGroup->readEntry("gradient", gradientKeys[type]);
	setGradient(gradientType);
}

void Slope::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("reversed", reversed);
	cfgGroup->writeEntry("width", width());
	cfgGroup->writeEntry("height", height());
	cfgGroup->writeEntry("gradient", gradientKeys[type]);
	cfgGroup->writeEntry("grade", grade);
	cfgGroup->writeEntry("stuckOnGround", stuckOnGround);
}

void Slope::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/ ) 
{
	painter->drawPixmap(0, 0, pixmap);  
}

QPainterPath Slope::shape() const
{       
	if(type == CrossDiagonal) {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect().x(), rect().y());
		polygon[1] = QPointF(rect().x() + width(), rect().y() + height());
		polygon[2] = reversed? QPointF(rect().x() + width(), rect().y()) : QPointF(rect().x(), rect().y() + height());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == Diagonal) {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect().x() + width(), rect().y());
		polygon[1] = QPointF(rect().x(), rect().y() + height());
		polygon[2] = !reversed? QPointF(rect().x() + width(), rect().y() + height()) : QPointF(rect().x(), rect().y());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == Elliptic) {
		QPainterPath path;
		path.addEllipse(rect().x(), rect().y(), width(), height());
		return path;
	}
	else {
		QPainterPath path;
		path.addRect(rect().x(), rect().y(), width(), height());
		return path;
	}
}

bool Slope::collision(Ball *ball, long int /*id*/)
{
	if (grade <= 0)
		return false;

	double vx = ball->getXVelocity();
	double vy = ball->getYVelocity();
	double addto = 0.013 * grade;

	const bool diag = type == Diagonal || type == CrossDiagonal;
	const bool circle = type == Elliptic;

	double slopeAngle = 0;

	if (diag) 
		slopeAngle = atan((double)width() / (double)height());
	else if (circle)
	{
		const QPointF start((x() + width() / 2.0), y() + height() / 2.0);
		const QPointF end(ball->x(), ball->y());

		Vector betweenVector(start, end);
		const double factor = betweenVector.magnitude() / ((double)width() / 2.0);
		slopeAngle = betweenVector.direction();

		// this little bit by Daniel
		addto *= factor * M_PI / 2;
		addto = sin(addto);
	}

	switch (type)
	{
		case Horizontal:
			reversed? vx += addto : vx -= addto;
		break;

		case Vertical:
			reversed? vy += addto : vy -= addto;
		break;

		case Diagonal:
		case Elliptic:
			reversed? vx += cos(slopeAngle) * addto : vx -= cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		case CrossDiagonal:
			reversed? vx -= cos(slopeAngle) * addto : vx += cos(slopeAngle) * addto;
			reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
		break;

		default:
		break;
	}

	ball->setVelocity(vx, vy);
	// check if the ball is at the center of a pit or mound
	// or has otherwise stopped.
	if (vx == 0 && vy ==0) 
		ball->setState(Stopped);
	else 
		ball->setState(Rolling);

	// do NOT do terrain collidingItems
	return false;
}

void Slope::setGradient(const QString &text)
{
	for (QMap<GradientType, QString>::Iterator it = gradientKeys.begin(); it != gradientKeys.end(); ++it)
	{
		if (it.value() == text)
		{
			setType(it.key());
			return;
		}
	}

	// extra forgiveness ;-) (note it's i18n keys)
	for (QMap<GradientType, QString>::Iterator it = gradientI18nKeys.begin(); it != gradientI18nKeys.end(); ++it)
	{
		if (it.value() == text)
		{
			setType(it.key());
			return;
		}
	}
}

void Slope::setType(GradientType type)
{
	this->type = type;

	if (type == Elliptic)
	{
		// calls updatePixmap
		newSize(width(), height());
	}
	else
		updatePixmap();
}

void Slope::updatePixmap() //this needs work so that the slope colour depends on angle again
{
	if(game == 0)
		return;

	QString slopeName;

	switch(type) {
		case Horizontal:
			if(reversed)
				slopeName = "slope_e";
			else
				slopeName = "slope_w";
			break;

		case Vertical:
			if(reversed)
				slopeName = "slope_s";
			else
				slopeName = "slope_n";
			break;

		case Diagonal:
			if(reversed)
				slopeName = "slope_se";
			else
				slopeName = "slope_nw";
			break;

		case CrossDiagonal:
			if(reversed)
				slopeName = "slope_sw";
			else
				slopeName = "slope_ne";
			break;
		case Elliptic:
			if(reversed)
				slopeName = "slope_dip";
			else
				slopeName = "slope_bump";
			break;
		default:
			break;
	}

	pixmap=game->renderer->renderSvg(slopeName, (int)width(), (int)height(), 0);

	// we update the arrows in this function
	clearArrows();

	const double length = sqrt(double(width() * width() + height() * height())) / 4;

	if (type == Elliptic)
	{
		double angle = 0;
		for (int i = 0; i < 4; ++i)
		{
			angle += M_PI / 2;
			Arrow *arrow = new Arrow(0, scene());
			arrow->setLength(length);
			arrow->setAngle(angle);
			arrow->setPen(QPen(Qt::black, arrowPenThickness));
			arrow->setReversed(reversed);
			arrow->updateSelf();
			arrows.append(arrow);
		}
	}
	else
	{
		Arrow *arrow = new Arrow(0, scene());
		double angle = 0;

		switch(type) {
			case Horizontal:
				angle = 0;
				break;
			case Vertical:
				angle = M_PI / 2;
				break;
			case Diagonal:
				angle = atan((double)width() / (double)height());
				break;
			case CrossDiagonal:
				angle = M_PI - atan((double)width() / (double)height());
				break;
			default:
				break;
		}

		if (!reversed)
			angle += M_PI;

		arrow->setAngle(angle);
		arrow->setLength(length);
		arrow->setPen(QPen(Qt::black, arrowPenThickness));
		arrow->updateSelf();

		arrows.append(arrow);
	}

	text->setText(QString::number(grade));

	moveArrow();
}

void Slope::updateBaseResizeInfo()
{
		baseX = x() / resizeFactor;
		baseY = y() / resizeFactor;
		baseWidth = width() / resizeFactor;
		baseHeight = height() / resizeFactor;
}

/////////////////////////

SlopeConfig::SlopeConfig(Slope *slope, QWidget *parent)
	: Config(parent)
{
	this->slope = slope;
	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );

	KComboBox *gradient = new KComboBox(this);
	QStringList items;
	QString curText;
	for (QMap<GradientType, QString>::Iterator it = slope->gradientI18nKeys.begin(); it != slope->gradientI18nKeys.end(); ++it)
	{
		if (it.key() == slope->curType())
			curText = it.value();
		items.append(it.value());
	}
	gradient->addItems(items);
	gradient->setCurrentItem(curText);
	layout->addWidget(gradient);
	connect(gradient, SIGNAL(activated(const QString &)), this, SLOT(setGradient(const QString &)));

	layout->addStretch();

	QCheckBox *reversed = new QCheckBox(i18n("Reverse direction"), this);
	reversed->setChecked(slope->isReversed());
	layout->addWidget(reversed);
	connect(reversed, SIGNAL(toggled(bool)), this, SLOT(setReversed(bool)));

	QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing( spacingHint() );
        layout->addLayout( hlayout );
	hlayout->addWidget(new QLabel(i18n("Grade:"), this));
	KDoubleNumInput *grade = new KDoubleNumInput(this);
	grade->setRange(0, 8, 1, true);
	grade->setValue(slope->curGrade());
	hlayout->addWidget(grade);
	connect(grade, SIGNAL(valueChanged(double)), this, SLOT(gradeChanged(double)));

	QCheckBox *stuck = new QCheckBox(i18n("Unmovable"), this);
	stuck->setWhatsThis( i18n("Whether or not this slope can be moved by other objects, like floaters."));
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
