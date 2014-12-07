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

#include <QFile>

#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <K4AboutData>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include "kolf.h"

#include <iostream>
#include <kdemacros.h>
using namespace std;

static const char description[] =
I18N_NOOP("KDE Minigolf Game");

static const char version[] = "1.10"; // = KDE 4.6 Release


int main(int argc, char **argv)
{
	K4AboutData aboutData( "kolf", 0, ki18n("Kolf"), version, ki18n(description), K4AboutData::License_GPL, ki18n("(c) 2002-2010, Kolf developers"), KLocalizedString(), "http://games.kde.org/kolf");

	aboutData.addAuthor(ki18n("Stefan Majewsky"), ki18n("Current maintainer"), "majewsky@gmx.net");
	aboutData.addAuthor(ki18n("Jason Katz-Brown"), ki18n("Former main author"), "jasonkb@mit.edu");
	aboutData.addAuthor(ki18n("Niklas Knutsson"), ki18n("Advanced putting mode"));
	aboutData.addAuthor(ki18n("Rik Hemsley"), ki18n("Border around course"));
	aboutData.addAuthor(ki18n("Timo A. Hummel"), ki18n("Some good sound effects"), "timo.hummel@gmx.net");

	aboutData.addCredit(ki18n("Rob Renaud"), ki18n("Wall-bouncing help"));
	aboutData.addCredit(ki18n("Aaron Seigo"), ki18n("Suggestions, bug reports"));
	aboutData.addCredit(ki18n("Erin Catto"), ki18n("Developer of Box2D physics engine"));
	aboutData.addCredit(ki18n("Ryan Cumming"), ki18n("Vector class (Kolf 1)"));
	aboutData.addCredit(ki18n("Daniel Matza-Brown"), ki18n("Working wall-bouncing algorithm (Kolf 1)"));

	KCmdLineArgs::init(argc, argv, &aboutData);

	KCmdLineOptions options;
	options.add("+file", ki18n("File"));
	options.add("course-info ", ki18n("Print course information and exit"));
	KCmdLineArgs::addCmdLineOptions(options);

	// I've actually added this for my web site uploaded courses display
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if (args->isSet("course-info"))
	{
		KCmdLineArgs::enable_i18n();

		QString filename(args->getOption("course-info"));
		if (QFile::exists(filename))
		{
			CourseInfo info;
			KolfGame::courseInfo(info, filename);

			cout << info.name.toLatin1().constData() 
			     << " - " << i18n("By %1", info.author).toLatin1().constData()
			     << " - " << i18n("%1 holes", info.holes).toLatin1().constData()
			     << " - " << i18n("par %1", info.par).toLatin1().constData()
			     << endl;

			return 0;
		}
		else
		{
			KCmdLineArgs::usageError(i18n("Course %1 does not exist.", filename));
		}
	}

	QApplication::setColorSpec(QApplication::ManyColor);
	KApplication a;
	//KF5 port: remove this line and define TRANSLATION_DOMAIN in CMakeLists.txt instead
//KLocale::global()->insertCatalog( QLatin1String( "libkdegames" ));

	KolfWindow *top = new KolfWindow;

	if (args->count() >= 1)
	{
		KUrl url = args->url(args->count() - 1);
		top->openUrl(url);
		args->clear();
	}
	else
		top->closeGame();

	top->show();

	return a.exec();
}

