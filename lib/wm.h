#ifndef WM_H
#define WM_H

#include<X11/Xlib.h>
#include<X11/keysym.h>
#include<X11/XKBlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<unistd.h>

#include "utils.h"

#define LOG(...)\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n")

typedef union
{
    int int_;
    float float_;
    char* char_ptr;
} arg_t;

void event_key_press(XEvent *ev);

void event_map(XEvent *ev);
void event_unmap(XEvent *ev);

void event_configure(XEvent *ev);
void event_reparent(XEvent *ev);
void motion_notify(XEvent* ev);
void button_press(XEvent* ev);
void button_release(XEvent* ev);

void client_message(XEvent* ev);

typedef void (*event_handler_t) (XEvent*);

static
event_handler_t event_handlers[]  =
{  
    [KeyPress] = event_key_press,
    [MapRequest] = event_map,
    [UnmapNotify] = event_unmap,
    [ReparentNotify] = event_reparent,
    [ConfigureRequest] = event_configure,
    [MotionNotify] = motion_notify,
    [ButtonPressMask] = button_press,
    [ButtonReleaseMask] = button_release,
    [ClientMessage] = client_message
};

struct client_t
{
    Window frame, window;
    struct client_t *next, *prev;
};
typedef struct client_t client_t;

typedef struct
{
    Display *dpy;
    Window root;
    bool running, ignore;
    XButtonEvent diff;

//    Window focus;
    client_t* focus;
    XWindowAttributes focus_attr;

    client_t *head, *tail;
    int client_count;
} wm_t;

extern wm_t wm;

void wm_init(void);
void wm_deinit(void);

void wm_run(void);

void frame_existing_window(Window window);
void layout_windows(void);

client_t* client_from_window(Window *window);
client_t* client_from_frame(Window frame);

#endif
