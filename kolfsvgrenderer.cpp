/*
    Copyright (C) 2006 KDE Game Team
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

#include "kolfsvgrenderer.h"

#include <QPixmapCache>
#include <QPainter>
#include <QSvgRenderer>

KolfSvgRenderer::KolfSvgRenderer(const QString& pathToSvg)
	: m_renderer(new QSvgRenderer(pathToSvg))
{
}

KolfSvgRenderer::~KolfSvgRenderer()
{
	delete m_renderer;
}

QPixmap KolfSvgRenderer::render(const QString& name, const QSize& size, bool useCache)
{
	QPixmap pix;
	//try to obtain from cache
	if (useCache)
		if (QPixmapCache::find(name, &pix))
			return pix;
	//render from SVG
	QImage baseImg = QImage(size, QImage::Format_ARGB32_Premultiplied);
	baseImg.fill(0);
	QPainter p(&baseImg);
	m_renderer->render(&p, name, QRect(QPoint(), size));
	p.end();
	pix = QPixmap::fromImage(baseImg);
	//serve pixmap
	if (useCache)
		QPixmapCache::insert(name, pix);
	return pix;
}
