#ifndef KOLF_SVG_RENDERER_H
#define KOLF_SVG_RENDERER_H

#include <ksvgrenderer.h>
#include <QPixmap>

class KSvgRenderer;

class KolfSvgRenderer
{
public:
	explicit KolfSvgRenderer(const QString& pathToSvg);
	~KolfSvgRenderer();
        QPixmap renderSvg(const QString &cacheName, int width, int height, bool useCache);
        QPixmap renderWithoutCache(const QString &cacheName, int width, int height);
        QPixmap renderWithCache(const QString &cacheName, int width, int height);

private:
	KSvgRenderer *renderer;
};

#endif
