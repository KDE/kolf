#ifndef FLOATER_H
#define FLOATER_H

#include "game.h"

class Floater;
class FloaterConfig : public BridgeConfig
{
	Q_OBJECT

public:
	FloaterConfig(Floater *floater, QWidget *parent);

private slots:
	void speedChanged(int news);

private:
	Floater *floater;
};

class FloaterGuide : public Wall
{
public:
	FloaterGuide(Floater *floater, QGraphicsItem *parent, QGraphicsScene *scene) : Wall(parent, scene) { this->floater = floater; almostDead = false; }
	virtual void setPoints(double xa, double ya, double xb, double yb);
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent);
	virtual void aboutToDelete();
	virtual void aboutToDie();

private:
	Floater *floater;
	bool almostDead;
};

class Floater : public Bridge
{
public:
	Floater(QRect rect,  QGraphicsItem * parent, QGraphicsScene *scene);
	virtual bool collision(Ball *ball, long int id) { Bridge::collision(ball, id); return false; }
	virtual void saveState(StateDB *db);
	virtual void loadState(StateDB *db);
	virtual void save(KConfig *cfg);
	virtual void load(KConfig *cfg);
	virtual bool loadLast() const { return true; }
	virtual void firstMove(int x, int y);
	virtual void aboutToSave();
	virtual void aboutToDie();
	virtual void savingDone();
	virtual void setGame(KolfGame *game);
	virtual void editModeChanged(bool changed);
	virtual bool moveable() const { return false; }
	virtual void moveBy(double dx, double dy);
	virtual Config *config(QWidget *parent) { return new FloaterConfig(this, parent); }
	virtual QList<QGraphicsItem *> moveableItems() const;
	virtual void advance(int phase);
	void doAdvance();
	void setSpeed(int news);
	int curSpeed() const { return speed; }

	// called by floaterguide when changed;
	void reset();

private:
	int speedfactor;
	int speed;
	FloaterGuide *wall;
	QPoint origin;
	Vector vector;
	bool noUpdateZ;
	bool haventMoved;
	QPoint firstPoint;
};

class FloaterObj : public Object
{
public:
	FloaterObj() { m_name = i18n("Floater"); m__name = "floater"; }
	virtual QGraphicsItem *newObject(QGraphicsItem * parent, QGraphicsScene *scene) { return new Floater(QRect(0, 0, 80, 40), parent, scene); }

};

#endif
