#include <QLabel>
#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>
#include <Q3Frame>

#include <kdialog.h>
#include <klocale.h>

#include "config.h"

Config::Config(QWidget *parent, const char *name)
	: Q3Frame(parent, name)
{
	startedUp = false;
}

void Config::ctorDone()
{
	startedUp = true;
}

int Config::spacingHint()
{
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

MessageConfig::MessageConfig(QString text, QWidget *parent, const char *name)
	: Config(parent, name)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );
	layout->addWidget(new QLabel(text, this));
}

DefaultConfig::DefaultConfig(QWidget *parent, const char *name)
	: MessageConfig(i18n("No configuration options"), parent, name)
{
}

#include "config.moc"
