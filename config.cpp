#include <qlabel.h>
#include <qlayout.h>

#include <kdialog.h>
#include <klocale.h>

#include "config.h"

Config::Config(QWidget *parent, const char *name)
	: QFrame(parent, name)
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
	QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
	layout->addWidget(new QLabel(text, this));
}

DefaultConfig::DefaultConfig(QWidget *parent, const char *name)
	: MessageConfig(i18n("No configuration options"), parent, name)
{
}

#include "config.moc"
