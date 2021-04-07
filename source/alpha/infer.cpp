// infer.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <string.h>

#include "ui.h"
#include "alpha.h"

namespace ui
{
	// ui/layout.cpp
	void calculate_size_for_var(alpha::Item* item);
}

namespace alpha
{
	void eraseItemFromParent(Graph* graph, Item* item)
	{
		auto parent = item->parent();
		assert(parent != nullptr);

		auto it = std::find(parent->subs.begin(), parent->subs.end(), item);
		assert(it != parent->subs.end());
		parent->subs.erase(it);

		ui::selection().remove(item);       // automatically deselect it
		ui::selection().refresh();          // if necessary

		// more safety stuff -- if it was an iteration target,
		// we need to clear the graph's cached deiteration targets
		if(item->flags & FLAG_ITERATION_TARGET)
		{
			graph->iteration_target = nullptr;
			graph->deiteration_targets = alpha::getDeiterationTargets(graph);
		}

		// note: we shouldn't need to remove it from deiteration_targets,
		// since you have no way to 'access' the item after you delete it.
		graph->flags |= FLAG_GRAPH_MODIFIED;
	}


	void selectTargetForIteration(Graph* graph, Item* item)
	{
		if(graph->iteration_target)
			graph->iteration_target->flags &= ~FLAG_ITERATION_TARGET;

		graph->iteration_target = item;
		if(item != nullptr)
			item->flags |= FLAG_ITERATION_TARGET;

		// it's way too troublesome to have to set/reset the flags for each item
		// so instead, we just store the list in the graph and check when rendering.
		graph->deiteration_targets = alpha::getDeiterationTargets(graph);
	}

	bool haveIterationTarget(Graph* graph)
	{
		return graph->iteration_target != nullptr;
	}

	static bool is_ancestor(const Item* item, const Item* ancestor)
	{
		if(item->parent() == nullptr)
			return false;

		return item->parent() == ancestor
			|| is_ancestor(item->parent(), ancestor);
	}

	bool canIterateInto(Graph* graph, const Item* selection)
	{
		// iteration can paste to either the sibling level (ie. it becomes a sibling)
		// or the level of niece and below (ie. paste as a child (or sub-child) of a
		// sibling). "selection" is the target box, ie. when we paste, the pastee will
		// end up inside this box. so, the box itself is already the parent.

		auto iter = graph->iteration_target;
		if(iter == nullptr || iter == selection)
			return false;

		return selection->isBox && (iter->parent() == selection
			|| is_ancestor(selection, iter->parent()));
	}

	void iterate(Graph* graph, Item* target, bool log_action)
	{
		assert(canIterateInto(graph, target));

		auto iter = graph->iteration_target->clone();
		iter->flags &= ~FLAG_ITERATION_TARGET;

		iter->setParent(target);

		target->subs.push_back(iter);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, iter);
		ui::selection().refresh();

		if(log_action)
		{
			ui::performAction(ui::Action {
				.type = ui::Action::INFER_ITERATION,
				.items = { iter },
				.oldParent = target
			});
		}
	}

	void deiterate(Graph* graph, Item* target, bool log_action)
	{
		assert(graph->deiteration_targets.find(target) != graph->deiteration_targets.end());

		eraseItemFromParent(graph, target);
		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, target);
		ui::selection().refresh();

		if(log_action)
		{
			ui::performAction(ui::Action {
				.type = ui::Action::INFER_DEITERATION,
				.items = { target },
				.oldParent = target->parent()
			});
		}
	}

	static void get_deiteration_targets(Graph* graph, std::set<const Item*>& tgts, const Item* thing)
	{
		if(areGraphsEquivalent(graph->iteration_target, thing))
			tgts.insert(thing);

		if(thing->isBox)
		{
			for(auto child : thing->subs)
				get_deiteration_targets(graph, tgts, child);
		}
	}

	std::set<const Item*> getDeiterationTargets(Graph* graph)
	{
		std::set<const Item*> tgts;
		if(graph->iteration_target == nullptr)
			return tgts;

		auto iter = graph->iteration_target;
		assert(iter->parent());
		assert(iter->parent()->isBox);

		// start it by looking at all siblings; recursion will handle their children.
		for(auto sibling : iter->parent()->subs)
		{
			if(sibling != iter)
				get_deiteration_targets(graph, tgts, sibling);
		}

		return tgts;
	}



	void insertAtOddDepth(Graph* graph, Item* parent, Item* item, bool log_action)
	{
		item->setParent(parent);
		parent->subs.push_back(item);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, item);
		ui::selection().refresh();

		if(log_action)
		{
			ui::performAction(ui::Action {
				.type = ui::Action::INFER_INSERTION,
				.items = { item },
				.oldParent = parent
			});
		}
	}

	void eraseFromEvenDepth(Graph* graph, Item* item, bool log_action)
	{
		eraseItemFromParent(graph, item);

		item->flags &= ~FLAG_SELECTED;
		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, item);
		ui::selection().refresh();

		if(log_action)
		{
			ui::performAction(ui::Action {
				.type = ui::Action::INFER_ERASURE,
				.items = { item },
				.oldParent = item->parent()
			});
		}
	}




	void insertDoubleCut(Graph* graph, const ui::Selection& sel, bool log_action)
	{
		if(sel.empty() || !sel.allSiblings())
			return;

		auto items = sel.get();
		for(auto item : items)
			eraseItemFromParent(graph, item);

		auto oldp = items[0]->parent();

		auto p = Item::box(items);      // box() sets the parent of the stuff automatically
		auto gp = Item::box({ p });     // (which is also why we need to cache the old parent)
		p->setParent(gp);

		oldp->subs.push_back(gp);
		gp->setParent(oldp);

		ui::relayout(graph, p);
		ui::relayout(graph, gp);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		sel.refresh();

		// note that this flag is required because we'll be smart obviously and just use
		// this function to undo a double cut removal; we don't want to add an action for that.
		if(log_action)
		{
			ui::performAction(ui::Action {
				.type   = ui::Action::INFER_ADD_DOUBLE_CUT,
				.items  = items
			});
		}
	}

	void removeDoubleCut(Graph* graph, const ui::Selection& sel, bool log_action)
	{
		// we only operate on the first one! every other item selected must be a sibling.
		if(sel.empty() || !hasDoubleCut(sel[0]) || !sel.allSiblings())
			return;

		auto item = sel[0];

		// basically, get the great grandparent, and reparent the item (and all its siblings)
		// to the great grandparent instead.
		auto p = item->parent();
		auto gp = p->parent();
		auto ggp = gp->parent();

		// save the positions of the parent and the grandparent...
		auto ppos = p->pos + p->content_offset;
		auto gppos = gp->pos + gp->content_offset;

		// make a copy first. we cannot iterate over parent->subs and
		// eraseItemFromParent at the same time...
		auto siblings = p->subs;
		for(auto sibling : siblings)
			eraseItemFromParent(graph, sibling);

		for(auto sibling : siblings)
		{
			sibling->setParent(ggp);
			ggp->subs.push_back(sibling);
		}

		// now we can yeet the parent and the grandparent.
		// TODO: this leaks the heck out of the memory
		eraseItemFromParent(graph, p);
		eraseItemFromParent(graph, gp);

		graph->flags |= FLAG_GRAPH_MODIFIED;

		for(auto sibling : siblings)
		{
			// ensure that the items don't move after removing the cut surrounding them,
			// because that would be very annoying.
			sibling->pos += ppos + gppos;
			ui::relayout(graph, sibling);
		}

		sel.refresh();

		if(log_action)
		{
			// here, the list of affected items is all the siblings, not just the selected one.
			ui::performAction(ui::Action {
				.type   = ui::Action::INFER_DEL_DOUBLE_CUT,
				.items  = siblings
			});
		}
	}



	bool hasDoubleCut(Item* item)
	{
		if(item == nullptr)
			return false;

		// first check that it has a grandparent:
		auto p = item->parent();
		if(!p || !p->isBox) return false;

		auto gp = p->parent();
		if(!gp || !gp->isBox || gp->flags & FLAG_ROOT) return false;

		// check that the grandparent has the parent as its only child
		// (ie. there is nothing between the two cuts) (ie. the item has no aunts/uncles)
		// note that siblings are ok -- they'll just get their double cut removed also.
		return gp->subs.size() == 1
			&& gp->subs[0] == p;
	}



	bool canInsertDoubleCut(Graph* graph)
	{
		auto& sel = ui::selection();
		return !sel.empty() && sel.allSiblings();
	}

	bool canRemoveDoubleCut(Graph* graph)
	{
		auto& sel = ui::selection();
		return (sel.count() == 1 || (sel.count() > 1 && sel.allSiblings()))
			&& hasDoubleCut(sel[0]);
	}

	bool canInsert(Graph* graph, const char* name, bool use_prop_name)
	{
		auto& sel = ui::selection();
		/*
			the thing is, we are placing in a box --- so the "final resting place"
			of the inserted item is actually 1 + the depth of the box -- so we
			need to check that the box is an even depth!

			the other good thing is that we don't need to handle the case where we want
			to insert into the outermost "grid" (ie. the field itself) because anything
			added there will be at an even depth -- so the inference rule prevents that
			from happening entirely.
		*/
		auto asdf = sel.count() == 1
			&& sel[0]->isBox
			&& sel[0]->depth() % 2 == 0;

		if(use_prop_name)   return asdf && strlen(name) > 0;
		else                return asdf && haveIterationTarget(graph);
	}

	bool canErase(Graph* graph)
	{
		auto& sel = ui::selection();

		// erase anything from an even depth
		return sel.count() == 1 && sel[0]->depth() % 2 == 0;
	}

	bool canSelect(Graph* graph)
	{
		auto& sel = ui::selection();
		bool desel = (sel.count() == 0 && graph->iteration_target != nullptr);

		// if the selection is empty, make it deselect; selectTargetForIteration knows
		// how to handle nullptrs.
		return (sel.count() == 1) || desel;
	}

	bool canIterate(Graph* graph)
	{
		auto& sel = ui::selection();
		return sel.count() == 1 && alpha::canIterateInto(graph, sel[0]);
	}

	bool canDeiterate(Graph* graph)
	{
		auto& sel = ui::selection();
		auto deiterable = [&](const Item* item) -> bool {
			return graph->deiteration_targets.find(item) != graph->deiteration_targets.end();
		};

		// map-marker-minus
		return (sel.count() == 1 && deiterable(sel[0]));
	}



	// editing stuff
	void insert(Graph* graph, Item* parent, Item* item)
	{
		// fun fact -- insertAtOddDepth does not check the depth.
		insertAtOddDepth(graph, parent, item, /* log_action: */	true);
	}

	void insertEmptyBox(Graph* graph, Item* parent, const lx::vec2& pos)
	{
		auto thing = Item::box({ });
		thing->pos = pos;
		thing->size = lx::vec2(30, 30);

		alpha::insert(graph, parent, thing);
	}

	void surround(Graph* graph, const ui::Selection& sel)
	{
		if(sel.empty() || !sel.allSiblings())
			return;

		auto items = sel.get();
		for(auto item : items)
			eraseItemFromParent(graph, item);

		auto oldp = items[0]->parent();

		auto p = Item::box(items);  // box() sets the parent of the stuff automatically
		p->setParent(oldp);
		oldp->subs.push_back(p);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, p);

		ui::performAction(ui::Action {
			.type   = ui::Action::EDIT_SURROUND,
			.items  = items
		});

		ui::selection().set(p);
	}
}
