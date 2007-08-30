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

#ifndef KOLF_CANVASITEM_H
#define KOLF_CANVASITEM_H

#include <kdebug.h>

#include <QGraphicsView>
#include <QGraphicsRectItem>

#include "config.h"

class Ball;
class KConfigGroup;
class StateDB;
class KolfGame;

class CanvasItem
{
public:
	CanvasItem() { game = 0; }
	virtual ~CanvasItem() {}
	/**
	 * load your settings from the KConfigGroup, which represents a course.
	 */
	virtual void load(KConfigGroup *) {}
	/**
	 * load a point if you wish. Rarely necessary.
	 */
	virtual void loadState(StateDB * /*db*/) {}
	/**
	 * returns a bool that is true if your item needs to load after other items
	 */
	virtual bool loadLast() const { return false; }
	/**
	 * called if the item is made by user while editing, with the item that was selected on the hole;
	 */
	virtual void selectedItem(QGraphicsItem * /*item*/) {}
	/**
	 * called after the item is moved the very first time by the game
	 */
	virtual void firstMove(int /*x*/, int /*y*/) {}
	/**
	 * used for resizing, also moves the item appropriately
	 */
	virtual void resize(double /*resize factor*/) { kDebug(12007) << "Warning, empty resize used\n"; }
	/**
	 * save your settings.
	 */
	virtual void save(KConfigGroup *cfg);
	/**
	 * save a point if you wish. Rarely necessary.
	 */
	virtual void saveState(StateDB * /*db*/) {}
	/**
	 * called for information when shot started
	 */
	virtual void shotStarted() {}
	/**
	 * called right before any items are saved.
	 */
	virtual void aboutToSave() {}
	/**
	 * called right after all items are saved.
	 */
	virtual void savingDone() {}
	/**
	 * called when the edit mode has been changed.
	 */
	virtual void editModeChanged(bool /*editing*/) {}
	/**
	 * the item should delete any other objects it's created.
	 * DO NOT DO THIS KIND OF STUFF IN THE DESTRUCTOR!
	 */
	virtual void aboutToDie() {}
	/**
	 * returns the object to get rid of when the delete button is pressed on this item. Some sub-objects will return something other than this.
	 */
	virtual CanvasItem *itemToDelete() { return this; }
	/**
	 * called when user presses delete key while editing. This is very rarely reimplemented, and generally shouldn't be.
	 */
	virtual void aboutToDelete() {}
	/**
	 * returns whether this item should be able to be deleted by user while editing.
	 */
	virtual bool deleteable() const { return true; }
	/**
	 * returns whether this item should get doAdvance called -- it is called in sync with ball advancing (which is twice as fast as the advance() calling rate)
	 */
	virtual bool fastAdvance() const { return false; }
	/**
	 * called when all items have had their chance at a doAdvance
	 */
	virtual void fastAdvanceDone() {}
	/**
	 * called if fastAdvance is enabled
	 */
	virtual void doAdvance() {}
	/**
	 * if all items of this type of item (based on data()) that are "colliding" (ie, in the same spot) with ball should get collision() called.
	 */
	virtual bool terrainCollisions() const { return false; }
	/**
	 * returns whether or not this item lifts items on top of it.
	 */
	virtual bool vStrut() const { return false; }
	/**
	 * show extra item info
	 */
	virtual void showInfo() {}
	/**
	 * hide extra item info
	 */
	virtual void hideInfo() {}
	/**
	 * update your Z value (this is called by various things when perhaps the value should change) if this is called by a vStrut, it will pass 'this'.
	 */
	virtual void updateZ(QGraphicsRectItem * /*vStrut*/ = 0) {}
	/**
	 * clean up for prettyness
	 */
	virtual void clean() {}
	/**
	 * scale factor changed (game->scaleFactor(), the world matrix is game->worldMatrix())
	 * NOTE: not used in Kolf 1.1, which comes with KDE 3.1.
	 */
	virtual void scaleChanged() {}
	/**
	 * returns whether this item can be moved by others (if you want to move an item, you should honor this!)
	 */
	virtual bool canBeMovedByOthers() const { return false; }
	/**
	 * returns a Config that can be used to configure this item by the user.
	 * The default implementation returns one that says 'No configuration options'.
	 */
	virtual Config *config(QWidget *parent) { return new DefaultConfig(parent); }
	/**
	 * returns other items that should be moveable (besides this one of course).
	 */
	virtual QList<QGraphicsItem *> moveableItems() const { return QList<QGraphicsItem *>(); }
	/**
	 * returns whether this can be moved by the user while editing.
	 */
	virtual bool moveable() const { return true; }

	void setId(int newId) { id = newId; }
	int curId() const { return id; }

	/**
	 * call to play sound (ie, playSound("wall") plays kdedir/share/apps/kolf/sounds/wall.wav).
	 * optionally, specify vol to be between 0-1, for no sound to full volume, respectively.
	 */
        void playSound(const QString &file, double vol = 1);

	/**
	 * called on ball's collision. Return if terrain collidingItems should be processed.
	 */
	virtual bool collision(Ball * /*ball*/, long int /*id*/) { return false; }

	/**
	 * reimplement if you want extra items to have access to the game object.
	 * playSound() relies on having this.
	 */
	virtual void setGame(KolfGame *game) { this->game = game; }

	/**
	 * returns whether this is a corner resizer
	 */
	virtual bool cornerResize() const { return false; }

	QString name() const { return m_name; }
	void setName(const QString &newname) { m_name = newname; }
	virtual void setSize(double /*width*/, double /*height*/) {;}

	virtual void updateBaseResizeInfo() {}

	/**
	 * custom animation code
	 */
	void setAnimated(bool animated) { this->animated=animated; }
	void setVelocity(double xv, double yv) { setXVelocity(xv); setYVelocity(yv); }
	void setXVelocity(double xVelocity) { this->xVelocity=xVelocity; } 
	void setYVelocity(double yVelocity) { this->yVelocity=yVelocity; }
	double getXVelocity() { return xVelocity; }
	double getYVelocity() { return yVelocity; }
	virtual void moveBy(double , double) { kDebug(12007) << "Warning, empty moveBy used";} //needed so that float can call the own custom moveBy()s of everything on it

protected:
	/**
	 * pointer to main KolfGame
	 */
	KolfGame *game;

	/**
	 * returns the highest vertical strut the item is on
	 */
	QGraphicsRectItem *onVStrut();

	/**
	 * custom animation code
	 */
	bool animated;
	double xVelocity;
	double yVelocity;

private:
	QString m_name;
	int id;
};

#endif
