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

class Slope : public QGraphicsRectItem, public CanvasItem, public RectItem
{
public:
	Slope(QGraphicsItem *parent);

	virtual void aboutToDie();

	virtual void showInfo();
	virtual void hideInfo();
	void resize(double resizeFactor);
	void firstMove(int x, int y);
	virtual void editModeChanged(bool changed);
	virtual bool canBeMovedByOthers() const { return !stuckOnGround; }
	virtual QList<QGraphicsItem *> moveableItems() const;
	virtual Config *config(QWidget *parent) { return new SlopeConfig(this, parent); }
	virtual void setSize(double width, double height);
	virtual void newSize(double width, double height);
	virtual void moveBy(double dx, double dy);

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
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

	double width() const { return rect().width(); }
	double height() const { return rect().height(); }

private:
	GradientType type;
	inline void setType(GradientType type);
	bool showingInfo;
	double grade;
	bool reversed;
	QColor color;
	QPixmap pixmap;
	void updatePixmap();
	bool stuckOnGround;
	//base numbers are the size or position when no resizing has taken place (i.e. the defaults)
	double baseX, baseY, baseWidth, baseHeight;
	double baseArrowPenThickness, arrowPenThickness;
	int baseFontPixelSize;
	double resizeFactor;

	void clearArrows();

	QList<Arrow *> arrows;
	QGraphicsSimpleTextItem *text;
	RectPoint *point;
};

#endif
