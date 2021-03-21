// defs.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "zpr.h"
#include "zbuf.h"

#include "lx.h"

namespace lg
{
	bool isDebugEnabled();
	std::string getLogMessagePreambleString(int lvl, zbuf::str_view sys);

	template <typename... Args>
	static inline void __generic_log(int lvl, zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		if(!isDebugEnabled() && lvl < 0)
			return;

		auto out = getLogMessagePreambleString(lvl, sys);

		out += zpr::sprint(fmt, args...);

		if(lvl >= 2)    fprintf(stderr, "%s\n", out.c_str());
		else            printf("%s\n", out.c_str());
	}

	template <typename... Args>
	static void log(zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		__generic_log(0, sys, fmt, args...);
	}

	template <typename... Args>
	static void warn(zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		__generic_log(1, sys, fmt, args...);
	}

	template <typename... Args>
	static void error(zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		__generic_log(2, sys, fmt, args...);
	}

	template <typename... Args>
	[[noreturn]] static void fatal(zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		__generic_log(3, sys, fmt, args...);
		abort();
	}

	template <typename... Args>
	static void dbglog(zbuf::str_view sys, const std::string& fmt, Args&&... args)
	{
		__generic_log(-1, sys, fmt, args...);
	}
}

namespace util
{
	uint32_t loadImageFromFile(zbuf::str_view path);

	namespace random
	{
		template <typename T> T get();
		template <typename T> T get(T min, T max);

		template <typename T> T get_float();
		template <typename T> T get_float(T min, T max);
	}

	struct colour
	{
	#if 0
		operator lx::vec4() const { return this->vec4(); }
		operator lx::fvec4() const { return this->fvec4(); }


		lx::vec4 vec4() const
		{
			return lx::vec4(this->r, this->g, this->b, this->a);
		}

		lx::fvec4 fvec4() const
		{
			return lx::fvec4(this->r, this->g, this->b, this->a);
		}
	#endif

		constexpr colour(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) { }
		constexpr colour(float r, float g, float b) : r(r), g(g), b(b), a(1.0) { }
		constexpr colour() : r(0), g(0), b(0), a(0) { }

		colour(colour&&) = default;
		colour(const colour&) = default;
		colour& operator= (colour&&) = default;
		colour& operator= (const colour&) = default;

		float r;
		float g;
		float b;
		float a;

		constexpr uint32_t u32() const
		{
			uint8_t bytes[4] = {
				(uint8_t) (r * 255.f),
				(uint8_t) (g * 255.f),
				(uint8_t) (b * 255.f),
				(uint8_t) (a * 255.f),
			};

			return (uint32_t) bytes[3] << 24
				| (uint32_t) bytes[2] << 16
				| (uint32_t) bytes[1] << 8
				| (uint32_t) bytes[0] << 0;
		}

		constexpr inline colour operator+(colour other)
		{
			// stop overflow to zero
			return colour(std::max(this->r + other.r, 1.0f), std::max(this->g + other.g, 1.0f),
				std::max(this->b + other.b, 1.0f), std::max(this->a + other.a, 1.0f));
		}

		constexpr static inline colour fromHexRGBA(uint32_t hex)
		{
			return colour(
				((hex & 0xFF000000) >> 24) / 255.0,
				((hex & 0x00FF0000) >> 16) / 255.0,
				((hex & 0x0000FF00) >>  8) / 255.0,
				((hex & 0x000000FF) >>  0) / 255.0
			);
		}

		constexpr static inline colour fromHexRGB(uint32_t hex)
		{
			return colour(
				((hex & 0xFF0000) >> 16) / 255.0,
				((hex & 0x00FF00) >>  8) / 255.0,
				((hex & 0x0000FF) >>  0) / 255.0
			);
		}

		static constexpr colour black()   { return colour(0.0, 0.0, 0.0); }
		static constexpr colour white()   { return colour(1.0, 1.0, 1.0); }

		static constexpr colour red()     { return colour(1.0, 0.0, 0.0); }
		static constexpr colour blue()    { return colour(0.0, 0.0, 1.0); }
		static constexpr colour green()   { return colour(0.0, 1.0, 0.0); }
		static constexpr colour cyan()    { return colour::green() + colour::blue(); }
		static constexpr colour yellow()  { return colour::red() + colour::green(); }
		static constexpr colour magenta() { return colour::blue() + colour::red(); }

		static inline colour random()
		{
			return colour(random::get_float(0.2, 0.9),
				random::get_float(0.2, 0.9),
				random::get_float(0.2, 0.9));
		}
	};
}



namespace unicode
{
	size_t is_category(zbuf::str_view str, const std::initializer_list<int>& categories);
	size_t get_codepoint_length(zbuf::str_view str);
	size_t is_letter(zbuf::str_view str);
	size_t is_digit(zbuf::str_view str);
}



namespace zpr
{
	template <>
	struct print_formatter<lx::vec2>
	{
		template <typename Cb>
		void print(const lx::vec2& v, Cb&& cb, format_args args)
		{
			cb("(");
			detail::print_one(static_cast<Cb&&>(cb), args, v.x);
			cb(", ");
			detail::print_one(static_cast<Cb&&>(cb), args, v.y);
			cb(")");
		}
	};
}
