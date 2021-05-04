// util.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <time.h>
#include <chrono>
#include <string>
#include <random>

#include "defs.h"
#include "utf8proc/utf8proc.h"

namespace lg
{
	constexpr bool ENABLE_DEBUG = false;

#if defined(__EMSCRIPTEN__)
	constexpr bool USE_COLOURS  = false;
#else
	constexpr bool USE_COLOURS  = true;
#endif

	constexpr const char* DBG    = (USE_COLOURS ? "\x1b[37m" : "");
	constexpr const char* LOG    = (USE_COLOURS ? "\x1b[30;1m" : "");
	constexpr const char* WRN    = (USE_COLOURS ? "\x1b[1m\x1b[33m" : "");
	constexpr const char* ERR    = (USE_COLOURS ? "\x1b[1m\x1b[31m" : "");
	constexpr const char* FTL    = (USE_COLOURS ? "\x1b[1m\x1b[37m\x1b[48;5;9m" : "");

	constexpr const char* RESET  = (USE_COLOURS ? "\x1b[0m" : "");
	constexpr const char* SUBSYS = (USE_COLOURS ? "\x1b[1m\x1b[34m" : "");

	std::string getCurrentTimeString()
	{
		auto tp = std::chrono::system_clock::now();
		auto t = std::chrono::system_clock::to_time_t(tp);

		std::tm tm;
		if(localtime_r(&t, &tm) == nullptr)
			return "??";

		return zpr::sprint("{02}:{02}:{02}", tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	std::string getLogMessagePreambleString(int lvl, zbuf::str_view sys)
	{
		const char* lvlcolour = 0;
		const char* str = 0;

		if(lvl == -1)       { lvlcolour = DBG;  str = "[dbg]"; }
		else if(lvl == 0)   { lvlcolour = LOG;  str = "[log]"; }
		else if(lvl == 1)   { lvlcolour = WRN;  str = "[wrn]"; }
		else if(lvl == 2)   { lvlcolour = ERR;  str = "[err]"; }
		else if(lvl == 3)   { lvlcolour = FTL;  str = "[ftl]"; }

		auto timestamp = zpr::sprint("{} {}|{}", getCurrentTimeString(), "\x1b[1m\x1b[37m", RESET);
		auto loglevel  = zpr::sprint("{}{}{}", lvlcolour, str, RESET);
		auto subsystem = zpr::sprint("{}{}{}", SUBSYS, sys, RESET);

		return zpr::sprint("{} {} {}: ", timestamp, loglevel, subsystem);
	}

	bool isDebugEnabled() { return ENABLE_DEBUG; }
}

namespace util::random
{
	// this is kinda dumb but... meh.
	template <typename T>
	struct rd_state_t
	{
		rd_state_t() : mersenne(std::random_device()()) { }

		std::mt19937 mersenne;
	};

	template <typename T>
	rd_state_t<T> rd_state;

	template <typename T>
	T get()
	{
		auto& st = rd_state<T>;
		return std::uniform_int_distribution<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max())(st.mersenne);
	}

	template <typename T>
	T get(T min, T max)
	{
		auto& st = rd_state<T>;
		return std::uniform_int_distribution<T>(min, max)(st.mersenne);
	}

	template <typename T>
	T get_float()
	{
		auto& st = rd_state<T>;
		return std::uniform_real_distribution<T>(-1, +1)(st.mersenne);
	}

	template <typename T>
	T get_float(T min, T max)
	{
		auto& st = rd_state<T>;
		return std::uniform_real_distribution<T>(min, max)(st.mersenne);
	}

	template unsigned char  get<unsigned char>();
	template unsigned char  get<unsigned char>(unsigned char, unsigned char);

	template unsigned short get<unsigned short>();
	template unsigned short get<unsigned short>(unsigned short, unsigned short);

	template unsigned int get<unsigned int>();
	template unsigned int get<unsigned int>(unsigned int, unsigned int);

	template unsigned long get<unsigned long>();
	template unsigned long get<unsigned long>(unsigned long, unsigned long);

	template unsigned long long get<unsigned long long>();
	template unsigned long long get<unsigned long long>(unsigned long long, unsigned long long);

	template char get<char>();
	template char get<char>(char, char);

	template short get<short>();
	template short get<short>(short, short);

	template int get<int>();
	template int get<int>(int, int);

	template long get<long>();
	template long get<long>(long, long);

	template long long get<long long>();
	template long long get<long long>(long long, long long);

	template double get_float<double>();
	template double get_float<double>(double, double);
}


namespace unicode
{
	size_t is_category(zbuf::str_view str, const std::initializer_list<int>& categories)
	{
		utf8proc_int32_t cp = { };
		auto sz = utf8proc_iterate((const uint8_t*) str.data(), str.size(), &cp);
		if(cp == -1)
			return 0;

		auto cat = utf8proc_category(cp);
		for(auto c : categories)
			if(cat == c)
				return sz;

		return 0;
	}

	size_t get_codepoint_length(zbuf::str_view str)
	{
		utf8proc_int32_t cp = { };
		auto sz = utf8proc_iterate((const uint8_t*) str.data(), str.size(), &cp);
		if(cp == -1)
			return 1;

		return sz;
	}

	size_t is_letter(zbuf::str_view str)
	{
		return is_category(str, {
			UTF8PROC_CATEGORY_LU, UTF8PROC_CATEGORY_LL, UTF8PROC_CATEGORY_LT,
			UTF8PROC_CATEGORY_LM, UTF8PROC_CATEGORY_LO
		});
	}

	size_t is_digit(zbuf::str_view str)
	{
		return is_category(str, {
			UTF8PROC_CATEGORY_ND
		});
	}
}
