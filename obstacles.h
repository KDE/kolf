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

#include <config.h>
#include "canvasitem.h"
#include "overlay.h"

#include <QPen>

class QCheckBox;
class QGridLayout;

namespace Kolf
{
	class LineShape;
	class RectShape;

	class Bumper : public EllipticalCanvasItem
	{
		Q_OBJECT
		public:
			Bumper(QGraphicsItem* parent, b2World* world);

			bool collision(Ball* ball) override;
		protected:
			Kolf::Overlay* createOverlay() override;
		public Q_SLOTS:
			void turnBumperOff();
	};

	class Wall : public QGraphicsLineItem, public CanvasItem
	{
		public:
			Wall(QGraphicsItem* parent, b2World* world);

			void load(KConfigGroup* cfgGroup) override;
			void save(KConfigGroup* cfgGroup) override;
			void setVisible(bool visible);

			virtual void setLine(const QLineF& line);
			void moveBy(double dx, double dy) override;
			QPointF getPosition() const override;
		protected:
			Kolf::Overlay* createOverlay() override;
		private:
			Kolf::LineShape* m_shape;
	};

	class WallOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			explicit WallOverlay(Kolf::Wall* wall);
			void update() override;
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
		Q_OBJECT
		public:
			RectangleItem(const QString& type, QGraphicsItem* parent, b2World* world);
			~RectangleItem() override;

			bool hasWall(Kolf::WallIndex index) const;
			bool isWallAllowed(Kolf::WallIndex index) const;
			void setWall(Kolf::WallIndex index, bool hasWall);
			void setWallAllowed(Kolf::WallIndex index, bool wallAllowed);
			void setSize(const QSizeF& size) override;
			QPointF getPosition() const override;
			void moveBy(double dx, double dy) override;

			void setWallColor(const QColor& color);
			void applyWallStyle(Kolf::Wall* wall, bool adjustPainting = true);

			void load(KConfigGroup* group) override;
			void save(KConfigGroup* group) override;

			Config* config(QWidget* parent) override;
		Q_SIGNALS:
			void wallChanged(Kolf::WallIndex index, bool hasWall, bool wallAllowed);
		protected:
			Kolf::Overlay* createOverlay() override;
			virtual void updateWallPosition();
		private:
			QPen m_wallPen;
			QList<bool> m_wallAllowed;
			QList<Kolf::Wall*> m_walls;
			Kolf::RectShape* m_shape;
	};

	class RectangleOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			explicit RectangleOverlay(Kolf::RectangleItem* item);
			void update() override;
		private Q_SLOTS:
			//interface to handles
			void moveHandle(const QPointF& handleScenePos);
		private:
			QList<Kolf::OverlayHandle*> m_handles;
	};

	class RectangleConfig : public Config
	{
		Q_OBJECT
		public:
			RectangleConfig(Kolf::RectangleItem* item, QWidget* parent);
		protected Q_SLOTS:
			void setWall(bool hasWall);
			void wallChanged(Kolf::WallIndex index, bool hasWall, bool wallAllowed);
		protected:
			QGridLayout* m_layout;
			QList<QCheckBox*> m_wallCheckBoxes;
			Kolf::RectangleItem* const m_item;
	};

	class Bridge : public Kolf::RectangleItem
	{
		public:
			Bridge(QGraphicsItem* parent, b2World* world);
			bool collision(Ball* ball) override;
	};

	class Floater : public Kolf::RectangleItem
	{
		Q_OBJECT
		public:
			Floater(QGraphicsItem* parent, b2World* world);
			void editModeChanged(bool changed) override;
			void moveBy(double dx, double dy) override;

			QLineF motionLine() const;
			void setMotionLine(const QLineF& motionLine);
			int speed() const;
			void advance(int phase) override;

			void load(KConfigGroup* group) override;
			void save(KConfigGroup* group) override;
		public Q_SLOTS:
			void setSpeed(int speed);
		protected:
			Kolf::Overlay* createOverlay() override;
		private:
			void setMlPosition(qreal position);

			QLineF m_motionLine;
			int m_speed;
			qreal m_velocity;
			qreal m_position; //parameter on motion line (see QLineF::pointAt)
			bool m_moveByMovesMotionLine, m_animated;
	};

	class FloaterOverlay : public Kolf::RectangleOverlay
	{
		Q_OBJECT
		public:
			explicit FloaterOverlay(Kolf::Floater* floater);
			void update() override;
		private Q_SLOTS:
			//interface to handles
			void moveMotionLineHandle(const QPointF& handleScenePos);
		private:
			Kolf::OverlayHandle* m_handle1;
			Kolf::OverlayHandle* m_handle2;
			QGraphicsLineItem* m_motionLineItem;
	};

	class Sign : public Kolf::RectangleItem
	{
		Q_OBJECT
		public:
			Sign(QGraphicsItem* parent, b2World* world);

			QString text() const;
			void setSize(const QSizeF& size) override;

			void load(KConfigGroup* group) override;
			void save(KConfigGroup* group) override;
		public Q_SLOTS:
			void setText(const QString& text);
		private:
			QString m_text;
			QGraphicsTextItem* m_textItem;
	};

	class Windmill : public Kolf::RectangleItem
	{
		Q_OBJECT
		public:
			Windmill(QGraphicsItem* parent, b2World* world);
			~Windmill() override;

			bool guardAtTop() const;
			int speed() const;
			void advance(int phase) override;
			void moveBy(double dx, double dy) override;

			void load(KConfigGroup* group) override;
			void save(KConfigGroup* group) override;
		public Q_SLOTS:
			void setGuardAtTop(bool guardAtTop);
			void setSpeed(int speed);
		protected:
			void updateWallPosition() override;
		private:
			Kolf::Wall* m_leftWall;
			Kolf::Wall* m_rightWall;
			Kolf::Wall* m_guardWall;
			bool m_guardAtTop;
			int m_speed;
			double m_velocity;
	};
}

#endif // KOLF_OBSTACLES_H
