#include <qbrush.h>
#include <qcolor.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>

#include <klocale.h>
#include <klibloader.h>
#include <kapplication.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kconfig.h>

#include <kolf/statedb.h>
#include <kolf/canvasitem.h>
#include "poolball.h"

K_EXPORT_COMPONENT_FACTORY(libkolfpoolball, PoolBallFactory)
QObject *PoolBallFactory::createObject (QObject *, const char *, const char *, const QStringList &) { return new PoolBallObj; }

PoolBall::PoolBall(QCanvas *canvas)
	: Ball(canvas)
{
	setBrush(black);
	m_number = 1;
}

void PoolBall::save(KConfig *cfg)
{
	cfg->writeEntry("number", number());
}

void PoolBall::saveState(StateDB *db)
{
	db->setPoint(QPoint(x(), y()));
}

void PoolBall::load(KConfig *cfg)
{
	setNumber(cfg->readNumEntry("number", 1));
}

void PoolBall::loadState(StateDB *db)
{
	move(db->point().x(), db->point().y());
	setVelocity(0, 0);
	setState(Stopped);
}

void PoolBall::draw(QPainter &p)
{
	// we should draw the number here
	Ball::draw(p);
}

PoolBallConfig::PoolBallConfig(PoolBall *poolBall, QWidget *parent)
	: Config(parent), m_poolBall(poolBall)
{
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());

	layout->addStretch();

	QLabel *num = new QLabel(i18n("Number:"), this);
	layout->addWidget(num);
	KIntNumInput *slider = new KIntNumInput(m_poolBall->number(), this);
	slider->setRange(1, 15);
	layout->addWidget(slider);

	layout->addStretch();

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(numberChanged(int)));
}

void PoolBallConfig::numberChanged(int newNumber)
{
	m_poolBall->setNumber(newNumber);
	changed();
}

Config *PoolBall::config(QWidget *parent)
{
	return new PoolBallConfig(this, parent);
}

#include "poolball.moc"
