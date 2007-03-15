#ifndef SLOPE_H
#define SLOPE_H

#include <kimageeffect.h>

#include "game.h"
#include <QPixmap>

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
	Slope(QRect rect, QGraphicsItem *parent, QGraphicsScene *scene);

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

	void setGradient(QString text);
	QString curType() const { return type; }
	void setGrade(double grade);

	double curGrade() const { return grade; }
	void setColor(QColor color) { this->color = color; updatePixmap(); }
	void setReversed(bool reversed) { this->reversed = reversed; updatePixmap(); }
	bool isReversed() const { return reversed; }

	bool isStuckOnGround() const { return stuckOnGround; }
	void setStuckOnGround(bool yes) { stuckOnGround = yes; updateZ(); }

	virtual void load(KConfigGroup *cfgGroup);
	virtual void save(KConfigGroup *cfgGroup);

	virtual bool collision(Ball *ball, long int id);
	virtual bool terrainCollisions() const;

	QMap<KImageEffect::GradientType, QString> gradientI18nKeys;
	QMap<KImageEffect::GradientType, QString> gradientKeys;

	virtual void updateZ(QGraphicsRectItem *vStrut = 0);

	void moveArrow();

	double width() const { return rect().width(); }
	double height() const { return rect().height(); }

private:
	QString type;
	inline void setType(QString type);
	bool showingInfo;
	double grade;
	bool reversed;
	QColor color;
	QPixmap pixmap;
	void updatePixmap();
	bool stuckOnGround;
	/*
	 * base numbers are the size or position when no resizing has taken place (i.e. the defaults)
	 */
	double baseX, baseY, baseWidth, baseHeight;
	double baseArrowPenThickness, arrowPenThickness;
	int baseFontPixelSize;

	void clearArrows();

	QList<Arrow *> arrows;
	QGraphicsSimpleTextItem *text;
	RectPoint *point;
};

class SlopeObj : public Object
{
public:
	SlopeObj() { m_name = i18n("Slope"); m__name = "slope"; }
	virtual QGraphicsItem *newObject(QGraphicsItem * parent, QGraphicsScene *scene) { return new Slope(QRect(0, 0, 40, 40), parent, scene); }
};

#endif
