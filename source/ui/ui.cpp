// ui.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <chrono>
#include <numeric>

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

		double frameBegin;

		double fps;
		double frametime;

		ImFont* mainFont;
		ImFont* smallFont;
		ImFont* bigIconFont;

		SDL_Cursor* resizeCursorNESW = 0;
		SDL_Cursor* resizeCursorNWSE = 0;
	} uiState;

	static std::pair<double, double> calculate_fps(double frametime);

	// defined in util/cursors.m
	extern "C" SDL_Cursor* macos_create_system_cursor(SDL_SystemCursor id);
	static SDL_Cursor* create_sdl_system_cursor(SDL_SystemCursor id)
	{
	#ifdef __APPLE__
		return macos_create_system_cursor(id);
	#else
		return SDL_CreateSystemCursor(id);
	#endif
	}


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

		// glorious 16:10
		constexpr int MIN_WIDTH = 880;
		constexpr int MIN_HEIGHT = 550;

		// setup the window
		uiState.sdlWindow = SDL_CreateWindow(title.str().c_str(),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			MIN_WIDTH, MIN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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

		uiState.resizeCursorNWSE = create_sdl_system_cursor(SDL_SYSTEM_CURSOR_SIZENWSE);
		uiState.resizeCursorNESW = create_sdl_system_cursor(SDL_SYSTEM_CURSOR_SIZENESW);

		SDL_SetWindowMinimumSize(uiState.sdlWindow, MIN_WIDTH, MIN_HEIGHT);
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

		uiState.mainFont = io.Fonts->AddFontFromFileTTF("assets/menlo.ttf", fontsize, &config, 0);

		const ImWchar symbols_ranges[] = { 0x2200, 0x22ff, 0 }; // mathematical symbols
		const ImWchar icons_ranges[]   = { 0xf000, 0xf8ff, 0 }; // private use area

		config.MergeMode = true;

		io.Fonts->AddFontFromFileTTF("assets/dejavu-mono.ttf", fontsize - 2, &config, symbols_ranges);

		auto iconConf = config;
		iconConf.MergeMode = true;
		iconConf.GlyphMinAdvanceX = 1.5 * fontsize;
		iconConf.GlyphMaxAdvanceX = 1.5 * fontsize;

		io.Fonts->AddFontFromFileTTF("assets/font-awesome-pro-regular.otf", fontsize - 2, &iconConf, icons_ranges);

		uiState.smallFont = io.Fonts->AddFontFromFileTTF("assets/menlo.ttf", 12, &config, 0);
		iconConf.MergeMode = false;

		uiState.bigIconFont = io.Fonts->AddFontFromFileTTF("assets/font-awesome-pro-regular.otf",
			fontsize * 1.2, &iconConf, icons_ranges);

		io.Fonts->Build();

		setTheme(std::move(theme));

		auto& style = ImGui::GetStyle();
		{
			style.ScaleAllSizes(uiscale);
			style.ScrollbarSize = 12;
			style.ScrollbarRounding = 2;

			// button backgrounds don't exist
			style.Colors[ImGuiCol_Button] = util::colour::fromHexRGBA(0);

			style.WindowBorderSize = 0;
		}
	}

	ImFont* getBigIconFont()
	{
		return uiState.bigIconFont;
	}

	void setTheme(Theme theme)
	{
		auto& style = ImGui::GetStyle();
		{
			style.Colors[ImGuiCol_Text] = theme.foreground;
			style.Colors[ImGuiCol_WindowBg] = theme.background;
			style.Colors[ImGuiCol_PopupBg] = theme.tooltipBg;

			style.Colors[ImGuiCol_ButtonActive] = theme.buttonClickedBg;
			style.Colors[ImGuiCol_ButtonHovered] = theme.buttonHoverBg;
		}

		uiState.theme = std::move(theme);
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

	int poll()
	{
		int num = 0;

		SDL_Event event = { };
		while(SDL_PollEvent(&event))
		{
			num++;
			ImGui_ImplSDL2_ProcessEvent(&event);

			if(event.type == SDL_QUIT)
			{
				return -1;
			}
			else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
				event.window.windowID == SDL_GetWindowID(uiState.sdlWindow))
			{
				return -1;
			}
		}

		return num;
	}

	static double timestamp()
	{
		std::chrono::duration<double> dur = std::chrono::system_clock::now().time_since_epoch();
		return dur.count();
	}

	void startFrame()
	{
		uiState.frameBegin = timestamp();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(uiState.sdlWindow);
		ImGui::NewFrame();
	}

	void endFrame()
	{
		auto& io = ImGui::GetIO();

	#if 1
		ImGui::GetForegroundDrawList()->AddText(
			uiState.smallFont, 12,
			lx::vec2(geometry::get().display.size.x - 115, 5),
			uiState.theme.foreground.u32(),
			zpr::sprint("{.1f} fps / {.1f} ms", uiState.fps, uiState.frametime * 1000).c_str()
		);
	#endif

		ImGui::Render();
		glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);

		const auto& bg = uiState.theme.background;
		glClearColor(bg.r, bg.g, bg.b, bg.a);

		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(uiState.sdlWindow);

		std::tie(uiState.fps, uiState.frametime) = calculate_fps(timestamp() - uiState.frameBegin);
	}


	static std::pair<double, double> calculate_fps(double frametime)
	{
		constexpr size_t BUFFER_SIZE = 90;

		static size_t idx = 0;
		static double buffer[BUFFER_SIZE] = { };

		buffer[idx++ % BUFFER_SIZE] = frametime;
		auto ft = std::accumulate(buffer, buffer + BUFFER_SIZE, 0.0) / (double) BUFFER_SIZE;

		return { 1.0 / ft, ft };
	}

	void setCursor(int cursor)
	{
		if(cursor == ImGuiMouseCursor_ResizeNESW)
			SDL_SetCursor(uiState.resizeCursorNESW);

		else if(cursor == ImGuiMouseCursor_ResizeNWSE)
			SDL_SetCursor(uiState.resizeCursorNWSE);

		else
			ImGui::SetMouseCursor(cursor);
	}
}
