#ifndef KOLFPOOLBALL_H
#define KOLFPOOLBALL_H

#include <q3canvas.h>
#include <QObject>
#include <qpainter.h>

#include <klibloader.h>

#include <kolf/ball.h>
#include <kolf/canvasitem.h>
#include <kolf/config.h>
#include <kolf/object.h>

class StateDB;
class KConfig;

class PoolBallFactory : KLibFactory { Q_OBJECT public: QObject *createObject(QObject *, const char *, const char *, const QStringList & = QStringList()); };

class PoolBall : public Ball
{
public:
	PoolBall(Q3Canvas *canvas);

	virtual bool deleteable() const { return true; }

	virtual Config *config(QWidget *parent);
	virtual void saveState(StateDB *);
	virtual void save(KConfig *cfg);
	virtual void loadState(StateDB *);
	virtual void load(KConfig *cfg);
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
	PoolBall *m_poolBall;
};

class PoolBallObj : public Object
{
public:
	PoolBallObj() { m_name = i18n("Pool Ball"); m__name = "poolball"; m_author = "Jason Katz-Brown"; }
	virtual Q3CanvasItem *newObject(Q3Canvas *canvas) { return new PoolBall(canvas); }
};

#endif
