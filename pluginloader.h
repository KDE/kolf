#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <object.h>
#include <QString>

namespace PluginLoader
{
	ObjectList *loadAll();
	Object *load(const QString &);
}

#endif
