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

#include "scene.h"
#include "scene_p.h"

#include <QEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>

Tagaro::Scene::Private::Private(KGameRenderer* backgroundRenderer, const QString& backgroundSpriteKey, Tagaro::Scene* parent)
	: KGameRendererClient(backgroundRenderer, backgroundSpriteKey)
	, m_parent(parent)
	, m_mainView(0)
	, m_renderSize() //constructed with invalid size (as documented)
	, m_adjustingSceneRect(false)
{
	connect(parent, SIGNAL(sceneRectChanged(QRectF)), parent, SLOT(_k_updateSceneRect(QRectF)));
}

Tagaro::Scene::Scene(QObject* parent)
	: QGraphicsScene(parent)
	, d(new Private(0, QString(), this))
{
}

Tagaro::Scene::Scene(KGameRenderer* backgroundRenderer, const QString& backgroundSpriteKey, QObject* parent)
	: QGraphicsScene(parent)
	, d(new Private(backgroundRenderer, backgroundSpriteKey, this))
{
}

Tagaro::Scene::~Scene()
{
	delete d;
}

//BEGIN scene rect stuff

QGraphicsView* Tagaro::Scene::mainView() const
{
	return d->m_mainView;
}

void Tagaro::Scene::setMainView(QGraphicsView* mainView)
{
	if (d->m_mainView == mainView)
	{
		return;
	}
	//remove connections to old main view
	if (d->m_mainView)
	{
		d->m_mainView->removeEventFilter(this);
	}
	//connect to new main view
	if ((d->m_mainView = mainView))
	{
		d->m_mainView->setScene(this);
		d->m_mainView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		d->m_mainView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		d->_k_resetSceneRect();
		d->m_mainView->installEventFilter(this);
	}
}

bool Tagaro::Scene::Private::_k_resetSceneRect()
{
	//force correct scene rect if necessary
	if (m_mainView && !m_adjustingSceneRect)
	{
		m_adjustingSceneRect = true;
		m_parent->setSceneRect(m_mainView->rect());
		m_mainView->setTransform(QTransform());
		m_adjustingSceneRect = false;
		return true;
	}
	return false;
}

bool Tagaro::Scene::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == d->m_mainView && event->type() == QEvent::Resize)
	{
		d->_k_resetSceneRect();
	}
	return QGraphicsScene::eventFilter(watched, event);
}

void Tagaro::Scene::Private::_k_updateSceneRect(const QRectF& rect)
{
	if (!_k_resetSceneRect())
	{
		//The condition ensures that updateRenderSize() gets only called once
		//with the correct scene rect if the scene rect has to be reset.
		updateRenderSize(rect.size().toSize());
	}
}

//END scene rect stuff
//BEGIN background brush stuff

KGameRendererClient* Tagaro::Scene::backgroundBrushClient() const
{
	return d;
}

QSize Tagaro::Scene::backgroundBrushRenderSize() const
{
	return d->m_renderSize;
}

void Tagaro::Scene::setBackgroundBrushRenderSize(const QSize& size)
{
	if (d->m_renderSize != size)
	{
		d->m_renderSize = size;
		d->updateRenderSize(sceneRect().size().toSize());
	}
}

void Tagaro::Scene::Private::updateRenderSize(const QSize& sceneSize)
{
	setRenderSize(m_renderSize.isValid() ? m_renderSize : sceneSize);
}

void Tagaro::Scene::Private::receivePixmap(const QPixmap& pixmap)
{
	m_parent->setBackgroundBrush(pixmap);
}

//END background brush stuff
#include "moc_scene.cpp"

