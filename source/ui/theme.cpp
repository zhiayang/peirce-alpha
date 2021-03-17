// theme.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"

namespace ui
{
	Theme light()
	{
		using colour = util::colour;
		return Theme {
			.foreground = colour::fromHexRGB(0x3d3d48),
			.background = colour::fromHexRGB(0xf8f8f4),

			.sidebarBg = colour::fromHexRGB(0xf6f6f6),
		};
	}

	Theme dark()
	{
		// TODO
		return light();
	}
}
