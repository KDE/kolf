#ifndef SLOPE_H
#define SLOPE_H

#include <kimageeffect.h>

#include "game.h"

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

class Slope : public QCanvasRectangle, public CanvasItem, public RectItem
{
public:
	Slope(QRect rect, QCanvas *canvas);
	virtual void aboutToDie();
	virtual int rtti() const { return 1031; }

	virtual void showInfo();
	virtual void hideInfo();
	virtual void editModeChanged(bool changed);
	virtual bool canBeMovedByOthers() const { return !stuckOnGround; }
	virtual QPtrList<QCanvasItem> moveableItems() const;
	virtual Config *config(QWidget *parent) { return new SlopeConfig(this, parent); }
	void setSize(int, int);
	virtual void newSize(int width, int height);

	virtual void moveBy(double dx, double dy);

	virtual void draw(QPainter &painter);
	virtual QPointArray areaPoints() const;

	void setGradient(QString text);
	KImageEffect::GradientType curType() const { return type; }
	void setGrade(double grade);

	double curGrade() const { return grade; }
	void setColor(QColor color) { this->color = color; updatePixmap(); }
	void setReversed(bool reversed) { this->reversed = reversed; updatePixmap(); }
	bool isReversed() const { return reversed; }

	bool isStuckOnGround() const { return stuckOnGround; }
	void setStuckOnGround(bool yes) { stuckOnGround = yes; updateZ(); }

	virtual void load(KConfig *cfg);
	virtual void save(KConfig *cfg);

	virtual bool collision(Ball *ball, long int id);
	virtual bool terrainCollisions() const;

	QMap<KImageEffect::GradientType, QString> gradientI18nKeys;
	QMap<KImageEffect::GradientType, QString> gradientKeys;

	virtual void updateZ(QCanvasRectangle *vStrut = 0);

	void moveArrow();

private:
	KImageEffect::GradientType type;
	inline void setType(KImageEffect::GradientType type);
	bool showingInfo;
	double grade;
	bool reversed;
	QColor color;
	QPixmap pixmap;
	void updatePixmap();
	bool stuckOnGround;
	QPixmap grass;

	void clearArrows();

	QPtrList<Arrow> arrows;
	QCanvasText *text;
	RectPoint *point;
};

class SlopeObj : public Object
{
public:
	SlopeObj() { m_name = i18n("Slope"); m__name = "slope"; }
	virtual QCanvasItem *newObject(QCanvas *canvas) { return new Slope(QRect(0, 0, 40, 40), canvas); }
};

#endif
