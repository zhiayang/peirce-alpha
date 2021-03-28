// infer.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui::alpha
{
	void insertDoubleCut(Graph* graph, const Selection& sel)
	{
		// if(item == nullptr)
		// 	return;

		// // this one is pretty simple.
		// eraseItemFromParent(item);

		// auto p = Item::box({ item });
		// auto gp = Item::box({ p });


	}

	void removeDoubleCut(Graph* graph, Item* item)
	{
		if(item == nullptr)
			return;

		// it's a pretty cheap check, so it should be fine.
		assert(hasDoubleCut(item));

		// basically, get the great grandparent, and reparent the item (and all its siblings)
		// to the great grandparent instead.
		auto p = item->parent;
		auto gp = p->parent;
		auto ggp = gp->parent;

		for(auto sibling : p->subs)
		{
			eraseItemFromParent(sibling);
			sibling->parent = ggp;

			ggp->subs.push_back(sibling);
		}

		// now we can yeet the parent and the grandparent.
		// TODO: this leaks the heck out of the memory
		eraseItemFromParent(p);
		eraseItemFromParent(gp);

		graph->flags |= (FLAG_FORCE_AUTO_LAYOUT | FLAG_GRAPH_MODIFIED);
	}



	bool hasDoubleCut(Item* item)
	{
		if(item == nullptr)
			return false;

		// first check that it has a grandparent:
		auto p = item->parent;
		if(!p || !p->isBox) return false;

		auto gp = p->parent;
		if(!gp || !gp->isBox || gp->flags & FLAG_ROOT) return false;

		// check that the grandparent has the parent as its only child
		// (ie. there is nothing between the two cuts) (ie. the item has no aunts/uncles)
		// note that siblings are ok -- they'll just get their double cut removed also.
		return gp->subs.size() == 1
			&& gp->subs[0] == p;
	}
}
