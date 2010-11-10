/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

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

#include "config.h"
#include "vector.h"
#include "tagaro/spriteobjectitem.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <KDebug>
class b2Body;
class b2World;

class Ball;
class KConfigGroup;
class KolfGame;

namespace Kolf
{
	class EllipseShape;
	class Overlay;
	class Shape;
}

class CanvasItem
{
public:
	CanvasItem(b2World* world);
	virtual ~CanvasItem();
	///load your settings from the KConfigGroup, which represents a course.
	virtual void load(KConfigGroup *) {}
	///returns a bool that is true if your item needs to load after other items
	virtual bool loadLast() const { return false; }
	///called if the item is made by user while editing, with the item that was selected on the hole;
	virtual void selectedItem(QGraphicsItem * /*item*/) {}
	///save your settings.
	virtual void save(KConfigGroup *cfg);
	///called for information when shot started
	virtual void shotStarted() {}
	///called right before any items are saved.
	virtual void aboutToSave() {}
	///called right after all items are saved.
	virtual void savingDone() {}
	///called when the edit mode has been changed.
	virtual void editModeChanged(bool editing);
	///The item should delete any other objects it's created. DO NOT DO THIS KIND OF STUFF IN THE DESTRUCTOR!
	virtual void aboutToDie() {}
	///Returns the object to get rid of when the delete button is pressed on this item.
	virtual CanvasItem *itemToDelete() { return this; }
	///called when user presses delete key while editing. This is very rarely reimplemented, and generally shouldn't be.
	virtual void aboutToDelete() {}
	///Returns whether this item should be able to be deleted by user while editing.
	virtual bool deleteable() const { return true; }
	///called if fastAdvance is enabled
	virtual void doAdvance() {}
	///Returns whether all items of this type of item (based on data()) that are "colliding" (ie, in the same spot) with ball should get collision() called.
	virtual bool terrainCollisions() const { return false; }
	///Returns whether or not this item lifts items on top of it.
	virtual bool vStrut() const { return false; }
	///Show extra item info
	virtual void showInfo() {}
	///Hide extra item info
	virtual void hideInfo() {}
	///update your Z value (this is called by various things when perhaps the value should change) if this is called by a vStrut, it will pass 'this'.
	virtual void updateZ(QGraphicsItem * /*vStrut*/ = 0) {}
	///clean up for prettyness
	virtual void clean() {}
	///returns whether this item can be moved by others (if you want to move an item, you should honor this!)
	virtual bool canBeMovedByOthers() const { return false; }
	///Returns a Config that can be used to configure this item by the user. The default implementation returns one that says 'No configuration options'.
	virtual Config *config(QWidget *parent) { return new DefaultConfig(parent); }
	///Returns other items that should be moveable (besides this one of course).
	virtual QList<QGraphicsItem *> moveableItems() const { return QList<QGraphicsItem *>(); }
	///Returns whether this can be moved by the user while editing.
	virtual bool moveable() const { return true; }

	void setId(int newId) { m_id = newId; }
	int curId() const { return m_id; }

	///Play a sound (e.g. playSound("wall") plays kdedir/share/apps/kolf/sounds/wall.wav). Optionally, specify \a vol to be between 0-1, for no sound to full volume, respectively.
	void playSound(const QString &file, double vol = 1);

	///Called on ball's collision. Return if terrain collidingItems should be processed.
	virtual bool collision(Ball *ball) { Q_UNUSED(ball) return false; }

	///Reimplement if you want extra items to have access to the game object. playSound() relies on having this.
	virtual void setGame(KolfGame *game) { this->game = game; }

	///returns whether this is a corner resizer
	virtual bool cornerResize() const { return false; }

	QString name() const { return m_name; }
	void setName(const QString &newname) { m_name = newname; }
	virtual void setSize(const QSizeF&) {}

	///custom animation code
	bool isAnimated() const { return m_animated; }
	void setAnimated(bool animated) { m_animated = animated; }
	virtual void setVelocity(const Vector& velocity) { m_velocity = velocity; }
	Vector velocity() const { return m_velocity; }
	virtual void moveBy(double , double) { kDebug(12007) << "Warning, empty moveBy used";} //needed so that float can call the own custom moveBy()s of everything on it

	//The following is needed temporarily while CanvasItem is not a QGraphicsItem by itself.
	void setPosition(const QPointF& pos) { const QPointF diff = pos - getPosition(); moveBy(diff.x(), diff.y()); }
	virtual QPointF getPosition() const = 0;
protected:
	friend class Kolf::Overlay; //for delivery of Kolf::Overlay::stateChanged signal
	///pointer to main KolfGame
	KolfGame *game;
	///returns the highest vertical strut the item is on
	QGraphicsRectItem *onVStrut();
private:
	QString m_name;
	int m_id;
	///custom animation code
	bool m_animated;
	Vector m_velocity;

//AFTER THIS LINE follows what I have inserted during the refactoring
	public:
		enum SimulationFlag
		{
			CollisionFlag = 1 << 0,
			KinematicSimulationFlag = 1 << 1,
			DynamicSimulationFlag = 1 << 2
		};
		enum SimulationType
		{
			///The object is immovable.
			NoSimulation = 0,
			///The object is immovable, but other objects can interact with it.
			CollisionSimulation = CollisionFlag,
			///The object moves according to its kinematic state.
			KinematicSimulation = CollisionSimulation | KinematicSimulationFlag,
			///This object collides with the shapes of other objects, and forces
			///can act on it.
			DynamicSimulation = KinematicSimulation | DynamicSimulationFlag
		};

		QList<Kolf::Shape*> shapes() const { return m_shapes; }
		Kolf::Overlay* overlay(bool createIfNecessary = true);

		///This is the velocity used by the physics engine: In each time step,
		///the position of this canvas item changes by the value of this property.
		QPointF physicalVelocity() const;
		void setPhysicalVelocity(const QPointF& physicalVelocity);
	protected:
		void addShape(Kolf::Shape* shape);
		///Configure how this object will participate in physical simulation.
		void setSimulationType(CanvasItem::SimulationType type);

		friend class ::KolfGame; //for the following two methods
		///The physics engine calls this method to prepare the object for the following simulation step. Subclass implementations have to call the base implementation just before returning.
		virtual void startSimulation();
		///The physics engine calls this method after calculating the next frame, to let the objects update their representation. Subclass implementations have to call the base implementation before anything else.
		virtual void endSimulation();

		///Creates the optimal overlay for this object. The implementation does not have to propagate its properties to the overlay, as the overlay is updated just after it has been created.
		///@warning Do not actually call this function from subclass implementations. Use overlay() instead.
		virtual Kolf::Overlay* createOverlay() { return 0; } //TODO: make this pure virtual when all CanvasItems are QGraphicsItems and implement createOverlay() (and then disallow createOverlay() == 0)
		///This function should be called whenever the value of an object's property changes. This will most prominently cause the overlay to be updated (if it exists).
		void propagateUpdate();
	private:
		friend class Kolf::Shape; //for access to m_body
		b2Body* m_body;

		Kolf::Overlay* m_overlay;
		QList<Kolf::Shape*> m_shapes;
		CanvasItem::SimulationType m_simulationType;
};

//WARNING: pos() is at center (not at top-left edge of bounding rect!)
class EllipticalCanvasItem : public Tagaro::SpriteObjectItem, public CanvasItem
{
	public:
		EllipticalCanvasItem(bool withEllipse, const QString& spriteKey, QGraphicsItem* parent, b2World* world);
		QGraphicsEllipseItem* ellipseItem() const { return m_ellipseItem; }

		virtual bool contains(const QPointF& point) const;
		virtual QPainterPath shape() const;

		QRectF rect() const;
		double width() const { return Tagaro::SpriteObjectItem::size().width(); }
		double height() const { return Tagaro::SpriteObjectItem::size().height(); }

		virtual void setSize(const QSizeF& size);
		void setSize(qreal width, qreal height) { setSize(QSizeF(width, height)); }
		virtual void moveBy(double x, double y);

		void saveSize(KConfigGroup* group);
		void loadSize(KConfigGroup* group);

		virtual QPointF getPosition() const { return QGraphicsItem::pos(); }
	private:
		QGraphicsEllipseItem* m_ellipseItem;
		Kolf::EllipseShape* m_shape;
};

#endif
