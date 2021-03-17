// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"

int main(int argc, char** argv)
{
	zbuf::str_view in = "";





#if 0
	(void) argc;
	(void) argv;

	ui::init(/* title: */ "Peirce Alpha System");
	ui::setup(/* ui scale: */ 2, /* font size: */ 18.0, ui::light());

	while(true)
	{
		if(!ui::poll())
			break;

		ui::startFrame();

		ui::draw();

		ui::endFrame();
	}

	ui::stop();
#endif
}
