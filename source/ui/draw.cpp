// draw.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "alpha.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;

namespace ui
{
	void draw_sidebar(alpha::Graph* graph);     // sidebar.cpp
	void draw_exprbar(alpha::Graph* graph);     // exprbar.cpp
	void draw_graph(alpha::Graph* graph);       // graph.cpp

	static std::unordered_map<int, int> buttonFlashCounters;

	void flashButton(int button)
	{
		static constexpr int FLASH_FRAMES = 5;

		buttonFlashCounters[button] = FLASH_FRAMES;
		ui::continueDrawing();
	}

	bool buttonFlashed(int button)
	{
		return buttonFlashCounters[button] > 0;
	}


	static alpha::Graph* make()
	{
		using namespace alpha;

	#if 0
		auto ret = new Graph({
			Item::box({
				Item::var("A"),
				Item::var("B"),
				Item::var("C"),
				Item::var("D"),
				Item::var("E"),
				Item::var("F"),
				Item::var("G"),
				Item::var("H"),
				Item::var("I"),
				Item::var("J"),
				Item::var("K"),
				Item::var("L"),
				Item::box({
					Item::var("M"),
					Item::var("N"),
					Item::var("O"),
					Item::var("P"),
					Item::var("Q"),
					Item::var("R"),
					Item::var("S"),
					Item::var("T"),
					Item::var("U"),
				})
			})
		});
	#else
	#if 1
		auto ret = new Graph({
			// Item::var("P"),
			// Item::box({ Item::var("Q") }),
			Item::box({
				Item::var("P"),
				Item::box({ Item::var("Q") })
			})
		});
	#else
		auto ret = new Graph({ Item::box({
				Item::var("A"),
				Item::box({
					// Item::var("B"),
					// Item::var("B"),
					// Item::var("B"),
					Item::var("B"),
					Item::box({
						Item::var("C"),
						Item::var("B"),
						Item::box({
							Item::box({
								Item::var("X"),
								Item::var("B"),
								Item::var("Z"),
							})
						})
					})
				})
			})
		});
	#endif
	#endif

		ret->flags |= FLAG_GRAPH_MODIFIED;
		return ret;
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

		draw_sidebar(g);
		draw_exprbar(g);
		draw_graph(g);

		// update all the flash counters
		for(auto& [ _, ctr ] : buttonFlashCounters)
			if(ctr > 0) ctr -= 1;
	}
}
