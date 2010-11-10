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

#ifndef KOLF_OBJECTS_H
#define KOLF_OBJECTS_H

//NOTE: Only refactored stuff goes into this header.

#include "canvasitem.h"
#include "overlay.h"

namespace Kolf
{
	class BlackHole;
	class BlackHoleOverlay;

	class BlackHoleExit : public QGraphicsLineItem, public CanvasItem
	{
		public:
			BlackHoleExit(Kolf::BlackHole* blackHole, QGraphicsItem* parent, b2World* world);
			virtual void moveBy(double dx, double dy);
			virtual bool deleteable() const { return false; }
			virtual bool canBeMovedByOthers() const { return true; }
			virtual void setPen(const QPen& p);
			virtual QList<QGraphicsItem*> infoItems() const;
			void updateArrowAngle();
			void updateArrowLength();
			virtual Config* config(QWidget* parent);

			virtual QPointF getPosition() const { return QGraphicsItem::pos(); }
		protected:
			BlackHole* m_blackHole;
			friend class Kolf::BlackHoleOverlay;
			ArrowItem* m_arrow;
	};

	class BlackHoleTimer : public QObject
	{
		Q_OBJECT
		public:
			BlackHoleTimer(Ball* ball, double speed, int msec);
		Q_SIGNALS:
			void eject(Ball* ball, double speed);
			void halfway();
		private Q_SLOTS:
			void emitEject();
		private:
			double m_speed;
			Ball* m_ball;
	};

	class BlackHole : public EllipticalCanvasItem
	{
		Q_OBJECT

		public:
			BlackHole(QGraphicsItem* parent, b2World* world);
			virtual bool canBeMovedByOthers() const { return true; }

			virtual void aboutToDie();
			virtual QList<QGraphicsItem*> infoItems() const;
			virtual void save(KConfigGroup* cfgGroup);
			virtual void load(KConfigGroup* cfgGroup);
			virtual Config* config(QWidget* parent);
			virtual QList<QGraphicsItem*> moveableItems() const;
			double minSpeed() const;
			double maxSpeed() const;
			void setMinSpeed(double news);
			void setMaxSpeed(double news);

			int curExitDeg() const;
			void setExitDeg(int newdeg);

			void updateInfo();

			virtual void moveBy(double dx, double dy);

			virtual void shotStarted();
			virtual bool collision(Ball* ball);
		public slots:
			void eject(Ball* ball, double speed);
			void halfway();
		protected:
			virtual Kolf::Overlay* createOverlay();

			int m_exitDeg;
			friend class Kolf::BlackHoleOverlay;
			BlackHoleExit* m_exitItem;
			double m_minSpeed;
			double m_maxSpeed;

		private:
			int m_runs;
			QGraphicsLineItem* m_infoLine;
			void finishMe();
	};

	class BlackHoleConfig : public Config
	{
		Q_OBJECT
		public:
			BlackHoleConfig(BlackHole* blackHole, QWidget* parent);
		private Q_SLOTS:
			void degChanged(int);
			void minChanged(double);
			void maxChanged(double);
		private:
			BlackHole* m_blackHole;
	};

	class BlackHoleOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			BlackHoleOverlay(Kolf::BlackHole* blackHole);
			virtual void update();
		private Q_SLOTS:
			//interface to handles
			void moveHandle(const QPointF& handleScenePos);
		private:
			QGraphicsLineItem* m_exitIndicator;
			Kolf::OverlayHandle* m_exitHandle;
			Kolf::OverlayHandle* m_speedHandle;
	};

	class Cup : public EllipticalCanvasItem
	{
		public:
			Cup(QGraphicsItem* parent, b2World* world);

			virtual Kolf::Overlay* createOverlay();
			virtual bool canBeMovedByOthers() const { return true; }
			virtual bool collision(Ball* ball);
	};
}

#endif // KOLF_OBJECTS_H
