// Enable the implementation of x11lite.h functions
#define X11LITE_IMPLEMENTATION
#include "../x11lite.h"
#include <stdio.h>
#include <string.h>

#define MAX_TEXT_LENGTH 256 // Maximum length for the input text buffer

int main()
{
    // Initialize the X11Lite library
    if (!x11lite_init())
    {
        fprintf(stderr, "Failed to initialize X11Lite\n");
        return 1;
    }

    // Create a window with a title "X11Lite Input Showcase"
    X11LiteWindow win = x11lite_create_window(800, 600, "X11Lite Input Showcase");

    // Create an off-screen buffer (double buffering)
    Pixmap buffer = XCreatePixmap(win.display, win.window, win.width, win.height, DefaultDepth(win.display, win.screen));
    GC buffer_gc = XCreateGC(win.display, buffer, 0, NULL);

    char text[MAX_TEXT_LENGTH] = ""; // Buffer to store the typed text
    int text_length = 0;              // Current length of the text

    // Main event loop
    while (x11lite_window_is_open(&win))
    {
        X11LiteEvent event;

        // Poll for events (keyboard, mouse, window close, etc.)
        while (x11lite_poll_event(&win, &event))
        {
            // Handle key press events
            if (event.type == X11LITE_EVENT_KEY_PRESS)
            {
                char buffer[32];
                KeySym keysym;

                // Convert keycode to string representation (character)
                XLookupString(&(XKeyEvent){.display = win.display, .keycode = event.data.key.keycode}, buffer, sizeof(buffer), &keysym, NULL);

                // Handle backspace: delete the last character
                if (keysym == XK_BackSpace && text_length > 0)
                {
                    text[--text_length] = '\0';
                }
                // Handle Enter key: add a new line
                else if (keysym == XK_Return)
                {
                    strcat(text, "\n");
                    text_length = strlen(text);
                }
                // Handle printable characters
                else if (keysym >= XK_space && keysym <= XK_asciitilde && text_length < MAX_TEXT_LENGTH - 1)
                {
                    text[text_length++] = buffer[0];
                    text[text_length] = '\0'; // Null-terminate the string
                }
            }
            // Handle window close event
            else if (event.type == X11LITE_EVENT_WINDOW_CLOSE)
            {
                win.is_open = 0; // Close the window
            }
        }

        // Clear the off-screen buffer with a white background
        XSetForeground(win.display, buffer_gc, 0xFFFFFF);
        XFillRectangle(win.display, buffer, buffer_gc, 0, 0, win.width, win.height);

        // Render the typed text as simple black rectangles
        int x = 10, y = 20; // Starting position for drawing text
        for (int i = 0; i < text_length; ++i)
        {
            if (text[i] == '\n') // Handle new line
            {
                y += 20; // Move to the next line
                x = 10;  // Reset x position
                continue;
            }

            // Draw a black rectangle for each character
            XSetForeground(win.display, buffer_gc, 0x000000);
            XFillRectangle(win.display, buffer, buffer_gc, x, y, 8, 16);
            x += 10; // Move to the right for the next character
        }

        // Copy the off-screen buffer to the window
        XCopyArea(win.display, buffer, win.window, win.gc, 0, 0, win.width, win.height, 0, 0);
        x11lite_present(&win);
    }

    // Cleanup double buffer resources
    XFreePixmap(win.display, buffer);
    XFreeGC(win.display, buffer_gc);

    // Close the window and shut down X11Lite
    x11lite_close_window(&win);
    x11lite_shutdown();

    return 0;
}
