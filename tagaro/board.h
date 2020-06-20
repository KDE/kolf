/***************************************************************************
 *   Copyright 2010 Stefan Majewsky <majewsky@gmx.net>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License          *
 *   version 2 as published by the Free Software Foundation                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TAGARO_BOARD_H
#define TAGARO_BOARD_H

#include <QGraphicsObject>

namespace Tagaro {

/**
 * @class Tagaro::Board board.h <Tagaro/Board>
 *
 * The Tagaro::Board is basically a usual QGraphicsItem which can be used to
 * group items. However, it has two special features:
 * @li It can adjust its size automatically to fit into the bounding rect of the
 *     parent item (or the scene rect, if there is no parent item). This
 *     behavior is controlled by the alignment() property.
 * @li When it is resized, it will automatically adjust the renderSize of any
 *     contained Tagaro::SpriteObjectItems.
 */
class Board : public QGraphicsObject
{
	Q_OBJECT
	public:
		///Creates a new Tagaro::Board instance below the given @a parent item.
		///The logicalSize() is initialized to (1,1). The physicalSize() is
		///determined from the parent item's bounding rect by using the default
		///alignment Qt::AlignCenter.
		explicit Board(QGraphicsItem* parent = nullptr);
		///Destroys the Tagaro::Board and all its children.
		virtual ~Board();

		///@return the logical size of this board, i.e. the size of its
		///bounding rect in the item's local coordinates
		QSizeF logicalSize() const;
		///Sets the logical size of this board, i.e. the size of its bounding
		///rect in the item's local coordinates.
		void setLogicalSize(const QSizeF& size);
		///@return the size of this board, i.e. the size of its bounding rect in
		///the parent item's local coordinates
		QSizeF size() const;
		///Sets the size of this board, i.e. the size of its bounding rect in
		///the parent item's local coordinates.
		///@warning Calls to this method will disable automatic alignment by
		///setting the alignment() to 0.
		void setSize(const QSizeF& size);
		///@return the physical size factor @see setPhysicalSizeFactor
		qreal physicalSizeFactor() const;
		///Sets the physical size factor. This factor will be used in the
		///calculation of renderSizes for contained Tagaro::SpriteObjectItems.
		///Leave this at 1.0 (the default) if scene coordinates and viewport
		///coordinates have an equal scale (e.g. Tagaro::Scene and its main
		///view).
		///
		///Values between 0.0 and 1.0 mean that the render size of the item is
		///smaller than its actual size, e.g. because the viewport is smaller
		///is smaller than the scene which contains. If the viewport is bigger,
		///you will have to increase the render sizes with factors above 1.0.
		///
		///So if the viewport renders the board with size @a s in an area of
		///size @a vs, you need to do:
		///@code setPhysicalSizeFactor(vs.width() / s.width()); @endcode
		///Of course, Repeat this whenever that ratio changes.
		void setPhysicalSizeFactor(qreal physicalSizeFactor);

		///@return the alignment of this board in the parent item's bounding
		///rect (or the scene rect, if there is no parent item)
		Qt::Alignment alignment() const;
		///Sets the @a alignment of this board in the parent item's bounding
		///rect (or the scene rect, if there is no parent item). If an alignment
		///is set, changes to the scene rect will cause the board to change its
		///size and location to fit into the parent item. The board keeps its
		///aspect ratio and determines its position from the @a alignment.
		///
		///The default alignment is Qt::AlignCenter. Call this function with
		///argument 0 to disable the alignment behavior.
		///@note The flag Qt::AlignJustify is not interpreted.
		void setAlignment(Qt::Alignment alignment);

		QRectF boundingRect() const Q_DECL_OVERRIDE;
		void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) Q_DECL_OVERRIDE;
	protected:
		QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) Q_DECL_OVERRIDE;
		void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE;
	private:
		struct Private;
		Private* const d;
		Q_PRIVATE_SLOT(d, void _k_update())
		Q_PRIVATE_SLOT(d, void _k_updateItem())
};

} //namespace Tagaro

#endif // TAGARO_BOARD_H
