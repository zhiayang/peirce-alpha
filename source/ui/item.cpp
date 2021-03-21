// item.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui::alpha
{
	Graph::Graph(std::vector<Item*> items) : box()
	{
		this->box.isBox = true;
		this->box.flags |= FLAG_ROOT;
		this->box.subs = std::move(items);
	}

	Item::~Item()
	{
		// an item owns its children.
		for(auto child : this->subs)
			delete child;
	}

	Item::Item()
	{
		this->id = ui::getNextId();
	}

	Item* Item::clone() const
	{
		// do a shallow copy first using the default copy-constructor
		auto foo = new Item(*this);

		// make deep copies of the children
		for(auto& child : foo->subs)
			child = child->clone();

		foo->id = ui::getNextId();
		return foo;
	}

	Item* Item::box(std::vector<Item*> items)
	{
		auto ret = new Item();
		ret->name = zpr::sprint("__box_{}", ret->id);
		ret->isBox = true;
		ret->subs = std::move(items);

		return ret;
	}

	Item* Item::var(zbuf::str_view name)
	{
		auto ret = new Item();
		ret->name = name.str();
		ret->isBox = false;

		return ret;
	}
}
