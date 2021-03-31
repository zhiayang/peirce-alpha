// infer.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui::alpha
{
	static Item* iterationTarget = nullptr;

	void selectTargetForIteration(Item* item)
	{
		if(iterationTarget)
			iterationTarget->flags &= ~FLAG_ITERATION_TARGET;

		iterationTarget = item;
		item->flags |= FLAG_ITERATION_TARGET;
	}

	bool haveIterationTarget()
	{
		return iterationTarget != nullptr;
	}

	static bool is_ancestor(const Item* item, const Item* ancestor)
	{
		if(item->parent() == nullptr)
			return false;

		return item->parent() == ancestor
			|| is_ancestor(item->parent(), ancestor);
	}

	bool canIterateInto(const Item* selection)
	{
		// iteration can paste to either the sibling level (ie. it becomes a sibling)
		// or the level of niece and below (ie. paste as a child (or sub-child) of a
		// sibling). "selection" is the target box, ie. when we paste, the pastee will
		// end up inside this box. so, the box itself is already the parent.

		auto iter = iterationTarget;
		if(iter == nullptr || iter == selection)
			return false;

		return selection->isBox && (iter->parent() == selection
			|| is_ancestor(selection, iter->parent()));
	}

	void iterate(Graph* graph, Item* target)
	{
		assert(canIterateInto(target));

		auto iter = iterationTarget->clone();
		iter->flags &= ~FLAG_ITERATION_TARGET;

		iter->setParent(target);

		target->subs.push_back(iter);

		graph->flags |= FLAG_GRAPH_MODIFIED;
		ui::relayout(graph, iter);
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
