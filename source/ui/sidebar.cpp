// sidebar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;

namespace ui
{
	void draw_sidebar(alpha::Graph* graph)
	{
		(void) graph;

		auto EXTRA_PADDING = lx::vec2(5);

		auto& theme = ui::theme();
		auto geom = geometry::get();


		imgui::SetNextWindowPos(geom.sidebar.pos - EXTRA_PADDING);
		imgui::SetNextWindowSize(geom.sidebar.size + EXTRA_PADDING);

		imgui::PushStyleColor(ImGuiCol_WindowBg, theme.sidebarBg);

		imgui::Begin("__sidebar", nullptr,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		imgui::Text("asdf %s", "bsdf");
		imgui::Text("asdf 1");
		imgui::Text("asdf 2");
		imgui::Text("asdf 3");
		imgui::Text("asdf 4");
		imgui::Text("asdf 5");
		imgui::Text("asdf 6");
		imgui::Text("asdf 7");
		imgui::Text("asdf 8");
		imgui::Text("asdf 9");
		imgui::Text("asdf 10");

		// since window borders are off, we draw the separator manually.
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		imgui::End();
		imgui::PopStyleColor();
	}
}
