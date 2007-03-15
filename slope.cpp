#include <qbitmap.h>
#include <QCheckBox>
#include <QLabel>
#include <qimage.h>
#include <qpixmapcache.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <knuminput.h>
#include <kpixmapeffect.h>
#include <kstandarddirs.h>

#include "slope.h"

Slope::Slope(QRect rect, QGraphicsItem * parent, QGraphicsScene *scene)
	: QGraphicsRectItem(rect, parent, scene), type("Vertical"), grade(4), reversed(false), color(QColor("#327501")) 
{
	setData(0, 1031);
	stuckOnGround = false;
	showingInfo = false;
	baseArrowPenThickness = arrowPenThickness = 1;

	setZValue(-50);

	point = new RectPoint(color.light(), this, parent, scene);

	QFont font(kapp->font());
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
	if (type == "slope_bump" || type == "slope_dip")
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
	if(type == "slope_ne"){
		xavg = boundingRect().width()*1/4 + x();
		yavg = boundingRect().height()*3/4 + y();
	}
	else if(type == "slope_nw"){
		xavg = boundingRect().width()*3/4 + x();
		yavg = boundingRect().height()*3/4 + y();
	}
	else if(type == "slope_se"){
		xavg = boundingRect().width()*1/4 + x();
		yavg = boundingRect().height()*1/4 + y();
	}
	else if(type == "slope_sw"){
		xavg = boundingRect().width()*3/4 + x();
		yavg = boundingRect().height()*1/4 + y();
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

	QString gradientType = cfgGroup->readEntry("gradient", "Vertical");
	setGradient(gradientType);
}

void Slope::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("reversed", reversed);
	cfgGroup->writeEntry("width", width());
	cfgGroup->writeEntry("height", height());
	cfgGroup->writeEntry("gradient", type);;
	cfgGroup->writeEntry("grade", grade);
	cfgGroup->writeEntry("stuckOnGround", stuckOnGround);
}

void Slope::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/ ) 
{
	painter->drawPixmap(0, 0, pixmap);  
}

QPainterPath Slope::shape() const
{       
	if(type == "slope_ne" || type == "slope_sw") {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect().x(), rect().y());
		polygon[1] = QPointF(rect().x() + width(), rect().y() + height());
		polygon[2] = reversed? QPointF(rect().x() + width(), rect().y()) : QPointF(rect().x(), rect().y() + height());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == "slope_nw" || type == "slope_se") {
		QPainterPath path;
		QPolygonF polygon(3);
		polygon[0] = QPointF(rect().x() + width(), rect().y());
		polygon[1] = QPointF(rect().x(), rect().y() + height());
		polygon[2] = !reversed? QPointF(rect().x() + width(), rect().y() + height()) : QPointF(rect().x(), rect().y());
		path.addPolygon(polygon);
		return path;
	}
	else if(type == "slope_bump" || type == "slope_dip") {
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

	const bool diag = (type == "slope_ne" || type == "slope_nw" || type == "slope_sw" || type == "slope_se");
	const bool circle = (type == "slope_bump" || type == "slope_dip");

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

	if(type == "slope_w" || type == "slope_e")
			reversed? vx += addto : vx -= addto;
	else if(type == "slope_n" || type == "slope_s")
			reversed? vy += addto : vy -= addto;
	else if(type == "slope_nw" || type == "slope_se" || type == "slope_bump" || type == "slope_dip") {
		reversed? vx += cos(slopeAngle) * addto : vx -= cos(slopeAngle) * addto;
		reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
	}
	else if(type == "slope_ne" || type == "slope_sw") {
		reversed? vx -= cos(slopeAngle) * addto : vx += cos(slopeAngle) * addto;
		reversed? vy += sin(slopeAngle) * addto : vy -= sin(slopeAngle) * addto;
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

void Slope::setGradient(QString text)
{
	if(text == "Vertical") {
		if(reversed)
			setType("slope_s");
		else
			setType("slope_n");
	}
	else if(text == "Horizontal") {
		if(reversed)
			setType("slope_e");
		else
			setType("slope_w");
	}
	else if(text == "Diagonal") {
		if(reversed)
			setType("slope_se");
		else
			setType("slope_nw");
	}
	else if(text == "Opposite Diagonal") {
		if(reversed)
			setType("slope_sw");
		else
			setType("slope_ne");
	}
	else if(text == "Elliptic") {
		if(reversed)
			setType("slope_dip");
		else
			setType("slope_bump");
	}
	else   
		kDebug(12007) << "Warning, slope type: " << text << endl;

	return;
}

void Slope::setType(QString type)
{
	this->type = type;

	if (type == "slope_bump" || type == "slope_dip")
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
	pixmap=game->renderer->renderSvg(type, (int)width(), (int)height(), 0);

	// we update the arrows in this function
	clearArrows();

	const double length = sqrt(double(width() * width() + height() * height())) / 4;

	if (type == "slope_bump" || type == "slope_dip")
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

		if(type == "slope_w" || type == "slope_e")
			angle = 0;
		else if(type == "slope_n" || type == "slope_s")
				angle = M_PI / 2;
		else if(type == "slope_nw" || type == "slope_se")
				angle = atan((double)width() / (double)height());
		else if(type == "slope_ne" || type == "slope_sw")
				angle = M_PI - atan((double)width() / (double)height());

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
	items.append("Vertical");
	items.append("Horizontal");
	items.append("Diagonal");
	items.append("Opposite Diagonal");
	items.append("Elliptic");
	gradient->addItems(items);
	gradient->setCurrentText(curText); //this is not yet set, needs to be changed when fixing editing mode
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
