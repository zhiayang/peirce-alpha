// interact.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;

namespace ui
{
	using namespace alpha;

	static int get_resize_edge(const lx::vec2& hit, Item* item);
	static Item* hit_test(const lx::vec2& hit, Item* item, bool only_boxes, Item* ignoring = nullptr);

	static void handle_drag(Graph* graph);
	static void handle_reattach(Graph* graph);
	static void erase_from_parent(Item* item);


	template <typename Cb>
	static void for_each_item(Item* item, Cb&& cb)
	{
		if(item->isBox)
		{
			for(auto i : item->box)
				for_each_item(i, static_cast<Cb&&>(cb));
		}

		cb(item);
	}

	template <typename Cb>
	static void for_each_item(Graph* graph, Cb&& cb) { for_each_item(&graph->box, static_cast<Cb&&>(cb)); }

	static struct {
		Item* selected = 0;
		Item* dropTarget = 0;

		int resizingEdge = 0;   // 1 = north, 2 = northeast, 3 = east, etc.
	} state;

	void interact(lx::vec2 origin, Graph* graph)
	{
		// reset this flag.
		graph->flags &= ~FLAG_GRAPH_MODIFIED;

		auto mouse = imgui::GetMousePos() - origin;
		if(imgui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			// deselect everything
			for_each_item(graph, [](auto item) { item->flags &= ~FLAG_SELECTED; });

			if(auto hit = hit_test(mouse, &graph->box, /* only_boxes: */ false);
				hit != nullptr && !(hit->flags & FLAG_ROOT))
			{
				hit->flags |= FLAG_SELECTED;
				state.selected = hit;
			}
			else
			{
				state.selected = nullptr;
			}
		}

		if(state.selected != nullptr && (imgui::IsKeyPressed(imgui::GetKeyIndex(ImGuiKey_Backspace))
			|| imgui::IsKeyPressed(imgui::GetKeyIndex(ImGuiKey_Delete))))
		{
			// first, remove it from its parent
			erase_from_parent(state.selected);

			// then delete it
			delete state.selected;

			// unselect it.
			state.selected = nullptr;

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}


		// note: handle selected things first, so that if something is deleted in this frame,
		// the cursor doesn't lag for one frame.
		if(state.selected != nullptr)
		{
			bool was_detached = (state.selected->flags & FLAG_DETACHED);
			if(was_detached)
			{
				// unset the old one first.
				if(state.dropTarget)
					state.dropTarget->flags &= ~FLAG_DROP_TARGET;

				// since the mouse is most likely going to be over the thing we just selected,
				// we have to ignore it so that it doesn't become its own drop target.
				auto drop = hit_test(mouse, &graph->box, /* only_boxes: */ true, /* ignoring: */ state.selected);
				state.dropTarget = drop;
				if(drop != nullptr)
					drop->flags |= FLAG_DROP_TARGET;
			}

			if(was_detached && (!imgui::GetIO().KeyShift            // if you released the shift key
				|| imgui::IsMouseReleased(ImGuiMouseButton_Left)))  // or you released the mouse
			{
				handle_reattach(graph);
			}
			else if((!was_detached && imgui::GetIO().KeyShift)      // if you pressed the shift key
				&& (imgui::IsMouseDown(ImGuiMouseButton_Left)       // and either the mouse is down
				|| imgui::IsMouseDragging(ImGuiMouseButton_Left)))  // or you are dragging it
			{
				// detach it
				state.selected->flags |= FLAG_DETACHED;
			}
		}


		// tentatively reset the cursor
		if(!imgui::IsMouseDown(ImGuiMouseButton_Left))
			state.resizingEdge = 0;

		// if the mouse is already clicked, then don't change the cursor halfway!
		if(auto edge = get_resize_edge(mouse, &graph->box); edge != 0 && !imgui::IsMouseDown(ImGuiMouseButton_Left))
			state.resizingEdge = edge;

		// imgui resets the cursor each frame, so we must re-set the cursor each frame.
		{
			auto edge = state.resizingEdge;

			if(edge == 2 || edge == 6)
				ui::setCursor(ImGuiMouseCursor_ResizeNESW);

			else if(edge == 4 || edge == 8)
				ui::setCursor(ImGuiMouseCursor_ResizeNWSE);

			else if(edge == 1 || edge == 5)
				ui::setCursor(ImGuiMouseCursor_ResizeNS);

			else if(edge == 3 || edge == 7)
				ui::setCursor(ImGuiMouseCursor_ResizeEW);

			else
				ui::setCursor(ImGuiMouseCursor_Arrow);
		}


		if(imgui::IsMouseDragging(ImGuiMouseButton_Left))
			handle_drag(graph);
	}


	static void erase_from_parent(Item* item)
	{
		auto parent = item->parent;
		assert(parent != nullptr);

		auto it = std::find(parent->box.begin(), parent->box.end(), item);
		assert(it != parent->box.end());
		parent->box.erase(it);
	}

	static void handle_reattach(Graph* graph)
	{
		auto item = state.selected;

		// reattach it
		item->flags &= ~FLAG_DETACHED;

		auto drop = state.dropTarget;
		if(drop == nullptr)
			drop = &graph->box;

		// unset the drop target.
		drop->flags &= ~FLAG_DROP_TARGET;

		// ok, now drop it -- only if the parent didn't change.
		if(item->parent != drop)
		{
			erase_from_parent(item);

			// also, recalculate its position relative to its new parent.
			struct { auto operator()(Item* item) -> lx::vec2 {
					if(!item)           return lx::vec2(0, 0);
					if(!item->parent)   return item->pos;
					return (*this)(item->parent) + item->parent->content_offset + item->pos;
				}
			} compute_abs_pos;

			auto old_abs_pos = compute_abs_pos(item);
			auto drop_abs_pos = compute_abs_pos(drop);

			item->pos = old_abs_pos - drop_abs_pos - drop->content_offset;

			// and insert it into the new parent.
			drop->box.push_back(item);
			item->parent = drop;

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}

		// and finally, relayout it.
		ui::relayout(graph, item);
	}


	static void handle_drag(Graph* graph)
	{
		if(state.selected == nullptr)
			return;

		lx::vec2 delta = imgui::GetMouseDragDelta();
		if(state.resizingEdge == 0)
		{
			// just move it graphically. relayout() handles shoving etc, but the item being moved
			// will stay in its current 'box' -- so the semantic meaning of the graph is preserved.
			state.selected->pos += delta;

			// follow the chain up and relayout all of them.
			ui::relayout(graph, state.selected);
		}
		else
		{
			auto item = state.selected;
			switch(state.resizingEdge)
			{
				case 1: // top
					item->pos.y += delta.y;
					item->size.y -= delta.y;
					break;

				case 2: // upper-right
					item->pos.y += delta.y;
					item->size.y -= delta.y;
					item->size.x += delta.x;
					break;

				case 3: // right
					item->size.x += delta.x;
					break;

				case 4: // lower-right
					item->size += delta;
					break;

				case 5: // bottom
					item->size.y += delta.y;
					break;

				case 6: // bottom-left
					item->pos.x += delta.x;
					item->size.x -= delta.x;
					item->size.y += delta.y;
					break;

				case 7: // left
					item->pos.x += delta.x;
					item->size.x -= delta.x;
					break;

				case 8: // top-left
					item->pos.y += delta.y;
					item->size.y -= delta.y;
					item->pos.x += delta.x;
					item->size.x -= delta.x;
					break;

				default:
					break;
			}

			ui::relayout(graph, state.selected);
		}

		imgui::ResetMouseDragDelta();
	}
















	static Item* hit_test(const lx::vec2& hit, Item* item, bool only_boxes, Item* ignoring)
	{
		if(item == ignoring || (only_boxes && !item->isBox))
			return nullptr;

		if(item->isBox)
		{
			// prioritise hitting the children
			for(auto child : item->box)
			{
				auto x = hit_test(hit - (item->pos + item->content_offset), child, only_boxes, ignoring);
				if(x != nullptr && x != ignoring && (!only_boxes || x->isBox))
					return x;
			}
		}

		return lx::inRect(hit, item->pos, item->size)
			? item
			: nullptr;
	}


	static constexpr double RESIZE_EDGE_THRESHOLD = 5;
	static int get_resize_edge(const lx::vec2& mouse, Item* item)
	{
		if(!item->isBox)
			return 0;

		if(!(item->flags & FLAG_ROOT))
		{
			// here we prioritise the box rather than its children.
			auto tl = item->pos;
			auto tr = item->pos + lx::vec2(item->size.x, 0);
			auto br = item->pos + item->size;
			auto bl = item->pos + lx::vec2(0, item->size.y);

			// we only want to check the "inside" corner (because clicking outside won't select the
			// box, and that's super dumb).
			auto check_corner = [&tl, &tr, &bl](const lx::vec2& mouse, const lx::vec2& pos, double thr) -> bool {
				return ((mouse - pos).magnitude() <= thr)
					&& mouse.x >= tl.x && mouse.x <= tr.x
					&& mouse.y >= tl.y && mouse.y <= bl.y;
			};

			auto check_edge = [&tl, &tr, &bl](const lx::vec2& mouse, const lx::vec2& c1, const lx::vec2& c2, double thr) -> bool {
				if(c1.x == c2.x)    // vertical edge
				{
					return std::abs(mouse.x - c1.x) <= thr
						&& mouse.y >= c1.y && mouse.y <= c2.y
						&& mouse.x <= tr.x && mouse.x >= tl.x;
				}
				else                // horizontal edge
				{
					return std::abs(mouse.y - c1.y) <= thr
						&& mouse.x >= c1.x && mouse.x <= c2.x
						&& mouse.y <= bl.y && mouse.y >= tl.y;
				}
			};

			// check corners first
			if(check_corner(mouse, tr, 2 * RESIZE_EDGE_THRESHOLD))      return 2;
			else if(check_corner(mouse, br, 2 * RESIZE_EDGE_THRESHOLD)) return 4;
			else if(check_corner(mouse, bl, 2 * RESIZE_EDGE_THRESHOLD)) return 6;
			else if(check_corner(mouse, tl, 2 * RESIZE_EDGE_THRESHOLD)) return 8;
			else if(check_edge(mouse, tl, tr, RESIZE_EDGE_THRESHOLD))   return 1;
			else if(check_edge(mouse, tr, br, RESIZE_EDGE_THRESHOLD))   return 3;
			else if(check_edge(mouse, bl, br, RESIZE_EDGE_THRESHOLD))   return 5;
			else if(check_edge(mouse, tl, bl, RESIZE_EDGE_THRESHOLD))   return 7;
		}

		for(auto child : item->box)
		{
			auto x = get_resize_edge(mouse - (item->pos + item->content_offset), child);
			if(x != 0) return x;
		}

		return 0;
	}
}
