/***************************************************************************
 *   Copyright 2010 Stefan Majewsky <majewsky@gmx.net>
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

#ifndef UTILS_ANIMATEDITEM_H
#define UTILS_ANIMATEDITEM_H

#include <QGraphicsObject>
class QPropertyAnimation;

namespace Utils
{
	/**
	 * \class AnimatedItem
	 * This QGraphicsItem has a simple interface for animated opacity changes.
	 */
	class AnimatedItem : public QGraphicsObject
	{
		Q_OBJECT
		Q_PROPERTY(qreal correct_opacity READ opacity WRITE setOpacity) //I'm talking of AnimatedItem::setOpacity, not QGraphicsItem::setOpacity!
		public:
			explicit AnimatedItem(QGraphicsItem* parent = 0);

			///Returns the duration of opacity animations.
			int animationTime() const;
			///\note This won't have an effect on running animations.
			void setAnimationTime(int time);
			///Returns whether the object's visibility will be set to false when the opacity is reduced to 0.
			bool hideWhenInvisible() const;
			void setHideWhenInvisible(bool hideWhenInvisible);
			void setOpacityAnimated(qreal opacity);
			///\warning Always use this instead of QGraphicsItem::setOpacity for direct opacity adjustments.
			void setOpacity(qreal opacity);

			QRectF boundingRect() const Q_DECL_OVERRIDE;
			void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) Q_DECL_OVERRIDE;
		private:
			int m_animationTime;
			bool m_hideWhenInvisible;
			QPropertyAnimation* m_animator;
	};
}

#endif // KOLF_OVERLAY_P_H
