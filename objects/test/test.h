#ifndef KOLFTEST_H
#define KOLFTEST_H

#include <qcanvas.h>
#include <qobject.h>

#include <klibloader.h>

#include "canvasitem.h"
#include "object.h"

class KSimpleConfig;

class TestFactory : KLibFactory { Q_OBJECT public: QObject *createObject(QObject *, const char *, const char *, const QStringList & = QStringList()); };

class Test : public QCanvasEllipse, public CanvasItem
{
public:
	Test(QCanvas *canvas);

	virtual Config *config(QWidget *parent);
	virtual void save(KSimpleConfig *cfg);
	virtual void load(KSimpleConfig *cfg);

	virtual void advance(int phase);

	int switchEvery() const { return m_switchEvery / 2; }
	void setSwitchEvery(int news) { m_switchEvery = news * 2; }

private:
	int count;
	int m_switchEvery;
};

class TestConfig : public Config
{
	Q_OBJECT

public:
	TestConfig(Test *test, QWidget *parent);

private slots:
	void switchEveryChanged(int news);

private:
	Test *test;
};

class TestObj : public Object
{
public:
	TestObj() { m_name = i18n("Flash"); m__name = "flash"; m_author = "Jason Katz-Brown"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) const { return new Test(canvas); }
};

#endif
