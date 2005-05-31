#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <q3table.h>

class QWidget;
class Q3Header;

class ScoreBoard : public Q3Table
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
	Q3Table *table;
	Q3Header *vh;
	Q3Header *hh;
};

#endif
