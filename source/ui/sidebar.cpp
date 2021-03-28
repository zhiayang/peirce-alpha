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
	static void interaction_tools(Graph* graph);
	static Styler disabled_style(bool disabled);
	static Styler toggle_enabled_style(bool enabled);

	void draw_sidebar(Graph* graph)
	{
		(void) graph;

		auto EXTRA_PADDING = lx::vec2(5);

		auto& theme = ui::theme();
		auto geom = geometry::get();

		imgui::SetNextWindowPos(geom.sidebar.pos);
		imgui::SetNextWindowSize(geom.sidebar.size);

		auto s = Styler();
		s.push(ImGuiCol_WindowBg, theme.sidebarBg);
		s.push(ImGuiCol_FrameBg, theme.sidebarBg);
		s.push(ImGuiStyleVar_FramePadding, lx::vec2(4));
		s.push(ImGuiStyleVar_ItemSpacing, lx::vec2(1));
		s.push(ImGuiStyleVar_IndentSpacing, 16);
		s.push(ImGuiStyleVar_FrameRounding, 2);

		imgui::Begin("__sidebar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		// since window borders are off, we draw the separator manually.
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		interaction_tools(graph);
		imgui::NewLine();

		if(toolEnabled(TOOL_EDIT))  editing_tools(graph);
		else                        inference_tools(graph);


		imgui::End();
	}



	static void interaction_tools(Graph* graph)
	{
		auto& theme = ui::theme();
		{
			lx::vec2 cursor = imgui::GetCursorPos();
			imgui::Text("interact");

			auto s = Styler();
			s.push(ImGuiStyleVar_FramePadding, lx::vec2(0, 4));
			s.push(ImGuiStyleVar_FrameRounding, 2);

			// fa-undo-alt
			imgui::SetCursorPos(cursor + lx::vec2(135, -4));
			if(imgui::Button("\uf2ea"))
				ui::performUndo(graph);

			// fa-redo-alt
			imgui::SetCursorPos(cursor + lx::vec2(165, -4));
			if(imgui::Button("\uf2f9"))
				ui::performRedo(graph);
		}

		imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			bool edit = toolEnabled(TOOL_EDIT);
			bool move = toolEnabled(TOOL_MOVE);
			bool resize = toolEnabled(TOOL_RESIZE);

			if(auto s = toggle_enabled_style(edit); true)
			{
				// fa-edit, fa-exchange-alt
				// note that enabling edit will always enable the move tool.
				if(imgui::Button(edit ? "\uf044 edit " : "\uf362 infer "))
					ui::toggleTool(TOOL_EDIT) ? ui::enableTool(TOOL_MOVE) : (void) 0;
			}

			if(auto s = toggle_enabled_style(move); true)
			{
				// fa-arrows
				if(imgui::Button("\uf047 move "))
					ui::toggleTool(TOOL_MOVE);

				if(auto& s = ui::selection(); s.count() == 1 && s[0]->flags & FLAG_DETACHED)
				{
					auto ss = Styler(); ss.push(ImGuiCol_Text, theme.boxSelection);
					imgui::SameLine(130);
					imgui::Text("detached");
				}
			}

			if(auto s = toggle_enabled_style(resize); true)
			{
				// fa-expand-alt
				if(imgui::Button("\uf424 resize "))
					ui::toggleTool(TOOL_RESIZE);
			}

			// fa-cut, fa-copy, fa-paste
			imgui::Button("\uf0c5 copy ");
			imgui::Button("\uf0c4 cut ");
			imgui::Button("\uf0ea paste ");
		}

		imgui::Unindent();
	}


	static void inference_tools(Graph* graph)
	{
		auto& theme = ui::theme();
		auto& sel = selection();

		imgui::Text("inference"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			// fa-plus-circle
			imgui::Button("\uf055 insert "); imgui::SameLine();

			auto s = Styler();
			imgui::SetNextItemWidth(64);
			s.push(ImGuiCol_FrameBg, theme.tooltipBg);

			// static!
			static char buf[17] = { };
			imgui::InputTextWithHint("", " prop", buf, 16);


			// fa-minus-circle
			imgui::Button("\uf056 erase ");
		}
		imgui::Unindent();

		imgui::NewLine();

		imgui::Text("double cut"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				auto s = disabled_style(sel.empty() || !sel.allSiblings());
				if(imgui::Button("\uf5ff add "))
					alpha::insertDoubleCut(graph, sel);
			}

			{
				auto enable = (sel.count() == 1 || (sel.count() > 1 && sel.allSiblings())) && hasDoubleCut(sel[0]);
				auto s = disabled_style(!enable);
				if(imgui::Button("\uf5fe remove "))
					alpha::removeDoubleCut(graph, sel);
			}
		}
		imgui::Unindent();

		imgui::NewLine();

		imgui::Text("iteration"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				// map-marker-plus
				// auto s = disabled_style(sel.empty() || !sel.allSiblings());
				if(imgui::Button("\uf60a iterate "))
					;
			}

			{
				// map-marker-minus
				// auto enable = (sel.count() == 1 || (sel.count() > 1 && sel.allSiblings())) && hasDoubleCut(sel[0]);
				// auto s = disabled_style(!enable);
				if(imgui::Button("\uf609 deiterate "))
					;
			}
		}
		imgui::Unindent();
	}

	static void editing_tools(Graph* graph)
	{
	}













	static Styler disabled_style(bool disabled)
	{
		auto s = Styler();
		if(disabled)
		{
			s.push(ImGuiItemFlags_Disabled, true);
			s.push(ImGuiStyleVar_Alpha, 0.5);
		}
		return s;
	}

	static Styler toggle_enabled_style(bool enabled)
	{
		auto& theme = ui::theme();

		auto s = Styler();
		if(enabled)
		{
			s.push(ImGuiCol_ButtonActive, theme.buttonClickedBg);
			s.push(ImGuiCol_ButtonHovered, theme.buttonClickedBg2);
			s.push(ImGuiCol_Button, theme.buttonClickedBg);
		}
		return s;
	}
}

