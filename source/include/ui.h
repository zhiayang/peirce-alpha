// ui.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include "defs.h"

namespace ast
{
	struct Expr;
}

namespace ui
{
	namespace alpha { struct Graph; struct Item; }
	struct Theme;


	void init(zbuf::str_view title);
	void setup(double uiscale, double fontsize, Theme theme);
	void stop();
	void quit();


	bool isEditing();
	void toggleEditing();

	// returns < 0 if we should quit, or else it returns the number of events processed.
	int poll();

	void startFrame();
	void endFrame();

	void draw();
	void update();

	// takes in the ImGuiMouseCursor_ enum.
	void setCursor(int cursor);

	int getNextId();
	void interact(lx::vec2 origin, alpha::Graph* graph);

	void autoLayout(alpha::Graph* graph, double width);
	void relayout(alpha::Graph* graph, alpha::Item* item);

	Theme dark();
	Theme light();
	const Theme& theme();

	struct Theme
	{
		util::colour foreground;
		util::colour background;

		util::colour sidebarBg;
		util::colour exprbarBg;

		util::colour buttonHoverBg;
		util::colour buttonClickedBg;

		util::colour boxSelection;
		util::colour boxHover;

		util::colour boxDetached;
		util::colour boxDropTarget;

		struct {
			void* submit;
			void* edit;
		} textures;
	};

	namespace geometry
	{
		struct Geometry
		{
			struct {
				lx::vec2 pos;
				lx::vec2 size;
			} sidebar;

			struct {
				lx::vec2 pos;
				lx::vec2 size;
			} graph;

			struct {
				lx::vec2 pos;
				lx::vec2 size;
			} exprbar;

			struct {
				lx::vec2 size;
			} display;
		};

		void update();
		Geometry get();
	}

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
			Item* parent = 0;

			// state tracking for the auto(re)-layout system
			lx::vec2 min_pos;
			lx::vec2 max_size;

			uint32_t flags = 0;

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
		};

		// the top-level has special meaning (there is no box around it), so it
		// needs its own struct... probably.
		struct Graph
		{
			uint32_t flags = 0;
			int maxid = 0;
			Item box;

			ast::Expr* expr() const;
			void setAst(ast::Expr* expr);

			Graph(std::vector<Item*> items);

			Graph(const Graph&) = delete;
			Graph& operator= (const Graph&) = delete;
		};
	}
}
