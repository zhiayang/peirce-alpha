// ui.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <GL/gl3w.h>
#include <SDL2/SDL.h>

#include "ui.h"
#include "defs.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

namespace ui
{
	static struct {
		SDL_Window* sdlWindow;
		SDL_GLContext glContext;

		Theme theme;
	} uiState;

	void init(zbuf::str_view title)
	{
		// setup the context
		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
			lg::fatal("ui", "couldn't initialise SDL: {}", SDL_GetError());

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // required on osx
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		// setup the window
		uiState.sdlWindow = SDL_CreateWindow(title.str().c_str(),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

		if(uiState.sdlWindow == nullptr)
			lg::fatal("ui", "couldn't create SDL window: {}", SDL_GetError());

		uiState.glContext = SDL_GL_CreateContext(uiState.sdlWindow);
		SDL_GL_MakeCurrent(uiState.sdlWindow, uiState.glContext);
		SDL_GL_SetSwapInterval(1);

		// setup the opengl function loader
		if(bool err = gl3wInit(); err != 0)
			lg::fatal("ui", "couldn't initialise gl3w");

		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForOpenGL(uiState.sdlWindow, uiState.glContext);
		ImGui_ImplOpenGL3_Init("#version 330 core");
	}

	void setup(double uiscale, double fontsize, Theme theme)
	{
		auto& io = ImGui::GetIO();
		io.IniFilename = 0;
		ImGui::StyleColorsDark();

		auto config = ImFontConfig();
		{
			config.OversampleV = 4;
			config.OversampleH = 4;
		}

		io.Fonts->AddFontFromFileTTF("build/assets/menlo.ttf", fontsize, &config, 0);
		io.Fonts->Build();

		uiState.theme = std::move(theme);


		auto& style = ImGui::GetStyle();
		{
			style.ScaleAllSizes(uiscale);
			style.ScrollbarSize = 16;
			style.ScrollbarRounding = 4;

			style.Colors[ImGuiCol_Text] = theme.foreground;
			style.Colors[ImGuiCol_WindowBg] = theme.background;

			style.WindowBorderSize = 0;
		}
	}

	const Theme& theme()
	{
		return uiState.theme;
	}

	void stop()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_GL_DeleteContext(uiState.glContext);
		SDL_DestroyWindow(uiState.sdlWindow);
		SDL_Quit();
	}

	bool poll()
	{
		SDL_Event event = { };
		while(SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			if(event.type == SDL_QUIT)
			{
				return false;
			}
			else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
				event.window.windowID == SDL_GetWindowID(uiState.sdlWindow))
			{
				return false;
			}
		}

		return true;
	}

	void startFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(uiState.sdlWindow);
		ImGui::NewFrame();
	}

	void endFrame()
	{
		auto& io = ImGui::GetIO();

		ImGui::Render();
		glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);

		const auto& bg = uiState.theme.background;
		glClearColor(bg.r, bg.g, bg.b, bg.a);

		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(uiState.sdlWindow);
	}
}
