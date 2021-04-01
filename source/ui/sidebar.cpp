// sidebar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"
#include "alpha.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace imgui = ImGui;
using namespace alpha;

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
				if(imgui::Button(edit ? "\uf044 edit " : "\uf362 infer "))
					edit = ui::toggleTool(TOOL_EDIT);
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

			if(edit)
			{
				// fa-copy, fa-cut, fa-paste
				imgui::Button("\uf0c5 copy ");
				imgui::Button("\uf0c4 cut ");
				imgui::Button("\uf0ea paste ");
			}
		}

		imgui::Unindent();
	}


	static void inference_tools(Graph* graph)
	{
		auto geom = geometry::get();
		auto& theme = ui::theme();
		auto& sel = selection();

		imgui::Text("inference"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			// insert anything into an odd depth
			{
				/*
					the thing is, we are placing in a box --- so the "final resting place"
					of the inserted item is actually 1 + the depth of the box -- so we
					need to check that the box is an even depth!

					the other good thing is that we don't need to handle the case where we want
					to insert into the outermost "grid" (ie. the field itself) because anything
					added there will be at an even depth -- so the inference rule prevents that
					from happening entirely.
				*/
				auto en = sel.count() == 1
					&& sel[0]->isBox
					&& sel[0]->depth() % 2 == 0;


				// static!
				static char buf[17] = { };

				auto s = disabled_style(!en || strlen(buf) == 0);

				// fa-plus-circle
				bool insert = imgui::Button("\uf055 insert "); imgui::SameLine();

				s.pop();
				{
					auto s = Styler();
					imgui::SetNextItemWidth(64);
					s.push(ImGuiCol_FrameBg, theme.tooltipBg);

					imgui::InputTextWithHint("", " prop", buf, 16);
				}

				auto prop = std::string(buf, strlen(buf));

				if(insert)
				{
					alpha::insertAtOddDepth(graph, /* parent: */ sel[0], Item::var(prop));
				}
			}

			{
				// erase anything from an even depth
				auto en = sel.count() == 1
					&& sel[0]->depth() % 2 == 0;

				// fa-minus-circle
				auto s = disabled_style(!en);
				if(imgui::Button("\uf056 erase "))
				{
					alpha::eraseFromEvenDepth(graph, sel[0]);
					sel.clear();
				}
			}
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
				bool desel = (sel.count() == 0 && graph->iteration_target != nullptr);

				// if the selection is empty, make it deselect; selectTargetForIteration knows
				// how to handle nullptrs.
				auto en = (sel.count() == 1) || desel;
				auto s = disabled_style(!en);

				// crosshairs
				if(imgui::Button(desel ? "\uf05b deselect " : "\uf05b select "))
					alpha::selectTargetForIteration(graph, sel.count() == 1 ? sel[0] : nullptr);
			}

			{
				// map-marker-plus
				auto s = disabled_style(sel.count() != 1 || !alpha::canIterateInto(graph, sel[0]));
				if(imgui::Button("\uf60a iterate "))
					alpha::iterate(graph, sel[0]);
			}

			{
				auto deiterable = [&](const Item* item) -> bool {
					return graph->deiteration_targets.find(item) != graph->deiteration_targets.end();
				};

				// map-marker-minus
				auto s = disabled_style(sel.count() != 1 || !deiterable(sel[0]));
				if(imgui::Button("\uf609 deiterate "))
					alpha::deiterate(graph, sel[0]);
			}
		}
		imgui::Unindent();

		imgui::NewLine();

		if(alpha::haveIterationTarget(graph))
		{
			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 56));
			imgui::TextUnformatted("iteration target");
		}

		if(sel.count() == 1)
		{
			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 34));
			imgui::TextUnformatted(zpr::sprint("depth: {}", sel[0]->depth()).c_str());
		}
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

