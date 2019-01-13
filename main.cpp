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

#include "kolf.h"

#include <iostream>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QUrl>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

using namespace std;

static const char description[] =
I18N_NOOP("KDE Minigolf Game");

static const char version[] = "1.20";


int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kolf");

	KAboutData aboutData( QStringLiteral("kolf"), i18n("Kolf"), version, i18n(description), KAboutLicense::GPL, i18n("(c) 2002-2010, Kolf developers"),  QStringLiteral("http://games.kde.org/kolf"));

	aboutData.addAuthor(i18n("Stefan Majewsky"), i18n("Current maintainer"), QStringLiteral("majewsky@gmx.net"));
	aboutData.addAuthor(i18n("Jason Katz-Brown"), i18n("Former main author"), QStringLiteral("jasonkb@mit.edu"));
	aboutData.addAuthor(i18n("Niklas Knutsson"), i18n("Advanced putting mode"));
	aboutData.addAuthor(i18n("Rik Hemsley"), i18n("Border around course"));
	aboutData.addAuthor(i18n("Timo A. Hummel"), i18n("Some good sound effects"), QStringLiteral("timo.hummel@gmx.net"));

	aboutData.addCredit(i18n("Rob Renaud"), i18n("Wall-bouncing help"));
	aboutData.addCredit(i18n("Aaron Seigo"), i18n("Suggestions, bug reports"));
	aboutData.addCredit(i18n("Erin Catto"), i18n("Developer of Box2D physics engine"));
	aboutData.addCredit(i18n("Ryan Cumming"), i18n("Vector class (Kolf 1)"));
	aboutData.addCredit(i18n("Daniel Matza-Brown"), i18n("Working wall-bouncing algorithm (Kolf 1)"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();
        parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("+file"), i18n("File")));
        parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("course-info"), i18n("Print course information and exit")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    KDBusService service;

	// I've actually added this for my web site uploaded courses display
	if (parser.isSet(QStringLiteral("course-info")))
	{
		QString filename(parser.value(QStringLiteral("course-info")));
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
			qCritical() << i18n("Course %1 does not exist.", filename);
		}
	}

	KolfWindow *top = new KolfWindow;

	if (parser.positionalArguments().count() >= 1)
	{
		QUrl url = QUrl::fromUserInput(parser.positionalArguments().at(parser.positionalArguments().count() - 1));
		top->openUrl(url);
		
	}
	else
		top->closeGame();

	top->show();

	app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kolf")));

	return app.exec();
}

