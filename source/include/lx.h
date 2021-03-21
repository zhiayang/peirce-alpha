// lx.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <stddef.h>

struct ImVec2;

namespace lx
{
	struct vec2
	{
		vec2(const ImVec2& imv2);
		vec2(double a, double b) : x(a), y(b) { }

		vec2() : x(0.0f), y(0.0f) { }
		explicit vec2(double a) : x(a), y(a) { }

		union
		{
			double ptr[2];
			struct { double x; double y; };
			struct { double u; double v; };
		};

		double& operator[] (size_t i);
		const double& operator[] (size_t i) const;
		vec2 operator- () const;

		vec2& operator+= (const vec2& v);
		vec2& operator-= (const vec2& v);

		vec2& operator*= (const vec2& v);
		vec2& operator/= (const vec2& v);

		vec2& operator*= (double s);
		vec2& operator/= (double s);

		double magnitudeSquared() const;
		double magnitude() const;
		vec2 normalised() const;

		// swizzle
		vec2 xy() const;
		vec2 yx() const;
		vec2 xx() const;
		vec2 yy() const;
	};


	vec2 operator+ (const vec2& a, const vec2& b);
	vec2 operator- (const vec2& a, const vec2& b);
	vec2 operator* (const vec2& a, const vec2& b);
	vec2 operator/ (const vec2& a, const vec2& b);
	bool operator== (const vec2& a, const vec2& b);
	bool operator!= (const vec2& a, const vec2& b);

	vec2 operator* (const vec2& a, double b);
	vec2 operator/ (const vec2& a, double b);
	vec2 operator* (double a, const vec2& b);
	vec2 operator/ (double a, const vec2& b);

	// bool operator< (const vec2& a, const vec2& b);
	// bool operator> (const vec2& a, const vec2& b);
	// bool operator<= (const vec2& a, const vec2& b);
	// bool operator>= (const vec2& a, const vec2& b);

	vec2 round(const vec2& v);
	vec2 normalise(const vec2& v);
	double magnitude(const vec2& v);
	double magnitudeSquared(const vec2& v);

	vec2 min(const vec2& a, const vec2& b);
	vec2 max(const vec2& a, const vec2& b);
	double dot(const vec2& a, const vec2& b);
	double distance(const vec2& a, const vec2& b);

	double clamp(double value, double min, double max);

	bool inRect(const vec2& pt, const vec2& corner, const vec2& size);
}
