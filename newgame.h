#ifndef NEWGAME_H
#define NEWGAME_H

#include <kdialogbase.h>
#include <klineedit.h>
#include <kcolorbutton.h>

#include <qcheckbox.h>
#include <qcolor.h>
#include <q3ptrlist.h>
#include <qstring.h>
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <Q3Frame>
#include <QLabel>
#include <QVBoxLayout>
#include <kpushbutton.h>
#include <qstringlist.h>
#include <q3valuelist.h>
#include <qwidget.h>

#include "game.h"

class KLineEdit;
class Q3Frame;
class QVBoxLayout;
class Q3VBox;
class QPainter;
class KListBox;
class QEvent;

class PlayerEditor : public QWidget
{
	Q_OBJECT
	
public:
	PlayerEditor(QString name = QString::null, QColor = Qt::red, QWidget *parent = 0, const char *_name = 0);
	QColor color() { return colorButton->color(); }
	QString name() { return editor->text(); }
	void setColor(QColor col) { colorButton->setColor(col); }
	void setName(const QString &newname) { editor->setText(newname); }

signals:
	void deleteEditor(PlayerEditor *editor);

private slots:
	void removeMe();

private:
	KLineEdit *editor;
	KColorButton *colorButton;
	QPixmap grass;
};

class NewGameDialog : public KDialogBase
{
	Q_OBJECT

public:
	NewGameDialog(bool enableCourses, QWidget *parent, const char *_name = 0);
	Q3PtrList<PlayerEditor> *players() { return &editors; }
	bool competition() { return mode->isChecked(); }
	QString course() { return currentCourse; }

public slots:
	void deleteEditor(PlayerEditor *);

protected slots:
	void slotOk();

private slots:
	void addPlayer();
	void courseSelected(int);
	void addCourse();
	void removeCourse();
	void selectionChanged();
	void showHighscores();

private:
	Q3VBox *layout;
	KPushButton *addButton;
	Q3Frame *playerPage;
	Q3ScrollView *scroller;
	Q3Frame *coursePage;
	Q3Frame *optionsPage;
	Q3ValueList<QColor> startColors;
	Q3PtrList<PlayerEditor> editors;
	KPushButton *remove;
	QCheckBox *mode;

	QPixmap grass;

	QStringList names;
	QStringList externCourses;
	QMap<QString, CourseInfo> info;

	QStringList extraCourses;

	KListBox *courseList;
	QLabel *name;
	QLabel *author;
	QLabel *par;
	QLabel *holes;

	QString currentCourse;

	void enableButtons();
	bool enableCourses;
};

#endif
