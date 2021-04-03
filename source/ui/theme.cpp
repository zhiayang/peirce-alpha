// theme.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace imgui = ImGui;

namespace ui
{
	static struct {
		struct {
			uint32_t imgSubmit = 0;
			uint32_t imgEdit = 0;
		} light, dark;

		bool loaded = false;
	} resources;

	static void load_resources();

	Theme light()
	{
		if(!resources.loaded)
			load_resources();

		using colour = util::colour;
		return Theme {
			.dark               = false,
			.foreground         = colour::fromHexRGB(0x3d3d48),
			.background         = colour::fromHexRGB(0xf8f8f4),

			.sidebarBg          = colour::fromHexRGB(0xf6f6f6),
			.exprbarBg          = colour::fromHexRGB(0xebebeb),

			.buttonHoverBg      = colour::fromHexRGB(0xb8b8b8),
			.buttonClickedBg    = colour::fromHexRGB(0xbe93d9),

			.buttonHoverBg2     = colour::fromHexRGB(0x787878),
			.buttonClickedBg2   = colour::fromHexRGB(0x8494d1),

			.boxSelection       = colour::fromHexRGB(0xd64754),
			.boxHover           = colour::fromHexRGB(0xf2b8bd),

			.boxDetached        = colour::fromHexRGB(0x718c00),
			.boxDropTarget      = colour::fromHexRGB(0x4271ae),

			.tooltipBg          = colour::fromHexRGB(0xe0e0e0),
			.tooltipText        = colour::fromHexRGB(0x3d3d48),

			.textures = {
				.submit = (void*) (uintptr_t) resources.light.imgSubmit,
				.edit = (void*) (uintptr_t) resources.light.imgEdit,
			}
		};
	}

	Theme dark()
	{
		if(!resources.loaded)
			load_resources();

		using colour = util::colour;
		return Theme {
			.dark               = true,
			.foreground         = colour::fromHexRGB(0xc7ccd1),
			.background         = colour::fromHexRGB(0x171717),

			.sidebarBg          = colour::fromHexRGB(0x252525),
			.exprbarBg          = colour::fromHexRGB(0x252525),

			.buttonHoverBg      = colour::fromHexRGB(0x787878),
			.buttonClickedBg    = colour::fromHexRGB(0x8f62ac),

			.buttonHoverBg2     = colour::fromHexRGB(0xb8b8b8),
			.buttonClickedBg2   = colour::fromHexRGB(0x6272ac),

			.boxSelection       = colour::fromHexRGB(0xf0717d),
			.boxHover           = colour::fromHexRGB(0xf2b8bd),

			.boxDetached        = colour::fromHexRGB(0x718c00),
			.boxDropTarget      = colour::fromHexRGB(0x4271ae),

			.tooltipBg          = colour::fromHexRGB(0x202020),
			.tooltipText        = colour::fromHexRGB(0xc7ccd1),

			.textures = {
				.submit = (void*) (uintptr_t) resources.dark.imgSubmit,
				.edit = (void*) (uintptr_t) resources.dark.imgEdit,
			}
		};
	}

	static void load_resources()
	{
		resources.light.imgSubmit = util::loadImageFromFile("assets/icons/light/forward.png");
		resources.light.imgEdit = util::loadImageFromFile("assets/icons/light/edit.png");

		resources.dark.imgSubmit = util::loadImageFromFile("assets/icons/dark/forward.png");
		resources.dark.imgEdit = util::loadImageFromFile("assets/icons/dark/edit.png");

		resources.loaded = true;
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
