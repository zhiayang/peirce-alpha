// expr.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"

using namespace ast;
namespace ui::alpha
{
	Expr* Graph::expr() const
	{
		return this->box.expr();
	}

	Expr* Item::expr() const
	{
		// the root box is not really a box, so we need to special-case it.

		if(!this->isBox)
			return new Var(this->var);

		if(this->box.empty())
			return new Lit((this->flags & FLAG_ROOT) ? true : false);

		Expr* inside = nullptr;

		if(this->box.size() == 1)
		{
			inside = this->box[0]->expr();
		}
		else
		{
			auto a = new And(this->box[0]->expr(), this->box[1]->expr());
			for(size_t i = 2; i < this->box.size(); i++)
				a = new And(a, this->box[i]->expr());

			inside = a;
		}

		if(this->flags & FLAG_ROOT) return inside;
		else                        return new Not(inside);
	}
}
