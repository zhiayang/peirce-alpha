// exprbar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"
#include "alpha.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace imgui = ImGui;
namespace ui
{
	using namespace alpha;

	static void submit_expr(Graph* graph);

	static constexpr size_t BUFFER_SIZE = 1024;
	static struct {
		char text_buffer[BUFFER_SIZE];

		ast::Expr* cachedExpr = 0;
	} state;

	static void expr_bar(Graph* graph);
	static void edit_bar(Graph* graph);
	std::string expr_to_string(ast::Expr* expr);

	// sidebar.cpp
	Styler toggle_enabled_style(bool enabled);
	void rescan_variables(Graph* graph);

	ast::Expr* get_cached_expr(Graph* graph)
	{
		if(graph->flags & FLAG_GRAPH_MODIFIED)
		{
			if(state.cachedExpr)
				delete state.cachedExpr;

			state.cachedExpr = nullptr;
		}

		if(state.cachedExpr == nullptr)
		{
			state.cachedExpr = graph->expr();
			strncpy(state.text_buffer, expr_to_string(state.cachedExpr).c_str(), BUFFER_SIZE);
		}

		return state.cachedExpr;
	}

	void draw_exprbar(Graph* graph)
	{
		auto& theme = ui::theme();
		auto geom = geometry::get();

		imgui::SetNextWindowPos(geom.exprbar.pos);
		imgui::SetNextWindowSize(geom.exprbar.size);

		imgui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		imgui::PushStyleVar(ImGuiStyleVar_WindowPadding, lx::vec2(0));
		imgui::PushStyleVar(ImGuiStyleVar_FramePadding, lx::vec2(10));

		imgui::PushStyleColor(ImGuiCol_WindowBg, theme.exprbarBg);
		imgui::PushStyleColor(ImGuiCol_FrameBg, theme.exprbarBg);

		imgui::Begin("__exprbar", nullptr,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if(state.cachedExpr == nullptr || graph->flags & FLAG_GRAPH_MODIFIED)
		{
			if(state.cachedExpr)
				delete state.cachedExpr;

			state.cachedExpr = graph->expr();
			strncpy(state.text_buffer, expr_to_string(state.cachedExpr).c_str(), BUFFER_SIZE);
		}

		auto mode = ui::getMode();
		if(mode == MODE_EDIT)
			edit_bar(graph);

		else
			expr_bar(graph);

		imgui::GetForegroundDrawList()->AddLine(
			lx::vec2(geom.exprbar.pos.x, geom.exprbar.pos.y),
			lx::vec2((geom.exprbar.pos + geom.exprbar.size).x, geom.exprbar.pos.y),
			mode == MODE_EDIT ? theme.boxDetached.u32() : theme.foreground.u32(),
			/* thickness: */ 2.0
		);

		imgui::End();
		imgui::PopStyleColor(2);
		imgui::PopStyleVar(3);
	}



	static void edit_bar(Graph* graph)
	{
		auto geom = geometry::get();

		imgui::SetNextItemWidth(-44);

		// note: callback replaces the ascii things with their nicer-looking unicode counterparts.
		bool submit = imgui::InputTextWithHint("", "expression", state.text_buffer, BUFFER_SIZE,
			/* flags: */ ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter,
			/* callback: */ [](ImGuiInputTextCallbackData* callback) -> int {
				auto& ch = callback->EventChar;
				if(ch == '&' || ch == '*')      ch = u'∧';
				else if(ch == '|' || ch == '+') ch = u'∨';
				else if(ch == '~' || ch == '!') ch = u'¬';

				return 0;
			});

		if(submit)
		{
			// for some reason the field automatically rescinds focus when enter is pressed;
			// re-focus it so you can (for example) edit-enter-edit-enter.
			imgui::SetKeyboardFocusHere(-1);
			ui::flashButton(EB_BUTTON_SUBMIT);
		}

		auto s = Styler(); s.push(ImGuiStyleVar_FramePadding, lx::vec2(4));
		imgui::SetCursorPos(lx::vec2(geom.exprbar.size.x - 40, geom.exprbar.size.y - 40));

		imgui::PushFont(ui::getBigIconFont());

		// sign-in-alt
		auto ss = toggle_enabled_style(ui::buttonFlashed(EB_BUTTON_SUBMIT));
		if(imgui::ButtonEx("\uf2f6", lx::vec2(40, 40)) || submit)
			submit_expr(graph);

		imgui::PopFont();
	}




	static void render_expr(ast::Expr* expr, std::string* out, bool omit_parens = false)
	{
		auto& theme = ui::theme();

		auto s = Styler();
		if(expr->original && expr->original->flags & FLAG_SELECTED)
			s.push(ImGuiCol_Text, theme.boxSelection);

		auto text = [&out](zbuf::str_view sv) {
			if(out)
			{
				*out += sv.sv();
			}
			else
			{
				imgui::TextUnformatted(sv.begin(), sv.end());
				imgui::SameLine(0, 0);
			}
		};

		if(auto v = dynamic_cast<ast::Var*>(expr); v != nullptr)
		{
			text(v->name);
		}
		else if(auto l = dynamic_cast<ast::Lit*>(expr); l != nullptr)
		{
			text(l->value ? "1" : "0");
		}
		else if(auto n = dynamic_cast<ast::Not*>(expr); n != nullptr)
		{
			text("¬"); render_expr(n->e, out);
		}
		else if(auto a = dynamic_cast<ast::And*>(expr); a != nullptr)
		{
			if(!omit_parens) text("(");
			render_expr(a->left, out, dynamic_cast<ast::And*>(a->left));
			text(" ∧ ");
			render_expr(a->right, out);
			if(!omit_parens) text(")");
		}
		else if(auto o = dynamic_cast<ast::Or*>(expr); o != nullptr)
		{
			if(!omit_parens) text("(");
			render_expr(o->left, out, dynamic_cast<ast::Or*>(o->left));
			text(" ∨ ");
			render_expr(o->right, out);
			if(!omit_parens) text(")");
		}
		else if(auto i = dynamic_cast<ast::Implies*>(expr); i != nullptr)
		{
			text("("); render_expr(i->left, out); text(" -> "); render_expr(i->right, out); text(")");
		}
		else if(auto b = dynamic_cast<ast::BidirImplies*>(expr); b != nullptr)
		{
			text("("); render_expr(b->left, out); text(" <-> "); render_expr(b->right, out); text(")");
		}
		else
		{
			abort();
		}

		imgui::SameLine(0, 0);
	}

	std::string expr_to_string(ast::Expr* expr)
	{
		std::string s;
		render_expr(expr, &s);
		return s;
	}

	static void expr_bar(Graph* graph)
	{
		auto geom = geometry::get();

		lx::vec2 pos = imgui::GetCursorPos();
		pos += imgui::GetStyle().FramePadding;

		imgui::BeginChild("__expr_scroll", geom.exprbar.size - lx::vec2(44, 0));
		imgui::SetCursorPos(pos);

		// we don't need this.
		render_expr(state.cachedExpr, nullptr, /* omit_parens: */ true);

		imgui::EndChild();

		{
			auto s = Styler(); s.push(ImGuiStyleVar_FramePadding, lx::vec2(4));
			imgui::SetCursorScreenPos(imgui::GetWindowPos() + lx::vec2(geom.exprbar.size.x - 40, geom.exprbar.size.y - 40));

			// align-right
			auto ss = toggle_enabled_style(ui::buttonFlashed(EB_BUTTON_RELAYOUT));
			if(imgui::ButtonEx("\uf038", lx::vec2(40, 40)))
				ui::autoLayoutGraph(graph);
		}
	}







	static void submit_expr(Graph* graph)
	{
		auto txt = zbuf::str_view((const char*) state.text_buffer);
		auto expr = parser::parse(txt);
		lg::log("expr", "parsing: '{}'", txt);

		if(!expr)
		{
			// TODO: open a box to tell the user
			ui::logMessage(zpr::sprint("parse error: {}", expr.error().msg), 5);
			lg::warn("expr", "parse error: {}", expr.error().msg);
		}
		else
		{
			ui::logMessage("graph updated", 1);
			graph->setAst(expr.unwrap());
		}

		if(state.cachedExpr)
			delete state.cachedExpr;

		state.cachedExpr = graph->expr();
		rescan_variables(graph);
	}
}
