# CPU Software Renderer in C

A simple software renderer written in C that draws basic shapes into a framebuffer and writes the result to a 32-bit BMP image file.

## Features

- Framebuffer implementation using a 32-bit pixel buffer
- Clear screen operation with a solid color
- Rectangle rasterization
- Bresenham line drawing algorithm
- BMP file export

## Files

- `main.c` — renderer implementation and BMP writer
- `CPU Software Renderer.pdf` — supporting document for the assignment

## Build

Compile with a standard C compiler:

```sh
cc main.c -o renderer
```

## Run

```sh
./renderer
```

This produces `output.bmp` in the same directory.

## Notes

The current implementation clears the screen, draws a green rectangle, and draws a white line over it. The BMP writer uses a basic 32-bit BMP format for easy viewing.
