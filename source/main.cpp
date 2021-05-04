// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <thread>
#include <chrono>

#include "ui.h"
#include "ast.h"

// #include <GL/gl3w.h>
#include <SDL2/SDL.h>
#include <SDL_opengles2.h>

#include "ui.h"
#include "defs.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"


static bool quit = false;
void ui::quit()
{
	::quit = true;
}

static int counter = 0;
void ui::continueDrawing()
{
	counter = 60;
}

#if defined(__EMSCRIPTEN__)
static void main_loop_once()
{
	auto n = ui::poll();

	// don't draw unless we have events --- saves CPU. but, also update for a few more frames
	// after the last event, so that keys/mouse won't get stuck on.
	if(n < 0)
	{
		ui::stop();
		emscripten_cancel_main_loop();
	}

	ui::update();
}
#endif

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;

	ui::init(/* title: */ "Peirce Alpha System");
	ui::setup(argv[0], /* ui scale: */ 2, /* font size: */ 18.0, ui::dark());


#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop(main_loop_once, 0, true);
#else
	while(!quit)
	{
		auto n = ui::poll();
		if(n < 0)
			break;

		ui::update();
	}

	ui::stop();
#endif
}
