// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <thread>
#include <chrono>

#include "ui.h"
#include "ast.h"


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

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;

	ui::init(/* title: */ "Peirce Alpha System");
	ui::setup(/* ui scale: */ 2, /* font size: */ 18.0, ui::dark());

	while(!quit)
	{
		using namespace std::chrono_literals;

		auto n = ui::poll();

		constexpr int CUTOFF = 30;

		// don't draw unless we have events --- saves CPU. but, also update for a few more frames
		// after the last event, so that keys/mouse won't get stuck on.
		if(n < 0)
			break;

	#if 0
		// use short-circuiting to check counter first, *then* decrement
		else if(n > 0)
			ui::update(), counter = -1;

		else if(counter > 0)
			ui::update(), counter -= 1;

		else if(counter == -1)
			counter = CUTOFF;

		else if(counter == 0)
			std::this_thread::sleep_for(16ms);  // target 60fps
	#else
		ui::update();
	#endif
	}

	ui::stop();
}
