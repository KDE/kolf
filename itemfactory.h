/*
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

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

#ifndef KOLF_ITEMFACTORY_H
#define KOLF_ITEMFACTORY_H

#include <QGraphicsItem>
class b2World;

namespace Kolf
{
	struct ItemMetadata
	{
		QString identifier, name;
		bool addOnNewHole;
	};

	//This class registers maps identifiers and other metadata to QGraphicsItem subclasses, and is able to create item instances as needed.
	class ItemFactory
	{
		public:
			QList<Kolf::ItemMetadata> knownTypes() const;
			QGraphicsItem* createInstance(const QString& identifier, QGraphicsItem* parent, b2World* world) const;

			template<typename T> void registerType(const QString& identifier, const QString& name, bool addOnNewHole = false)
			{
				const Kolf::ItemMetadata metadata = { identifier, name, addOnNewHole };
				registerType(metadata, &Kolf::ItemFactory::create<T>);
			}
		private:
			typedef QGraphicsItem* (*ItemCreator)(QGraphicsItem* parent, b2World* world);
			void registerType(const Kolf::ItemMetadata& metadata, ItemCreator creator);
			template<typename T> static QGraphicsItem* create(QGraphicsItem* parent, b2World* world)
			{
				return new T(parent, world);
			}
		private:
			typedef QPair<Kolf::ItemMetadata, ItemCreator> Entry;
			QList<Entry> m_entries;
	};
}

#endif // KOLF_ITEMFACTORY_H
