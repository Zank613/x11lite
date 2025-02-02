## X11Lite

**`x11lite.h`**, a minimalist C library designed to simplify window creation, input handling, and basic graphics operations on X11 systems.

## Features
- Basic window management
- Keyboard input handling
- Double buffering to eliminate flickering
- Simple text display (each character as a rectangle)

## Getting Started

### Prerequisites
Ensure you have the X11 development libraries installed on your system. On Debian-based systems:

```bash
sudo apt-get install libx11-dev
```

On Arch Linux:

```bash
sudo pacman -S libx11
```

### Compilation
Compile the example using `gcc`:

```bash
gcc -o x11lite_example example.c -lX11
```

## Code Structure
- `x11lite.h`: The minimalist header library for X11.
- `input_showcase.c`: Demonstrates window creation, input handling, and basic rendering.

## Key Code Snippets
### Initialization
```c
if (!x11lite_init()) {
    fprintf(stderr, "Failed to initialize X11Lite\n");
    return 1;
}
X11LiteWindow win = x11lite_create_window(800, 600, "X11Lite Input Showcase");
```

### Input Handling
```c
if (event.type == X11LITE_EVENT_KEY_PRESS) {
    if (keysym == XK_BackSpace) { /* Handle backspace */ }
    else if (keysym == XK_Return) { /* Handle new line */ }
    else { /* Handle printable characters */ }
}
```

## Dependencies
- **Xlib** (X11 development library)

## Contributing
Feel free to fork the repository, make changes, and submit pull requests.

## License
This project is licensed under the MIT License.

## Acknowledgments
- Inspired by `stb`-style single-header libraries.
