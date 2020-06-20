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

	class BlackHole : public EllipticalCanvasItem
	{
		Q_OBJECT

		public:
			BlackHole(QGraphicsItem* parent, b2World* world);
			~BlackHole() override;
			//FIXME: strutted moving of exit is broken since refactoring.

			QList<QGraphicsItem*> infoItems() const Q_DECL_OVERRIDE;
			void save(KConfigGroup* cfgGroup) Q_DECL_OVERRIDE;
			void load(KConfigGroup* cfgGroup) Q_DECL_OVERRIDE;
			Config* config(QWidget* parent) Q_DECL_OVERRIDE;
			double minSpeed() const;
			double maxSpeed() const;
			void setMinSpeed(double news);
			void setMaxSpeed(double news);

			QPointF exitPos() const;
			void setExitPos(const QPointF& exitPos);
			int curExitDeg() const;
			void setExitDeg(int newdeg);
			Vector exitDirection() const; //for overlay

			void updateInfo();

			void moveBy(double dx, double dy) Q_DECL_OVERRIDE;

			void shotStarted() Q_DECL_OVERRIDE;
			bool collision(Ball* ball) Q_DECL_OVERRIDE;
		public Q_SLOTS:
			void eject(Ball* ball, double speed);
			void halfway();
		protected:
			Kolf::Overlay* createOverlay() Q_DECL_OVERRIDE;
		private:
			double m_minSpeed, m_maxSpeed;
			int m_runs, m_exitDeg;
			QGraphicsLineItem* m_exitItem;
			ArrowItem* m_directionItem;
			QGraphicsLineItem* m_infoLine;
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
			explicit BlackHoleOverlay(Kolf::BlackHole* blackHole);
			void update() Q_DECL_OVERRIDE;
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

			Kolf::Overlay* createOverlay() Q_DECL_OVERRIDE;
			bool collision(Ball* ball) Q_DECL_OVERRIDE;
	};
}

#endif // KOLF_OBJECTS_H
