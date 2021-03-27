// action.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <deque>

#include "ui.h"

namespace ui
{
	using namespace alpha;

	static struct {
		// 0 = past-the-end; 1 = last action, etc. for each undo index++, and
		// for each redo index--.
		size_t actionIndex = 0;
		std::deque<Action> actions;
	} state;

	constexpr size_t MAX_ACTIONS = 69;


	void performAction(Action action)
	{
		while(state.actionIndex > 0)
			state.actionIndex--, state.actions.pop_front();

		state.actions.push_front(std::move(action));

		// prune the actions;
		while(state.actions.size() > MAX_ACTIONS)
			state.actions.pop_back();
	}


	void performUndo(Graph* graph)
	{
		// no more actions.
		if(state.actionIndex >= state.actions.size())
		{
			lg::log("ui", "no more actions to undo");
			return;
		}

		graph->flags |= FLAG_GRAPH_MODIFIED;
		auto& action = state.actions[state.actionIndex++];
		switch(action.type)
		{
			case Action::CUT:
			case Action::DELETE:
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			case Action::PASTE:
				ui::eraseItemFromParent(action.item);
				break;

			case Action::REPARENT:
				// since we are undoing, the existing parent and positions are the new ones
				ui::eraseItemFromParent(action.item);
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

	void performRedo(Graph* graph)
	{
		// no more actions.
		if(state.actionIndex == 0)
		{
			lg::log("ui", "no more actions to redo");
			return;
		}

		graph->flags |= FLAG_GRAPH_MODIFIED;
		auto& action = state.actions[--state.actionIndex];
		switch(action.type)
		{
			case Action::CUT:
				ui::setClipboard(action.item);
				[[fallthrough]];

			case Action::DELETE:
				ui::eraseItemFromParent(action.item);
				break;

			case Action::PASTE:
				action.item->parent->subs.push_back(action.item);
				ui::relayout(graph, action.item);
				break;

			case Action::REPARENT:
				// since we are redoing, the existing parent and positions are the old ones
				ui::eraseItemFromParent(action.item);
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
}
