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
#include <qvaluelist.h>
#include <qwidget.h>

class KLineEdit;
class KPushButton;
class QFrame;
class QVBoxLayout;
class QPainter;

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
	NewGameDialog(QWidget *parent, const char *name = 0);
	QPtrList<PlayerEditor> *players() { return &editors; }
	bool competition() { return mode->isChecked(); }

protected slots:
	void slotOk();

private slots:
	void addPlayer();
	void delPlayer();

private:
	QVBoxLayout *layout;
	KPushButton *addButton;
	KPushButton *delButton;
	QFrame *dummy;
	QValueList<QColor> startColors;
	QPtrList<PlayerEditor> editors;
	QCheckBox *mode;

	void enableButtons();
};

#endif
