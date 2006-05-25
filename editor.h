#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <QWidget>

#include "game.h"

class KListBox;
class QHBoxLayout;
class Q3ListBoxItem;
class Config;

class Editor : public QWidget
{
	Q_OBJECT

public:
	Editor(ObjectList *list, QWidget * = 0);

signals:
	void changed();
	void addNewItem(Object *);

public slots:
	void setItem(CanvasItem *);

private slots:
	void listboxExecuted(Q3ListBoxItem *);

private:
	ObjectList *list;
	QHBoxLayout *hlayout;
	KListBox *listbox;
	Config *config;
};

#endif
