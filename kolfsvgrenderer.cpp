/*
    Copyright (C) 2006 KDE Game Team

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

#include <ksvgrenderer.h>
#include <QPixmap>
#include <QPixmapCache>
#include <QPainter>

KolfSvgRenderer::KolfSvgRenderer(const QString& pathToSvg)
{
	renderer = new KSvgRenderer(pathToSvg);
}

KolfSvgRenderer::~KolfSvgRenderer()
{
	delete renderer;
}

QPixmap KolfSvgRenderer::renderSvg(const QString &name, int width, int height, bool useCache)
{
	QPixmap pix;

	if(useCache)
		pix=renderWithCache(name, width, height);
	else
		pix=renderWithoutCache(name, width, height);

	return pix;
}

QPixmap KolfSvgRenderer::renderWithoutCache(const QString &name, int width, int height)
{
	QImage baseImg = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
	baseImg.fill(0);
	QPainter p(&baseImg);
	renderer->render(&p, name, QRectF(0, 0, width, height));
	p.end();
	QPixmap pix = QPixmap::fromImage(baseImg);

	return pix;
}

QPixmap KolfSvgRenderer::renderWithCache(const QString &name, int width, int height)
{
	if(!QPixmapCache::find(name))
	{
		QImage baseImg = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
		baseImg.fill(0);
		QPainter p(&baseImg);
		renderer->render(&p, name, QRectF(0, 0, width, height));
		p.end();
		QPixmap pix = QPixmap::fromImage(baseImg);
		QPixmapCache::insert(name, pix);
	}

	QPixmap pix;
	QPixmapCache::find(name, pix);
	return pix;
}

