#include <math.h>

#include <kdebug.h>

#include "vector.h"

// this and vector.h by Ryan Cummings

// Creates a vector with between two points
Vector::Vector(const QPoint &source, const QPoint &dest) {
	_magnitude = sqrt(pow(source.x() - dest.x(), 2) + pow(source.y() - dest.y(), 2));
	_direction = atan2(source.y() - dest.y(), source.x() - dest.x());
}

// Creates a vector with between two points
Vector::Vector(const Point &source, const Point &dest) {
	_magnitude = sqrt(pow(source.x - dest.x, 2) + pow(source.y - dest.y, 2));
	_direction = atan2(source.y - dest.y, source.x - dest.x);
}

// Creates an empty Vector
Vector::Vector() {
	_magnitude = 0.0;
	_direction = 0.0;
}

// Copy another Vector object
Vector::Vector(const Vector& v) {
	_magnitude = v._magnitude;
	_direction = v._direction;
}

// Set the X component
void Vector::setComponentX(double x) {
	setComponents(x, componentY());
}

// Set the Y component
void Vector::setComponentY(double y) {
	setComponents(componentX(), y);
}

// Operations with another Vector performs vector math
Vector Vector::operator+(const Vector& v) {
	double x = componentX() + v.componentX();
	double y = componentY() + v.componentY();

	return Vector(sqrt((x * x) + (y * y)), atan2(y, x));	
}

Vector Vector::operator-(const Vector& v) {
	double x = componentX() - v.componentX();
	double y = componentY() - v.componentY();

	return Vector(sqrt((x * x) + (y * y)), atan2(y, x));	
}

Vector& Vector::operator+=(const Vector& v) {
	setComponents(componentX() + v.componentX(), componentY() + v.componentY());
	return *this;
}

Vector& Vector::operator-=(const Vector& v) {
	setComponents(componentX() - v.componentX(), componentY() - v.componentY());
	return *this;
}

double Vector::operator*(const Vector& v) {
	return ((componentX() * v.componentX()) + (componentY() * v.componentY()));
}

// Operations with a single double value affects the magnitude
Vector& Vector::operator+= (double m) {
	_magnitude += m;
	return *this;
}

Vector& Vector::operator-= (double m) {
	_magnitude -= m;
	return *this;
}

Vector& Vector::operator*= (double m) {
	_magnitude *= m;
	return *this;
}

Vector& Vector::operator/= (double m) {
	_magnitude /= m;
	return *this;
}

// Sets both components at once (the only way to do it efficently)
void Vector::setComponents(double x, double y) {
	_direction = atan2(y, x);
	_magnitude = sqrt((x * x) + (y * y));
}

void debugPoint(const QString &text, const Point &p)
{
	kdDebug() << text << " (" << p.x << ", " << p.y << ")" << endl;
}

void debugVector(const QString &text, const Vector &p)
{
	kdDebug() << text << " (magnitude: " << p.magnitude() << ", direction: " << p.direction() << ")" << endl;
}
