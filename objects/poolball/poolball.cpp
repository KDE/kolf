#include <qbrush.h>
#include <qcolor.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>

#include <klocale.h>
#include <klibloader.h>
#include <kapp.h>
#include <kdebug.h>
#include <knuminput.h>
#include <ksimpleconfig.h>

#include "statedb.h"
#include "canvasitem.h"
#include "poolball.h"

K_EXPORT_COMPONENT_FACTORY( libkolfpoolball, PoolBallFactory )
QObject *PoolBallFactory::createObject (QObject *parent, const char *name, const char *classname, const QStringList &args) { return new PoolBallObj; }

PoolBall::PoolBall(QCanvas *canvas)
	: Ball(canvas)
{
	//kdDebug() << "PoolBall::PoolBall\n";
	setBrush(black);
	m_number = 1;
}

void PoolBall::save(KSimpleConfig *cfg)
{
	cfg->writeEntry("number", number());
}

void PoolBall::saveState(StateDB *db)
{
	db->setPoint(QPoint(x(), y()));
}

void PoolBall::load(KSimpleConfig *cfg)
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
	Ball::draw(p);
}

PoolBallConfig::PoolBallConfig(PoolBall *poolBall, QWidget *parent)
	: Config(parent)
{
	this->poolBall = poolBall;

	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());

	layout->addStretch();

	QLabel *slow = new QLabel(i18n("Number"), this);
	layout->addWidget(slow);
	KIntNumInput *slider = new KIntNumInput(poolBall->number(), this);
	slider->setRange(1, 15);
	layout->addWidget(slider);

	layout->addStretch();

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(numberChanged(int)));
}

void PoolBallConfig::numberChanged(int newNumber)
{
	poolBall->setNumber(newNumber);
	changed();
}

Config *PoolBall::config(QWidget *parent)
{
	return new PoolBallConfig(this, parent);
}

#include "poolball.moc"
