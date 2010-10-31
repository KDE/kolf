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

#include "slope.h"
#include "tagaro/board.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <KComboBox>
#include <KGameRenderer>
#include <KNumInput>

Slope::Slope(QGraphicsItem * parent, b2World* world)
	: Tagaro::SpriteObjectItem(Kolf::renderer(), QString(), parent)
	, CanvasItem(world)
	, type(Vertical), grade(4), reversed(false), color(QColor("#327501"))
{
	Tagaro::SpriteObjectItem::setSize(QSizeF(40, 40));

	setData(0, 1031);
	stuckOnGround = false;
	showingInfo = false;

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

	point = new RectPoint(color.light(), this, parent, world);

	QFont font(QApplication::font());
	font.setPixelSize(18);
	text = new QGraphicsSimpleTextItem(Kolf::findBoard(this));
	text->setZValue(99999.99);
	text->setFont(font);
	text->setBrush(Qt::white);

	editModeChanged(false);
	hideInfo();

	// this does setSpriteKey
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
	qDeleteAll(arrows);
	arrows.clear();
}

QList<QGraphicsItem *> Slope::moveableItems() const
{
	return QList<QGraphicsItem*>() << point;
}

void Slope::setGrade(double newGrade)
{
	if (newGrade >= 0 && newGrade < 11)
	{
		grade = newGrade;
		updatePixmap();
	}
}

void Slope::setSize(const QSizeF& size)
{
	if (type == Elliptic)
	{
		const double extent = qMin(size.width(), size.height());
		Tagaro::SpriteObjectItem::setSize(extent, extent);
		// move point back to good spot
		moveBy(0, 0);

		if (game && game->isEditing())
			game->updateHighlighter();
	}
	else
	{
		Tagaro::SpriteObjectItem::setSize(size);
	}

	updateZ();
}

void Slope::moveBy(double dx, double dy)
{
	Tagaro::SpriteObjectItem::moveBy(dx, dy);

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

void Slope::updateZ(QGraphicsItem *vStrut)
{
	const double area = (height() * width());
	const int defaultz = -50;

	double newZ = 0;

	QGraphicsItem *rect = 0;
	if (!stuckOnGround)
		rect = vStrut? vStrut : onVStrut();

	if (rect)
	{
		if (area > (rect->boundingRect().width() * rect->boundingRect().height()))
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

	// bypass updatePixmap which setSize normally does
	Tagaro::SpriteObjectItem::setSize(cfgGroup->readEntry("width", width()), cfgGroup->readEntry("height", height()));
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

QPainterPath Slope::shape() const
{
	const QRectF rect = boundingRect();
	if(type == CrossDiagonal) {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect.x(), rect.y());
		polygon[1] = QPointF(rect.x() + rect.width(), rect.y() + rect.height());
		polygon[2] = reversed? QPointF(rect.x() + rect.width(), rect.y()) : QPointF(rect.x(), rect.y() + rect.height());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == Diagonal) {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect.x() + rect.width(), rect.y());
		polygon[1] = QPointF(rect.x(), rect.y() + rect.height());
		polygon[2] = !reversed? QPointF(rect.x() + rect.width(), rect.y() + rect.height()) : QPointF(rect.x(), rect.y());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == Elliptic) {
		QPainterPath path;
		path.addEllipse(rect.x(), rect.y(), rect.width(), rect.height());
		return path;
	}
	else {
		QPainterPath path;
		path.addRect(rect.x(), rect.y(), rect.width(), rect.height());
		return path;
	}
}

bool Slope::collision(Ball *ball, long int /*id*/)
{
	if (grade <= 0)
		return false;

	Vector v = ball->velocity();
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

		Vector betweenVector = start - end;
		const double factor = betweenVector.magnitude() / ((double)width() / 2.0);
		slopeAngle = betweenVector.direction();

		// this little bit by Daniel
		addto *= factor * M_PI / 2;
		addto = sin(addto);
	}

	if (!reversed)
		addto = -addto;
	switch ((int) type)
	{
		case Horizontal:
			v.rx() += addto;
			break;
		case Vertical:
			v.ry() += addto;
			break;
		case Diagonal:
		case Elliptic:
			v.rx() += cos(slopeAngle) * addto;
			v.ry() += sin(slopeAngle) * addto;
			break;
		case CrossDiagonal:
			v.rx() -= cos(slopeAngle) * addto;
			v.ry() += sin(slopeAngle) * addto;
			break;
	}
	ball->setVelocity(v);

	// check if the ball is at the center of a pit or mound or has otherwise stopped.
	ball->setState(v.isNull() ? Stopped : Rolling);
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
		//ensure quadratic shape
		setSize(size());
	}

	updatePixmap();
}

void Slope::updatePixmap() //this needs work so that the slope colour depends on angle again
{
	switch(type) {
		case Horizontal:
			if(reversed)
				setSpriteKey(QLatin1String("slope_e"));
			else
				setSpriteKey(QLatin1String("slope_w"));
			break;

		case Vertical:
			if(reversed)
				setSpriteKey(QLatin1String("slope_s"));
			else
				setSpriteKey(QLatin1String("slope_n"));
			break;

		case Diagonal:
			if(reversed)
				setSpriteKey(QLatin1String("slope_se"));
			else
				setSpriteKey(QLatin1String("slope_nw"));
			break;

		case CrossDiagonal:
			if(reversed)
				setSpriteKey(QLatin1String("slope_sw"));
			else
				setSpriteKey(QLatin1String("slope_ne"));
			break;
		case Elliptic:
			if(reversed)
				setSpriteKey(QLatin1String("slope_dip"));
			else
				setSpriteKey(QLatin1String("slope_bump"));
			break;
		default:
			break;
	}

	// we update the arrows in this function
	clearArrows();

	const double length = sqrt(double(width() * width() + height() * height())) / 4;

	if (type == Elliptic)
	{
		double angle = 0;
		for (int i = 0; i < 4; ++i)
		{
			angle += M_PI / 2;
			Arrow *arrow = new Arrow(Kolf::findBoard(this));
			arrow->setLength(length);
			arrow->setAngle(angle);
			arrow->setPen(QPen(Qt::black));
			arrow->setReversed(reversed);
			arrow->updateSelf();
			arrows.append(arrow);
		}
	}
	else
	{
		Arrow *arrow = new Arrow(Kolf::findBoard(this));
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
		arrow->setPen(QPen(Qt::black));
		arrow->updateSelf();

		arrows.append(arrow);
	}

	text->setText(QString::number(grade));

	moveArrow();
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
