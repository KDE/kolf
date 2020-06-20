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

#include <config.h>
#include "vector.h"
#include "tagaro/spriteobjectitem.h"

#include <QGraphicsRectItem>

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

enum RttiCodes { Rtti_NoCollision = 1001, Rtti_DontPlaceOn = 1002, Rtti_Putter = 1004 };

class CanvasItem
{
public:
	explicit CanvasItem(b2World* world);
	virtual ~CanvasItem();
	///load your settings from the KConfigGroup, which represents a course.
	virtual void load(KConfigGroup *) {}
	///save your settings.
	virtual void save(KConfigGroup *cfg);
	///called for information when shot started
	virtual void shotStarted() {}
	///called when the edit mode has been changed.
	virtual void editModeChanged(bool editing);
	///Returns whether all items of this type of item (based on data()) that are "colliding" (ie, in the same spot) with ball should get collision() called.
	virtual bool terrainCollisions() const { return false; }
	///Returns a Config that can be used to configure this item by the user. The default implementation returns one that says 'No configuration options'.
	virtual Config *config(QWidget *parent) { return new DefaultConfig(parent); }
	///Returns other items that should be movable (besides this one of course).
	virtual QList<QGraphicsItem *> moveableItems() const { return QList<QGraphicsItem *>(); }

	void setId(int newId) { m_id = newId; }
	int curId() const { return m_id; }

	///Called on ball's collision. Return if terrain collidingItems should be processed.
	virtual bool collision(Ball *ball) { Q_UNUSED(ball) return false; }

	///Reimplement if you want extra items to have access to the game object.
	virtual void setGame(KolfGame *game) { this->game = game; }

	QString name() const { return m_name; }
	void setName(const QString &newname) { m_name = newname; }
	virtual void setSize(const QSizeF&) {}

	virtual void moveBy(double dx, double dy);

	//The following is needed temporarily while CanvasItem is not a QGraphicsItem by itself.
	void setPosition(const QPointF& pos) { const QPointF diff = pos - getPosition(); moveBy(diff.x(), diff.y()); }
	virtual QPointF getPosition() const = 0;

	enum ZBehavior { FixedZValue = 0, IsStrut = 1, IsRaisedByStrut = 2 };
	///This specifies how the object is Z-ordered.
	///\li FixedZValue: No special behavior.
	///\li IsStrut: This item is a vertical strut. It raises certain
	///    items when they move on top of it. Its zValue is \a zValueStep.
	///\li IsRaisedByStrut: This item can be raised by struts underneath
	///    it. \a zValueStep is the amount by which the zValue is raised
	///    then. (i.e. \a zValue is relative to the strut)
	//TODO: account for overlapping struts
	void setZBehavior(ZBehavior behavior, qreal zValue);
	///Struts are normally found by collision detection. This method
	///configures a static strut for this item (on a semantic basis;
	///e.g. the RectangleItem is the static strut for its walls).
	void setStaticStrut(CanvasItem* citem);
	void updateZ(QGraphicsItem* self);
	void moveItemsOnStrut(const QPointF& posDiff);
	static bool mayCollide(CanvasItem* citem1, CanvasItem* citem2);
protected:
	friend class Kolf::Overlay; //for delivery of Kolf::Overlay::stateChanged signal
	///pointer to main KolfGame
	KolfGame *game;
private:
	QString m_name;
	int m_id;
	CanvasItem::ZBehavior m_zBehavior;
	qreal m_zValue;
	CanvasItem* m_strut;
	CanvasItem* m_staticStrut;
	QList<CanvasItem*> m_struttedItems;

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

		b2World* world() const;
		QList<Kolf::Shape*> shapes() const { return m_shapes; }
		Kolf::Overlay* overlay(bool createIfNecessary = true);
		///@return items inside this CanvasItem which shall only be shown when
		///the user toggles additional info. Hide these items by default!
		virtual QList<QGraphicsItem*> infoItems() const { return QList<QGraphicsItem*>(); }

		///This is the velocity used by the physics engine: In each time step,
		///the position of this canvas item changes by the value of this property.
		QPointF velocity() const;
		void setVelocity(const QPointF& velocity);
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
		virtual Kolf::Overlay* createOverlay() { return nullptr; } //TODO: make this pure virtual when all CanvasItems are QGraphicsItems and implement createOverlay() (and then disallow createOverlay() == 0)
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

		bool contains(const QPointF& point) const Q_DECL_OVERRIDE;
		QPainterPath shape() const Q_DECL_OVERRIDE;

		QRectF rect() const;
		double width() const { return Tagaro::SpriteObjectItem::size().width(); }
		double height() const { return Tagaro::SpriteObjectItem::size().height(); }

		void setSize(const QSizeF& size) Q_DECL_OVERRIDE;
		void setSize(qreal width, qreal height) { setSize(QSizeF(width, height)); }
		void moveBy(double x, double y) Q_DECL_OVERRIDE;

		void saveSize(KConfigGroup* group);
		void loadSize(KConfigGroup* group);

		QPointF getPosition() const Q_DECL_OVERRIDE { return QGraphicsItem::pos(); }
	private:
		QGraphicsEllipseItem* m_ellipseItem;
		Kolf::EllipseShape* m_shape;
};

class ArrowItem : public QGraphicsPathItem
{
	public:
		explicit ArrowItem(QGraphicsItem* parent);

		qreal angle() const;
		void setAngle(qreal angle);
		qreal length() const;
		void setLength(qreal length);
		bool isReversed() const;
		void setReversed(bool reversed);

		Vector vector() const;
	private:
		void updatePath();
		qreal m_angle, m_length;
		bool m_reversed;
};

#endif
