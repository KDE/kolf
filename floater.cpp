#include <qlabel.h>
#include <qslider.h>
//Added by qt3to4:
#include <QHBoxLayout>
#include <Q3PtrList>

#include <kconfig.h>

#include "floater.h"

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
	if (fabs(xa - xb) > 0 || fabs(ya - yb) > 0)
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

Floater::Floater(QRect rect, Q3Canvas *canvas)
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
		if (Vector(origin, QPoint(x(), y())).magnitude() > vector.magnitude())
		{
			vector.setDirection(vector.direction() + M_PI);
			origin = (origin == wall->startPoint()? wall->endPoint() : wall->startPoint());

			setVelocity(-xVelocity(), -yVelocity());
		}
	}
}

void Floater::reset()
{
	QPoint start = wall->startPoint() + QPoint(wall->x(), wall->y());
	QPoint end = wall->endPoint() + QPoint(wall->x(), wall->y());

	vector = Vector(end, start);
	origin = end;

	move(origin.x(), origin.y());
	setSpeed(speed);
}

Q3PtrList<Q3CanvasItem> Floater::moveableItems() const
{
	Q3PtrList<Q3CanvasItem> ret(wall->moveableItems());
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

	const double factor = (double)speed / 3.5;
	setVelocity(-cos(vector.direction()) * factor, -sin(vector.direction()) * factor);
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

	Q3CanvasItemList l = collisions(false);
	for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		CanvasItem *item = dynamic_cast<CanvasItem *>(*it);

		if (!noUpdateZ && item && item->canBeMovedByOthers())
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
	Q3CanvasRectangle::moveBy(dx, dy);

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

	QPoint start(wall->startPoint() + QPoint(wall->x(), wall->y()));
	start = cfg->readPointEntry("startPoint", &start);
	QPoint end(wall->endPoint() + QPoint(wall->x(), wall->y()));
	end = cfg->readPointEntry("endPoint", &end);
	wall->setPoints(start.x(), start.y(), end.x(), end.y());
	wall->move(0, 0);

	setSpeed(cfg->readEntry("speed", -1));

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

#include "floater.moc"
