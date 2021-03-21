// exprbar.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"
#include "imgui/imgui.h"

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
			state.cachedExpr = graph->expr();
			strncpy(state.text_buffer, state.cachedExpr->str().c_str(), BUFFER_SIZE);
		}

		imgui::SetNextItemWidth(-40);
		imgui::InputTextWithHint("", "expression", state.text_buffer, BUFFER_SIZE);

		imgui::PushStyleVar(ImGuiStyleVar_FramePadding, lx::vec2(4));
		{
			imgui::SetCursorPos(lx::vec2(geom.exprbar.size.x - 40, geom.exprbar.size.y - 40));
			if(imgui::ImageButton(theme.textures.submit, lx::vec2(32, 32)))
				submit_expr(graph);
		}
		imgui::PopStyleVar();


		// since window borders are off, we draw the separator manually.
		{
			imgui::GetForegroundDrawList()->AddLine(
				lx::vec2(geom.exprbar.pos.x, geom.exprbar.pos.y),
				lx::vec2((geom.exprbar.pos + geom.exprbar.size).x, geom.exprbar.pos.y),
				theme.foreground.u32(), /* thickness: */ 2.0
			);
		}

		imgui::End();
		imgui::PopStyleColor(2);
		imgui::PopStyleVar(3);
	}

	static void submit_expr(Graph* graph)
	{
		auto txt = zbuf::str_view((const char*) state.text_buffer);
		auto expr = parser::parse(txt);
		lg::log("expr", "parsing: '{}'", txt);

		if(!expr)
		{
			// TODO: open a box to tell the user
			lg::warn("expr", "parse error: {}", expr.error().msg);
		}
		else
		{
			graph->setAst(expr.unwrap());
		}
	}
}
