#include <QFile>

#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include "kolf.h"

#include <iostream>
#include <kdemacros.h>
using namespace std;

static const char description[] =
I18N_NOOP("KDE Minigolf Game");

static const char version[] = "1.1";

static KCmdLineOptions options[] =
{
	{ "+file", I18N_NOOP("File"), 0 },
	{ "course-info ", I18N_NOOP("Print course information and exit"), 0 },
	KCmdLineLastOption
};


int main(int argc, char **argv)
{
	KAboutData aboutData( "kolf", I18N_NOOP("Kolf"), version, description, KAboutData::License_GPL, "(c) 2002-2005, Jason Katz-Brown", 0, "http://www.katzbrown.com/kolf/");

	aboutData.addAuthor("Jason Katz-Brown", I18N_NOOP("Main author"), "jasonkb@mit.edu");
	aboutData.addAuthor("Niklas Knutsson", I18N_NOOP("Advanced putting mode"), 0);
	aboutData.addAuthor("Rik Hemsley", I18N_NOOP("Border around course"), 0);
	aboutData.addAuthor("Ryan Cumming", I18N_NOOP("Vector class"), 0);
	aboutData.addAuthor("Daniel Matza-Brown", I18N_NOOP("Working wall-bouncing algorithm"), 0);
	aboutData.addAuthor("Timo A. Hummel", I18N_NOOP("Some good sound effects"), "timo.hummel@gmx.net");

	aboutData.addCredit("Rob Renaud", I18N_NOOP("Wall-bouncing help"), 0);
	aboutData.addCredit("Aaron Seigo", I18N_NOOP("Suggestions, bug reports"), 0);

	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);

	// I've actually added this for my web site uploaded courses display
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if (args->isSet("course-info"))
	{
		KCmdLineArgs::enable_i18n();

		QString filename(QFile::decodeName(args->getOption("course-info")));
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
			KCmdLineArgs::usage(i18n("Course %1 does not exist.", filename));
		}
	}

	QApplication::setColorSpec(QApplication::ManyColor);
	KApplication a;
	KGlobal::locale()->insertCatalog("libkdegames");

	Kolf *top = new Kolf;

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

