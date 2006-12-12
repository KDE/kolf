#ifndef KOLF_SVG_RENDERER_H
#define KOLF_SVG_RENDERER_H

class KSvgRenderer;

class KolfSvgRenderer
{
public:
	explicit KolfSvgRenderer(const QString& pathToSvg);
	~KolfSvgRenderer();
	QPixmap renderSvg(QString cacheName, int width, int height, bool useCache);
	QPixmap renderWithoutCache(QString cacheName, int width, int height);
	QPixmap renderWithCache(QString cacheName, int width, int height);

private:
	KSvgRenderer *renderer;
};

#endif
