#include <kdebug.h>
#include <klocale.h>

#include <qlayout.h>
#include <qtable.h>
#include <qwidget.h>
#include <qheader.h>
#include <qstring.h>

#include "scoreboard.h"

ScoreBoard::ScoreBoard(QWidget *parent, const char *name)
	: QTable(1, 1, parent, name)
{
	vh = verticalHeader();
	hh = horizontalHeader();
	vh->setLabel(numRows() - 1, i18n("Par"));
	hh->setLabel(numCols() - 1, i18n("Total"));

	setFocusPolicy(QWidget::NoFocus);
	setRowReadOnly(0, true);
	setRowReadOnly(1, true);
}

void ScoreBoard::newHole(int par)
{
	int _numCols = numCols();
	insertColumns(_numCols - 1);
	hh->setLabel(numCols() - 2, QString::number(numCols() - 1));
	setText(numRows() - 1, numCols() - 2, QString::number(par));
	setColumnWidth(numCols() - 2, 40);

	// update total
	int tot = 0;
	for (int i = 0; i < numCols() - 1; ++i)
		tot += text(numRows() - 1, i).toInt();
	setText(numRows() - 1, numCols() - 1, QString::number(tot));
	
	// shrink cell...
	setColumnWidth(numCols() - 2, 3);
	// and make it big enough for the numbers
	adjustColumn(numCols() - 2);
}

void ScoreBoard::newPlayer(const QString &name)
{
	//kdDebug(12007) << "name of new player is " << name << endl;
	insertRows(numRows() - 1);
	vh->setLabel(numRows() - 2, name);
	setRowReadOnly(numRows() - 2, true);
}

void ScoreBoard::setScore(int id, int hole, int score)
{
	//kdDebug(12007) << "set score\n";
	setText(id - 1, hole - 1, score > 0? QString::number(score) : QString(""));

	QString name;
	setText(id - 1, numCols() - 1, QString::number(total(id, name)));
	if (hole >= numCols() - 2)
		ensureCellVisible(id - 1, numCols() - 1);
	else
		ensureCellVisible(id - 1, hole - 1);
	
	// shrink cell...
	setColumnWidth(hole - 1, 3);
	// and make it big enough for the numbers
	adjustColumn(hole - 1);
		
	setCurrentCell(id - 1, hole - 1);
}

void ScoreBoard::parChanged(int hole, int par)
{
	setText(numRows() - 1, hole - 1, QString::number(par));

	// update total
	int tot = 0;
	for (int i = 0; i < numCols() - 1; ++i)
		tot += text(numRows() - 1, i).toInt();
	setText(numRows() - 1, numCols() - 1, QString::number(tot));
}

int ScoreBoard::total(int id, QString &name)
{
	int tot = 0;
	for (int i = 0; i < numCols() - 1; i++)
		tot += text(id - 1, i).toInt();
	name = vh->label(id - 1);
	//kdDebug(12007) << "tot is " << tot << endl;
	return tot;
}

#include "scoreboard.moc"
