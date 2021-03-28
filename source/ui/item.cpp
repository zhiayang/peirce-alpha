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

	int Item::depth() const
	{
		return this->cached_depth;
	}

	Item* Item::parent() const
	{
		return this->_parent;
	}

	void Item::setParent(Item* parent)
	{
		this->_parent = parent;

		this->cached_depth = 0;
		if(this->_parent == nullptr || this->_parent->flags & FLAG_ROOT)
			return;

		this->cached_depth = 1 + this->_parent->depth();
	}

	Item* Item::box(std::vector<Item*> items)
	{
		auto ret = new Item();
		ret->name = zpr::sprint("__box_{}", ret->id);
		ret->isBox = true;
		ret->subs = std::move(items);
		for(auto i : ret->subs)
			i->setParent(ret);

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
