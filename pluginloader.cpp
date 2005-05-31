#include <qstring.h>
#include <q3valuelist.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qobject.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <klibloader.h>

#include "pluginloader.h"

ObjectList *PluginLoader::loadAll()
{
	ObjectList *ret = new ObjectList;

	QStringList libs;
	QStringList files = KGlobal::dirs()->findAllResources("appdata", "*.plugin", false, true);

	for (QStringList::Iterator it = files.begin(); it != files.end(); ++it)
	{
		KSimpleConfig cfg(*it);
		QString filename(cfg.readEntry("Filename", ""));

		libs.append(filename);
	}

	for (QStringList::Iterator it = libs.begin(); it != libs.end(); ++it)
	{
		Object *newObject = load(*it);
		if (newObject)
			ret->append(newObject);
	}

	return ret;
}

Object *PluginLoader::load(const QString &filename)
{
	KLibFactory *factory = KLibLoader::self()->factory(filename.latin1());

	if (!factory)
	{
		kdWarning() << "no factory for " << filename << "!" << endl;
		return 0;
	}

	QObject *newObject = factory->create(0, "objectInstance", "Object");
	
	if (!newObject)
	{
		kdWarning() << "no newObject for " << filename << "!" << endl;
		return 0;
	}

	Object *ret = dynamic_cast<Object *>(newObject);

	if (!ret)
		kdWarning() << "no ret for " << filename << "!" << endl;

	return ret;
}

