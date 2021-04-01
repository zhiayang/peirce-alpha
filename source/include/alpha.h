// alpha.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include "defs.h"

namespace alpha
{
	constexpr uint32_t FLAG_SELECTED            = 0x1;  // whether the thing is selected
	constexpr uint32_t FLAG_SHOVED              = 0x2;  // whether the thing was already shoved in the current frame
	constexpr uint32_t FLAG_ROOT                = 0x4;  // whether the box is the root node
	constexpr uint32_t FLAG_DETACHED            = 0x8;  // whether the thing is 'detached' -- is not constrained to its box
	constexpr uint32_t FLAG_DROP_TARGET         = 0x10; // the box is the drop-target of the detached item
	constexpr uint32_t FLAG_GRAPH_MODIFIED      = 0x20; // the graph was modified graphically
	constexpr uint32_t FLAG_FORCE_AUTO_LAYOUT   = 0x40; // force an autolayout of the graph on the next update
	constexpr uint32_t FLAG_MOUSE_HOVER         = 0x80; // the mouse is hovering over it
	constexpr uint32_t FLAG_ITERATION_TARGET    = 0x100;

	struct Item
	{
		bool isBox = false;

		// unions are more trouble than they're worth.
		std::vector<Item*> subs;
		std::string name;


		// these are all set automatically, don't modify them manually
		int id = 0;
		lx::vec2 pos;
		lx::vec2 size;
		lx::vec2 content_offset;

		// state tracking for the auto(re)-layout system
		lx::vec2 min_pos;
		lx::vec2 max_size;

		uint32_t flags = 0;

		int depth() const;
		Item* parent() const;
		void setParent(Item* p);

		ast::Expr* expr() const;
		Item* clone() const;
		~Item();

		Item& operator= (const Item&) = delete;

		static Item* var(zbuf::str_view name);
		static Item* box(std::vector<Item*> items);

	private:
		Item();
		Item(const Item&) = default;

		friend struct Graph;

		Item* _parent = 0;
		int cached_depth = 0;
	};

	// the top-level has special meaning (there is no box around it), so it
	// needs its own struct... probably.
	struct Graph
	{
		uint32_t flags = 0;
		int maxid = 0;
		Item box;

		// internal state.
		Item* iteration_target = 0;
		std::set<const Item*> deiteration_targets;

		ast::Expr* expr() const;
		void setAst(ast::Expr* expr);

		Graph(std::vector<Item*> items);

		Graph(const Graph&) = delete;
		Graph& operator= (const Graph&) = delete;
	};

	void eraseItemFromParent(Item* item);

	// inference rules
	void insertDoubleCut(Graph* graph, const ui::Selection& item, bool log_action = true);
	void removeDoubleCut(Graph* graph, const ui::Selection& item, bool log_action = true);

	void insertAtOddDepth(Graph* graph, Item* parent, Item* item, bool log_action = true);
	void eraseFromEvenDepth(Graph* graph, Item* item, bool log_action = true);

	void iterate(Graph* graph, Item* target, bool log_action = true);
	void deiterate(Graph* graph, Item* target, bool log_action = true);

	bool haveIterationTarget(Graph* graph);
	void selectTargetForIteration(Graph* graph, Item* item);

	bool hasDoubleCut(Item* item);
	bool canIterateInto(Graph* graph, const Item* selection);
	bool areGraphsEquivalent(const Item* a, const Item* b);

	std::set<const Item*> getDeiterationTargets(Graph* graph);
}
