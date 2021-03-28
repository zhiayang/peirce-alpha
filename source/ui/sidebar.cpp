// sidebar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace imgui = ImGui;
using namespace ui::alpha;

namespace ui
{
	static void editing_tools(Graph* graph);
	static void inference_tools(Graph* graph);
	static Styler disabled_style();

	void draw_sidebar(Graph* graph)
	{
		(void) graph;

		auto EXTRA_PADDING = lx::vec2(5);

		auto& theme = ui::theme();
		auto geom = geometry::get();


		imgui::SetNextWindowPos(geom.sidebar.pos - EXTRA_PADDING);
		imgui::SetNextWindowSize(geom.sidebar.size + EXTRA_PADDING);

		auto s = Styler();
		s.push(ImGuiCol_WindowBg, theme.sidebarBg);
		s.push(ImGuiCol_FrameBg, theme.sidebarBg);
		s.push(ImGuiStyleVar_FramePadding, lx::vec2(4));
		s.push(ImGuiStyleVar_ItemSpacing, lx::vec2(1));
		s.push(ImGuiStyleVar_IndentSpacing, 16);

		imgui::Begin("__sidebar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		{
			imgui::Text("interact"); imgui::Dummy(lx::vec2(4));
			imgui::Indent();
			{
				auto s = Styler();
				if(ui::isEditing())
				{
					s.push(ImGuiCol_ButtonActive, theme.buttonClickedBg);
					s.push(ImGuiCol_ButtonHovered, theme.buttonClickedBg2);
					s.push(ImGuiCol_Button, theme.buttonClickedBg);
				}

				// fa-edit, fa-exchange-alt
				if(imgui::Button(ui::isEditing() ? "\uf044 edit" : "\uf362 infer"))
					ui::toggleEditing();

				s.pop();

				// fa-undo-alt, fa-redo-alt
				if(imgui::Button("\uf2ea undo"))   ui::performUndo(graph);
				if(imgui::Button("\uf2f9 redo"))   ui::performRedo(graph);
			}

			imgui::Unindent();
		}

		imgui::NewLine();

		if(ui::isEditing()) editing_tools(graph);
		else                inference_tools(graph);

		// since window borders are off, we draw the separator manually.
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		imgui::End();
	}



	static void inference_tools(Graph* graph)
	{
		imgui::Text("double cut"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();

		// auto item = selection();
		// {
		// 	auto s = (item == nullptr ? disabled_style() : Styler());
		// 	if(imgui::Button("\uf5ff add"))
		// 		alpha::insertDoubleCut(graph, item);
		// }

		// {
		// 	auto s = !alpha::hasDoubleCut(item) ? disabled_style() : Styler();
		// 	if(imgui::Button("\uf5fe remove"))
		// 		alpha::removeDoubleCut(graph, item);
		// }

		imgui::Unindent();
	}

	static void editing_tools(Graph* graph)
	{
	}


	static Styler disabled_style()
	{
		auto s = Styler();
		s.push(ImGuiItemFlags_Disabled, true);
		s.push(ImGuiStyleVar_Alpha, 0.5);
		return s;
	}
}

