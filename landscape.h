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
#include <config.h>
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
			void advance(int phase) Q_DECL_OVERRIDE;

			void load(KConfigGroup* group) Q_DECL_OVERRIDE;
			void save(KConfigGroup* group) Q_DECL_OVERRIDE;

			Config* config(QWidget* parent) Q_DECL_OVERRIDE;
		public Q_SLOTS:
			void setBlinkEnabled(bool blinkEnabled);
			void setBlinkInterval(int blinkInterval);
		protected:
			Kolf::Overlay* createOverlay() Q_DECL_OVERRIDE;
		private:
			bool m_blinkEnabled;
			int m_blinkInterval, m_blinkFrame;
	};

	class LandscapeOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			explicit LandscapeOverlay(Kolf::LandscapeItem* item);
			void update() Q_DECL_OVERRIDE;
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
			bool collision(Ball* ball) Q_DECL_OVERRIDE;
	};

	class Sand : public Kolf::LandscapeItem
	{
		public:
			Sand(QGraphicsItem* parent, b2World* world);
			bool collision(Ball* ball) Q_DECL_OVERRIDE;
	};

	enum SlopeType
	{
		VerticalSlope = 0,
		HorizontalSlope,
		DiagonalSlope,
		CrossDiagonalSlope,
		EllipticSlope
	};

	class Slope : public Tagaro::SpriteObjectItem, public CanvasItem
	{
		Q_OBJECT
		public:
			Slope(QGraphicsItem* parent, b2World* world);

			double grade() const;
			bool isReversed() const;
			Kolf::SlopeType slopeType() const;
			bool isStuckOnGround() const;

			QPainterPath shape() const Q_DECL_OVERRIDE;
			void setSize(const QSizeF& size) Q_DECL_OVERRIDE;
			QPointF getPosition() const Q_DECL_OVERRIDE;
			void moveBy(double dx, double dy) Q_DECL_OVERRIDE;

			void load(KConfigGroup* group) Q_DECL_OVERRIDE;
			void save(KConfigGroup* group) Q_DECL_OVERRIDE;

			bool collision(Ball* ball) Q_DECL_OVERRIDE;
			bool terrainCollisions() const Q_DECL_OVERRIDE;
			QList<QGraphicsItem*> infoItems() const Q_DECL_OVERRIDE;
			Config* config(QWidget* parent) Q_DECL_OVERRIDE;
		public Q_SLOTS:
			void setGrade(double grade);
			void setReversed(bool reversed);
			void setSlopeType(int type);
			void setStuckOnGround(bool stuckOnGround);
		protected:
			Kolf::Overlay* createOverlay() Q_DECL_OVERRIDE;
		private:
			void updateAppearance();
			void updateInfo();

			double m_grade;
			bool m_reversed, m_stuckOnGround;
			Kolf::SlopeType m_type;

			QGraphicsSimpleTextItem* m_gradeItem;
			QList<ArrowItem*> m_arrows;
	};

	class SlopeConfig : public Config
	{
		public:
			SlopeConfig(Kolf::Slope* slope, QWidget* parent);
	};

	class SlopeOverlay : public Kolf::Overlay
	{
		Q_OBJECT
		public:
			explicit SlopeOverlay(Kolf::Slope* slope);
			void update() Q_DECL_OVERRIDE;
		private Q_SLOTS:
			//interface to handles
			void moveHandle(const QPointF& handleScenePos);
		private:
			QList<Kolf::OverlayHandle*> m_handles;
	};
}

#endif // KOLF_LANDSCAPE_H
