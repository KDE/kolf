#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <qtable.h>

class QWidget;
class QHeader;

class ScoreBoard : public QTable
{
	Q_OBJECT

public:
	ScoreBoard(QWidget *parent = 0, const char *name = 0);
	int total(int id, QString &name);

public slots:
	void newHole(int);
	void newPlayer(const QString &name);
	void setScore(int id, int hole, int score);
	void parChanged(int hole, int par);

private:
	QTable *table;
	QHeader *vh;
	QHeader *hh;
};

#endif
