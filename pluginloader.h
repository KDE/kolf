#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <object.h>
#include <qstring.h>

namespace PluginLoader
{
	ObjectList *loadAll();
	Object *load(const QString &);
}

#endif
