// ui.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <set>
#include "defs.h"

namespace ast
{
	struct Expr;
}

struct ImFont;

namespace alpha { struct Graph; struct Item; }

namespace ui
{
	struct Theme;

	void init(zbuf::str_view title);
	void setup(double uiscale, double fontsize, Theme theme);
	void stop();
	void quit();
	void setTheme(Theme theme);

	constexpr uint32_t TOOL_EDIT     = 0x1;
	constexpr uint32_t TOOL_MOVE     = 0x2;
	constexpr uint32_t TOOL_RESIZE   = 0x4;
	constexpr uint32_t TOOL_EVALUATE = 0x8;

	bool toolEnabled(uint32_t tool);
	void disableTool(uint32_t tool);
	void enableTool(uint32_t tool);
	bool toggleTool(uint32_t tool);

	ImFont* getBigIconFont();

	// returns < 0 if we should quit, or else it returns the number of events processed.
	int poll();

	void startFrame();
	void endFrame();

	void draw();
	void update();

	// postpone the 'no-event-sleep'.
	void continueDrawing();

	// takes in the ImGuiMouseCursor_ enum.
	void setCursor(int cursor);
	std::vector<alpha::Item*> getClipboard();
	void setClipboard(std::vector<alpha::Item*> item);

	int getNextId();
	void interact(lx::vec2 origin, alpha::Graph* graph);

	void relayout(alpha::Graph* graph, alpha::Item* item);

	void autoLayoutGraph(alpha::Graph* graph);

	struct Selection
	{
		Selection() = default;

		Selection(const Selection&) = delete;
		Selection& operator= (Selection&) = delete;

		bool empty() const;
		size_t count() const;
		bool contains(alpha::Item* item) const;
		const std::vector<alpha::Item*>& get() const;
		const std::vector<alpha::Item*>& uniqueAncestors() const;

		alpha::Item* const& operator[] (size_t i) const;;

		// flag accessors
		bool allSiblings() const { return this->flags & FLAG_ALL_SIBLINGS; }
		bool allHaveDoubleCut() const { return this->flags & FLAG_ALL_HAVE_DOUBLE_CUT; }

		void clear();
		void refresh() const;
		void set(alpha::Item* item);
		void set(std::vector<alpha::Item*> items);

		void add(alpha::Item* item);
		void remove(alpha::Item* item);
		void toggle(alpha::Item* item);

		auto begin()       { return this->items.begin(); }
		auto end()         { return this->items.end(); }

		auto begin() const { return this->items.cbegin(); }
		auto end() const   { return this->items.cend(); }

	private:
		std::vector<alpha::Item*> items;

		mutable uint32_t flags = 0;
		mutable std::vector<alpha::Item*> unique_ancestors;

		uint32_t FLAG_ALL_SIBLINGS          = 1;
		uint32_t FLAG_ALL_HAVE_DOUBLE_CUT   = 2;
	};

	Selection& selection();
	alpha::Item* getSelected();

	void logMessage(const std::string& msg, double timeout_secs);

	void showEvalExpr(const std::string& msg);
	bool showingEvalExpr();
	void resetEvalExpr();

	struct Action
	{
		int type = 0;

		// the thing that was either deleted, cut, or pasted
		std::vector<alpha::Item*> items;

		// for reparenting actions
		alpha::Item* oldParent = 0;
		lx::vec2 oldPos = {};

		static constexpr int DELETE                 = 1;
		static constexpr int CUT                    = 2;
		static constexpr int PASTE                  = 3;
		static constexpr int REPARENT               = 4;
		static constexpr int INFER_ADD_DOUBLE_CUT   = 5;
		static constexpr int INFER_DEL_DOUBLE_CUT   = 6;
		static constexpr int INFER_INSERTION        = 7;
		static constexpr int INFER_ERASURE          = 8;
		static constexpr int INFER_ITERATION        = 9;
		static constexpr int INFER_DEITERATION      = 10;
		static constexpr int EDIT_SURROUND          = 11;
	};

	void performAction(Action action);
	void performUndo(alpha::Graph* graph);
	void performRedo(alpha::Graph* graph);
	bool canUndo();
	bool canRedo();

	bool canPaste();
	bool canCopyOrCut();

	void performCopy(alpha::Graph* graph);
	void performCut(alpha::Graph* graph);
	void performPaste(alpha::Graph* graph, alpha::Item* paste_into, lx::vec2* position_hint = nullptr);

	Theme dark();
	Theme light();
	const Theme& theme();

	struct Theme
	{
		bool dark;

		util::colour foreground;
		util::colour background;

		util::colour sidebarBg;
		util::colour exprbarBg;

		util::colour buttonHoverBg;
		util::colour buttonClickedBg;

		util::colour buttonHoverBg2;
		util::colour buttonClickedBg2;

		util::colour boxSelection;
		util::colour boxHover;

		util::colour boxDetached;
		util::colour boxDropTarget;

		util::colour trueVar;
		util::colour falseVar;

		util::colour textFieldBg;
	};

	struct Styler
	{
		Styler() { }
		~Styler();

		Styler(const Styler&) = delete;
		Styler& operator= (const Styler&) = delete;

		Styler(Styler&& s);
		Styler& operator= (Styler&& s);

		Styler& push(int x, const util::colour& colour);
		Styler& push(int x, const lx::vec2& vec);
		Styler& push(int x, double value);
		Styler& push(int x, float value);
		Styler& push(int x, int value);
		Styler& push(int x, bool flag);

		void pop();

	private:
		int vars = 0;
		int flags = 0;
		int colours = 0;
	};

	// weird stuff.
	constexpr int SB_BUTTON_UNDO        = 1;
	constexpr int SB_BUTTON_REDO        = 2;
	// constexpr int SB_BUTTON_MOVE    = 3;        since these are toggle buttons already, we don't
	// constexpr int SB_BUTTON_RESIZE  = 4;        really need to flash them at all.
	constexpr int SB_BUTTON_COPY        = 5;
	constexpr int SB_BUTTON_CUT         = 6;
	constexpr int SB_BUTTON_PASTE       = 7;

	constexpr int SB_BUTTON_INSERT      = 101;
	constexpr int SB_BUTTON_ERASE       = 102;
	constexpr int SB_BUTTON_DBL_ADD     = 103;
	constexpr int SB_BUTTON_DBL_DEL     = 104;
	constexpr int SB_BUTTON_SELECT      = 105;
	constexpr int SB_BUTTON_ITER        = 106;
	constexpr int SB_BUTTON_DEITER      = 107;

	constexpr int SB_BUTTON_E_DELETE    = 201;
	constexpr int SB_BUTTON_E_INSERT    = SB_BUTTON_INSERT;
	constexpr int SB_BUTTON_E_ADD_BOX   = 203;
	constexpr int SB_BUTTON_E_SURROUND  = 204;

	constexpr int EB_BUTTON_SUBMIT      = 301;
	constexpr int EB_BUTTON_RELAYOUT    = 302;

	constexpr int SB_BUTTON_V_PREV_SOLN = 401;
	constexpr int SB_BUTTON_V_NEXT_SOLN = 402;
	constexpr int SB_BUTTON_V_SOLVE     = 403;

	void flashButton(int button);
	bool buttonFlashed(int button);

	// in sidebar.cpp
	void toggleVariableState(int num);


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
}
