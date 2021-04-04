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
	static void eval_tools(Graph* graph);
	static void editing_tools(Graph* graph);
	static void inference_tools(Graph* graph);
	static void interaction_tools(Graph* graph);
	static Styler disabled_style(bool disabled);

	Styler toggle_enabled_style(bool enabled);

	static char propNameBuf[16];
	static size_t PROP_NAME_LEN = 16;
	static std::set<std::string> foundVariables;
	static std::unordered_map<std::string, bool> varAssigns;

	static Styler flash_style(int button)
	{
		return toggle_enabled_style(ui::buttonFlashed(button));
	}

	static void find_variables(ast::Expr* expr);

	// used in interact.cpp
	const char* get_prop_name() { return propNameBuf; }

	// defined in interact.cpp
	bool is_mouse_in_bounds();

	// defined in exprbar.cpp
	ast::Expr* get_cached_expr(Graph* graph);
	std::string expr_to_string(ast::Expr* expr);


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
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos - EXTRA_PADDING).y),
				lx::vec2((geom.sidebar.pos + geom.sidebar.size).x, (geom.sidebar.pos + geom.sidebar.size + EXTRA_PADDING).y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		if(graph->flags & FLAG_GRAPH_MODIFIED || foundVariables.empty())
			find_variables(get_cached_expr(graph));

		interaction_tools(graph);
		imgui::NewLine();

		if(toolEnabled(TOOL_EVALUATE))  eval_tools(graph);
		else if(toolEnabled(TOOL_EDIT)) editing_tools(graph);
		else                            inference_tools(graph);

		if(!toolEnabled(TOOL_EVALUATE))
			ui::resetEvalExpr();

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
	static zbuf::str_view insert_prop_button(int shortcut_num, bool enabled)
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
			s.push(ImGuiCol_FrameBg, theme.tooltipBg);
			imgui::InputTextWithHint("", " prop", propNameBuf, PROP_NAME_LEN);
		}

		if(insert)
			return zbuf::str_view(propNameBuf, strlen(propNameBuf));

		else
			return { };
	}



	static void inference_tools(Graph* graph)
	{
		imgui::PushID("__scope_infer");

		auto geom = geometry::get();
		auto& sel = selection();

		imgui::Text("inference"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			// insert anything into an odd depth
			if(auto prop = insert_prop_button(1, alpha::canInsert(graph, propNameBuf)); !prop.empty())
				alpha::insertAtOddDepth(graph, /* parent: */ sel[0], Item::var(prop));

			{
				// fa-minus-circle
				auto s = disabled_style(!alpha::canErase(graph));
				auto ss = flash_style(SB_BUTTON_ERASE);
				if(imgui::Button("2 \uf056 erase "))
					alpha::eraseFromEvenDepth(graph, sel[0]);
			}
		}
		imgui::Unindent();

		imgui::NewLine();

		imgui::Text("double cut"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				auto s = disabled_style(!alpha::canInsertDoubleCut(graph));
				auto ss = flash_style(SB_BUTTON_DBL_ADD);
				if(imgui::Button("3 \uf5ff add "))
					alpha::insertDoubleCut(graph, sel);
			}

			{
				auto s = disabled_style(!alpha::canRemoveDoubleCut(graph));
				auto ss = flash_style(SB_BUTTON_DBL_DEL);
				if(imgui::Button("4 \uf5fe remove "))
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

				// crosshairs
				auto s = disabled_style(!alpha::canSelect(graph));
				auto ss = flash_style(SB_BUTTON_SELECT);
				if(imgui::Button(desel ? "5 \uf05b deselect " : "5 \uf05b select "))
					alpha::selectTargetForIteration(graph, sel.count() == 1 ? sel[0] : nullptr);
			}

			{
				// map-marker-plus
				auto s = disabled_style(!alpha::canIterate(graph));
				auto ss = flash_style(SB_BUTTON_ITER);
				if(imgui::Button("6 \uf60a iterate "))
					alpha::iterate(graph, sel[0]);
			}

			{
				// map-marker-minus
				auto s = disabled_style(!alpha::canDeiterate(graph));
				auto ss = flash_style(SB_BUTTON_DEITER);
				if(imgui::Button("7 \uf609 deiterate "))
					alpha::deiterate(graph, sel[0]);
			}
		}
		imgui::Unindent();


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

		imgui::PopID();
	}

	static void editing_tools(Graph* graph)
	{
		imgui::PushID("__scope_editing");

		auto& sel = selection();

		imgui::Text("editing"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();

		{
			auto s = disabled_style(selection().empty());
			auto ss = flash_style(SB_BUTTON_E_DELETE);
			if(imgui::Button("  \uf55a delete "))
			{
				auto items = sel.get();
				for(auto i : items)
					alpha::eraseItemFromParent(graph, i);
			}
		}

		{
			bool en = sel.count() == 1 || (sel.empty() && is_mouse_in_bounds());
			if(auto prop = insert_prop_button(1, en); !prop.empty() && en)
				alpha::insert(graph, sel[0], Item::var(prop));
		}

		{
			bool en = sel.count() == 1 || (sel.empty() && is_mouse_in_bounds());
			auto s = disabled_style(!en);
			auto ss = flash_style(SB_BUTTON_E_ADD_BOX);
			if(imgui::Button("2 \uf0fe add cut "))
				alpha::insertEmptyBox(graph, sel[0], lx::vec2(0, 0));
		}

		{
			bool en = !sel.empty() && sel.allSiblings();
			auto s = disabled_style(!en);
			auto ss = flash_style(SB_BUTTON_E_SURROUND);
			if(imgui::Button("3 \uf853 surround cut "))
				alpha::surround(graph, sel);
		}

		imgui::Unindent();

		if(auto clips = ui::getClipboard().size(); clips > 0)
		{
			auto geom = geometry::get();

			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 34));
			imgui::TextUnformatted(zpr::sprint("{} item{} copied", clips, clips == 1 ? "" : "s").c_str());

			imgui::SetCursorPos(curs);
		}

		imgui::PopID();
	}


	static int get_var_state(const std::unordered_map<std::string, bool>& vars, const std::string& name)
	{
		if(auto it = vars.find(name); it != vars.end())
		{
			if(it->second)  return 1;
			else            return 2;
		}
		else
		{
			return 0;
		}
	}

	static bool vars_updated = false;
	void toggleVariableState(int num)
	{
		int x = 0;
		auto it = foundVariables.begin();
		while(x < num)
		{
			if(it == foundVariables.end())
				return;

			++it;
			++x;
		}

		auto name = *it;
		int state = get_var_state(varAssigns, name);
		state = (state + 1) % 3;

		if(state == 0) varAssigns.erase(name);
		if(state == 1) varAssigns[name] = true;
		if(state == 2) varAssigns[name] = false;

		vars_updated = true;
	}

	static void eval_tools(Graph* graph)
	{
		imgui::PushID("__scope_eval");

		auto& theme = ui::theme();
		auto geom = geometry::get();

		auto copy = varAssigns;

		{
			lx::vec2 cursor = imgui::GetCursorPos();
			imgui::Text("variables");

			auto s = Styler();
			s.push(ImGuiStyleVar_FramePadding, lx::vec2(0, 4));
			s.push(ImGuiStyleVar_FrameRounding, 2);

			// add a reset button
			// fa-undo-alt
			imgui::SetCursorPos(cursor + lx::vec2(174, -4));
			if(imgui::Button("\uf2ea"))
			{
				zpr::println("reset");
				copy.clear();
			}
		}

		imgui::BeginChild("__variables", lx::vec2(0, geom.sidebar.size.y - 160));
		imgui::Indent(/* 23 */);

		auto button_style = [&copy, &theme](const std::string& name) -> Styler {
			if(auto it = copy.find(name); it != copy.end())
			{
				if(it->second)
				{
					return toggle_enabled_style(it->second);
				}
				else
				{
					auto s = Styler();
					s.push(ImGuiCol_ButtonActive, theme.buttonClickedBg2);
					s.push(ImGuiCol_ButtonHovered, theme.buttonClickedBg);
					s.push(ImGuiCol_Button, theme.buttonClickedBg2);
					return s;
				}
			}
			else
			{
				return Styler();
			}
		};

		// this will be sorted since we're using an ordered set,
		// which is nice.
		int n = 1;
		for(const auto& var : foundVariables)
		{
			// 0 = none, 1 = true, 2 = false
			int state = 0;
			if(auto it = copy.find(var); it != copy.end())
			{
				if(it->second)  state = 1;
				else            state = 2;
			}

			// question-circle: f059, dot-circle: f192, circle: f111
			auto shortcut = (n > 9 ? " " : std::to_string(n));

			auto text = zpr::sprint("{} {} {} ", shortcut,
				state == 0 ? "\uf059" : state == 1 ? "\uf192" : "\uf111", var);

			auto s = button_style(var);
			if(imgui::Button(text.c_str()))
			{
				state = (state + 1) % 3;
				if(state == 0) copy.erase(var);
				if(state == 1) copy[var] = true;
				if(state == 2) copy[var] = false;
			}

			n += 1;
		}

		imgui::Unindent(/* 23 */);
		imgui::EndChild();

		if(vars_updated || varAssigns != copy || !ui::showingEvalExpr())
		{
			varAssigns = copy;
			auto result = get_cached_expr(graph)->evaluate(varAssigns);

			ui::showEvalExpr(zpr::sprint("result: {}", expr_to_string(result)));
		}

		imgui::PopID();
		vars_updated = false;
	}









	static void find_variables(ast::Expr* expr)
	{
		if(auto l = dynamic_cast<ast::Lit*>(expr); l != nullptr)
			;

		else if(auto v = dynamic_cast<ast::Var*>(expr); v != nullptr)
			foundVariables.insert(v->name);

		else if(auto n = dynamic_cast<ast::Not*>(expr); n != nullptr)
			find_variables(n->e);

		else if(auto a = dynamic_cast<ast::And*>(expr); a != nullptr)
			find_variables(a->left), find_variables(a->right);

		else
			abort();
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

