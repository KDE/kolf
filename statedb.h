#ifndef KOLF_STATEDB_H
#define KOLF_STATEDB_H

#include <qmap.h>
#include <qstring.h>
#include <qpoint.h>

// items can save their per-game states here
// most don't have to do anything
class StateDB
{
public:
	void setPoint(const QPoint &point) { points[curName] = point; }
	QPoint point() { return points[curName]; }
	void setName(const QString &name) { curName = name; }
	void clear() { points.clear(); }

private:
	QMap<QString, QPoint> points;
	QString curName;
};

#endif
