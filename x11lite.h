#ifndef X11LITE_H
#define X11LITE_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// --- Data Structures ---
typedef struct
{
    Display* display;
    Window window;
    GC gc;
    int screen;
    int width, height;
    int is_open;
    Atom wm_delete_window;
} X11LiteWindow;

typedef enum
{
    X11LITE_EVENT_NONE,
    X11LITE_EVENT_KEY_PRESS,
    X11LITE_EVENT_KEY_RELEASE,
    X11LITE_EVENT_MOUSE_MOVE,
    X11LITE_EVENT_MOUSE_BUTTON_PRESS,
    X11LITE_EVENT_MOUSE_BUTTON_RELEASE,
    X11LITE_EVENT_WINDOW_CLOSE
} X11LiteEventType;

typedef struct
{
    X11LiteEventType type;
    union {
        struct { int keycode; } key;
        struct { int x, y; int button; } mouse;
    } data;
} X11LiteEvent;

// --- Initialization & Shutdown ---
int x11lite_init();
void x11lite_shutdown();

// --- Window Management ---
X11LiteWindow x11lite_create_window(int width, int height, const char* title);
void x11lite_close_window(X11LiteWindow* win);
int x11lite_window_is_open(X11LiteWindow* win);
void x11lite_set_title(X11LiteWindow* win, const char* title);

// --- Event Handling ---
int x11lite_poll_event(X11LiteWindow* win, X11LiteEvent* event);

// --- Drawing Operations ---
void x11lite_clear(X11LiteWindow* win, uint32_t color);
void x11lite_draw_pixel(X11LiteWindow* win, int x, int y, uint32_t color);
void x11lite_draw_rect(X11LiteWindow* win, int x, int y, int w, int h, uint32_t color);
void x11lite_draw_line(X11LiteWindow* win, int x1, int y1, int x2, int y2, uint32_t color);
void x11lite_present(X11LiteWindow* win);

#endif // X11LITE_H

#ifdef X11LITE_IMPLEMENTATION

static Display* global_display = NULL;

// --- Initialization ---
int x11lite_init()
{
    global_display = XOpenDisplay(NULL);
    if (!global_display)
    {
        return 0; // Failed to open display
    }
    return 1;
}

void x11lite_shutdown()
{
    if (global_display)
    {
        XCloseDisplay(global_display);
        global_display = NULL;
    }
}

// --- Window Management ---
X11LiteWindow x11lite_create_window(int width, int height, const char* title)
{
    X11LiteWindow win = {0};
    win.display = global_display;
    win.screen = DefaultScreen(win.display);
    win.width = width;
    win.height = height;

    win.window = XCreateSimpleWindow(
        win.display,
        RootWindow(win.display, win.screen),
        0, 0, width, height,
        1,
        BlackPixel(win.display, win.screen),
        WhitePixel(win.display, win.screen)
    );

    win.wm_delete_window = XInternAtom(win.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win.display, win.window, &win.wm_delete_window, 1);

    XSelectInput(win.display, win.window, ExposureMask | KeyPressMask | KeyReleaseMask |
                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
    XMapWindow(win.display, win.window);
    win.gc = XCreateGC(win.display, win.window, 0, NULL);
    win.is_open = 1;

    XStoreName(win.display, win.window, title);
    return win;
}

void x11lite_close_window(X11LiteWindow* win)
{
    XDestroyWindow(win->display, win->window);
    win->is_open = 0;
}

int x11lite_window_is_open(X11LiteWindow* win)
{
    return win->is_open;
}

void x11lite_set_title(X11LiteWindow* win, const char* title)
{
    XStoreName(win->display, win->window, title);
}

// --- Event Handling ---
int x11lite_poll_event(X11LiteWindow* win, X11LiteEvent* event)
{
    XEvent xevent;
    if (XPending(win->display))
    {
        XNextEvent(win->display, &xevent);
        switch (xevent.type)
        {
            case KeyPress:
                event->type = X11LITE_EVENT_KEY_PRESS;
                event->data.key.keycode = xevent.xkey.keycode;
                break;
            case KeyRelease:
                event->type = X11LITE_EVENT_KEY_RELEASE;
                event->data.key.keycode = xevent.xkey.keycode;
                break;
            case MotionNotify:
                event->type = X11LITE_EVENT_MOUSE_MOVE;
                event->data.mouse.x = xevent.xmotion.x;
                event->data.mouse.y = xevent.xmotion.y;
                break;
            case ButtonPress:
                event->type = X11LITE_EVENT_MOUSE_BUTTON_PRESS;
                event->data.mouse.button = xevent.xbutton.button;
                break;
            case ButtonRelease:
                event->type = X11LITE_EVENT_MOUSE_BUTTON_RELEASE;
                event->data.mouse.button = xevent.xbutton.button;
                break;
            case ClientMessage:
                if ((Atom)xevent.xclient.data.l[0] == win->wm_delete_window)
                {
                    event->type = X11LITE_EVENT_WINDOW_CLOSE;
                    win->is_open = 0;
                }
                break;
            default:
                event->type = X11LITE_EVENT_NONE;
                break;
        }
        return 1;
    }
    return 0;
}

// --- Drawing Operations ---
void x11lite_clear(X11LiteWindow* win, uint32_t color)
{
    XSetForeground(win->display, win->gc, color);
    XFillRectangle(win->display, win->window, win->gc, 0, 0, win->width, win->height);
}

void x11lite_draw_pixel(X11LiteWindow* win, int x, int y, uint32_t color)
{
    XSetForeground(win->display, win->gc, color);
    XDrawPoint(win->display, win->window, win->gc, x, y);
}

void x11lite_draw_rect(X11LiteWindow* win, int x, int y, int w, int h, uint32_t color)
{
    XSetForeground(win->display, win->gc, color);
    XDrawRectangle(win->display, win->window, win->gc, x, y, w, h);
}

void x11lite_draw_line(X11LiteWindow* win, int x1, int y1, int x2, int y2, uint32_t color)
{
    XSetForeground(win->display, win->gc, color);
    XDrawLine(win->display, win->window, win->gc, x1, y1, x2, y2);
}

void x11lite_present(X11LiteWindow* win)
{
    XFlush(win->display);
}

#endif // X11LITE_IMPLEMENTATION
