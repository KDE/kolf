#ifndef KOLF_CONFIG_H
#define KOLF_CONFIG_H

#include <QFrame>

class Config : public QFrame
{
	Q_OBJECT

public:
	Config(QWidget *parent);
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
	MessageConfig(const QString &text, QWidget *parent);
};

// internal
class DefaultConfig : public MessageConfig
{
	Q_OBJECT

public:
	DefaultConfig(QWidget *parent);
};

#endif
