/*
    Copyright 2008-2010 Stefan Majewsky <majewsky@gmx.net>

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

#ifndef KOLF_SHAPE_H
#define KOLF_SHAPE_H

#include <QPainterPath>
class CanvasItem;

namespace Kolf
{
	/**
	 * @class Kolf::Shape
	 *
	 * This class represents a part of the shape of an item. Simple objects can be represented by a single shape (e.g. boxes and spheres), while complex objects may have to be constructed with multiple shapes.
	 */
	class Shape
	{
		Q_DISABLE_COPY(Shape)
		public:
			///@warning Any subclass constructor *must* call update() before it exits.
			Shape();
			virtual ~Shape();

			///Returns the two-dimensional activation outline, i.e. the area of this geometry (plus some defined padding). The editor overlay is supposed to activate the editor interface of the object associated with this geometry, if a mouse click occurs inside the activation outline.
			///@see ActivationOutlinePadding
			QPainterPath activationOutline() const;
			///Returns the two-dimensional interaction outline, i.e. the exact outline of this geometry. The editor overlay is supposed to move the object associated with this geometry, if the mouse is clicked inside the interaction outline and then dragged.
			///@note The interaction outline is allowed to be greater than the object's exact shape for the sake of usability (e.g. a diagonal wall cannot be precisely hit with a mouse cursor).
			QPainterPath interactionOutline() const;
		protected:
			friend class ::CanvasItem; //for the following method
			///Binds this shape to the given @a item. Will fail if this shape is already bound to some item.
			bool attach(CanvasItem* item);

			///Call this method when the parameters of this geometry change (usually in setter methods of the subclass).
			void update();
			///Reimplement this method to create the outlines of this geometry and pass them to the caller via the arguments. You will not have to call this function in subclass implementations, it's invoked by Kolf::Geometry::update.
			virtual void createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline) = 0;
			///Use this padding as distance between the exact InteractionOutline and the fuzzy ActivationOutline.
			static const qreal ActivationOutlinePadding;
		private:
			CanvasItem* m_citem;
			QPainterPath m_activationOutline, m_interactionOutline;
	};

	class EllipseShape : public Kolf::Shape
	{
		public:
			EllipseShape(const QRectF& rect);

			QRectF rect() const;
			void setRect(const QRectF& rect);
		protected:
			virtual void createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline);
		private:
			QRectF m_rect;
	};

	class RectShape : public Kolf::Shape
	{
		public:
			RectShape(const QRectF& rect);

			QRectF rect() const;
			void setRect(const QRectF& rect);
		protected:
			virtual void createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline);
		private:
			QRectF m_rect;
	};

	class LineShape : public Kolf::Shape
	{
		public:
			LineShape(const QLineF& line);

			QLineF line() const;
			void setLine(const QLineF& line);
		protected:
			virtual void createOutlines(QPainterPath& activationOutline, QPainterPath& interactionOutline);
		private:
			QLineF m_line;
	};
}

#endif // KOLF_SHAPE_H
