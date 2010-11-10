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

#ifndef KOLF_OBSTACLES_H
#define KOLF_OBSTACLES_H

//NOTE: Only refactored stuff goes into this header.

#include "config.h"
#include "canvasitem.h"
#include "overlay.h"

namespace Kolf
{
	class LineShape;

	class Bumper : public EllipticalCanvasItem
	{
		Q_OBJECT
		public:
			Bumper(QGraphicsItem* parent, b2World* world);

			virtual bool collision(Ball* ball);
		protected:
			virtual Kolf::Overlay* createOverlay();
		public Q_SLOTS:
			void turnBumperOff();
	};

	class Wall : public QGraphicsLineItem, public CanvasItem
	{
		public:
			Wall(QGraphicsItem* parent, b2World* world);

			virtual void load(KConfigGroup* cfgGroup);
			virtual void save(KConfigGroup* cfgGroup);
			void setVisible(bool visible);
			void doAdvance();

			virtual void setLine(const QLineF& line);
			virtual void moveBy(double dx, double dy);
			virtual QPointF getPosition() const;
		protected:
			virtual Kolf::Overlay* createOverlay();
		private:
			Kolf::LineShape* m_shape;
	};

	class WallOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			WallOverlay(Kolf::Wall* wall);
			virtual void update();
		private Q_SLOTS:
			//interface to handles
			void moveHandle(const QPointF& handleScenePos);
		private:
			Kolf::OverlayHandle* m_handle1;
			Kolf::OverlayHandle* m_handle2;
	};

	//WARNING: WallIndex and WallFlag have to stay in sync like they are now!
	enum WallIndex {
		TopWallIndex = 0,
		LeftWallIndex = 1,
		RightWallIndex = 2,
		BottomWallIndex = 3,
		RectangleWallCount = 4
	};

	class RectangleItem : public Tagaro::SpriteObjectItem, public CanvasItem
	{
		public:
			RectangleItem(const QString& type, QGraphicsItem* parent, b2World* world);
			virtual ~RectangleItem();

			virtual bool vStrut() const { return true; }

			bool hasWall(Kolf::WallIndex index) const;
			void setWall(Kolf::WallIndex index, bool hasWall);
			virtual void setSize(const QSizeF& size);
			virtual QPointF getPosition() const;
			virtual void moveBy(double dx, double dy); 

			void setWallColor(const QColor& color);
			void applyWallStyle(Kolf::Wall* wall);
			void setZValue(qreal zValue);

			virtual void load(KConfigGroup* group);
			virtual void save(KConfigGroup* group);

			virtual Config* config(QWidget* parent);
		protected:
			virtual Kolf::Overlay* createOverlay();
		private:
			void updateWallPosition();

			QPen m_wallPen;
			QVector<Kolf::Wall*> m_walls;
	};

	class Bridge : public Kolf::RectangleItem
	{
		public:
			Bridge(QGraphicsItem* parent, b2World* world);
			virtual bool collision(Ball* ball);
	};
}

#endif // KOLF_OBSTACLES_H
