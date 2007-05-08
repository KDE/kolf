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
		QPixmap pix = QPixmap::fromImage(baseImg);
		QPixmapCache::insert(name, pix);
	}

	QPixmap pix;
	QPixmapCache::find(name, pix);
	return pix;
}

