// expr.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"

using namespace ast;
namespace ui::alpha
{
	static Expr* transform(Expr* expr)
	{
		if(auto a = dynamic_cast<And*>(expr))
		{
			return new And(transform(a->left), transform(a->right));
		}
		else if(auto o = dynamic_cast<Or*>(expr))
		{
			// A | B  ===  !(!A & !B)
			return new Not(new And(
				new Not(transform(o->left)),
				new Not(transform(o->right))
			));
		}
		else if(auto n = dynamic_cast<Not*>(expr))
		{
			return new Not(transform(n->e));
		}
		else if(auto i = dynamic_cast<Implies*>(expr))
		{
			// A -> B  ===  (!A | B)  ===  !(!!A & !B)  ===  !(A & !B)
			return new Not(new And(
				transform(i->left),
				new Not(transform(i->right))
			));
		}
		else if(auto b = dynamic_cast<BidirImplies*>(expr))
		{
			// A <-> B  ===  (!A & !B) | (A & B)  ===  !(!(!A & !B) & !(A & B))
			return new Not(new And(
				new Not(new And(new Not(transform(b->left)), new Not(transform(b->right)))),
				new Not(new And(transform(b->left), transform(b->right)))
			));
		}
		// we have to make copies of this because we delete the entire old AST after transforming it.
		else if(auto l = dynamic_cast<Lit*>(expr))
		{
			return new Lit(l->value);
		}
		else if(auto v = dynamic_cast<Var*>(expr))
		{
			return new Var(v->name);
		}
		else
		{
			lg::fatal("expr", "invalid expression");
		}
	}

	static std::vector<Item*> make_item(Expr* expr)
	{
		if(auto n = dynamic_cast<Not*>(expr))
		{
			return { Item::box(make_item(n->e)) };
		}
		else if(auto a = dynamic_cast<And*>(expr))
		{
			auto l = make_item(a->left);
			auto r = make_item(a->right);

			std::vector<Item*> ret;
			ret.insert(ret.end(), l.begin(), l.end());
			ret.insert(ret.end(), r.begin(), r.end());

			return ret;
		}
		else if(auto l = dynamic_cast<Lit*>(expr))
		{
			// false is an empty box, true is nothing.
			if(l->value) return { };
			else         return { Item::box({ }) };
		}
		else if(auto v = dynamic_cast<Var*>(expr))
		{
			return { Item::var(v->name) };
		}
		else
		{
			lg::fatal("expr", "invalid expression");
		}
	}



	void Graph::setAst(Expr* expr)
	{
		auto t = transform(expr);
		delete expr;

		this->box.flags |= FLAG_ROOT;
		this->box.subs = make_item(t);
		for(auto child : this->box.subs)
			child->parent = &this->box;

		this->flags |= (FLAG_FORCE_AUTO_LAYOUT | FLAG_GRAPH_MODIFIED);
	}



	Expr* Graph::expr() const
	{
		return this->box.expr();
	}

	Expr* Item::expr() const
	{
		// the root box is not really a box, so we need to special-case it.

		if(!this->isBox)
			return new Var(this->name);

		if(this->subs.empty())
			return new Lit((this->flags & FLAG_ROOT) ? true : false);

		Expr* inside = nullptr;

		if(this->subs.size() == 1)
		{
			inside = this->subs[0]->expr();
		}
		else
		{
			auto a = new And(this->subs[0]->expr(), this->subs[1]->expr());
			for(size_t i = 2; i < this->subs.size(); i++)
				a = new And(a, this->subs[i]->expr());

			inside = a;
		}

		if(this->flags & FLAG_ROOT) return inside;
		else                        return new Not(inside);
	}
}
