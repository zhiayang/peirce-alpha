// math.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <cmath>

#include "lx.h"

namespace lx
{
	double& vec2::operator[] (size_t i)
	{
		assert(i == 0 || i == 1);
		return this->ptr[i];
	}

	const double& vec2::operator[] (size_t i) const
	{
		assert(i == 0 || i == 1);
		return this->ptr[i];
	}

	vec2 vec2::operator - () const { return vec2(-this->x, -this->y); }

	vec2& vec2::operator += (const vec2& v) { this->x += v.x; this->y += v.y; return *this; }
	vec2& vec2::operator -= (const vec2& v) { this->x -= v.x; this->y -= v.y; return *this; }
	vec2& vec2::operator *= (const vec2& v) { this->x *= v.x; this->y *= v.y; return *this; }
	vec2& vec2::operator /= (const vec2& v) { this->x /= v.x; this->y /= v.y; return *this; }

	vec2& vec2::operator *= (double s) { this->x *= s; this->y *= s; return *this; }
	vec2& vec2::operator /= (double s) { this->x /= s; this->y /= s; return *this; }


	double vec2::magnitude() const { return sqrt(this->magnitudeSquared()); }
	double vec2::magnitudeSquared() const { return (this->x * this->x) + (this->y * this->y); }

	vec2 vec2::normalised() const
	{
		double mag = (this->x * this->x) + (this->y * this->y);
		double ivs = std::sqrt(mag);
		return vec2(this->x * ivs, this->y * ivs);
	}


	vec2 operator + (const vec2& a, const vec2& b) { return vec2(a.x + b.x, a.y + b.y); }
	vec2 operator - (const vec2& a, const vec2& b) { return vec2(a.x - b.x, a.y - b.y); }
	vec2 operator * (const vec2& a, const vec2& b) { return vec2(a.x * b.x, a.y * b.y); }
	vec2 operator / (const vec2& a, const vec2& b) { return vec2(a.x / b.x, a.y / b.y); }
	bool operator == (const vec2& a, const vec2& b) { return a.x == b.x && a.y == b.y; }

	vec2 operator * (const vec2& a, double b) { return vec2(a.x * b, a.y * b); }
	vec2 operator / (const vec2& a, double b) { return vec2(a.x / b, a.y / b); }

	vec2 operator * (double a, const vec2& b) { return b * a; }
	vec2 operator / (double a, const vec2& b) { return b / a; }

	vec2 round(const vec2&v) { return vec2(std::round(v.x), std::round(v.y)); }
	vec2 normalise(const vec2& v) { return v.normalised(); }
	double magnitude(const vec2& v) { return v.magnitude(); }
	double magnitudeSquared(const vec2& v) { return v.magnitudeSquared(); }

	double dot(const vec2& a, const vec2& b)
	{
		return (a.x * b.x) + (a.y * b.y);
	}

	double distance(const vec2& a, const vec2& b)
	{
		return std::sqrt(
			((a.x - b.x) * (a.x - b.x))
			+ ((a.y - b.y) * (a.y - b.y))
		);
	}

	double clamp(double v, double min, double max)
	{
		double ret = (v > max ? max : v);
		return (ret < min ? min : ret);
	}
}









