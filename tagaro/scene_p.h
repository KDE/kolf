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

#ifndef TAGARO_SCENE_P_H
#define TAGARO_SCENE_P_H

#include "scene.h"
#include <KGameRendererClient>

struct Tagaro::Scene::Private : public KGameRendererClient
{
	public:
		Private(KGameRenderer* backgroundRenderer, const QString& backgroundSpriteKey, Tagaro::Scene* parent);

		//Returns whether sceneRect() was reset to mainView->rect().
		bool _k_resetSceneRect();
		void _k_updateSceneRect(const QRectF& rect);
		inline void updateRenderSize(const QSize& sceneSize);

		Tagaro::Scene* m_parent;
		QGraphicsView* m_mainView;
		QSize m_renderSize;
		bool m_adjustingSceneRect;
	protected:
		virtual void receivePixmap(const QPixmap& pixmap);
};

#endif // TAGARO_SCENE_P_H
