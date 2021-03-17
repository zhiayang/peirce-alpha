// ui.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include "defs.h"

namespace ui
{
	struct Theme;

	void init(zbuf::str_view title);
	void setup(double uiscale, double fontsize, Theme theme);
	void stop();

	// returns false if we should quit.
	bool poll();

	void startFrame();
	void endFrame();

	void draw();

	Theme dark();
	Theme light();
	const Theme& theme();

	struct Theme
	{
		util::colour foreground;
		util::colour background;

		util::colour sidebarBg;
	};

	namespace geom
	{
		double sidebarWidth();
	}
}
