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

		return Graph {
			.box = Item { .isBox = true, .var = "__root", .box = {
				new Item { .isBox = false, .var = "A" },
				new Item { .isBox = true, .var = "box_Bs", .box = {
					new Item { .isBox = false, .var = "B" },
					new Item { .isBox = false, .var = "B" },
					new Item { .isBox = false, .var = "B" },
					new Item { .isBox = false, .var = "B" },
					new Item { .isBox = true, .var = "box_C+X", .box = {
						new Item { .isBox = false, .var = "C" },
						new Item { .isBox = true, .var = "box_X1", .box = {
							new Item { .isBox = true, .var = "box_X2", .box = {
								new Item { .isBox = false, .var = "X" }
							} }
						} }
					} }
				} }
			} }
		};
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

		draw_graph(&g);
		draw_sidebar(&g);
		draw_exprbar(&g);
	}
}
