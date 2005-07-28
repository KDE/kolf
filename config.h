#ifndef KOLF_CONFIG_H
#define KOLF_CONFIG_H

#include <q3frame.h>

class Config : public Q3Frame
{
	Q_OBJECT

public:
	Config(QWidget *parent, const char *name = 0);
	void ctorDone();

signals:
	void modified();

protected:
	int spacingHint();
	int marginHint();
	bool startedUp;
	void changed();
};

// this is easy to use to show a message
class MessageConfig : public Config
{
	Q_OBJECT

public:
	MessageConfig(QString text, QWidget *parent, const char *name = 0);
};

// internal
class DefaultConfig : public MessageConfig
{
	Q_OBJECT

public:
	DefaultConfig(QWidget *parent, const char *name = 0);
};

#endif
