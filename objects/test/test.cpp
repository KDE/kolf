#include <qbrush.h>
#include <qcolor.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qslider.h>

#include <klocale.h>
#include <klibloader.h>
#include <kapplication.h>
#include <kdebug.h>
#include <ksimpleconfig.h>

#include "canvasitem.h"
#include "test.h"

K_EXPORT_COMPONENT_FACTORY(libkolftest, TestFactory)
QObject *TestFactory::createObject (QObject * /*parent*/, const char * /*name*/, const char * /*classname*/, const QStringList & /*args*/)
{ return new TestObj; }

Test::Test(QCanvas *canvas)
	: QCanvasEllipse(60, 40, canvas)
{
	setZ(-100000);
	m_switchEvery = 20;

	count = 0;
	setAnimated(true);
}

void Test::advance(int phase)
{
	QCanvasEllipse::advance(phase);

	if (phase == 1)
	{
		if (count % m_switchEvery == 0)
		{
			const QColor myColor((QRgb)(kapp->random() % 0x01000000));
			setBrush(QBrush(myColor));
			count = 0;
		}

		count++;
	}
}

void Test::save(KSimpleConfig *cfg)
{
	cfg->writeEntry("switchEvery", switchEvery());
}

void Test::load(KSimpleConfig *cfg)
{
	setSwitchEvery(cfg->readNumEntry("switchEvery", 50));
}

TestConfig::TestConfig(Test *test, QWidget *parent)
	: Config(parent)
{
	this->test = test;

	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());

	layout->addStretch();

	layout->addWidget(new QLabel(i18n("Flash Speed"), this));

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	QLabel *slow = new QLabel(i18n("Slow"), this);
	hlayout->addWidget(slow);
	QSlider *slider = new QSlider(1, 100, 5, 101 - test->switchEvery(), Qt::Horizontal, this);
	hlayout->addWidget(slider);
	QLabel *fast = new QLabel(i18n("Fast"), this);
	hlayout->addWidget(fast);

	layout->addStretch();

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(switchEveryChanged(int)));
}

void TestConfig::switchEveryChanged(int news)
{
	test->setSwitchEvery((101 - news));
	changed();
}

Config *Test::config(QWidget *parent)
{
	return new TestConfig(this, parent);
}

#include "test.moc"
