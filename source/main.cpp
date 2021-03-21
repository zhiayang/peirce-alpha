// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "ast.h"

int main(int argc, char** argv)
{
	// zbuf::str_view in = "a & b & c & d -> x & y & z";
	auto in = "(a -> b) -> c";
	auto x = parser::parse(in);

	if(!x)  zpr::println("error: {}", x.error().msg);
	else    zpr::println("parsed: {}", x.unwrap()->str());





#if 1
	(void) argc;
	(void) argv;

	ui::init(/* title: */ "Peirce Alpha System");
	ui::setup(/* ui scale: */ 2, /* font size: */ 18.0, ui::light());

	while(true)
	{
		if(!ui::poll())
			break;

		ui::update();
	}

	ui::stop();
#endif
}
