/***************************************************************************
 *   Copyright 2008, 2009, 2010 Stefan Majewsky <majewsky@gmx.net>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ***************************************************************************/

#ifndef KOLF_OVERLAY_H
#define KOLF_OVERLAY_H

#include <QGraphicsItem>

namespace Utils
{
	class AnimatedItem;
}

class CanvasItem;

namespace Kolf
{
	//This can be used by Kolf::Overlay subclasses for modifying the physical appearance of an object.
	class OverlayHandle : public QObject, public QGraphicsPathItem
	{
		Q_OBJECT
		public:
			enum Shape
			{
				SquareShape,
				CircleShape,
				TriangleShape
			};

			OverlayHandle(Shape shape, QGraphicsItem* parent);
		Q_SIGNALS:
			void moveStarted();
			void moveRequest(const QPointF& targetScenePos);
			void moveEnded();
		protected:
			virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
			virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
			virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
			virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
			virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	};

	//This is used by Kolf::Overlay to paint the various outlines of an item.
	class OverlayAreaItem : public QObject, public QGraphicsPathItem
	{
		Q_OBJECT
		public:
			enum Feature
			{
				Draggable = 1 << 0,
				Clickable = 1 << 1,
				Hoverable = 1 << 2
			};
			Q_DECLARE_FLAGS(Features, Feature)

			OverlayAreaItem(Features features, QGraphicsItem* parent);
		Q_SIGNALS:
			void clicked(int button);
			void dragged(const QPointF& distance);
			void hoverEntered();
			void hoverLeft();
		protected:
			virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
			virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
			virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
			virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
			virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
		private:
			Features m_features;
	};

	/**
	 * \class Overlay
	 * \since 2.0
	 *
	 * Overlays are used by the editor to provide the on-screen editing interface. The most wellknown overlay is probably the rectangle with grips at every corner and edge. Overlays react on changes in the property set of their object and change specific properties through user interaction.
	 *
	 * By default, the Overlay renders interaction and activation outlines and allows to change the object's position; subclasses have to add handles to manipulate geometrical properties of the object. (This class may become pure virtual in the future, we therefore recommend to not instance this class. If you want an overlay without handles, use Kolf::EmptyOverlay instead.)
	 *
	 * \warning Do not add any handles or additional interface items through setParentItem() et al. Use the addHandle() method to ensure that handles are correctly shown and hidden.
	 * \sa Kolf::Object
	 */
	class Overlay : public QObject, public QGraphicsItem
	{
		Q_OBJECT
		Q_INTERFACES(QGraphicsItem)
		public:
			///This enum specifies states that an overlay can occupy.
			enum State
			{
				Passive = 0, ///< The overlay is invisible (not as in isVisible() == false, but all components are transparent or invisible).
				Hovered = 1, ///< The overlay's activation area has been hovered, and the interactor area is shown (though very translucent).
				Active = 2   ///< The interaction area is shown at a high opacity, and the overlay's handles are visible.
			};

			Overlay(CanvasItem* citem, QGraphicsItem* qitem);
			///Returns a reference to the CanvasItem that this overlay is associated with.
			CanvasItem* citem() const;
			///Returns a reference to the QGraphicsItem that is the CanvasItem that this overlay is associated with.
			QGraphicsItem* qitem() const;

			///Returns the current state of this overlay.
			State state() const;
			///Performs a state transition from the current towards the given \a state.
			void setState(State state);
			///Lets the overlay initialize its properties or react on property changes in the base object. Subclasses of Kolf::Overlay need to reimplement this to read all important properties of their object (aside from calling the base class implementation!).
			virtual void update();

			///Overlays should not allow to decrease an object's dimensions below this level, for the sake of usability.
			static const qreal MinimumObjectDimension;

			virtual QRectF boundingRect() const;
			virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
		Q_SIGNALS:
			///This signal is emitted if the overlay's state changes.
			void stateChanged();
		protected:
			///Adds a handle to the overlay interface. Handles are only shown if the overlay is visible and activated.
			void addHandle(QGraphicsItem* handle);
		private Q_SLOTS:
			void activatorEntered();
			void activatorLeft();
			void activatorClicked(int button);
			void interactorDragged(const QPointF& distance);
		private:
			CanvasItem* m_citem;
			QGraphicsItem* m_qitem;
			Kolf::Overlay::State m_state;
			//pre-defined handles and area items
			Kolf::OverlayAreaItem* m_activatorItem;
			Utils::AnimatedItem* m_interactorAnimator;
			Kolf::OverlayAreaItem* m_interactorItem;
			Utils::AnimatedItem* m_handleAnimator;
	};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Kolf::OverlayAreaItem::Features)

#endif // KOLF_OVERLAY_H
