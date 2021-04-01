// geometry.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace ui::geometry
{
	constexpr double SIDEBAR_MIN_WIDTH = 230;
	constexpr double SIDEBAR_MAX_WIDTH = 350;
	constexpr double SIDEBAR_WIDTH_PERCENT = 0.22;

	static Geometry geom;

	double sidebarWidth()
	{
		return lx::clamp(SIDEBAR_WIDTH_PERCENT * ImGui::GetIO().DisplaySize.x,
			SIDEBAR_MIN_WIDTH, SIDEBAR_MAX_WIDTH);
	}

	void update()
	{
		auto& io = ImGui::GetIO();

		auto sidebarWidth = lx::clamp(SIDEBAR_WIDTH_PERCENT * io.DisplaySize.x, SIDEBAR_MIN_WIDTH, SIDEBAR_MAX_WIDTH);
		auto exprbarHeight = 40.0;

		geom = {
			.sidebar = {
				.pos = { 0, 0 },
				.size = { sidebarWidth, io.DisplaySize.y }
			},
			.graph = {
				.pos = { sidebarWidth, 0 },
				.size = { io.DisplaySize.x - sidebarWidth, io.DisplaySize.y - exprbarHeight }
			},
			.exprbar = {
				.pos = { sidebarWidth, io.DisplaySize.y - exprbarHeight },
				.size = { io.DisplaySize.x - sidebarWidth, exprbarHeight }
			},
			.display = {
				.size  = { io.DisplaySize.x, io.DisplaySize.y }
			},
		};
	}

	Geometry get()
	{
		return geom;
	}
}
