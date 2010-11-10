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

#ifndef KOLF_LANDSCAPE_H
#define KOLF_LANDSCAPE_H

//NOTE: Only refactored stuff goes into this header.

#include "canvasitem.h"
#include "config.h"
#include "overlay.h"

namespace Kolf
{
	class LandscapeItem : public EllipticalCanvasItem
	{
		Q_OBJECT
		public:
			LandscapeItem(const QString& type, QGraphicsItem* parent, b2World* world);

			bool isBlinkEnabled() const;
			int blinkInterval() const;
			virtual void advance(int phase);

			virtual void load(KConfigGroup* group);
			virtual void save(KConfigGroup* group);
			
			virtual Config* config(QWidget* parent);
		public Q_SLOTS:
			void setBlinkEnabled(bool blinkEnabled);
			void setBlinkInterval(int blinkInterval);
		protected:
			virtual Kolf::Overlay* createOverlay();
		private:
			bool m_blinkEnabled;
			int m_blinkInterval, m_blinkFrame;
	};

	class LandscapeOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			LandscapeOverlay(Kolf::LandscapeItem* item);
			virtual void update();
		private Q_SLOTS:
			//interface to handles
			void moveHandle(const QPointF& handleScenePos);
		private:
			QList<Kolf::OverlayHandle*> m_handles;
	};

	class LandscapeConfig : public Config
	{
		Q_OBJECT
		public:
			LandscapeConfig(Kolf::LandscapeItem* item, QWidget* parent);
		Q_SIGNALS:
			void blinkIntervalChanged(int blinkInterval);
		public Q_SLOTS:
			void setBlinkInterval(int sliderValue);
	};

	class Puddle : public Kolf::LandscapeItem
	{
		public:
			Puddle(QGraphicsItem* parent, b2World* world);
			virtual bool collision(Ball* ball);
	};

	class Sand : public Kolf::LandscapeItem
	{
		public:
			Sand(QGraphicsItem* parent, b2World* world);
			virtual bool collision(Ball* ball);
	};
}

#endif // KOLF_LANDSCAPE_H
