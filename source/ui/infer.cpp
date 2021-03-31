// infer.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui::alpha
{
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

	void iterate(Graph* graph, Item* target)
	{
		assert(canIterateInto(graph, target));

		auto iter = graph->iteration_target->clone();
		iter->flags &= ~FLAG_ITERATION_TARGET;

		iter->setParent(target);

		target->subs.push_back(iter);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, iter);
	}

	void deiterate(Graph* graph, Item* target)
	{
		assert(graph->deiteration_targets.find(target) != graph->deiteration_targets.end());

		eraseItemFromParent(target);
		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, target);
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



	void insertAtOddDepth(Graph* graph, Item* parent, Item* item)
	{
		item->setParent(parent);
		parent->subs.push_back(item);

		graph->flags |= (FLAG_GRAPH_MODIFIED | FLAG_FORCE_AUTO_LAYOUT);
		ui::relayout(graph, item);
	}

	void eraseFromEvenDepth(Graph* graph, Item* item)
	{
		ui::eraseItemFromParent(item);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, item);
	}




	void insertDoubleCut(Graph* graph, const Selection& sel)
	{
		if(sel.empty() || !sel.allSiblings())
			return;

		for(auto item : sel)
			eraseItemFromParent(item);

		auto oldp = sel[0]->parent();

		auto p = Item::box(sel.get());  // box() sets the parent of the stuff automatically
		auto gp = Item::box({ p });     // (which is also why we need to cache the old parent)
		p->setParent(gp);

		oldp->subs.push_back(gp);
		gp->setParent(oldp);

		graph->flags |= (FLAG_FORCE_AUTO_LAYOUT | FLAG_GRAPH_MODIFIED);
		sel.refresh();
	}

	void removeDoubleCut(Graph* graph, const Selection& sel)
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

		for(auto sibling : p->subs)
		{
			eraseItemFromParent(sibling);
			sibling->setParent(ggp);

			ggp->subs.push_back(sibling);
		}

		// now we can yeet the parent and the grandparent.
		// TODO: this leaks the heck out of the memory
		eraseItemFromParent(p);
		eraseItemFromParent(gp);

		graph->flags |= (FLAG_FORCE_AUTO_LAYOUT | FLAG_GRAPH_MODIFIED);
		sel.refresh();
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
}
