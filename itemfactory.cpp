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

#include "itemfactory.h"

QList<Kolf::ItemMetadata> Kolf::ItemFactory::knownTypes() const
{
	QList<Kolf::ItemMetadata> result;
	QList<Entry>::const_iterator it1 = m_entries.begin(), it2 = m_entries.end();
	for (; it1 != it2; ++it1)
		result << it1->first;
	return result;
}

QGraphicsItem* Kolf::ItemFactory::createInstance(const QString& identifier, QGraphicsItem* parent, QGraphicsScene* scene) const
{
	QList<Entry>::const_iterator it1 = m_entries.begin(), it2 = m_entries.end();
	for (; it1 != it2; ++it1)
		if (it1->first.identifier == identifier)
			return (it1->second)(parent, scene);
	return 0;
}

void Kolf::ItemFactory::registerType(const Kolf::ItemMetadata& metadata, ItemCreator creator)
{
	m_entries << Entry(metadata, creator);
}
