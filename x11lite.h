#ifndef X11LITE_H
#define X11LITE_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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
X11LiteWindow x11lite_create_window(int width, int height, const char* title);
void x11lite_shutdown(X11LiteWindow* win);

// --- Event Handling ---
int x11lite_poll_event(X11LiteWindow* win, X11LiteEvent* event);
int x11lite_is_key_pressed(X11LiteEvent* event, int keycode);
int x11lite_is_mouse_button_pressed(X11LiteEvent* event, int button);

// --- Drawing Operations ---
uint32_t x11lite_rgb(uint8_t r, uint8_t g, uint8_t b);
void x11lite_clear(X11LiteWindow* win, uint32_t color);
void x11lite_draw_pixel(X11LiteWindow* win, int x, int y, uint32_t color);
void x11lite_draw_rect(X11LiteWindow* win, int x, int y, int w, int h, uint32_t color, int filled);
void x11lite_draw_line(X11LiteWindow* win, int x1, int y1, int x2, int y2, uint32_t color);
void x11lite_present(X11LiteWindow* win);

#endif // X11LITE_H

#ifdef X11LITE_IMPLEMENTATION

static Display* global_display = NULL;

// Helper to set foreground color efficiently
static inline void set_foreground(X11LiteWindow* win, uint32_t color)
{
    XSetForeground(win->display, win->gc, color);
}

// Robust signal handler for graceful termination
static void handle_signal(int sig)
{
    if (global_display)
    {
        XCloseDisplay(global_display);
        global_display = NULL;
    }
    exit(0);
}

// --- Initialization ---
X11LiteWindow x11lite_create_window(int width, int height, const char* title)
{
    X11LiteWindow win = {0};

    // Initialize display
    global_display = XOpenDisplay(NULL);
    if (!global_display)
    {
        fprintf(stderr, "Error: Unable to open X11 display.\n");
        exit(1);
    }

    win.display = global_display;
    win.screen = DefaultScreen(win.display);
    win.width = width;
    win.height = height;

    // Create window
    win.window = XCreateSimpleWindow(
        win.display,
        RootWindow(win.display, win.screen),
        0, 0, width, height,
        1,
        BlackPixel(win.display, win.screen),
        WhitePixel(win.display, win.screen)
    );

    // Set window title
    XStoreName(win.display, win.window, title);

    // Enable close button
    win.wm_delete_window = XInternAtom(win.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win.display, win.window, &win.wm_delete_window, 1);

    // Select input events
    XSelectInput(win.display, win.window, ExposureMask | KeyPressMask | KeyReleaseMask |
                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

    // Map window and create graphics context
    XMapWindow(win.display, win.window);
    win.gc = XCreateGC(win.display, win.window, 0, NULL);
    win.is_open = 1;

    // Setup signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    return win;
}

void x11lite_shutdown(X11LiteWindow* win)
{
    if (win->window)
    {
        XFreeGC(win->display, win->gc);
        XDestroyWindow(win->display, win->window);
        win->is_open = 0;
    }
    if (global_display)
    {
        XCloseDisplay(global_display);
        global_display = NULL;
    }
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
            case ConfigureNotify:
                win->width = xevent.xconfigure.width;
                win->height = xevent.xconfigure.height;
                break;
            default:
                event->type = X11LITE_EVENT_NONE;
                break;
        }
        return 1;
    }
    return 0;
}

int x11lite_is_key_pressed(X11LiteEvent* event, int keycode)
{
    return (event->type == X11LITE_EVENT_KEY_PRESS && event->data.key.keycode == keycode);
}

int x11lite_is_mouse_button_pressed(X11LiteEvent* event, int button)
{
    return (event->type == X11LITE_EVENT_MOUSE_BUTTON_PRESS && event->data.mouse.button == button);
}

// --- Drawing Operations ---
uint32_t x11lite_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) | (g << 8) | b;
}

void x11lite_clear(X11LiteWindow* win, uint32_t color)
{
    set_foreground(win, color);
    XFillRectangle(win->display, win->window, win->gc, 0, 0, win->width, win->height);
}

void x11lite_draw_pixel(X11LiteWindow* win, int x, int y, uint32_t color)
{
    set_foreground(win, color);
    XDrawPoint(win->display, win->window, win->gc, x, y);
}

void x11lite_draw_rect(X11LiteWindow* win, int x, int y, int w, int h, uint32_t color, int filled)
{
    set_foreground(win, color);
    if (filled)
        XFillRectangle(win->display, win->window, win->gc, x, y, w, h);
    else
        XDrawRectangle(win->display, win->window, win->gc, x, y, w, h);
}

void x11lite_draw_line(X11LiteWindow* win, int x1, int y1, int x2, int y2, uint32_t color)
{
    set_foreground(win, color);
    XDrawLine(win->display, win->window, win->gc, x1, y1, x2, y2);
}

void x11lite_present(X11LiteWindow* win)
{
    XFlush(win->display);
}

#endif // X11LITE_IMPLEMENTATION