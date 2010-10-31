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

#ifndef KOLF_SVG_RENDERER_H
#define KOLF_SVG_RENDERER_H

#include <QPixmap>

class QSvgRenderer;

class KolfSvgRenderer
{
public:
	explicit KolfSvgRenderer(const QString& pathToSvg);
	~KolfSvgRenderer();
        QPixmap renderSvg(const QString &cacheName, int width, int height, bool useCache);
        QPixmap renderWithoutCache(const QString &cacheName, int width, int height);
        QPixmap renderWithCache(const QString &cacheName, int width, int height);

private:
	QSvgRenderer *renderer;
};

#endif
