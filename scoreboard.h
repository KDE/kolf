/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
	void doUpdateHeight();
};

#endif
