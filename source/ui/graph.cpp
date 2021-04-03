// graph.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <chrono>

#include "ui.h"
#include "alpha.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace alpha;

namespace ui
{
	constexpr double TOP_LEVEL_PADDING = 20;

	static lx::vec2 calc_origin();
	static void render(ImDrawList* dl, Graph* graph);

	static struct {
		std::string msg;
		double begin;
		double duration;
	} messageState;

	static double get_time_now()
	{
		std::chrono::duration<double, std::ratio<1>> dur = std::chrono::system_clock::now().time_since_epoch();
		return dur.count();
	}


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

		auto s = Styler();
		s.push(ImGuiCol_WindowBg, ui::theme().background);


		imgui::Begin("__graph", nullptr,
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);

		// relayout *after* doing interaction.
		if(imgui::IsMousePosValid())
			ui::interact(calc_origin(), graph);

		render(imgui::GetWindowDrawList(), graph);

		// render the message
		constexpr double FADE_TIME = 0.5;
		if(auto msg = messageState.msg; !msg.empty() && get_time_now() - messageState.begin > messageState.duration + FADE_TIME)
		{
			messageState.msg = "";
		}
		else if(!msg.empty())
		{
			imgui::SetCursorPos(lx::vec2(12, geom.graph.size.y - 28));

			auto s = Styler();
			if(auto diff = get_time_now() - messageState.begin; diff > messageState.duration)
			{
				auto alpha = 1.0 - ((diff - messageState.duration) / FADE_TIME);
				s.push(ImGuiStyleVar_Alpha, alpha);
			}

			if(messageState.duration != INFINITY)
				ui::continueDrawing();

			imgui::TextUnformatted(msg.c_str());
		}

		imgui::End();
	}

	static void render(Graph* graph, ImDrawList* dl, lx::vec2 origin, const Item* item)
	{
		auto& theme = ui::theme();

		bool is_deiterable = graph->deiteration_targets.find(item) != graph->deiteration_targets.end();

		util::colour outlineColour;
		if(item->flags & FLAG_DETACHED)
			outlineColour = theme.boxDetached;

		else if(item->flags & FLAG_DROP_TARGET)
			outlineColour = theme.boxDropTarget;

		else if(item->flags & FLAG_SELECTED)
			outlineColour = theme.boxSelection;

		else if(item->flags & FLAG_MOUSE_HOVER)
			outlineColour = theme.boxHover;

		else if(item->flags & FLAG_ITERATION_TARGET)
			outlineColour = theme.boxDetached;

		else if(is_deiterable)
			outlineColour = theme.boxDropTarget;

		else
			outlineColour = theme.foreground;

		if(item->isBox)
		{
			if(!(item->flags & FLAG_ROOT))
			{
				dl->AddRect(origin + item->pos, origin + item->pos + item->size,
					outlineColour.u32(), 3, 0, 2);
			}

			for(auto child : item->subs)
				render(graph, dl, origin + item->pos + item->content_offset, child);
		}
		else
		{
			dl->AddText(origin + item->pos + item->content_offset,
				outlineColour.u32(), item->name.c_str());

			// draw an underline so it's easier to see.
			if(is_deiterable || item->flags & (FLAG_SELECTED | FLAG_MOUSE_HOVER | FLAG_ITERATION_TARGET))
			{
				dl->AddLine(
					origin + item->pos + lx::vec2(3, item->size.y - 2),
					origin + item->pos + lx::vec2(item->size.x - 5, item->size.y - 2),
					outlineColour.u32(), 2);
			}
		}
	}

	static void render(ImDrawList* dl, Graph* graph)
	{
		render(graph, dl, calc_origin(), &graph->box);
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


	void logMessage(const std::string& msg, double timeout_secs)
	{
		messageState = {
			.msg = msg,
			.begin = get_time_now(),
			.duration = (timeout_secs == 0 ? INFINITY : timeout_secs)
		};
	}
}
