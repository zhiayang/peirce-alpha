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

namespace alpha
{
	// util/solver.cpp
	void abort_solve();
}

namespace ui
{
	static void interaction_tools(Graph* graph);

	Styler disabled_style(bool disabled);
	Styler toggle_enabled_style(bool enabled);

	static bool was_eval = false;

	static char propNameBuf[16];
	static size_t PROP_NAME_LEN = 16;

	Styler flash_style(int button)
	{
		return toggle_enabled_style(ui::buttonFlashed(button));
	}

	// used in interact.cpp
	const char* get_prop_name() { return propNameBuf; }
	size_t get_prop_capacity() { return PROP_NAME_LEN; }

	// defined in interact.cpp
	bool is_mouse_in_bounds();

	// sidebar/inference.cpp
	void inference_tools(Graph* graph);

	// sidebar/edit.cpp
	void editing_tools(Graph* graph);

	// sidebar/evaluate.cpp
	void eval_tools(Graph* graph);
	void rescan_variables(Graph* graph);
	void set_flags(Graph* graph, const std::unordered_map<std::string, bool>& soln);

	void draw_sidebar(Graph* graph)
	{
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
		s.push(ImGuiStyleVar_IndentSpacing, 4);
		s.push(ImGuiStyleVar_FrameRounding, 2);

		imgui::Begin("__sidebar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

		// since window borders are off, we draw the separator manually.
		imgui::GetForegroundDrawList()->AddLine(
			lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
			lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
			theme.foreground.u32(), /* thickness: */ 2.0
		);

		rescan_variables(graph);

		interaction_tools(graph);
		imgui::NewLine();

		if(toolEnabled(TOOL_EVALUATE))  was_eval = true, eval_tools(graph);
		else if(toolEnabled(TOOL_EDIT)) editing_tools(graph);
		else                            inference_tools(graph);

		if(!toolEnabled(TOOL_EVALUATE))
		{
			ui::resetEvalExpr();
			if(was_eval)
				set_flags(graph, { });

			was_eval = false;
			alpha::abort_solve();
		}

		{
			auto s = Styler();
			s.push(ImGuiStyleVar_FramePadding, lx::vec2(0, 4));
			imgui::SetCursorPos(lx::vec2(geom.sidebar.size.x - 40, geom.sidebar.size.y - 36));
			if(imgui::Button("\uf0eb"))
			{
				if(ui::theme().dark)
					ui::setTheme(ui::light());
				else
					ui::setTheme(ui::dark());
			}
		}

		imgui::End();

		/*
			As an aside, the reason none of this code has to handle the mouse-cursor case
			(eg. when you want to paste at a mouse position) is because in order to click
			the button, your mouse must be outside of the graph area. so we can always
			enforce the "needs something selected" constraint here.
		*/
	}



	static void interaction_tools(Graph* graph)
	{
		auto& theme = ui::theme();

		bool eval = toolEnabled(TOOL_EVALUATE);
		bool edit = toolEnabled(TOOL_EDIT);
		bool move = toolEnabled(TOOL_MOVE);
		bool resize = toolEnabled(TOOL_RESIZE);

		{
			lx::vec2 cursor = imgui::GetCursorPos();
			imgui::Text("interact");

			auto s = Styler();
			s.push(ImGuiStyleVar_FramePadding, lx::vec2(0, 4));
			s.push(ImGuiStyleVar_FrameRounding, 2);

			// fa-undo-alt
			{
				auto s = disabled_style(!ui::canUndo() || eval);
				auto ss = flash_style(SB_BUTTON_UNDO);

				imgui::SetCursorPos(cursor + lx::vec2(144, -4));
				if(imgui::Button("\uf2ea"))
					ui::performUndo(graph);
			}

			// fa-redo-alt
			{
				auto s = disabled_style(!ui::canRedo() || eval);
				auto ss = flash_style(SB_BUTTON_REDO);

				imgui::SetCursorPos(cursor + lx::vec2(174, -4));
				if(imgui::Button("\uf2f9"))
					ui::performRedo(graph);
			}
		}

		imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			if(auto s = toggle_enabled_style(eval); true)
			{
				// inbox-in
				if(imgui::Button("f \uf310 evaluate"))
					eval = ui::toggleTool(TOOL_EVALUATE);
			}

			if(!eval)
			{
				if(auto s = toggle_enabled_style(edit); true)
				{
					// fa-edit, fa-exchange-alt
					if(imgui::Button(edit ? "e \uf044 edit " : "e \uf362 infer "))
						edit = ui::toggleTool(TOOL_EDIT);
				}

				if(auto s = toggle_enabled_style(move); true)
				{
					// fa-arrows
					if(imgui::Button("m \uf047 move "))
						ui::toggleTool(TOOL_MOVE);

					// obviously only show the detached thingy when in edit mode
					if(imgui::GetIO().KeyAlt && ui::toolEnabled(TOOL_EDIT) && is_mouse_in_bounds())
					{
						auto ss = Styler(); ss.push(ImGuiCol_Text, theme.boxSelection);
						imgui::SameLine(138);
						imgui::Text("detached");
					}
				}

				if(auto s = toggle_enabled_style(resize); true)
				{
					// fa-expand-alt
					if(imgui::Button("r \uf424 resize "))
						ui::toggleTool(TOOL_RESIZE);
				}

				if(edit)
				{
					// fa-copy, fa-cut, fa-paste
					{
						auto s = disabled_style(!ui::canCopyOrCut());
						{
							auto ss = flash_style(SB_BUTTON_COPY);
							if(imgui::Button("c \uf0c5 copy "))
								ui::performCopy(graph);
						}

						{
							auto ss = flash_style(SB_BUTTON_CUT);
							if(imgui::Button("x \uf0c4 cut "))
								ui::performCut(graph);
						}
					}

					{
						auto s = disabled_style(!ui::canPaste());
						auto ss = flash_style(SB_BUTTON_PASTE);
						if(imgui::Button("v \uf0ea paste "))
							ui::performPaste(graph, ui::selection()[0]);
					}
				}
			}
		}

		imgui::Unindent();
	}

	// since we need to reuse this button in two places, and it's a  little bit of a hassle,
	// factor it out. if the button was clicked, it returns the prop name; else it returns empty.
	zbuf::str_view insert_prop_button(int shortcut_num, bool enabled)
	{
		auto& theme = ui::theme();
		auto s = disabled_style(!enabled || strlen(propNameBuf) == 0);
		auto ss = flash_style(SB_BUTTON_INSERT);

		char button_text[] = "_ \uf055 insert ";
		button_text[0] = shortcut_num + '0';

		bool insert = imgui::Button(button_text); imgui::SameLine();
		s.pop();
		ss.pop();

		{
			auto s = Styler();
			imgui::SetNextItemWidth(64);
			s.push(ImGuiCol_FrameBg, theme.textFieldBg);
			imgui::InputTextWithHint("", " prop", propNameBuf, PROP_NAME_LEN);
		}

		if(insert)
			return zbuf::str_view(propNameBuf, strlen(propNameBuf));

		else
			return { };
	}

	Styler disabled_style(bool disabled)
	{
		auto s = Styler();
		if(disabled)
		{
			s.push(ImGuiItemFlags_Disabled, true);
			s.push(ImGuiStyleVar_Alpha, 0.5);
		}
		return s;
	}

	Styler toggle_enabled_style(bool enabled)
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

