#ifndef NEWGAME_H_INCLUDED
#define NEWGAME_H_INCLUDED

#include <kcolorbutton.h>
#include <kdialogbase.h>
#include <klineedit.h>
#include <kcolorbutton.h>

#include <qcheckbox.h>
#include <qcolor.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qvbox.h>
#include <qwidget.h>

#include "game.h"

class KLineEdit;
class KPushButton;
class QFrame;
class QVBoxLayout;
class QPainter;
class KListBox;

class ColorButton : public KColorButton
{
public:
	ColorButton(QColor color, QWidget *parent, const char *name = 0L) : KColorButton(color, parent, name) {}

protected:
	void drawButtonLabel(QPainter *painter);
};

class PlayerEditor : public QWidget
{
	Q_OBJECT
	
public:
	PlayerEditor(QString name = QString::null, QColor = red, QWidget *parent = 0, const char *_name = 0);
	QColor color() { return colorButton->color(); }
	QString name() { return editor->text(); }
	void setColor(QColor col) { colorButton->setColor(col); }
	void setName(const QString &newname) { editor->setText(newname); }

private:
	KLineEdit *editor;
	KColorButton *colorButton;
};

class NewGameDialog : public KDialogBase
{
	Q_OBJECT

public:
	NewGameDialog(QWidget *parent, const char *_name = 0);
	QPtrList<PlayerEditor> *players() { return &editors; }
	bool competition() { return mode->isChecked(); }
	QString course() { return currentCourse; }

protected slots:
	void slotOk();

private slots:
	void addPlayer();
	void delPlayer();
	void courseSelected(int);
	void addCourse();
	void removeCourse();
	void selectionChanged();

private:
	QVBox *layout;
	KPushButton *addButton;
	KPushButton *delButton;
	QFrame *playerPage;
	QScrollView *scroller;
	QFrame *coursePage;
	QFrame *optionsPage;
	QValueList<QColor> startColors;
	QPtrList<PlayerEditor> editors;
	QPushButton *remove;
	QCheckBox *mode;

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
};

#endif
