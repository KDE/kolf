#ifndef NEWGAME_H
#define NEWGAME_H

#include <kdialogbase.h>
#include <klineedit.h>
#include <kcolorbutton.h>

#include <qcheckbox.h>
#include <qcolor.h>
#include <qptrlist.h>
#include <qstring.h>
#include <kpushbutton.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qwidget.h>

#include "game.h"

class KLineEdit;
class QFrame;
class QVBoxLayout;
class QVBox;
class QPainter;
class KListBox;
class QEvent;

class ColorButton : public KColorButton
{
	Q_OBJECT

public:
	ColorButton(QColor color, QWidget *parent, const char *name = 0L);

protected:
	virtual void drawButtonLabel(QPainter *painter);
	virtual void drawButton(QPainter *painter);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);

private:
	bool mouseOver;
};

class TransparentButton : public KPushButton
{
	Q_OBJECT

public:
	TransparentButton(const QString &text, QWidget *parent, const char *name = 0L);

protected:
	virtual void drawButton(QPainter *painter);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);

private:
	bool mouseOver;
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
	QPtrList<PlayerEditor> *players() { return &editors; }
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
	QVBox *layout;
	KPushButton *addButton;
	QFrame *playerPage;
	QScrollView *scroller;
	QFrame *coursePage;
	QFrame *optionsPage;
	QValueList<QColor> startColors;
	QPtrList<PlayerEditor> editors;
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
	QString currentCourseName;

	void enableButtons();

	bool enableCourses;
};

#endif
