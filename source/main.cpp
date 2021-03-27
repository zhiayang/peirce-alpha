// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <thread>
#include <chrono>

#include "ui.h"
#include "ast.h"

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;

	ui::init(/* title: */ "Peirce Alpha System");
	ui::setup(/* ui scale: */ 2, /* font size: */ 18.0, ui::dark());

	int counter = 0;
	while(true)
	{
		using namespace std::chrono_literals;

		auto n = ui::poll();

		// don't draw unless we have events --- saves CPU. but, also update for a few more frames
		// after the last event, so that keys/mouse won't get stuck on.
		if(n < 0)
			break;

		else if(n > 0 || counter > 0)
			ui::update();

		else if(counter == -1)
			counter = 10;

		else if(counter > 0)
			counter -= 1;

		else if(counter == 0)
			counter = -1, std::this_thread::sleep_for(16ms);  // target 60fps
	}

	ui::stop();
}
