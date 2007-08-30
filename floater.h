/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
	void updateBaseResizeInfo();

private:
	Floater *floater;
	bool almostDead;
};

class Floater : public Bridge
{
public:
	Floater(const QRect &rect,  QGraphicsItem * parent, QGraphicsScene *scene);
	void resize(double resizeFactor);
	virtual bool collision(Ball *ball, long int id) { Bridge::collision(ball, id); return false; }
	virtual void saveState(StateDB *db);
	virtual void loadState(StateDB *db);
	virtual void save(KConfigGroup *cfgGroup);
	virtual void load(KConfigGroup *cfgGroup);
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
	void updateBaseResizeInfo();

private:
	int speedfactor;
	int speed;
	FloaterGuide *wall;
	/*
	 * base numbers are the size or position when no resizing has taken place (i.e. the defaults)
	 */
	double baseXVelocity, baseYVelocity;
	/*
	 * resizeFactor is the number to multiply base numbers by to get their resized value (i.e. if it is 1 then use default size, if it is >1 then everything needs to be bigger, and if it is <1 then everything needs to be smaller)
	 */
	double resizeFactor;
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
