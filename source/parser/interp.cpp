// interp.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ast.h"

namespace ast
{
	Expr* Var::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		if(auto it = syms.find(this->name); it != syms.end())
			return new Lit(it->second);

		return new Var(this->name);
	}

	Expr* Lit::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return new Lit(this->value);
	}

	Expr* Not::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		auto ee = this->e->evaluate(syms);
		if(auto elit = dynamic_cast<Lit*>(ee); elit != nullptr)
		{
			auto v = elit->value;
			delete ee;

			return new Lit(!v);
		}

		// check if the inside is a not -- eliminate the double negation
		if(auto enot = dynamic_cast<Not*>(ee); enot)
			return enot->e;

		return new Not(ee);
	}

	Expr* And::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		auto ll = this->left->evaluate(syms);
		if(auto llit = dynamic_cast<Lit*>(ll); llit != nullptr)
		{
			if(llit->value)
			{
				delete ll;
				return this->right->evaluate(syms);
			}
			else
			{
				return new Lit(false);
			}
		}

		auto rr = this->right->evaluate(syms);
		if(auto rlit = dynamic_cast<Lit*>(rr); rlit != nullptr)
		{
			if(rlit->value)
			{
				delete rr;
				return ll;
			}
			else
			{
				delete ll;
				return new Lit(false);
			}
		}

		return new And(ll, rr);
	}

	// these 3 don't need to be evaluated, since we'll use transform()
	// to reduce everything to ands and nots.
	Expr* Or::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr* Implies::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr* BidirImplies::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr::~Expr() { }
	Var::~Var() { }
	Lit::~Lit() { }
	And::~And() { delete this->left; delete this->right; }
	Not::~Not() { delete this->e; }
	Or::~Or()   { delete this->left; delete this->right; }
	Implies::~Implies() { delete this->left; delete this->right; }
	BidirImplies::~BidirImplies() { delete this->left; delete this->right; }
}
