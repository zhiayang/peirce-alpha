// draw.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;

namespace ui
{
	void draw_sidebar(alpha::Graph* graph);     // sidebar.cpp
	void draw_exprbar(alpha::Graph* graph);     // exprbar.cpp
	void draw_graph(alpha::Graph* graph);       // graph.cpp

	static alpha::Graph make()
	{
		using namespace alpha;

		return Graph({ Item::box({
				Item::var("A"),
				Item::box({
					// Item::var("B"),
					// Item::var("B"),
					// Item::var("B"),
					Item::var("B"),
					Item::box({
						Item::var("C"),
						Item::box({
							Item::box({
								Item::var("X")
							})
						})
					})
				})
			})
		});
	}

	void update()
	{
		ui::startFrame();

		geometry::update();
		ui::draw();

		ui::endFrame();
	}

	void draw()
	{
		static auto g = make();

		draw_sidebar(&g);
		draw_exprbar(&g);
		draw_graph(&g);
	}
}
