#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <QWidget>

#include "game.h"

class K3ListBox;
class QHBoxLayout;
class Q3ListBoxItem;
class Config;

class Editor : public QWidget
{
	Q_OBJECT

public:
	explicit Editor(ObjectList *list, QWidget * = 0);

signals:
	void changed();
	void addNewItem(Object *);

public slots:
	void setItem(CanvasItem *);

private slots:
	void listboxExecuted(Q3ListBoxItem *); //Q3ListBoxItem used here because that is what KListBox uses


private:
	ObjectList *list;
	QHBoxLayout *hlayout;
	K3ListBox *listbox; //note: this uses Q3 listbox
	Config *config;
};

#endif
