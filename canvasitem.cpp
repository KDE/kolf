#include <q3canvas.h>

#include <kconfig.h>

#include "game.h"
#include "canvasitem.h"

Q3CanvasRectangle *CanvasItem::onVStrut()
{
	Q3CanvasItem *qthis = dynamic_cast<Q3CanvasItem *>(this);
	if (!qthis)
		return 0;
	Q3CanvasItemList l = qthis->collisions(true);
	l.sort();
	bool aboveVStrut = false;
	CanvasItem *item = 0;
	Q3CanvasItem *qitem = 0;
	for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		item = dynamic_cast<CanvasItem *>(*it);
		if (item)
		{
			qitem = *it;
			if (item->vStrut())
			{
				//kDebug(12007) << "above vstrut\n";
				aboveVStrut = true;
				break;
			}
		}
	}

	Q3CanvasRectangle *ritem = dynamic_cast<Q3CanvasRectangle *>(qitem);

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

