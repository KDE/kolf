#ifndef KOLFPOOLBALL_H
#define KOLFPOOLBALL_H

#include <qcanvas.h>
#include <qobject.h>
#include <qpainter.h>

#include <klibloader.h>

#include "ball.h"
#include "canvasitem.h"
#include "object.h"

class StateDB;
class KSimpleConfig;

class PoolBallFactory : KLibFactory { Q_OBJECT public: QObject *createObject(QObject *, const char *, const char *, const QStringList & = QStringList()); };

class PoolBall : public Ball
{
public:
	PoolBall(QCanvas *canvas);

	virtual Config *config(QWidget *parent);
	virtual void saveState(StateDB *);
	virtual void save(KSimpleConfig *cfg);
	virtual void loadState(StateDB *);
	virtual void load(KSimpleConfig *cfg);
	virtual void draw(QPainter &);
	virtual bool fastAdvance() const { return true; }

	int number() const { return m_number; }
	void setNumber(int newNumber) { m_number = newNumber; update(); }

private:
	int m_number;
};

class PoolBallConfig : public Config
{
	Q_OBJECT

public:
	PoolBallConfig(PoolBall *poolBall, QWidget *parent);

private slots:
	void numberChanged(int);

private:
	PoolBall *poolBall;
};

class PoolBallObj : public Object
{
public:
	PoolBallObj() { m_name = i18n("Pool Ball"); m__name = "poolball"; m_author = "Jason Katz-Brown"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) const { return new PoolBall(canvas); }
};

#endif
