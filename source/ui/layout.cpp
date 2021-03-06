// layout.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <cmath>
#include <algorithm>

#include "ui.h"
#include "alpha.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace alpha;

namespace ui
{
	// the layout algorithm is as simple as possible -- stack items horizontally
	// as long as we are within the width limit, else move vertically and restart.
	// scrolling both vertically and horizontally is annoying, so don't make the
	// contents wider than the screen width.
	struct LayoutState
	{
		double maxWidth = 0;

		int boxNesting = 0;
	};

	constexpr double BOX_V_PADDING = 3;
	constexpr double BOX_H_PADDING = 5;

	constexpr double OUTER_ITEM_PADDING = 5;
	constexpr double INTER_ITEM_SPACING = 20;

	static void reflow_cursor(LayoutState& st)
	{

	}

	void autoLayoutGraph(Graph* graph)
	{
		graph->flags |= FLAG_FORCE_AUTO_LAYOUT;
		ui::logMessage("graph automatically laid out", 1);
	}

	void calculate_size_for_var(Item* item)
	{
		if(item->isBox)
			return;

		auto _sz = imgui::CalcTextSize(item->name.c_str());
		auto sz = lx::vec2(_sz.x, _sz.y);

		item->size = sz + 2 * item->content_offset;
	}

	static void auto_layout(LayoutState& st, lx::vec2 pos, Item* item)
	{
		item->content_offset = lx::vec2(OUTER_ITEM_PADDING);

		if(item->isBox)
		{
			// sort the box by putting all vars before all boxes.
			std::sort(item->subs.begin(), item->subs.end(), [](Item* a, Item* b) -> bool {
				if(!a->isBox && b->isBox)
					return true;

				else if(a->isBox && b->isBox)
					return a->subs.size() < b->subs.size();

				else if(!a->isBox && !b->isBox)
					return a->name < b->name;

				return false;
			});

			double stackWidth = 0;

			item->pos = pos;
			auto cursor = lx::vec2(BOX_H_PADDING, BOX_V_PADDING);
			auto startX = cursor.x;

			double row_height = 0;
			double total_height = 0;

			for(size_t i = 0; i < item->subs.size(); i++)
			{
				auto_layout(st, cursor, item->subs[i]);
				item->subs[i]->setParent(item);

				cursor.x += item->subs[i]->size.x;

				if(cursor.x >= st.maxWidth - 3 * INTER_ITEM_SPACING)
				{
					// move it to the next line, please.
					cursor.x = BOX_H_PADDING;
					cursor.y += row_height + INTER_ITEM_SPACING;
					total_height += row_height + INTER_ITEM_SPACING;

					row_height = 0;

					item->subs[i]->pos = cursor;
					cursor.x += item->subs[i]->size.x;
				}

				stackWidth = std::max(stackWidth, cursor.x - startX);

				if(i + 1 < item->subs.size())
					cursor.x += INTER_ITEM_SPACING;

				row_height = std::max(row_height, item->subs[i]->size.y);
			}
			total_height += row_height;

			item->size = lx::vec2(BOX_H_PADDING + stackWidth + BOX_H_PADDING, total_height + 2 * BOX_V_PADDING)
				+ 2 * item->content_offset;
		}
		else
		{
			item->pos = pos;
			calculate_size_for_var(item);
		}

		reflow_cursor(st);
	}

	void autoLayout(Graph* graph, double width)
	{
		graph->flags &= ~FLAG_FORCE_AUTO_LAYOUT;

		LayoutState st = { .maxWidth = width };
		auto_layout(st, lx::vec2(0, 0), &graph->box);
	}


	template <int Axis>
	static std::pair<bool, double> overlap_rects(const lx::vec2& p1, const lx::vec2& s1, const lx::vec2& p2, const lx::vec2& s2)
	{
		bool ov = p1.ptr[Axis] + s1.ptr[Axis] >= p2.ptr[Axis]
			&& p1.ptr[Axis] + s1.ptr[Axis] <= p2.ptr[Axis] + s2.ptr[Axis];

		return { ov, ov
			? p1.ptr[Axis] + s1.ptr[Axis] - p2.ptr[Axis]
			: INFINITY
		};
	}
	static auto overlap_x(const lx::vec2& p1, const lx::vec2& s1, const lx::vec2& p2, const lx::vec2& s2)
	{
		return overlap_rects<0>(p1, s1, p2, s2);
	}
	static auto overlap_y(const lx::vec2& p1, const lx::vec2& s1, const lx::vec2& p2, const lx::vec2& s2)
	{
		return overlap_rects<1>(p1, s1, p2, s2);
	}


	// shove the siblings of `item` that are in the `box`
	static void shove(Item* item, Item* box)
	{
		// if this got shoved already, don't go into a loop... just give up.
		if(item->flags & FLAG_SHOVED)
			return;

		item->flags |= FLAG_SHOVED;
		for(auto sib : box->subs)
		{
			if(sib == item)
				continue;

			bool shoved = false;

			{
				auto [ ovxL, mxL ] = overlap_x(sib->pos, sib->size, item->pos, item->size);
				auto [ ovxR, mxR ] = overlap_x(item->pos, item->size, sib->pos, sib->size);
				auto [ ovyT, myT ] = overlap_y(sib->pos, sib->size, item->pos, item->size);
				auto [ ovyB, myB ] = overlap_y(item->pos, item->size, sib->pos, sib->size);

				if((ovxL || ovxR) && (ovyT || ovyB))
				{
					if(ovxL && (ovyT || ovyB) && mxL <= myT && mxL <= myB)
						sib->pos.x -= mxL, shoved = true;

					else if(ovxR && (ovyT || ovyB) && mxR <= myT && mxR <= myB)
						sib->pos.x += mxR, shoved = true;

					else if(ovyT && (ovxL || ovxR) && myT <= mxL && myT <= mxR)
						sib->pos.y -= myT, shoved = true;

					else if(ovyB && (ovxL || ovxR) && myB <= mxL && myB <= mxR)
						sib->pos.y += myB, shoved = true;
				}
			}

			// if we moved something, then that something might end up moving something else.
			if(shoved)
				shove(sib, box);
		}

		item->flags &= ~FLAG_SHOVED;
	}






	static void relayout(Item* box)
	{
		// don't re-layout detached items (since they need to clip into other things)
		if(box->flags & FLAG_DETACHED)
			return;


		box->content_offset = lx::vec2(OUTER_ITEM_PADDING);

		// we really only need to do stuff if this is a box.
		if(box->isBox)
		{
			auto tl = lx::vec2(INFINITY);   // top left
			auto br = lx::vec2(-INFINITY);  // bottom right

			// first, calculate the top-left bound
			for(auto child : box->subs)
				tl = lx::min(tl, child->pos);

			bool adjusted_x = false;
			bool adjusted_y = false;
			if(tl.x - BOX_H_PADDING < 0)
			{
				auto tl_offset = tl.x - BOX_H_PADDING;
				box->pos.x += tl_offset;
				box->size.x -= tl_offset;
				adjusted_x = true;
			}
			if(tl.y - BOX_V_PADDING < 0)
			{
				auto tl_offset = tl.y - BOX_V_PADDING;
				box->pos.y += tl_offset;
				box->size.y -= tl_offset;
				adjusted_y = true;
			}

			// after the adjustments above, we know that this->pos is not negative (at least (0, 0)).
			if(adjusted_x || adjusted_y)
			{
				for(auto& child : box->subs)
				{
					if(adjusted_x) child->pos.x -= tl.x - BOX_H_PADDING;
					if(adjusted_y) child->pos.y -= tl.y - BOX_V_PADDING;
				}
			}

			// then calculate the bottom right bound (after shifting all the children)
			for(auto child : box->subs)
				br = lx::max(br, child->pos + child->size);

			// if(br + lx::vec2(BOX_H_PADDING, BOX_V_PADDING) + 2 * box->content_offset > box->size)
			{
				auto max_sz = br + 2 * lx::vec2(BOX_H_PADDING, BOX_V_PADDING) + 2 * box->content_offset;
				box->size = lx::max(box->size, max_sz);
			}
		}
		else
		{
			// for vars, we need to refresh their size.
			calculate_size_for_var(box);
		}

		if(box->parent() != nullptr)
		{
			// if necessary, shove it with its siblings
			shove(box, box->parent());
			relayout(box->parent());
		}
	}

	void relayout(Graph* graph, Item* item)
	{
		// relayout the child.
		relayout(item);

		// the top-level container should always be pinned at (0, 0)
		graph->box.pos = lx::vec2(0, 0);
	}
}
