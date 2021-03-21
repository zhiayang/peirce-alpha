// graph.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace ui::alpha;

namespace ui
{
	constexpr double TOP_LEVEL_PADDING = 20;

	static lx::vec2 calc_origin();
	static void render(ImDrawList* dl, Graph* graph);

	int getNextId()
	{
		static int nextId = 0;
		return nextId++;
	}

	void draw_graph(Graph* graph)
	{
		auto geom = geometry::get();

		imgui::SetNextWindowPos(geom.graph.pos);
		imgui::SetNextWindowSize(geom.graph.size);

		graph->box.flags |= FLAG_ROOT;

		// if we already laid it out, then don't re-lay it out. we shouldn't be re-laying it
		// every frame, because then resizing the window will move things and it'll be very annoying.
		if(graph->box.size == lx::vec2() || (graph->flags & FLAG_FORCE_AUTO_LAYOUT))
			autoLayout(graph, geom.graph.size.x - 2 * TOP_LEVEL_PADDING);

		imgui::SetNextWindowContentSize(graph->box.size);

		imgui::Begin("__graph", nullptr,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);

		// relayout *after* doing interaction.
		{
			auto mouse = imgui::GetMousePos();
			auto min = geom.graph.pos;
			auto max = geom.graph.pos + geom.graph.size;

			if(mouse.x >= min.x && mouse.y >= min.y && mouse.x <= max.x && mouse.y <= max.y)
				ui::interact(calc_origin(), graph);
		}

		render(imgui::GetWindowDrawList(), graph);
		imgui::End();
	}

	static void render(ImDrawList* dl, lx::vec2 origin, const Item* item)
	{
		auto& theme = ui::theme();

		util::colour outlineColour;
		if(item->flags & FLAG_DETACHED)
			outlineColour = theme.boxDetached;

		else if(item->flags & FLAG_DROP_TARGET)
			outlineColour = theme.boxDropTarget;

		else if(item->flags & FLAG_SELECTED)
			outlineColour = theme.boxSelection;

		else if(item->flags & FLAG_MOUSE_HOVER)
			outlineColour = theme.boxHover;

		else
			outlineColour = theme.foreground;

		if(item->isBox)
		{
			if(!(item->flags & FLAG_ROOT))
			{
				dl->AddRect(origin + item->pos, origin + item->pos + item->size,
					outlineColour.u32(), 0, 0, 2);
			}

			for(auto child : item->subs)
				render(dl, origin + item->pos + item->content_offset, child);
		}
		else
		{
			dl->AddText(origin + item->pos + item->content_offset,
				theme.foreground.u32(), item->name.c_str());

			if(item->flags & (FLAG_SELECTED | FLAG_MOUSE_HOVER))
			{
				dl->AddRect(origin + item->pos, origin + item->pos + item->size,
					outlineColour.u32(), 0, 0, 2);
			}
		}
	}

	static void render(ImDrawList* dl, Graph* graph)
	{
		render(dl, calc_origin(), &graph->box);
	}

	static lx::vec2 calc_origin()
	{
		auto sx = imgui::GetScrollX();
		auto sy = imgui::GetScrollY();

		auto origin = lx::vec2(imgui::GetWindowPos().x, imgui::GetWindowPos().y);
		origin += lx::vec2(TOP_LEVEL_PADDING);
		origin -= lx::vec2(sx, sy);

		return origin;
	}
}
