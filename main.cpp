#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include "kolf.h"

static const char *description =
        I18N_NOOP("KDE Minigolf Game");

static const char *version = "0.1";


int main(int argc, char **argv)
{
  KAboutData aboutData( "kolf", I18N_NOOP("Kolf"),
    version, description, KAboutData::License_GPL,
    "(c) 2002, Jason Katz-Brown", "", "http://www.katzbrown.com/kolf/");

  aboutData.addAuthor("Jason Katz-Brown", I18N_NOOP("Main author"), "jason@katzbrown.com");
  aboutData.addAuthor("Niklas Knutsson", I18N_NOOP("Advanced putting mode"), 0);
  aboutData.addAuthor("Daniel Matza-Brown", I18N_NOOP("Working wall-bouncing algorithm"), 0);
  aboutData.addCredit("Rob Renaud", I18N_NOOP("Wall-bouncing help"), 0);
  aboutData.addCredit("Aaron Segio", I18N_NOOP("Suggestions, bug reports"), 0);

  KCmdLineArgs::init(argc, argv, &aboutData);

  QApplication::setColorSpec(QApplication::ManyColor);
  KApplication a;

  Kolf *top = new Kolf;
  a.setMainWidget(top);
  top->show();

  return a.exec();
}

