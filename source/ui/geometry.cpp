// geometry.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace ui::geom
{
	constexpr double SIDEBAR_MIN_WIDTH = 250;
	constexpr double SIDEBAR_MAX_WIDTH = 400;
	constexpr double SIDEBAR_WIDTH_PERCENT = 0.22;

	double sidebarWidth()
	{
		return lx::clamp(SIDEBAR_WIDTH_PERCENT * ImGui::GetIO().DisplaySize.x,
			SIDEBAR_MIN_WIDTH, SIDEBAR_MAX_WIDTH);
	}
}
