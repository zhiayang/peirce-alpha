// action.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <deque>

#include "ui.h"
#include "alpha.h"

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
		// discard the rest of actions
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
				for(auto item : action.items)
				{
					item->parent()->subs.push_back(item);
					ui::relayout(graph, item);
				}
				break;

			case Action::PASTE:
				for(auto item : action.items)
					ui::eraseItemFromParent(item);
				break;

			case Action::REPARENT: {
				assert(action.items.size() == 1);

				// since we are undoing, the existing parent and positions are the new ones
				ui::eraseItemFromParent(action.items[0]);
				std::swap(action.items[0]->pos, action.oldPos);

				auto tmp = action.items[0]->parent();
				action.items[0]->setParent(action.oldParent);
				action.oldParent = tmp;

				// the parent got swapped, so add it to the old parent.
				action.items[0]->parent()->subs.push_back(action.items[0]);
				ui::relayout(graph, action.items[0]);
			} break;

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
				ui::setClipboard(action.items);
				[[fallthrough]];

			case Action::DELETE:
				for(auto item : action.items)
					ui::eraseItemFromParent(item);
				break;

			case Action::PASTE:
				for(auto item : action.items)
				{
					item->parent()->subs.push_back(item);
					ui::relayout(graph, item);
				}
				break;

			case Action::REPARENT: {
				assert(action.items.size() == 1);

				// since we are redoing, the existing parent and positions are the old ones
				ui::eraseItemFromParent(action.items[0]);
				std::swap(action.items[0]->pos, action.oldPos);

				auto tmp = action.items[0]->parent();
				action.items[0]->setParent(action.oldParent);
				action.oldParent = tmp;

				// the parent got swapped, so add it to the new parent.
				action.items[0]->parent()->subs.push_back(action.items[0]);
				ui::relayout(graph, action.items[0]);
			} break;

			default:
				lg::error("ui", "unknown action type '{}'", action.type);
				break;
		}
	}
}
