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
	ui::setup(/* ui scale: */ 2, /* font size: */ 18.0, ui::light());

	bool flag = false;
	while(true)
	{
		using namespace std::chrono_literals;

		auto n = ui::poll();

		// don't draw unless we have events --- saves CPU. but, also update for one more frame
		// after the last event, so that keys/mouse won't get stuck on.
		if(n < 0)               break;
		else if(n > 0 || flag)  ui::update();
		else if(!flag)          flag = true;
		else                    std::this_thread::sleep_for(16ms);  // target 60fps

		flag = false;
	}

	ui::stop();
}
