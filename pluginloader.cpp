/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

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

#include "pluginloader.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klibloader.h>

ObjectList *PluginLoader::loadAll()
{
	ObjectList *ret = new ObjectList;

	QStringList libs;
	QStringList files = KGlobal::dirs()->findAllResources("appdata", "*.plugin", KStandardDirs::NoDuplicates);

	for (QStringList::Iterator it = files.begin(); it != files.end(); ++it)
	{
		KConfig cfg(*it, KConfig::OnlyLocal);
		KConfigGroup cfgGroup(cfg.group("General")); //probably a bug here, come back and test
		QString filename(cfgGroup.readEntry("Filename", ""));

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
	KLibFactory *factory = KLibLoader::self()->factory(filename.toLatin1());

	if (!factory)
	{
		kWarning() << "no factory for" << filename << "!";
		return 0;
	}

	QObject *newObject = factory->create(0, "Object");

	if (!newObject)
	{
		kWarning() << "no newObject for" << filename << "!";
		return 0;
	}

	newObject->setObjectName("objectInstance");
	Object *ret = dynamic_cast<Object *>(newObject);

	if (!ret)
		kWarning() << "no ret for" << filename << "!";

	return ret;
}

