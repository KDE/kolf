/*
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

#ifndef KOLF_VECTOR_H
#define KOLF_VECTOR_H

#include <QPointF>
#include <cmath>

inline int rad2deg(double radians)
{
    return (180 / M_PI) * radians;
}

inline double deg2rad(int degrees)
{
	return (M_PI / 180) * degrees;
}

class Vector : public QPointF
{
	public:
		inline Vector(const QPointF& point = QPointF()) : QPointF(point) {}
		inline Vector(qreal x, qreal y) : QPointF(x, y) {}
		inline Vector& operator=(const QPointF& point) { setX(point.x()); setY(point.y()); return *this; }

		//dot product
		inline qreal operator*(const Vector& rhs) const;

		//getters and setters for polar coordinates
		inline qreal magnitude() const;
		inline qreal direction() const; //in radians!
		inline void setMagnitude(qreal magnitude);
		inline void setDirection(qreal direction);
		inline void setMagnitudeDirection(qreal magnitude, qreal direction);
		static inline Vector fromMagnitudeDirection(qreal magnitude, qreal direction);

		//some further convenience
		inline Vector unitVector() const;
};

qreal Vector::operator*(const Vector& rhs) const
{
	return x() * rhs.x() + y() * rhs.y();
}

qreal Vector::magnitude() const
{
	return sqrt(*this * *this);
}

qreal Vector::direction() const
{
	return atan2(y(), x());
}

void Vector::setMagnitude(qreal magnitude)
{
	setMagnitudeDirection(magnitude, this->direction());
}

void Vector::setDirection(qreal direction)
{
	setMagnitudeDirection(this->magnitude(), direction);
}

void Vector::setMagnitudeDirection(qreal magnitude, qreal direction)
{
	setX(magnitude * cos(direction));
	setY(magnitude * sin(direction));
}

Vector Vector::fromMagnitudeDirection(qreal magnitude, qreal direction)
{
	Vector v;
	v.setMagnitudeDirection(magnitude, direction);
	return v;
}

Vector Vector::unitVector() const
{
	return *this / magnitude();
}

#endif // KOLF_VECTOR_H
