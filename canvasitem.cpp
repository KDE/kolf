#include <qcanvas.h>

#include <kconfig.h>

#include "game.h"
#include "canvasitem.h"

QCanvasRectangle *CanvasItem::onVStrut()
{
	QCanvasItem *qthis = dynamic_cast<QCanvasItem *>(this);
	if (!qthis)
		return 0;
	QCanvasItemList l = qthis->collisions(true);
	l.sort();
	bool aboveVStrut = false;
	CanvasItem *item = 0;
	QCanvasItem *qitem = 0;
	for (QCanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		item = dynamic_cast<CanvasItem *>(*it);
		if (item)
		{
			qitem = *it;
			if (item->vStrut())
			{
				//kdDebug() << "above vstrut\n";
				aboveVStrut = true;
				break;
			}
		}
	}

	QCanvasRectangle *ritem = dynamic_cast<QCanvasRectangle *>(qitem);

	return aboveVStrut && ritem? ritem : 0;
}

void CanvasItem::save(KConfig *cfg)
{
	cfg->writeEntry("dummykey", true);
}

void CanvasItem::playSound(QString file, double vol)
{
	if (game)
		game->playSound(file, vol);
}

