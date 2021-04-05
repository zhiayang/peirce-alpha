// solver.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <set>
#include <unordered_map>

#include "ui.h"
#include "ast.h"

namespace alpha
{
	using Assignment = std::unordered_map<std::string, bool>;

	static volatile bool should_abort = false;
	void abort_solve()
	{
		__atomic_store_n(&should_abort, true, __ATOMIC_SEQ_CST);
	}

	std::vector<Assignment> generate_solutions(ast::Expr* expr, const std::set<std::string>& vars,
		std::pair<size_t, size_t>& progress)
	{
		// cancel any pending aborts
		__atomic_store_n(&should_abort, false, __ATOMIC_SEQ_CST);

		std::vector<Assignment> solns;
		progress.second = (1 << vars.size());

		// uwu.
		for(size_t i = 0; i < progress.second; i++)
		{
			if(__atomic_load_n(&should_abort, __ATOMIC_SEQ_CST))
			{
				should_abort = false;
				break;
			}

			Assignment ass;

			size_t k = 0;
			for(auto& v : vars)
			{
				ass[v] = (i & (1 << k));
				k++;
			}

			auto v = expr->evaluate(ass);
			if(auto l = dynamic_cast<ast::Lit*>(v); l != nullptr && l->value)
				solns.push_back(std::move(ass));

			delete v;
			progress.first = i;
		}

		progress.first = progress.second;
		return solns;
	}
}
