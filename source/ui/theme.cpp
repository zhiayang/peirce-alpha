// theme.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace imgui = ImGui;

namespace ui
{
	Theme light()
	{
		using colour = util::colour;
		return Theme {
			.dark               = false,
			.foreground         = colour::fromHexRGB(0x3d3d48),
			.background         = colour::fromHexRGB(0xf8f8f4),

			.sidebarBg          = colour::fromHexRGB(0xf6f6f6),
			.exprbarBg          = colour::fromHexRGB(0xebebeb),

			.buttonHoverBg      = colour::fromHexRGB(0xb8b8b8),
			.buttonClickedBg    = colour::fromHexRGB(0xd1a6ed),
			.buttonClickedBg2   = colour::fromHexRGB(0x98abf5),

			.boxSelection       = colour::fromHexRGB(0xd64754),
			.boxHover           = colour::fromHexRGB(0xf2aaaf),

			.boxDetached        = colour::fromHexRGB(0x718c00),
			.boxDropTarget      = colour::fromHexRGB(0x4271ae),

			.trueVar            = colour::fromHexRGB(0x69db9b),
			.falseVar           = colour::fromHexRGB(0xee7268),

			.textFieldBg        = colour::fromHexRGB(0xe0e0e0),
		};
	}

	Theme dark()
	{
		using colour = util::colour;
		return Theme {
			.dark               = true,
			.foreground         = colour::fromHexRGB(0xd3d7db),
			.background         = colour::fromHexRGB(0x171717),

			.sidebarBg          = colour::fromHexRGB(0x252525),
			.exprbarBg          = colour::fromHexRGB(0x252525),

			.buttonHoverBg      = colour::fromHexRGB(0x787878),
			.buttonClickedBg    = colour::fromHexRGB(0x774694),
			.buttonClickedBg2   = colour::fromHexRGB(0x42528f),

			.boxSelection       = colour::fromHexRGB(0xb83b47),
			.boxHover           = colour::fromHexRGB(0xdb8f95),

			.boxDetached        = colour::fromHexRGB(0x718c00),
			.boxDropTarget      = colour::fromHexRGB(0x4271ae),

			.trueVar            = colour::fromHexRGB(0x348557),
			.falseVar           = colour::fromHexRGB(0xa84840),

			.textFieldBg        = colour::fromHexRGB(0x202020),
		};
	}


	Styler::~Styler() { this->pop(); }

	Styler::Styler(Styler&& s)
	{
		this->vars = s.vars; s.vars = 0;
		this->flags = s.flags; s.flags = 0;
		this->colours = s.colours; s.colours = 0;
	}

	Styler& Styler::operator= (Styler&& s)
	{
		if(this != &s)
		{
			this->vars = s.vars; s.vars = 0;
			this->flags = s.flags; s.flags = 0;
			this->colours = s.colours; s.colours = 0;
		}
		return *this;
	}

	Styler& Styler::push(int x, const util::colour& colour)
	{
		this->colours++;
		imgui::PushStyleColor(x, colour);
		return *this;
	}

	Styler& Styler::push(int x, const lx::vec2& vec)
	{
		this->vars++;
		imgui::PushStyleVar(x, vec);
		return *this;
	}

	Styler& Styler::push(int x, float value)
	{
		this->vars++;
		imgui::PushStyleVar(x, value);
		return *this;
	}

	Styler& Styler::push(int x, bool flag)
	{
		this->flags++;
		imgui::PushItemFlag(x, flag);
		return *this;
	}

	Styler& Styler::push(int x, int value) { return this->push(x, (float) value); }
	Styler& Styler::push(int x, double value) { return this->push(x, (float) value); }


	void Styler::pop()
	{
		imgui::PopStyleVar(this->vars);
		imgui::PopStyleColor(this->colours);

		this->vars = 0;
		this->colours = 0;

		while(this->flags > 0)
			this->flags -= 1, imgui::PopItemFlag();
	}
}
