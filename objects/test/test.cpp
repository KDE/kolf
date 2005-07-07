#include <qbrush.h>
#include <qcolor.h>
#include <q3canvas.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qslider.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <klocale.h>
#include <klibloader.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kconfig.h>

#include "test.h"

K_EXPORT_COMPONENT_FACTORY(libkolftest, TestFactory)
QObject *TestFactory::createObject (QObject * /*parent*/, const char * /*name*/, const char * /*classname*/, const QStringList & /*args*/)
{ return new TestObj; }

Test::Test(Q3Canvas *canvas)
	: Q3CanvasEllipse(60, 40, canvas), count(0), m_switchEvery(20)
{
	// force to the bottom of other objects
	setZ(-100000);

	// we want calls to advance() even though we have no velocity
	setAnimated(true);
}

void Test::advance(int phase)
{
	Q3CanvasEllipse::advance(phase);

	// phase is either 0 or 1, only calls with 1 should be handled
	if (phase == 1)
	{
		// this makes it so the body is called every
		// m_switchEvery times
		if (count % m_switchEvery == 0)
		{
			// random color
			const QColor myColor((QRgb)(kapp->random() % 0x01000000));

			// set the brush, so our shape is drawn
			// with the random color
			setBrush(QBrush(myColor));

			count = 0;
		}

		count++;
	}
}

void Test::save(KConfig *cfg)
{
	// save our option from the course
	// (courses are represented as KConfig files)
	cfg->writeEntry("switchEvery", switchEvery());
}

void Test::load(KConfig *cfg)
{
	// load our option
	setSwitchEvery(cfg->readNumEntry("switchEvery", 50));
}

TestConfig::TestConfig(Test *test, QWidget *parent)
	: Config(parent), m_test(test)
{
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());

	layout->addStretch();

	layout->addWidget(new QLabel(i18n("Flash speed"), this));

	QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
	QLabel *slow = new QLabel(i18n("Slow"), this);
	hlayout->addWidget(slow);
	QSlider *slider = new QSlider(1, 100, 5, 101 - m_test->switchEvery(), Qt::Horizontal, this);
	hlayout->addWidget(slider);
	QLabel *fast = new QLabel(i18n("Fast"), this);
	hlayout->addWidget(fast);

	layout->addStretch();

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(switchEveryChanged(int)));
}

void TestConfig::switchEveryChanged(int news)
{
	// update our object
	m_test->setSwitchEvery((101 - news));

	// tells Kolf the hole was modified
	changed();
}

Config *Test::config(QWidget *parent)
{
	return new TestConfig(this, parent);
}

#include "test.moc"
