// interp.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ast.h"

namespace ast
{
	Expr* Var::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr* Lit::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr* And::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

	Expr* Not::evaluate(const std::unordered_map<std::string, bool>& syms) const
	{
		return nullptr;
	}

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
