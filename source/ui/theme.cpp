// theme.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

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
			.foreground = colour::fromHexRGB(0x3d3d48),
			.background = colour::fromHexRGB(0xf8f8f4),

			.sidebarBg = colour::fromHexRGB(0xf6f6f6),
			.exprbarBg = colour::fromHexRGB(0xebebeb),

			.buttonHoverBg   = colour::fromHexRGB(0xb8b8b8),
			.buttonClickedBg = colour::fromHexRGB(0x8f62ac),

			.boxSelection = colour::fromHexRGB(0xd64754),
			.boxHover = colour::fromHexRGB(0xf2b8bd),

			.boxDetached = colour::fromHexRGB(0x718c00),
			.boxDropTarget = colour::fromHexRGB(0x4271ae),

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
			.foreground = colour::fromHexRGB(0xc7ccd1),
			.background = colour::fromHexRGB(0x171717),

			.sidebarBg = colour::fromHexRGB(0x252525),
			.exprbarBg = colour::fromHexRGB(0x252525),

			.buttonHoverBg   = colour::fromHexRGB(0x787878),
			.buttonClickedBg = colour::fromHexRGB(0x8f62ac),

			.boxSelection = colour::fromHexRGB(0xf0717d),
			.boxHover = colour::fromHexRGB(0xf2b8bd),

			.boxDetached = colour::fromHexRGB(0x718c00),
			.boxDropTarget = colour::fromHexRGB(0x4271ae),

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
}
