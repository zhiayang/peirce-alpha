// cursors.m
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <SDL2/SDL.h>

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

typedef Uint32 SDL_MouseID;

struct SDL_Cursor
{
    struct SDL_Cursor *next;
    void *driverdata;
};

typedef struct
{
    int last_x, last_y;
    Uint32 last_timestamp;
    Uint8 click_count;
} SDL_MouseClickState;

// copied from events/SDL_mouse_c.h... dangerous AF.
struct SDL_Mouse
{
	SDL_Cursor *(*CreateCursor) (SDL_Surface * surface, int hot_x, int hot_y);
    SDL_Cursor *(*CreateSystemCursor) (SDL_SystemCursor id);
    int (*ShowCursor) (SDL_Cursor * cursor);
    void (*MoveCursor) (SDL_Cursor * cursor);
    void (*FreeCursor) (SDL_Cursor * cursor);
    void (*WarpMouse) (SDL_Window * window, int x, int y);
    int (*WarpMouseGlobal) (int x, int y);
    int (*SetRelativeMouseMode) (SDL_bool enabled);
    int (*CaptureMouse) (SDL_Window * window);
    Uint32 (*GetGlobalMouseState) (int *x, int *y);
    SDL_MouseID mouseID;
    SDL_Window *focus;
    int x;
    int y;
    int xdelta;
    int ydelta;
    int last_x, last_y;
    float accumulated_wheel_x;
    float accumulated_wheel_y;
    Uint32 buttonstate;
    SDL_bool has_position;
    SDL_bool relative_mode;
    SDL_bool relative_mode_warp;
    float normal_speed_scale;
    float relative_speed_scale;
    float scale_accum_x;
    float scale_accum_y;
    Uint32 double_click_time;
    int double_click_radius;
    SDL_bool touch_mouse_events;
    SDL_bool mouse_touch_events;
    SDL_bool was_touch_mouse_events;
    int num_clickstates;
    SDL_MouseClickState *clickstate;
    SDL_Cursor *cursors;
    SDL_Cursor *def_cursor;
    SDL_Cursor *cur_cursor;
    SDL_bool cursor_shown;
    void *driverdata;
};

struct SDL_Mouse* SDL_GetMouse(void);

static void add_cursor(SDL_Cursor* cur)
{
	struct SDL_Mouse* mouse = SDL_GetMouse();
	cur->next = mouse->cursors;
	mouse->cursors = cur;
}

SDL_Cursor* macos_create_system_cursor(SDL_SystemCursor cur)
{
	NSCursor* nscursor = nil;

	if(cur == SDL_SYSTEM_CURSOR_SIZENESW)
		nscursor = [[NSCursor class] performSelector:@selector(_windowResizeNorthEastSouthWestCursor)];

	else
		nscursor = [[NSCursor class] performSelector:@selector(_windowResizeNorthWestSouthEastCursor)];

	SDL_Cursor* ret = SDL_calloc(1, sizeof(struct SDL_Cursor));
	if(!ret) return NULL;

	[nscursor retain];
	ret->driverdata = nscursor;

	add_cursor(ret);
	return ret;
}
