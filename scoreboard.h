#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <QTableWidget>

class QWidget;

class ScoreBoard : public QTableWidget
{
	Q_OBJECT

public:
	ScoreBoard(QWidget *parent = 0);
	int total(int id, QString &name);

public slots:
	void newHole(int);
	void newPlayer(const QString &name);
	void setScore(int id, int hole, int score);
	void parChanged(int hole, int par);

private:
	QTableWidget *table;
};

#endif
