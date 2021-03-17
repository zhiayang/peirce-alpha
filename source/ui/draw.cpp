// draw.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;

namespace ui
{
	static void draw_sidebar();

	void draw()
	{
		// auto& io = imgui::GetIO();
		// auto& theme = ui::theme();
		draw_sidebar();
	}



	static void draw_sidebar()
	{
		auto EXTRA_PADDING = lx::vec2(5);

		auto& io = imgui::GetIO();
		auto& theme = ui::theme();

		imgui::SetNextWindowPos(-1 * EXTRA_PADDING);
		imgui::SetNextWindowSize(lx::vec2(geom::sidebarWidth(), io.DisplaySize.y) + EXTRA_PADDING);

		imgui::PushStyleColor(ImGuiCol_WindowBg, theme.sidebarBg);

		imgui::Begin(" ", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		imgui::Text("asdf %s", "bsdf");

		// since window borders are off, we draw the separator manually.
		{
			imgui::GetBackgroundDrawList()->AddLine(
				lx::vec2(geom::sidebarWidth(), -EXTRA_PADDING.y),
				lx::vec2(geom::sidebarWidth(), io.DisplaySize.y + EXTRA_PADDING.y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		imgui::End();
		imgui::PopStyleColor();
	}
}
