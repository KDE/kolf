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
	setSelectionMode(NoSelection);

	setRowReadOnly(0, true);
	setRowReadOnly(1, true);
	setRowReadOnly(2, true);
	setRowReadOnly(3, true);
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
	{
		tot += text(numRows() - 1, i).toInt();
	}
	setText(numRows() - 1, numCols() - 1, QString::number(tot));
}

void ScoreBoard::newPlayer(const QString &name)
{
	//kdDebug() << "name of new player is " << name << endl;
	insertRows(numRows() - 1);
	vh->setLabel(numRows() - 2, name);
	setRowReadOnly(numRows() - 2, true);
}

void ScoreBoard::setScore(int id, int hole, int score)
{
	//kdDebug() << "set score\n";
	setText(id - 1, hole - 1, score > 0? QString::number(score) : QString(""));

	QString name;
	setText(id - 1, numCols() - 1, QString::number(total(id, name)));
	ensureCellVisible(id - 1, hole - 1);
	setCurrentCell(id - 1, hole - 1);
}

void ScoreBoard::parChanged(int hole, int par)
{
	kdDebug() << "parChange - hole is " << hole << ", par is " << par << endl;
	setText(numRows() - 1, hole - 1, QString::number(par));

	// update total
	int tot = 0;
	for (int i = 0; i < numCols() - 1; ++i)
	{
		tot += text(numRows() - 1, i).toInt();
	}
	setText(numRows() - 1, numCols() - 1, QString::number(tot));
}

int ScoreBoard::total(int id, QString &name)
{
	int tot = 0;
	for (int i = 0; i < numCols() - 1; i++)
		tot += text(id - 1, i).toInt();
	name = vh->label(id - 1);
	//kdDebug() << "tot is " << tot << endl;
	return tot;
}

#include "scoreboard.moc"
