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

#ifndef NEWGAME_H
#define NEWGAME_H

#include <kpagedialog.h>
#include <klineedit.h>
#include <kcolorbutton.h>
#include <kpushbutton.h>

#include <QColor>

#include <QPixmap>
#include <QWidget>
#include <QCheckBox>

#include "game.h"

class KLineEdit;
class K3ListBox;
class QScrollArea;

class PlayerEditor : public QWidget
{
	Q_OBJECT
	
public:
	explicit PlayerEditor(QString name = QString(), QColor = Qt::red, QWidget *parent = 0);
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

class NewGameDialog : public KPageDialog
{
	Q_OBJECT

public:
	NewGameDialog(bool enableCourses);
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

	K3ListBox *courseList;
	QLabel *name;
	QLabel *author;
	QLabel *par;
	QLabel *holes;

	QString currentCourse;

	void enableButtons();
	bool enableCourses;
};

#endif
