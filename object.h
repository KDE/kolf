// it seems that OBJECT_H is used by something else

#ifndef KOLF_OBJECT_H
#define KOLF_OBJECT_H

#include <qcanvas.h>
#include <qstring.h>
#include <qobject.h>

class Object : public QObject
{
	Q_OBJECT

public:
	Object(QObject *parent = 0, const char *name = 0) : QObject(parent, name) { m_addOnNewHole = false; }
	virtual QCanvasItem *newObject(QCanvas * /*canvas*/) { return 0; }
	QString name() { return m_name; }
	QString _name() { return m__name; }
	QString author() { return m_author; }
	bool addOnNewHole() { return m_addOnNewHole; }

protected:
	QString m_name;
	QString m__name;
	QString m_author;
	bool m_addOnNewHole;
};
typedef QPtrList<Object> ObjectList;

#endif
