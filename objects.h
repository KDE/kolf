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
#include "game.h" //TODO: remove

namespace Kolf
{
	class BlackHole;
	class BlackHoleExit : public QGraphicsLineItem, public CanvasItem
	{
		public:
			BlackHoleExit(Kolf::BlackHole* blackHole, QGraphicsItem* parent, b2World* world);
			virtual void moveBy(double dx, double dy);
			virtual bool deleteable() const { return false; }
			virtual bool canBeMovedByOthers() const { return true; }
			virtual void editModeChanged(bool editing);
			virtual void setPen(const QPen& p);
			virtual void showInfo();
			virtual void hideInfo();
			void updateArrowAngle();
			void updateArrowLength();
			virtual Config* config(QWidget* parent);

			virtual QPointF getPosition() const { return QGraphicsItem::pos(); }
		protected:
			BlackHole* m_blackHole;
			ArrowItem* m_arrow;
	};

	class BlackHoleTimer : public QObject
	{
		Q_OBJECT

		public:
			BlackHoleTimer(Ball* ball, double speed, int msec);

		signals:
			void eject(Ball* ball, double speed);
			void halfway();

		protected slots:
			void mySlot();
			void myMidSlot();

		protected:
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
			virtual void showInfo();
			virtual void hideInfo();
			virtual void save(KConfigGroup* cfgGroup);
			virtual void load(KConfigGroup* cfgGroup);
			virtual Config* config(QWidget* parent);
			virtual QList<QGraphicsItem*> moveableItems() const;
			double minSpeed() const { return m_minSpeed; }
			double maxSpeed() const { return m_maxSpeed; }
			void setMinSpeed(double news);
			void setMaxSpeed(double news);

			int curExitDeg() const { return exitDeg; }
			void setExitDeg(int newdeg);

			virtual void editModeChanged(bool editing);
			void updateInfo();

			virtual void shotStarted() { runs = 0; }

			virtual void moveBy(double dx, double dy);

			virtual bool collision(Ball* ball);

		public slots:
			void eject(Ball* ball, double speed);
			void halfway();

		protected:
			int exitDeg;
			BlackHoleExit* exitItem;
			double m_minSpeed;
			double m_maxSpeed;

		private:
			int runs;
			QGraphicsLineItem* infoLine;
			void finishMe();
	};

	class BlackHoleConfig : public Config
	{
		Q_OBJECT
		public:
			BlackHoleConfig(BlackHole* blackHole, QWidget* parent);

		private slots:
			void degChanged(int);
			void minChanged(double);
			void maxChanged(double);

		private:
			BlackHole* m_blackHole;
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
