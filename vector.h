#ifndef KOLF_VECTOR_H
#define KOLF_VECTOR_H

#include <math.h>

class Point
{
public:
	Point(double _x, double _y)
	{
		x = _x;
		y = _y;
	}

	Point()
	{
		x = 0;
		y = 0;
	}

	double x;
	double y;
};

void debugPoint(const QString &, const Point &);

// This and vector.cpp by Ryan Cummings

class QPointF;

// Implements a vector in 2D
class Vector {
  public:
	// Normal constructors
	Vector(double magnitude, double direction) { _magnitude = magnitude; _direction = direction; }
	Vector(const QPoint& source, const QPoint& dest);
	Vector(const QPointF& source, const QPointF& dest);
	Vector(const Point& source, const Point& dest);
	Vector();

	// Copy constructor
	Vector(const Vector&);

	// Accessors, sorta
	double componentX() const { return (_magnitude * cos(_direction)); }
	double componentY() const { return (_magnitude * sin(_direction)); }

	// Sets individual components
	// Wrappers around setComponents(double, double) - below
	void setComponentX(double x);
	void setComponentY(double y);

	// Sets both components at once
	void setComponents(double x, double y);

	// Accessors
	double magnitude() const { return _magnitude; }
	double direction() const { return _direction; }
	void setMagnitude(double m) { _magnitude = m; }
	void setDirection(double d) { _direction = d; }

	// Vector math
	Vector operator+(const Vector&);
	Vector operator-(const Vector&);

	Vector& operator+=(const Vector&);
	Vector& operator-=(const Vector&);

	// Dot product
	double operator*(const Vector&);

	// Magnitude math
	Vector operator+(double m) { return Vector(_magnitude + m, _direction); }
	Vector operator-(double m) { return Vector(_magnitude - m, _direction); }
	Vector operator*(double m) { return Vector(_magnitude * m, _direction); }
	Vector operator/(double m) { return Vector(_magnitude / m, _direction); }

	Vector& operator+=(double m);
	Vector& operator-=(double m);
	Vector& operator*=(double m);
	Vector& operator/=(double m);

	// Return the vector's equalivent on the unit circle
	Vector unit() const { return Vector(1.0, _direction); }

  protected:
	double _magnitude;
	double _direction;
};

void debugVector(const QString &, const Vector &);

#endif
