// evaluate.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <thread>

#include "ui.h"
#include "ast.h"
#include "alpha.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace alpha;

namespace alpha
{
	// util/solver.cpp
	std::vector<std::unordered_map<std::string, bool>>
	generate_solutions(ast::Expr* expr, const std::set<std::string>& vars,
		std::pair<size_t, size_t>& progress);

	void abort_solve();
}

namespace ui
{
	// defined in exprbar.cpp
	ast::Expr* get_cached_expr(Graph* graph);
	std::string expr_to_string(ast::Expr* expr);

	Styler flash_style(int button);
	Styler disabled_style(bool disabled);
	Styler toggle_enabled_style(bool enabled);

	static void find_variables(ast::Expr* expr);
	static int get_var_state(const std::unordered_map<std::string, bool>& vars, const std::string& name);

	static void solver_tool(Graph* graph);
	static void variable_assign_tool(Graph* graph);

	static bool vars_updated = false;
	static std::set<std::string> foundVariables;
	static std::unordered_map<std::string, bool> varAssigns;

	static bool solve_requested = false;
	static volatile bool solver_done = true;
	static std::pair<size_t, size_t> solver_progress;
	static struct {
		size_t index = 0;
		bool waiting = false;
		bool did_solve = false;
		std::vector<std::unordered_map<std::string, bool>> solns;
	} solver_state;

	void set_flags(Graph* graph, const std::unordered_map<std::string, bool>& soln);

	static void reset_soln()
	{
		solver_done = true;
		solver_state.solns.clear();
		solver_state.did_solve = false;
		solver_state.waiting = false;
	}

	// called when the mode changes
	void evalModeChanged(Graph* graph, bool active)
	{
		if(!active)
		{
			ui::resetEvalExpr();
			set_flags(graph, { });
			alpha::abort_solve();
		}
	}

	void rescan_variables(Graph* graph)
	{
		if((graph->flags & FLAG_GRAPH_MODIFIED) || foundVariables.empty())
		{
			reset_soln();
			foundVariables.clear();
			find_variables(get_cached_expr(graph));
		}
	}

	void eval_tools(Graph* graph)
	{
		imgui::PushID("__scope_eval");

		solver_tool(graph);
		imgui::NewLine();

		variable_assign_tool(graph);

		imgui::PopID();
	}



	static void solver_tool(Graph* graph)
	{
		auto& theme = ui::theme();

		imgui::PushID("__scope_solve");

		imgui::Text("solver");
		imgui::Indent();

		// tasks
		{
			auto done = __atomic_load_n(&solver_done, __ATOMIC_SEQ_CST);
			{
				auto s = disabled_style(done == false || foundVariables.size() > 32);
				auto ss = flash_style(SB_BUTTON_V_SOLVE);
				if((imgui::Button("s \uf0ae solve ") || solve_requested) && done)
				{
					solve_requested = false;

					__atomic_store_n(&solver_done, false, __ATOMIC_SEQ_CST);
					if(auto sz = foundVariables.size(); sz >= 16)
						ui::logMessage(zpr::sprint("solving {} variables; this might take some time...", sz), 3);

					solver_state.waiting = true;
					solver_state.did_solve = true;
					auto t = std::thread([](ast::Expr* expr) {
						solver_state.solns = alpha::generate_solutions(expr,
							foundVariables, solver_progress);

						__atomic_store_n(&solver_done, true, __ATOMIC_SEQ_CST);
						auto n = solver_state.solns.size();

						lg::log("solver", "done: {} solution(s)", solver_state.solns.size());
						if(solver_state.solns.size() > 0)
							ui::logMessage(zpr::sprint("{} solution{} found", n, n == 1 ? "" : "s"), 5);

						else
							ui::logMessage("unsatisfiable", 5);
					}, get_cached_expr(graph));

					t.detach();
				}
			}

			if(!done && solver_state.did_solve)
			{
				auto [ c, t ] = solver_progress;
				double prog = (double) c / (double) t;
				imgui::ProgressBar(prog, lx::vec2(0, 0), zpr::sprint("{.1f}%", 100 * prog).c_str());
			}
			else if(done && solver_state.did_solve)
			{
				if(solver_state.solns.empty())
				{
					auto s = Styler();
					s.push(ImGuiCol_Text, theme.boxSelection);

					imgui::NewLine();
					imgui::SameLine(0, 4);
					imgui::Text("unsatisfiable");
				}
				else
				{
					{
						auto s = Styler();
						s.push(ImGuiCol_Text, theme.boxDropTarget);

						auto n = solver_state.solns.size();
						imgui::NewLine();
						imgui::SameLine(0, 4);
						imgui::TextUnformatted(zpr::sprint("{} solution{}",
							n, n == 1 ? "" : "s").c_str());
					}

					bool changed = false;

					if(solver_state.waiting)
					{
						// on the first go-around, assign the first solution.
						changed = true;
						solver_state.index = 0;
						solver_state.waiting = false;
					}

					{
						auto s = disabled_style(solver_state.index == 0);
						auto ss = flash_style(SB_BUTTON_V_PREV_SOLN);
						if(imgui::Button("\uf177 prev "))
							changed = true, solver_state.index -= 1;
					}

					imgui::SameLine();

					{
						auto s = disabled_style(solver_state.index + 1 >= solver_state.solns.size());
						auto ss = flash_style(SB_BUTTON_V_NEXT_SOLN);
						if(imgui::Button(" next \uf178"))
							changed = true, solver_state.index += 1;
					}

					if(changed)
					{
						varAssigns = solver_state.solns[solver_state.index];
						set_flags(graph, varAssigns);
					}
				}
			}
		}

		imgui::Unindent();
		imgui::PopID();
	}

	// used by interact.cpp
	void prev_solution(Graph* graph)
	{
		if(!solver_state.did_solve)
			return;

		if(solver_state.index > 0)
			solver_state.index -= 1;

		varAssigns = solver_state.solns[solver_state.index];
		set_flags(graph, varAssigns);
	}

	void next_solution(Graph* graph)
	{
		if(!solver_state.did_solve)
			return;

		if(solver_state.index + 1 < solver_state.solns.size())
			solver_state.index += 1;

		varAssigns = solver_state.solns[solver_state.index];
		set_flags(graph, varAssigns);
	}

	void solve_expression()
	{
		solve_requested = true;
	}



	static void variable_assign_tool(Graph* graph)
	{
		auto& theme = ui::theme();
		auto geom = geometry::get();

		imgui::PushID("__scope_assign");
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
				copy.clear();
		}

		auto ypos = imgui::GetCursorPos().y;
		imgui::BeginChild("__variables", lx::vec2(0, geom.sidebar.size.y - 45 - ypos));
		imgui::Indent();

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
		int n = 0;
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
			auto shortcut = (n > 9 ? " " : std::to_string((1 + n) % 10));

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

		imgui::Unindent();
		imgui::EndChild();

		if(vars_updated || varAssigns != copy || !ui::showingEvalExpr())
		{
			varAssigns = copy;
			auto result = get_cached_expr(graph)->evaluate(varAssigns);

			reset_soln();
			ui::showEvalExpr(zpr::sprint("result: {}", expr_to_string(result)));
			set_flags(graph, varAssigns);
		}

		vars_updated = false;
		imgui::PopID();
	}
















	template <typename Fn>
	static void for_each_var(Item* item, const Fn& fn)
	{
		if(item->isBox)
		{
			for(auto child : item->subs)
				for_each_var(child, fn);
		}
		else
		{
			fn(item);
		}
	}

	void set_flags(Graph* graph, const std::unordered_map<std::string, bool>& soln)
	{
		for_each_var(&graph->box, [&soln](auto item) {
			item->flags &= ~(FLAG_VAR_ASSIGN_TRUE | FLAG_VAR_ASSIGN_FALSE);
			if(auto it = soln.find(item->name); it != soln.end())
			{
				if(it->second)  item->flags |= FLAG_VAR_ASSIGN_TRUE;
				else            item->flags |= FLAG_VAR_ASSIGN_FALSE;
			}
		});
	}

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
		reset_soln();
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
}
