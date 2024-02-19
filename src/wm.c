
#include "wm.h"

wm_t wm;

static int border_width = 3;
long border_color = 0xff0000;
long bg_color = 0x0000ff; 

void wm_init(void)
{
    wm.dpy = XOpenDisplay(NULL);
    assert(wm.dpy != NULL);
    wm.root = DefaultRootWindow(wm.dpy);
    wm.running = false;
    wm.head = wm.tail = NULL;
    wm.ignore = true;
    wm.client_count = 0;
    wm.focus = 0;
    wm.diff.subwindow = None;
    XFlush(wm.dpy);
}

void wm_deinit(void)
{

   for(client_t *client = wm.head; client != NULL; client = client->next)
        free(client);

   (void)XCloseDisplay(wm.dpy);
}

void wm_run(void)
{
    XSelectInput(wm.dpy, wm.root,
            SubstructureNotifyMask | SubstructureRedirectMask
             );
    XGrabKey(wm.dpy, AnyKey, Mod4Mask, wm.root, True, GrabModeAsync, GrabModeAsync);
   
    XGrabButton(wm.dpy, AnyButton,
            AnyModifier, wm.root,
            True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
            GrabModeAsync, GrabModeAsync,
            wm.root , None);

    XGrabServer(wm.dpy);

    Window root_ret, parent_ret;
    Window *children;
    unsigned int len;
    XQueryTree(wm.dpy, wm.root,
            &root_ret, &parent_ret,
            &children, &len);
    LOG("Len of tree %d", len);
    for(unsigned int i = 0; i < len; i++)
    {
        frame_existing_window(children[i]);
    }
    wm.client_count = len;
    XFree(children);

    XUngrabServer(wm.dpy);
    
    XSync(wm.dpy, False);
    
    wm.running = true;
    XEvent ev;
    while(wm.running)
    {
        XNextEvent(wm.dpy, &ev);
        if(event_handlers[ev.type] != 0)
            event_handlers[ev.type](&ev);
    }

}

void event_key_press(XEvent *ev)
{
    LOG("Key press");
    KeySym keysym;
    XKeyEvent* xkey = &ev->xkey;

    keysym = XkbKeycodeToKeysym(wm.dpy, xkey->keycode, 0,
            xkey->state & ShiftMask ? 1 : 0);

    switch(keysym)
    {
        case XK_q:
            wm.running = false;
            break;
        case XK_Return:
            exec("/usr/bin/xterm");
            break;
        case XK_l:
            layout_windows();
            break;
        case XK_p:
            for(client_t *i = wm.head; i != NULL; i = i->next)
                LOG("%ld\n", i->window);
            break;
    }
}

void event_map(XEvent *ev)
{
    LOG("Map event");
    XWindowAttributes attrs;
    XMapRequestEvent *map_request = &ev->xmaprequest;


    if(map_request->window == wm.root)
        return;

    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        if(map_request->window == client->window || map_request->window == client->frame)
            return;
    }

    LOG("%p", map_request);
    XGetWindowAttributes(wm.dpy, map_request->window, &attrs);

    wm.client_count++;
    client_t *new_client = (client_t*)malloc(sizeof(client_t));
    new_client->next = NULL;

    client_t *prev_head = wm.head;
    new_client->next = prev_head;

    wm.head = new_client;

    new_client->window = map_request->window;

    new_client->frame = XCreateSimpleWindow(wm.dpy,
            wm.root, attrs.x, attrs.y,
            attrs.width, attrs.height,
           border_width, border_color, bg_color);

    LOG("Storing newclient: %p, with window: %ld and frame %ld", new_client, new_client->window, new_client->frame);
    //XSelectInput(wm.dpy, new_client->frame,
    XSelectInput(wm.dpy, new_client->frame,
            SubstructureNotifyMask | SubstructureRedirectMask );

    XReparentWindow(wm.dpy, new_client->window, new_client->frame, 0, 0);
    XMapWindow(wm.dpy, new_client->frame);
    XMapWindow(wm.dpy, map_request->window);

 //   layout_windows();

    XAddToSaveSet(wm.dpy, new_client->window);

}

// TODO: move the contents of this function somewhere else
// using XUnmapWindow() makes this window call itself.
// A check for NULL prevents errors for now
void event_unmap(XEvent *ev)
{
    LOG("Unmap event");
    XUnmapEvent *unmap_event = &ev->xunmap;
    
    client_t *client = wm.head;
    client_t *prev = NULL;
    for(; client != NULL; client = client->next)
    {
        // NOTE: for some fucking reason the actual window ID is stored
        // in the member called "event" and not the member called "window"
        // WHAT THE ACTUAL FUCK
        if(client->window == unmap_event->event || client->frame == unmap_event->event)
        {
            XUnmapWindow(wm.dpy, client->frame);
            break;
        }
        prev = client;
    }
    if(client == NULL) 
        return;

    if(client == wm.head)
        wm.head = client->next;
    else
        prev->next = client->next;

    free(client); 
    wm.client_count--;
    
}

void event_configure(XEvent *ev)
{
    if(ev->xconfigurerequest.window == wm.root)
    {
        return;
    }
    LOG("Configure event");

    XWindowChanges window_changes;
    XConfigureRequestEvent *configure_event = &ev->xconfigurerequest;

//    window_changes.x = configure_event->x;
    window_changes.x = configure_event->x;
    window_changes.y = configure_event->y;
    window_changes.width = configure_event->width;
    window_changes.height = configure_event->height;
    LOG("x, y: %d %d, height, width: %d %d", configure_event->x, configure_event->y,
            configure_event->height, configure_event->width);
    LOG("SET: x, y: %d %d, height, width: %d %d", window_changes.x, window_changes.y,
            window_changes.height, window_changes.width);
    window_changes.sibling = configure_event->above;
    window_changes.stack_mode = configure_event->detail;

    Window frame = {0}; // Find frame from window

    XConfigureWindow(wm.dpy, configure_event->window,
            configure_event->value_mask, &window_changes);
/*
    int count = 0;
    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        if(client->window == configure_event->window)
        {
            LOG("Found frame");
            frame = client->frame;
//            window_changes.x = (1920/wm.client_count)*count;
  //          XConfigureWindow(wm.dpy, frame,
   //         configure_event->value_mask, &window_changes);
            break;
        }
        count++;
    }
//    layout_windows();
*/
}

void event_reparent(XEvent *ev)
{}

void motion_notify(XEvent* ev)
{
//    Window w = wm.head->frame;
//    Window w = wm.focus;

    if(wm.diff.subwindow != None)
    {
        int x = (ev->xmotion.x_root - wm.diff.x_root) + wm.focus_attr.x;
        int y = (ev->xmotion.y_root - wm.diff.y_root) + wm.focus_attr.y;

        int y_resize = (ev->xmotion.y_root - wm.diff.y_root) + wm.focus_attr.height;
        int x_resize = (ev->xmotion.x_root - wm.diff.x_root) + wm.focus_attr.width;

        switch(wm.diff.button)
        {
            case Button1:
                XMoveWindow(wm.dpy, wm.focus->frame, ( x < 0 ? 0 : x ), (y < 0 ? 0 : y));
                break;
            case Button3:

                XResizeWindow(wm.dpy, wm.focus->frame, ( x_resize < 1 ? 1 : x_resize ), (y_resize < 1 ? 1 : y_resize));
                XResizeWindow(wm.dpy, wm.focus->window, ( x_resize < 1 ? 1 : x_resize ), (y_resize < 1 ? 1 : y_resize));
                break;

        }
//        XMoveWindow(wm.dpy, w, ev->xmotion.x, ev->xmotion.y);
    }
}

void button_press(XEvent* ev)
{
    LOG("click");
    XButtonEvent e = ev->xbutton;
    
    if(e.subwindow != None)
    {
        XRaiseWindow(wm.dpy, e.subwindow);
        wm.focus = client_from_frame(e.subwindow);
        wm.diff = e;
        XGetWindowAttributes(wm.dpy, e.subwindow, &wm.focus_attr);
        if(e.button == Button2)
        {
            XEvent event;
            event.xclient.type = ClientMessage;
            event.xclient.window = wm.focus->frame;
            event.xclient.message_type = XInternAtom(wm.dpy, "WM_PROTOCOLS", True);
            event.xclient.format = 32;
            event.xclient.data.l[0] = XInternAtom(wm.dpy, "WM_DELETE_WINDOW", False);
            event.xclient.data.l[1] = CurrentTime;
            XSendEvent(wm.dpy, wm.focus->frame, False, NoEventMask, &event);
//            XDestroyWindow(wm.dpy, e.subwindow);
            wm.diff.subwindow = None;
            wm.focus = NULL;
            LOG("Destroy sent");
        }
    }


}

void button_release(XEvent* ev)
{
    LOG("\"unclick\"");
    wm.focus = 0;
    wm.diff.subwindow = None;
}

void client_message(XEvent* ev)
{
//    XClientMessageEvent *client_message = &ev->xclient;

    event_unmap(ev);
}


void frame_existing_window(Window window)
{
    LOG("Framing rxisting window");
    XWindowAttributes attrs;


    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        if(window == client->window || window == client->frame)
            return;
    }

    XGetWindowAttributes(wm.dpy, window, &attrs);

    client_t *new_client = malloc(sizeof(client_t));

    new_client->next = NULL;

    client_t *prev_head = wm.head;
    new_client->next = prev_head;

    wm.head = new_client;

    new_client->window = window;

    new_client->frame = XCreateSimpleWindow(wm.dpy,
            wm.root, attrs.x, attrs.y,
            attrs.width, attrs.height,
           border_width, border_color, bg_color);

    XSelectInput(wm.dpy, new_client->frame,
            SubstructureNotifyMask | SubstructureRedirectMask);
//    XSelectInput(wm.dpy, new_client->window,
 //           SubstructureNotifyMask | SubstructureRedirectMask);

    XAddToSaveSet(wm.dpy, new_client->window);
    XReparentWindow(wm.dpy, new_client->window, new_client->frame, 0, 0);
    XMapWindow(wm.dpy, new_client->frame);
//    XMapWindow(wm.dpy, new_client->window);
    
}

void layout_windows(void)
{
    int count = 0;
    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        LOG("layout cycle, win: %ld", client->window);
        XMoveResizeWindow(wm.dpy, client->frame,
                (1920/wm.client_count)*count, 0,
                1920/wm.client_count, 1080);
//        XSync(wm.dpy, False);

        XResizeWindow(wm.dpy, client->window, 1920/wm.client_count, 1080);
        count++;
    }
}

client_t* client_from_window(Window *window)
{
    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        LOG("Searching for frame from: %ld, client %p window %ld frame %ld", *window, client, client->window, client->frame);
        if(client->window == *window)
        {
            LOG("found frame");
            return client;
        }
    }
    LOG("frame not found");
    return NULL;
}

client_t* client_from_frame(Window frame)
{
    for(client_t *client = wm.head; client != NULL; client = client->next)
    {
        LOG("Searching for client from: %ld, client %p window %ld frame %ld", frame, client, client->window, client->frame);
        if(client->frame == frame)
        {
            LOG("found frame");
            return client;
        }
    }
    LOG("frame not found");
    return NULL;
}

