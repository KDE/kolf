#ifndef _CONFIG_H
#define _CONFIG_H

#include <kdialog.h>
#include <qframe.h>

class Config : public QFrame
{
	Q_OBJECT

public:
	Config(QWidget *parent, const char *name = 0) : QFrame(parent, name) { startedUp = false; }
	void ctorDone() { startedUp = true; }

signals:
	void modified();

protected:
	inline int spacingHint() { return KDialog::spacingHint() / 2; }
	inline int marginHint() { return KDialog::marginHint(); }
	bool startedUp;
	inline void changed() { if (startedUp) emit modified(); }
};

#endif
