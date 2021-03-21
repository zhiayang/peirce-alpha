// interact.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <deque>
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
	static void erase_from_parent(Item* item);
	static void handle_shortcuts(const lx::vec2& mouse, Graph* graph);

	static void handle_undo(Graph* graph);
	static void handle_redo(Graph* graph);

	static bool is_char_pressed(char key);
	static bool is_key_pressed(uint32_t scancode);

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

	constexpr int ACTION_DELETE     = 1;
	constexpr int ACTION_CUT        = 2;
	constexpr int ACTION_PASTE      = 3;
	constexpr int ACTION_REPARENT   = 4;
	constexpr size_t MAX_ACTIONS    = 69;

	struct Action
	{
		int type = 0;

		// the thing that was either deleted, cut, or pasted
		Item* item = 0;

		// for reparenting actions
		Item* oldParent = 0;
		lx::vec2 oldPos = {};
	};

	static struct {
		Item* selected = 0;
		Item* dropTarget = 0;

		int resizingEdge = 0;   // 1 = north, 2 = northeast, 3 = east, etc.

		Item* clipboard = 0;

		// 0 = past-the-end; 1 = last action, etc. for each undo index++, and
		// for each redo index--.
		size_t actionIndex = 0;
		std::deque<Action> actions;
	} state;


	void interact(lx::vec2 origin, Graph* graph)
	{
		auto mouse = imgui::GetMousePos() - origin;

		// reset this flag.
		graph->flags &= ~FLAG_GRAPH_MODIFIED;

		if(imgui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			// deselect everything
			for_each_item(&graph->box, [](auto item) { item->flags &= ~FLAG_SELECTED; });

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


		// prune the actions;
		while(state.actions.size() > MAX_ACTIONS)
			state.actions.pop_front();
	}





	static void handle_shortcuts(const lx::vec2& mouse, Graph* graph)
	{
		if(state.selected != nullptr && (is_key_pressed(SDL_SCANCODE_BACKSPACE) || is_key_pressed(SDL_SCANCODE_DELETE)))
		{
			state.actions.push_back(Action {
				.type   = ACTION_DELETE,
				.item   = state.selected
			});

			// first, remove it from its parent
			erase_from_parent(state.selected);

			// unselect it.
			state.selected->flags &= ~FLAG_SELECTED;
			state.selected = nullptr;

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(state.selected != nullptr && (is_char_pressed('x') || is_char_pressed('c'))
			&& (imgui::GetIO().KeySuper || imgui::GetIO().KeyCtrl))
		{
			state.clipboard = state.selected;
			state.selected->flags &= ~FLAG_SELECTED;

			if(is_char_pressed('x'))
			{
				state.actions.push_back(Action {
					.type = ACTION_CUT,
					.item = state.selected
				});

				erase_from_parent(state.selected);
				state.selected = nullptr;
			}

			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(is_char_pressed('v') && (imgui::GetIO().KeySuper || imgui::GetIO().KeyCtrl))
		{
			auto paste = state.clipboard->clone();

			// if there's something selected (and it's a box), paste it inside
			if(state.selected != nullptr && state.selected->isBox)
			{
				state.selected->subs.push_back(paste);

				// if the parent changed, then move it to a sensible location (ie. [0, 0])
				if(paste->parent != state.selected)
					paste->pos = lx::vec2(0, 0);

				paste->parent = state.selected;
			}
			else
			{
				// else paste it at the mouse position.
				auto drop = hit_test(mouse, &graph->box, /* only_boxes: */ true);
				if(drop == nullptr)
					drop = &graph->box;

				auto drop_abs_pos = compute_abs_pos(drop);
				paste->pos = mouse - drop_abs_pos - drop->content_offset;

				paste->parent = drop;
				drop->subs.push_back(paste);
			}

			state.actions.push_back(Action {
				.type = ACTION_PASTE,
				.item = paste
			});

			relayout(graph, paste);
			graph->flags |= FLAG_GRAPH_MODIFIED;
		}
		else if(is_char_pressed('z') && (imgui::GetIO().KeySuper && !imgui::GetIO().KeyShift))
		{
			handle_undo(graph);
		}
		else if(is_char_pressed('z') && (imgui::GetIO().KeySuper && imgui::GetIO().KeyShift))
		{
			handle_redo(graph);
		}
	}


	static void erase_from_parent(Item* item)
	{
		auto parent = item->parent;
		assert(parent != nullptr);

		auto it = std::find(parent->subs.begin(), parent->subs.end(), item);
		assert(it != parent->subs.end());
		parent->subs.erase(it);
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
			state.actions.push_back(Action {
				.type       = ACTION_REPARENT,
				.item       = item,
				.oldParent  = item->parent,
				.oldPos     = item->pos,
			});

			erase_from_parent(item);

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


	void handle_undo(Graph* graph)
	{
		// no more actions.
		if(state.actionIndex >= state.actions.size())
		{
			lg::log("ui", "no more actions to undo");
			return;
		}

		graph->flags |= FLAG_GRAPH_MODIFIED;
		auto& action = state.actions[state.actions.size() - (++state.actionIndex)];
		switch(action.type)
		{
			case ACTION_CUT:
			case ACTION_DELETE:
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			case ACTION_PASTE:
				erase_from_parent(action.item);
				break;

			case ACTION_REPARENT:
				// since we are undoing, the existing parent and positions are the new ones
				erase_from_parent(action.item);
				std::swap(action.item->parent, action.oldParent);
				std::swap(action.item->pos, action.oldPos);

				// the parent got swapped, so add it to the old parent.
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			default:
				lg::error("ui", "unknown action type '{}'", action.type);
				break;
		}
	}

	void handle_redo(Graph* graph)
	{
		// no more actions.
		if(state.actionIndex == 0)
		{
			lg::log("ui", "no more actions to redo");
			return;
		}

		graph->flags |= FLAG_GRAPH_MODIFIED;
		auto& action = state.actions[state.actions.size() - (state.actionIndex--)];
		switch(action.type)
		{
			case ACTION_CUT:
				state.clipboard = action.item;
				[[fallthrough]];

			case ACTION_DELETE:
				erase_from_parent(action.item);
				break;

			case ACTION_PASTE:
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			case ACTION_REPARENT:
				// since we are redoing, the existing parent and positions are the old ones
				erase_from_parent(action.item);
				std::swap(action.item->parent, action.oldParent);
				std::swap(action.item->pos, action.oldPos);

				// the parent got swapped, so add it to the new parent.
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			default:
				lg::error("ui", "unknown action type '{}'", action.type);
				break;
		}
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
}
