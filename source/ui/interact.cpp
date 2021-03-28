// interact.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <deque>
#include <algorithm>
#include <SDL2/SDL.h>

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
	static void handle_shortcuts(const lx::vec2& mouse, Graph* graph);

	// static void add_action(Graph*);

	static bool is_char_pressed(char key);
	static bool is_key_pressed(uint32_t scancode);

	static bool has_cmd();
	static bool has_ctrl();
	static bool has_shift();

	static lx::vec2 compute_abs_pos(Item* item);


	template <typename Cb>
	static void for_each_item(Item* item, Cb&& cb)
	{
		if(item->isBox)
		{
			for(auto i : item->subs)
				for_each_item(i, static_cast<Cb&&>(cb));
		}

		cb(item);
	}

	static struct {
		Selection selection;
		Item* dropTarget = 0;

		int resizingEdge = 0;   // 1 = north, 2 = northeast, 3 = east, etc.

		std::vector<Item*> clipboard;
		bool editing = false;
	} state;

	bool isEditing() { return state.editing; }
	void toggleEditing() { state.editing ^= true; lg::log("ui", "editing = {}", state.editing); }

	void setClipboard(std::vector<alpha::Item*> items)
	{
		state.clipboard = std::move(items);
	}

	std::vector<alpha::Item*> getClipboard()
	{
		return state.clipboard;
	}

	void interact(lx::vec2 origin, Graph* graph)
	{
		auto mouse = imgui::GetMousePos() - origin;

		// reset this flag.
		graph->flags &= ~FLAG_GRAPH_MODIFIED;

		if(imgui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if(auto hit = hit_test(mouse, &graph->box, /* only_boxes: */ false);
				hit != nullptr && !(hit->flags & FLAG_ROOT))
			{
				if(has_shift())
					state.selection.toggle(hit);    // add

				else if(!state.selection.contains(hit))
					state.selection.set(hit);       // set (ie. remove everything else)
			}
			else if(!has_shift())
			{
				state.selection.clear();
			}
		}

		// only hover things if the mouse is up.
		if(!imgui::IsMouseDown(ImGuiMouseButton_Left))
		{
			// unhover everything
			for_each_item(&graph->box, [](auto item) { item->flags &= ~FLAG_MOUSE_HOVER; });
			if(auto hit = hit_test(mouse, &graph->box, /* only_boxes: */ false); hit != nullptr && !(hit->flags & FLAG_ROOT))
				hit->flags |= FLAG_MOUSE_HOVER;
		}

		handle_shortcuts(mouse, graph);

		// note: handle selected things first, so that if something is deleted in this frame,
		// the cursor doesn't lag for one frame.
		if(state.selection.count() == 1 && state.editing)
		{
			bool was_detached = (state.selection[0]->flags & FLAG_DETACHED);
			if(was_detached)
			{
				// unset the old one first.
				if(state.dropTarget)
					state.dropTarget->flags &= ~FLAG_DROP_TARGET;

				// since the mouse is most likely going to be over the thing we just selected,
				// we have to ignore it so that it doesn't become its own drop target.
				auto drop = hit_test(mouse, &graph->box, /* only_boxes: */ true, /* ignoring: */ state.selection[0]);
				state.dropTarget = drop;
				if(drop != nullptr)
					drop->flags |= FLAG_DROP_TARGET;
			}

			if(was_detached && (!has_shift()                        // if you released the shift key
				|| imgui::IsMouseReleased(ImGuiMouseButton_Left)))  // or you released the mouse
			{
				handle_reattach(graph);
			}
			else if((!was_detached && has_shift())                  // if you pressed the shift key
				&& (imgui::IsMouseDown(ImGuiMouseButton_Left)       // and either the mouse is down
				|| imgui::IsMouseDragging(ImGuiMouseButton_Left)))  // or you are dragging it
			{
				// detach it
				state.selection[0]->flags |= FLAG_DETACHED;
			}
		}


		// tentatively reset the cursor
		if(!imgui::IsMouseDown(ImGuiMouseButton_Left))
			state.resizingEdge = 0;

		// if the mouse is already clicked, then don't change the cursor halfway!
		if(auto edge = get_resize_edge(mouse, &graph->box); edge != 0 && state.selection.count() <= 1 &&
			!imgui::IsMouseDown(ImGuiMouseButton_Left))
		{
			state.resizingEdge = edge;
		}

		// imgui resets the cursor each frame, so we must re-set the cursor each frame.
		{
			auto edge = state.resizingEdge;

			if(edge == 2 || edge == 6)      ui::setCursor(ImGuiMouseCursor_ResizeNESW);
			else if(edge == 4 || edge == 8) ui::setCursor(ImGuiMouseCursor_ResizeNWSE);
			else if(edge == 1 || edge == 5) ui::setCursor(ImGuiMouseCursor_ResizeNS);
			else if(edge == 3 || edge == 7) ui::setCursor(ImGuiMouseCursor_ResizeEW);
			else                            ui::setCursor(ImGuiMouseCursor_Arrow);
		}

		if(imgui::IsMouseDragging(ImGuiMouseButton_Left))
			handle_drag(graph);
	}





	static void handle_shortcuts(const lx::vec2& mouse, Graph* graph)
	{
		if(state.editing && !state.selection.empty() && (is_key_pressed(SDL_SCANCODE_BACKSPACE) || is_key_pressed(SDL_SCANCODE_DELETE)))
		{
			ui::performAction(Action {
				.type   = Action::DELETE,
				.items  = state.selection.get()
			});

			// first, remove it from its parent
			for(auto x : state.selection)
				eraseItemFromParent(x);

			// unselect it.
			state.selection.clear();

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(!state.selection.empty() && (is_char_pressed('x') || is_char_pressed('c')) && (has_cmd() || has_ctrl()))
		{
			state.clipboard = state.selection.get();

			if(state.editing && is_char_pressed('x'))
			{
				ui::performAction(Action {
					.type  = Action::CUT,
					.items = state.selection.get()
				});

				for(auto x : state.selection)
					eraseItemFromParent(x);

				state.selection.clear();
			}

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(state.editing && is_char_pressed('v') && (has_cmd() || has_ctrl()))
		{
			// if you have more than one thing selected, idk where tf to paste to, so i won't.
			if(state.selection.count() > 1)
				return;

			std::vector<Item*> clones;
			for(auto x : state.clipboard)
				clones.push_back(x->clone());

			// if there's something selected (and it's a box), paste it inside
			if(!state.selection.empty() && state.selection[0]->isBox)
			{
				auto foster = state.selection[0];
				foster->subs.insert(foster->subs.end(), clones.begin(), clones.end());

				for(auto x : clones)
				{
					if(x->parent != foster)
						x->pos = lx::vec2(0, 0);

					x->parent = foster;
				}
			}
			else
			{
				// else paste it at the mouse position.
				auto drop = hit_test(mouse, &graph->box, /* only_boxes: */ true);
				if(drop == nullptr)
					drop = &graph->box;

				auto drop_abs_pos = compute_abs_pos(drop);

				for(auto clone : clones)
				{
					clone->pos = mouse - drop_abs_pos - drop->content_offset;
					clone->parent = drop;

					drop->subs.push_back(clone);
				}
			}

			ui::performAction(Action {
				.type  = Action::PASTE,
				.items = clones
			});

			for(auto x : clones)
			{
				x->flags &= ~FLAG_SELECTED;
				relayout(graph, x);
			}

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(is_char_pressed('z') && ((has_cmd() || has_ctrl()) && !has_shift()))
		{
			performUndo(graph);
		}
		else if(is_char_pressed('z') && ((has_cmd() || has_ctrl()) && has_shift()))
		{
			performRedo(graph);
		}
		else if(is_char_pressed('q') && (has_ctrl()))
		{
			lg::log("ui", "quitting");
			ui::quit();
		}
	}


	void eraseItemFromParent(Item* item)
	{
		auto parent = item->parent;
		assert(parent != nullptr);

		auto it = std::find(parent->subs.begin(), parent->subs.end(), item);
		assert(it != parent->subs.end());
		parent->subs.erase(it);
	}

	static void handle_reattach(Graph* graph)
	{
		assert(state.selection.count() == 1);

		auto item = state.selection[0];

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
			ui::performAction(Action {
				.type       = Action::REPARENT,
				.items      = { item },
				.oldParent  = item->parent,
				.oldPos     = item->pos,
			});

			eraseItemFromParent(item);

			// also, recalculate its position relative to its new parent.
			auto old_abs_pos = compute_abs_pos(item);
			auto drop_abs_pos = compute_abs_pos(drop);

			item->pos = old_abs_pos - drop_abs_pos - drop->content_offset;

			// and insert it into the new parent.
			drop->subs.push_back(item);
			item->parent = drop;

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}

		// and finally, relayout it.
		ui::relayout(graph, item);
	}


	static void handle_drag(Graph* graph)
	{
		if(state.selection.empty())
			return;

		lx::vec2 delta = imgui::GetMouseDragDelta();
		if(state.resizingEdge == 0)
		{
			// just move it graphically. relayout() handles shoving etc, but the item being moved
			// will stay in its current 'box' -- so the semantic meaning of the graph is preserved.
			for(auto sel : state.selection.uniqueAncestors())
			{
				sel->pos += delta;
				ui::relayout(graph, sel);
			}
		}
		else
		{
			assert(state.selection.count() == 1);

			auto item = state.selection[0];
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

			ui::relayout(graph, item);
		}

		imgui::ResetMouseDragDelta();
	}












	static lx::vec2 compute_abs_pos(Item* item)
	{
		if(!item)
			return lx::vec2(0, 0);

		if(!item->parent)
			return item->pos;

		return compute_abs_pos(item->parent) + item->parent->content_offset + item->pos;
	}

	static Item* hit_test(const lx::vec2& hit, Item* item, bool only_boxes, Item* ignoring)
	{
		if(item == ignoring || (only_boxes && !item->isBox))
			return nullptr;

		if(item->isBox)
		{
			// prioritise hitting the children
			for(auto child : item->subs)
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

		for(auto child : item->subs)
		{
			auto x = get_resize_edge(mouse - (item->pos + item->content_offset), child);
			if(x != 0) return x;
		}

		return 0;
	}

	static bool is_char_pressed(char key)
	{
		if('A' <= key && key <= 'Z')    return imgui::IsKeyPressed(SDL_SCANCODE_A + (key - 'A'));
		if('a' <= key && key <= 'z')    return imgui::IsKeyPressed(SDL_SCANCODE_A + (key - 'a'));
		if('1' <= key && key <= '9')    return imgui::IsKeyPressed(SDL_SCANCODE_1 + (key - '1'));
		if(key == '0')                  return imgui::IsKeyPressed(SDL_SCANCODE_0);

		return false;
	}

	static bool is_key_pressed(uint32_t scancode)
	{
		return imgui::IsKeyPressed(scancode);
	}

	static bool has_cmd()   { return imgui::GetIO().KeySuper; }
	static bool has_ctrl()  { return imgui::GetIO().KeyCtrl; }
	static bool has_shift() { return imgui::GetIO().KeyShift; }
}
