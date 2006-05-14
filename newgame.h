#ifndef NEWGAME_H
#define NEWGAME_H

#include <kdialogbase.h>
#include <klineedit.h>
#include <kcolorbutton.h>

#include <QCheckBox>
#include <QColor>
#include <QString>
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <kpushbutton.h>
#include <qstringlist.h>
#include <QWidget>

#include "game.h"

class KLineEdit;
class QVBoxLayout;
class QPainter;
class KListBox;
class QEvent;
class QScrollArea;

class PlayerEditor : public QWidget
{
	Q_OBJECT
	
public:
	PlayerEditor(QString name = QString::null, QColor = Qt::red, QWidget *parent = 0);
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
	~NewGameDialog();
	QList<PlayerEditor*> *players() { return &editors; }
	bool competition() { return mode->isChecked(); }
	QString course() { return currentCourse; }

public slots:
	void deleteEditor(PlayerEditor *);

protected slots:
	void slotOk();
	void invokeBrowser(const QString &_url);
private slots:
	void addPlayer();
	void courseSelected(int);
	void addCourse();
	void removeCourse();
	void selectionChanged();
	void showHighscores();

private:
	QWidget *playersWidget;
	KPushButton *addButton;
	QFrame *playerPage;
	QScrollArea *scroller;
	QFrame *coursePage;
	QFrame *optionsPage;
	QList<QColor> startColors;
	QList<PlayerEditor*> editors;
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
