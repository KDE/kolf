/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

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

#ifndef SLOPE_H
#define SLOPE_H

#include "game.h"

enum GradientType{ Vertical, Horizontal, Diagonal, CrossDiagonal, Elliptic };

class Slope;
class SlopeConfig : public Config
{
	Q_OBJECT

public:
	SlopeConfig(Slope *slope, QWidget *parent);

private slots:
	void setGradient(const QString &text);
	void setReversed(bool);
	void setStuckOnGround(bool);
	void gradeChanged(double);

private:
	Slope *slope;
};

class Slope : public Tagaro::SpriteObjectItem, public CanvasItem
{
public:
	Slope(QGraphicsItem *parent);

	virtual void aboutToDie();

	virtual void showInfo();
	virtual void hideInfo();
	virtual void editModeChanged(bool changed);
	virtual bool canBeMovedByOthers() const { return !stuckOnGround; }
	virtual QList<QGraphicsItem *> moveableItems() const;
	virtual Config *config(QWidget *parent) { return new SlopeConfig(this, parent); }
	virtual void setSize(double width, double height);
	virtual void moveBy(double dx, double dy);

	virtual QPainterPath shape () const;

        void setGradient(const QString &text);
	void setGrade(double grade);

	double curGrade() const { return grade; }
	GradientType curType() const { return type; }
	void setColor(QColor color) { this->color = color; updatePixmap(); }
	void setReversed(bool reversed) { this->reversed = reversed; updatePixmap(); }
	bool isReversed() const { return reversed; }

	bool isStuckOnGround() const { return stuckOnGround; }
	void setStuckOnGround(bool yes) { stuckOnGround = yes; updateZ(); }

	virtual void load(KConfigGroup *cfgGroup);
	virtual void save(KConfigGroup *cfgGroup);

	virtual bool collision(Ball *ball, long int id);
	virtual bool terrainCollisions() const;

	QMap<GradientType, QString> gradientI18nKeys;
	QMap<GradientType, QString> gradientKeys;

	virtual void updateZ(QGraphicsItem *vStrut = 0);

	void moveArrow();

	double width() const { return size().width(); }
	double height() const { return size().height(); }

private:
	GradientType type;
	inline void setType(GradientType type);
	bool showingInfo;
	double grade;
	bool reversed;
	QColor color;
	void updatePixmap();
	bool stuckOnGround;

	void clearArrows();

	QList<Arrow *> arrows;
	QGraphicsSimpleTextItem *text;
	RectPoint *point;
};

#endif
