// selection.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui
{
	using namespace alpha;

	bool Selection::empty() const
	{
		return this->items.empty();
	}

	size_t Selection::count() const
	{
		return this->items.size();
	}

	const std::vector<Item*>& Selection::get() const
	{
		return this->items;
	}

	void Selection::clear()
	{
		for(auto x : this->items)
			x->flags &= ~FLAG_SELECTED;

		this->items.clear();
		this->refresh();
	}

	void Selection::add(Item* item)
	{
		if(this->contains(item))
			return;

		item->flags |= FLAG_SELECTED;
		this->items.push_back(item);
		this->refresh();
	}

	void Selection::remove(Item* item)
	{
		item->flags &= ~FLAG_SELECTED;
		this->items.erase(std::remove(this->items.begin(), this->items.end(), item), this->items.end());
		this->refresh();
	}

	void Selection::set(Item* item)
	{
		this->set(std::vector<Item*>({ item }));
	}

	void Selection::toggle(Item* item)
	{
		if(this->contains(item))    this->remove(item);
		else                        this->add(item);
	}

	void Selection::set(std::vector<Item*> items)
	{
		for(auto i : items)
			i->flags |= FLAG_SELECTED;

		this->clear();
		this->items = std::move(items);
		this->refresh();
	}

	bool Selection::contains(Item* item) const
	{
		return std::find(this->items.begin(), this->items.end(), item) != this->items.end();
	}

	Item* const& Selection::operator[] (size_t i) const
	{
		assert(i < this->count());
		return this->items[i];
	}

	void Selection::refresh() const
	{
		this->flags = 0;

		std::optional<Item*> common_parent;
		bool all_have_double_cut = true;

		for(auto item : this->items)
		{
			if(!common_parent || *common_parent == item->parent())
				common_parent = item->parent();

			else
				common_parent = nullptr;

			all_have_double_cut &= hasDoubleCut(item);
		}

		if(common_parent.has_value() && *common_parent != nullptr)
			this->flags |= FLAG_ALL_SIBLINGS;

		if(all_have_double_cut)
			this->flags |= FLAG_ALL_HAVE_DOUBLE_CUT;




		// recalculate the "unique ancestors". basically if you are selected and your parent is also
		// selected, we don't care about you. this applies recursively.
		this->unique_ancestors.clear();

		for(auto x : this->items)
		{
			auto y = x->parent();
			while(y)
			{
				if(y->flags & FLAG_SELECTED)
					goto out;

				y = y->parent();
			}

			this->unique_ancestors.push_back(x);
		out:
			;
		}
	}

	const std::vector<Item*>& Selection::uniqueAncestors() const
	{
		return this->unique_ancestors;
	}
}
