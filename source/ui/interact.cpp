// interact.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <deque>
#include <algorithm>
#include <SDL2/SDL.h>

#include "ui.h"
#include "alpha.h"
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
	static bool has_alt();
	static bool has_ctrl();
	static bool has_shift();

	static lx::vec2 compute_abs_pos(Item* item);

	// defined in sidebar.cpp
	const char* get_prop_name();

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
		uint32_t tools = TOOL_MOVE; // enable moving by default.
	} state;

	bool toolEnabled(uint32_t tool) { return state.tools & tool; }
	void disableTool(uint32_t tool) { state.tools &= ~tool; }
	void enableTool(uint32_t tool)
	{
		state.tools |= tool;

		if(tool & TOOL_EVALUATE)
		{
			disableTool(TOOL_EDIT);
			disableTool(TOOL_RESIZE);
		}
	}

	bool toggleTool(uint32_t tool)
	{
		if(state.tools & tool)
			disableTool(tool);

		else
			enableTool(tool);

		return state.tools & tool;
	}

	Selection& selection()    { return state.selection; }

	void setClipboard(std::vector<alpha::Item*> items)
	{
		state.clipboard = std::move(items);
	}

	std::vector<alpha::Item*> getClipboard()
	{
		return state.clipboard;
	}

	bool is_mouse_in_bounds()
	{
		auto geom = geometry::get();

		auto mouse = imgui::GetMousePos();
		auto min = geom.graph.pos;
		auto max = geom.graph.pos + geom.graph.size;

		return (mouse.x >= min.x && mouse.y >= min.y && mouse.x <= max.x && mouse.y <= max.y);
	}

	void interact(lx::vec2 origin, Graph* graph)
	{
		auto mouse = imgui::GetMousePos() - origin;

		// reset this flag.
		graph->flags &= ~FLAG_GRAPH_MODIFIED;

		// a flag that the mouse is in the graphical area. we don't want to limit keyboard shortcuts
		// to only trigger when the mouse is in the graph area, but at the same time we don't want
		// to deselect stuff if we click one of the tool buttons.
		bool mouse_in_bounds = is_mouse_in_bounds();

		if(mouse_in_bounds && imgui::IsMouseClicked(ImGuiMouseButton_Left))
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
		if(mouse_in_bounds && !imgui::IsMouseDown(ImGuiMouseButton_Left))
		{
			// unhover everything
			for_each_item(&graph->box, [](auto item) { item->flags &= ~FLAG_MOUSE_HOVER; });
			if(auto hit = hit_test(mouse, &graph->box, /* only_boxes: */ false); hit != nullptr && !(hit->flags & FLAG_ROOT))
				hit->flags |= FLAG_MOUSE_HOVER;
		}

		handle_shortcuts(mouse, graph);


		// note: handle selected things first, so that if something is deleted in this frame,
		// the cursor doesn't lag for one frame.
		if(state.selection.count() == 1 && toolEnabled(TOOL_EDIT))
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

			if(was_detached && (!has_alt()                          // if you released the shift key
				|| imgui::IsMouseReleased(ImGuiMouseButton_Left)))  // or you released the mouse
			{
				handle_reattach(graph);
			}
			else if((!was_detached && has_alt())                    // if you pressed the shift key
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
		if(auto edge = get_resize_edge(mouse, &graph->box); edge != 0
			&& toolEnabled(TOOL_RESIZE)
			&& state.selection.count() <= 1
			&& !imgui::IsMouseDown(ImGuiMouseButton_Left))
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
			else if(mouse_in_bounds)        ui::setCursor(ImGuiMouseCursor_Arrow);
		}

		if(imgui::IsMouseDragging(ImGuiMouseButton_Left))
			handle_drag(graph);
	}



	void performCopy(alpha::Graph* graph)
	{
		state.clipboard = state.selection.get();
		graph->flags |= FLAG_GRAPH_MODIFIED;
	}

	void performCut(alpha::Graph* graph)
	{
		performCopy(graph);

		ui::performAction(Action {
			.type  = Action::CUT,
			.items = state.selection.get()
		});

		for(auto x : state.selection)
			eraseItemFromParent(graph, x);

		state.selection.clear();
	}

	void performPaste(alpha::Graph* graph, alpha::Item* paste_into, lx::vec2* position_hint)
	{
		std::vector<alpha::Item*> clones;
		for(auto x : state.clipboard)
			clones.push_back(x->clone());

		assert(paste_into && paste_into->isBox);

		paste_into->subs.insert(paste_into->subs.begin(), clones.begin(), clones.end());
		for(auto x : clones)
		{
			if(position_hint != nullptr)
				x->pos = *position_hint;

			else if(x->parent() != paste_into)
				x->pos = lx::vec2(0, 0);

			x->setParent(paste_into);
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

	bool canPaste()
	{
		return state.clipboard.size() > 0
			&& (selection().count() == 1
				|| (selection().empty() && is_mouse_in_bounds())
			);
	}

	bool canCopyOrCut()
	{
		return !state.selection.empty();
	}


	// ...
	static std::pair<Item*, lx::vec2> get_single_selected_box_or_mouse_hover_box(const lx::vec2& mouse, Graph* graph)
	{
		Item* drop = nullptr;
		if(state.selection.empty())
		{
			drop = hit_test(mouse, &graph->box, /* only_boxes: */ true);
			if(drop == nullptr)
				drop = &graph->box;
		}
		else if(state.selection.count() == 1)
		{
			// if you have more than one thing selected, idk where tf to paste to, so i won't.
			drop = state.selection[0];
		}

		if(drop == nullptr)
			return { nullptr, lx::vec2() };

		if(!drop->isBox)
			drop = drop->parent();

		auto drop_abs_pos = compute_abs_pos(drop);
		auto pos = mouse - drop_abs_pos - drop->content_offset;

		return { drop, pos };
	}

	static void handle_shortcuts(const lx::vec2& mouse, Graph* graph)
	{
		constexpr bool REQUIRE_MODS = false;
		if(imgui::GetIO().WantTextInput)
			return;

		if(is_char_pressed('e'))
		{
			ui::toggleTool(TOOL_EDIT);
		}
		else if(is_char_pressed('f'))
		{
			ui::toggleTool(TOOL_EVALUATE);
		}
		else if(is_char_pressed('m'))
		{
			ui::toggleTool(TOOL_MOVE);
		}
		else if(is_char_pressed('r'))
		{
			ui::toggleTool(TOOL_RESIZE);
		}
		else if(is_char_pressed('l'))
		{
			ui::autoLayoutGraph(graph);
			ui::flashButton(EB_BUTTON_RELAYOUT);
		}
		else if(is_key_pressed(SDL_SCANCODE_ESCAPE))
		{
			state.selection.clear();
		}
		else if(is_char_pressed('z') && ((has_cmd() || has_ctrl()) && !has_shift()))
		{
			performUndo(graph);
			ui::flashButton(SB_BUTTON_UNDO);
		}
		else if(is_char_pressed('z') && ((has_cmd() || has_ctrl()) && has_shift()))
		{
			performRedo(graph);
			ui::flashButton(SB_BUTTON_REDO);
		}
		else if(is_char_pressed('q') && (has_ctrl()))
		{
			lg::log("ui", "quitting");
			ui::quit();
		}
		else if(ui::toolEnabled(TOOL_EDIT))
		{
			if(canCopyOrCut() && (is_char_pressed('x') || is_char_pressed('c')) && (!REQUIRE_MODS || has_cmd() || has_ctrl()))
			{
				if(is_char_pressed('c'))
				{
					performCopy(graph);
					ui::flashButton(SB_BUTTON_COPY);
				}
				else
				{
					performCut(graph);
					ui::flashButton(SB_BUTTON_CUT);
				}
			}
			else if(is_char_pressed('v') && canPaste() && (!REQUIRE_MODS || has_cmd() || has_ctrl()))
			{
				auto [ drop, pos ] = get_single_selected_box_or_mouse_hover_box(mouse, graph);
				if(drop == nullptr)
					return;

				performPaste(graph, drop, &pos);
				ui::flashButton(SB_BUTTON_PASTE);
			}
			else if(!state.selection.empty() && (is_key_pressed(SDL_SCANCODE_BACKSPACE) || is_key_pressed(SDL_SCANCODE_DELETE)))
			{
				ui::performAction(Action {
					.type   = Action::DELETE,
					.items  = state.selection.get()
				});

				// first, remove it from its parent
				for(auto x : state.selection)
					eraseItemFromParent(graph, x);

				// unselect it.
				state.selection.clear();

				graph->flags |= FLAG_GRAPH_MODIFIED;
				ui::flashButton(SB_BUTTON_E_DELETE);
			}
			else if(is_char_pressed('1'))
			{
				auto name = ui::get_prop_name();
				if(strlen(name) == 0)
					return;

				auto [ drop, pos ] = get_single_selected_box_or_mouse_hover_box(mouse, graph);
				if(drop == nullptr)
					return;

				auto thing = Item::var(name);
				thing->pos = pos;

				alpha::insert(graph, drop, thing);
				ui::flashButton(SB_BUTTON_E_INSERT);
			}
			else if(is_char_pressed('2'))
			{
				auto [ drop, pos ] = get_single_selected_box_or_mouse_hover_box(mouse, graph);
				if(drop == nullptr)
					return;

				alpha::insertEmptyBox(graph, drop, pos);
				ui::flashButton(SB_BUTTON_E_ADD_BOX);
			}
			else if(is_char_pressed('3'))
			{
				if(selection().empty() && !selection().allSiblings())
					return;

				alpha::surround(graph, selection());
				ui::flashButton(SB_BUTTON_E_SURROUND);
			}
		}
		else if(ui::toolEnabled(TOOL_EVALUATE))
		{
			if(is_char_pressed('1')) ui::toggleVariableState(0);
			if(is_char_pressed('2')) ui::toggleVariableState(1);
			if(is_char_pressed('3')) ui::toggleVariableState(2);
			if(is_char_pressed('4')) ui::toggleVariableState(3);
			if(is_char_pressed('5')) ui::toggleVariableState(4);
			if(is_char_pressed('6')) ui::toggleVariableState(5);
			if(is_char_pressed('7')) ui::toggleVariableState(6);
			if(is_char_pressed('8')) ui::toggleVariableState(7);
			if(is_char_pressed('9')) ui::toggleVariableState(8);
			if(is_char_pressed('0')) ui::toggleVariableState(9);
		}
		else
		{
			// note that the alpha::canFooBar functions check the size of the selection,
			// so that if we need to access sel[0], we know it exists.

			auto& sel = selection();
			if(is_char_pressed('1') && alpha::canInsert(graph, ui::get_prop_name()))
			{
				alpha::insertAtOddDepth(graph, /* parent: */ sel[0], Item::var(ui::get_prop_name()));
				ui::flashButton(SB_BUTTON_INSERT);
			}
			else if(is_char_pressed('2') && alpha::canErase(graph))
			{
				alpha::eraseFromEvenDepth(graph, sel[0]);
				ui::flashButton(SB_BUTTON_ERASE);
			}
			else if(is_char_pressed('3') && alpha::canInsertDoubleCut(graph))
			{
				alpha::insertDoubleCut(graph, sel);
				ui::flashButton(SB_BUTTON_DBL_ADD);
			}
			else if(is_char_pressed('4') && alpha::canRemoveDoubleCut(graph))
			{
				alpha::removeDoubleCut(graph, sel);
				ui::flashButton(SB_BUTTON_DBL_DEL);
			}
			else if(is_char_pressed('5') && alpha::canSelect(graph))
			{
				alpha::selectTargetForIteration(graph, sel.count() == 1 ? sel[0] : nullptr);
				ui::flashButton(SB_BUTTON_SELECT);
			}
			else if(is_char_pressed('6') && alpha::canIterate(graph))
			{
				alpha::iterate(graph, sel[0]);
				ui::flashButton(SB_BUTTON_ITER);
			}
			else if(is_char_pressed('7') && alpha::canDeiterate(graph))
			{
				alpha::deiterate(graph, sel[0]);
				ui::flashButton(SB_BUTTON_DEITER);
			}
		}
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
		if(item->parent() != drop)
		{
			ui::performAction(Action {
				.type       = Action::REPARENT,
				.items      = { item },
				.oldParent  = item->parent(),
				.oldPos     = item->pos,
			});

			eraseItemFromParent(graph, item);

			// also, recalculate its position relative to its new parent.
			auto old_abs_pos = compute_abs_pos(item);
			auto drop_abs_pos = compute_abs_pos(drop);

			item->pos = old_abs_pos - drop_abs_pos - drop->content_offset;

			// and insert it into the new parent.
			drop->subs.push_back(item);
			item->setParent(drop);

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
		imgui::ResetMouseDragDelta();

		if(state.resizingEdge == 0 && toolEnabled(TOOL_MOVE))
		{
			// just move it graphically. relayout() handles shoving etc, but the item being moved
			// will stay in its current 'box' -- so the semantic meaning of the graph is preserved.
			for(auto sel : state.selection.uniqueAncestors())
			{
				sel->pos += delta;
				ui::relayout(graph, sel);
			}
		}
		else if(toolEnabled(TOOL_RESIZE))
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
	}












	static lx::vec2 compute_abs_pos(Item* item)
	{
		if(!item)
			return lx::vec2(0, 0);

		if(item->parent() == nullptr)
			return item->pos;

		return compute_abs_pos(item->parent()) + item->parent()->content_offset + item->pos;
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
		if(imgui::GetIO().WantTextInput)
			return false;

		if('A' <= key && key <= 'Z')    return imgui::IsKeyPressed(SDL_SCANCODE_A + (key - 'A'));
		if('a' <= key && key <= 'z')    return imgui::IsKeyPressed(SDL_SCANCODE_A + (key - 'a'));
		if('1' <= key && key <= '9')    return imgui::IsKeyPressed(SDL_SCANCODE_1 + (key - '1'));
		if(key == '0')                  return imgui::IsKeyPressed(SDL_SCANCODE_0);

		return false;
	}

	static bool is_key_pressed(uint32_t scancode)
	{
		if(imgui::GetIO().WantTextInput)
			return false;

		return imgui::IsKeyPressed(scancode);
	}

	static bool has_cmd()   { return imgui::GetIO().KeySuper; }
	static bool has_alt()   { return imgui::GetIO().KeyAlt; }
	static bool has_ctrl()  { return imgui::GetIO().KeyCtrl; }
	static bool has_shift() { return imgui::GetIO().KeyShift; }
}
