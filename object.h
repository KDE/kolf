// it seems that OBJECT_H is used by something else

#ifndef _OBJECT_H
#define _OBJECT_H

#include <qcanvas.h>
#include <qstring.h>
#include <qobject.h>

class Object : public QObject
{
	Q_OBJECT

public:
	Object(QObject *parent = 0, const char *name = 0) : QObject(parent, name) {};
	virtual QCanvasItem *newObject(QCanvas * /*canvas*/) { return 0; }
	virtual QString name() { return m_name; }
	virtual QString _name() { return m__name; }
	virtual QString author() { return m_author; }

protected:
	QString m_name;
	QString m__name;
	QString m_author;
};
typedef QPtrList<Object> ObjectList;

#endif
