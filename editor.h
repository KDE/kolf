#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <qwidget.h>

#include "game.h"

class KListBox;
class QHBoxLayout;
class QListBoxItem;
class QFrame;

class Editor : public QWidget
{
	Q_OBJECT

public:
	Editor(ObjectList *list, QWidget * = 0, const char * = 0);

signals:
	void changed();
	void addNewItem(Object *);

public slots:
	void setItem(CanvasItem *);

private slots:
	void listboxExecuted(QListBoxItem *);

private:
	ObjectList *list;
	QHBoxLayout *hlayout;
	KListBox *listbox;
	QFrame *config;
};

#endif
