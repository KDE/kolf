#include <QLabel>
#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kdialog.h>
#include <klocale.h>

#include "config.h"

Config::Config(QWidget *parent)
	: QFrame(parent)
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

MessageConfig::MessageConfig(const QString &text, QWidget *parent)
	: Config(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin( marginHint() );
        layout->setSpacing( spacingHint() );
	layout->addWidget(new QLabel(text, this));
}

DefaultConfig::DefaultConfig(QWidget *parent)
	: MessageConfig(i18n("No configuration options"), parent)
{
}

#include "config.moc"
