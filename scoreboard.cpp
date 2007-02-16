#include <kdebug.h>
#include <klocale.h>

#include "scoreboard.h"

ScoreBoard::ScoreBoard(QWidget *parent)
	: QTableWidget(1, 1, parent) 
{
	setVerticalHeaderItem(rowCount() -1,  new QTableWidgetItem(i18n("Par")));
	setHorizontalHeaderItem(columnCount() -1, new QTableWidgetItem(i18n("Total")));

	setFocusPolicy(Qt::NoFocus);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void ScoreBoard::newHole(int par)
{
	int _columnCount = columnCount();
	insertColumn(_columnCount - 1);
	setHorizontalHeaderItem(columnCount() -2, new QTableWidgetItem(QString::number(columnCount() - 1)));
	//set each player's score to 0
	for (int i = 0; i < rowCount() - 1; i++)
		setItem(i, columnCount() -2, new QTableWidgetItem(QString::number(0)));
	setItem(rowCount() - 1, columnCount() - 2, new QTableWidgetItem(QString::number(par)));

	// update total
	int tot = 0;
	for (int i = 0; i < columnCount() - 1; ++i)
		tot += item(rowCount() - 1, i)->text().toInt();
	setItem(rowCount() - 1, columnCount() - 1, new QTableWidgetItem(QString::number(tot)));
	
	resizeColumnToContents(columnCount() - 2); 
}

void ScoreBoard::newPlayer(const QString &name)
{
	//kDebug(12007) << "name of new player is " << name << endl;
	insertRow(rowCount() - 1);
	setVerticalHeaderItem(rowCount() -2, new QTableWidgetItem(name));
}

void ScoreBoard::setScore(int id, int hole, int score)
{
	setItem(id - 1, hole - 1, new QTableWidgetItem(QString::number(score)));

	QString name;
	setItem(id - 1, columnCount() - 1, new QTableWidgetItem(QString::number(total(id, name))));
	
	resizeColumnToContents(hole -1);
		
	setCurrentCell(id - 1, hole - 1);
}

void ScoreBoard::parChanged(int hole, int par)
{
	setItem(rowCount() - 1, hole - 1, new QTableWidgetItem(QString::number(par)));

	// update total
	int tot = 0;
	for (int i = 0; i < columnCount() - 1; ++i)
		tot += item(rowCount() - 1, i)->text().toInt();
	setItem(rowCount() - 1, columnCount() - 1, new QTableWidgetItem(QString::number(tot)));
}

int ScoreBoard::total(int id, QString &name)
{
	int tot = 0;
	for (int i = 0; i < columnCount() - 1; i++) 
		tot += item(id - 1, i)->text().toInt();
		
	name = verticalHeaderItem(id - 1)->text();

	//kDebug(12007) << "tot is " << tot << endl;
	return tot;
}

#include "scoreboard.moc"
