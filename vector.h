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

#ifndef KOLF_VECTOR_H
#define KOLF_VECTOR_H

#include <math.h>
#include <QPoint>

// This and vector.cpp by Ryan Cummings

class QPointF;

// Implements a vector in 2D
class Vector
{
public:
	// Normal constructors
	Vector(double magnitude, double direction) { _magnitude = magnitude; _direction = direction; }
	Vector(const QPoint& source, const QPoint& dest);
	Vector(const QPointF& source, const QPointF& dest);
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
