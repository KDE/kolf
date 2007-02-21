#include <QGraphicsView>

#include <kconfig.h>

#include "game.h"
#include "canvasitem.h"

QGraphicsRectItem *CanvasItem::onVStrut()
{
	QGraphicsItem *qthis = dynamic_cast<QGraphicsItem *>(this);
	if (!qthis) 
		return 0;
	QList<QGraphicsItem *> l = qthis->collidingItems();
	bool aboveVStrut = false;
	CanvasItem *item = 0;
	QGraphicsItem *qitem = 0;
	for (QList<QGraphicsItem *>::Iterator it = l.begin(); it != l.end(); ++it)
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

	QGraphicsRectItem *ritem = dynamic_cast<QGraphicsRectItem *>(qitem);

	return aboveVStrut && ritem? ritem : 0;
}

void CanvasItem::save(KConfigGroup *cfgGroup)
{
	cfgGroup->writeEntry("dummykey", true);
}

void CanvasItem::playSound(QString file, double vol)
{
	if (game)
		game->playSound(file, vol);
}

