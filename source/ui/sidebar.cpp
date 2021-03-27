// sidebar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace ui::alpha;

namespace ui
{
	void draw_sidebar(Graph* graph)
	{
		(void) graph;

		auto EXTRA_PADDING = lx::vec2(5);

		auto& theme = ui::theme();
		auto geom = geometry::get();


		imgui::SetNextWindowPos(geom.sidebar.pos - EXTRA_PADDING);
		imgui::SetNextWindowSize(geom.sidebar.size + EXTRA_PADDING);

		imgui::PushStyleColor(ImGuiCol_WindowBg, theme.sidebarBg);
		imgui::PushStyleColor(ImGuiCol_FrameBg, theme.sidebarBg);

		imgui::PushStyleColor(ImGuiCol_ButtonActive, util::colour::blue());
		imgui::PushStyleColor(ImGuiCol_ButtonHovered, util::colour::red());
		imgui::PushStyleColor(ImGuiCol_Button, util::colour::fromHexRGB(0xe6e6e6));

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

		if(imgui::ImageButton(theme.textures.edit, lx::vec2(32, 32)))
			ui::toggleEditing();


		// since window borders are off, we draw the separator manually.
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		imgui::End();
		imgui::PopStyleColor(5);
	}
}
