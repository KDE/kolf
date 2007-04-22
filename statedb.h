#ifndef KOLF_STATEDB_H
#define KOLF_STATEDB_H

#include <QMap>

#include <QPoint>

// items can save their per-game states here
// most don't have to do anything
class StateDB
{
public:
	void setPoint(const QPointF &point) { points[curName] = point; }
	QPointF point() { return points[curName]; }
	void setName(const QString &name) { curName = name; }
	void clear() { points.clear(); }

private:
	QMap<QString, QPointF> points;
	QString curName;
};

#endif
