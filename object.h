// it seems that OBJECT_H is used by something else

#ifndef KOLF_OBJECT_H
#define KOLF_OBJECT_H

#include <QGraphicsView>

#include <QObject>

class KolfSvgRenderer;

class Object : public QObject
{
	Q_OBJECT

public:
	Object(QObject *parent = 0) : QObject(parent) { m_addOnNewHole = false; }
	virtual QGraphicsItem *newObject(QGraphicsItem *, QGraphicsScene * /*scene*/) { return 0; }
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
typedef QList<Object *> ObjectList;

#endif
