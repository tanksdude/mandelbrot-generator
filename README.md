# Mandelbrot set Image Generator

A small program that generates a portion of the [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set).

Uses [ImageMagick](https://imagemagick.org/) to make the image. The slow version creates a text file for ImageMagick to use, and the fast version uses the C++ API (Magick++).

## Prerequisites

You need to have ImageMagick installed on your system. Linux: `sudo apt install imagemagick`

If you wish to use Magick++ (which is much faster!), you need to install that too. For Windows, just check the option in the installer (a Windows compile script for this program is not provided). For Linux, I believe `sudo apt install libmagick++-dev graphicsmagick-libmagick-dev-compat` is enough.

## Multi-threaded Results

### Text file version

From my testing, >50% of the time spent in this program (depending on thread count and Mandelbrot location) is waiting on ImageMagick. I didn't expect ImageMagick to be such a bottleneck. Regardless, multi-threading the Mandelbrot calculations are vital to good performance when increasing the image resolution.

This program splits the image into equal vertical portions and assigns each to a different thread. This isn't particularly efficient because the top/bottom slices finish way faster than the center slices. In the future, I may edit this to instead split the image into small squares/rectangles and have the threads cooperatively work through them. However, this change will not improve ImageMagick's performance.

### Magick++ version

Due to using ImageMagick's C++ API rather than pasting an *enormous* text file, the Magick++ version runs much faster (>10x). ImageMagick now only takes ~15% of the total time (depending on thread count and Mandelbrot location). Also the RAM requirements for large images is much reasonable due to the enormous text file not being needed.

Still, splitting the image into equal vertical portions is a moderate loss of potential performance. Oh well, there are better Mandelbrot set libraries out there, and thread workload managing libraries exist.

## Building (Linux)

Just run `make`. If you want to use Magick++ (recommended, as it's much faster!), then run `make plusplus`.

Optionally, you can increase the color precision. Change `typedef uint8_t color_type;` to `typedef uint16_t color_type;` for 16-bit color. If you are making very zoomed-in images, you should probably find or make a better program because this is not very efficient for that use case, but if you want to use this one then you may have to change `typedef float c_float;` to `typedef double c_float;`. Also remove `-ffast-math` from the Makefile in case float precision is really an issue.

## Running (Linux)

`./mandelbrot.out <num_threads> <x_start> <x_end> <y_start> <y_end> <image_width> <image_height> <output_name> <optional coloring file>`

`x_start`, `x_end`, `y_start`, and `y_end` are the bounds on the Mandelbrot set that will be used in the image.

Included in this repository is the result of running `./mandelbrot.out <irrelevant> -2 2 -2 2 1000 1000 example1.png`, `./mandelbrot.out <irrelevant> -2 1 -1.25 1.25 3000 2500 example2.png`, and `./mandelbrot.out <irrelevant> -.65 -.45 .4 .6 2000 2000 example3.png` (see below).

Change the colors by editing the constants "near" the top of `main.cpp`, or by passing in a coloring file (see syntax below).

In my testing I discovered BMP images to be the fastest to make and AVIF to be the slowest. PNG tends to have smaller filesizes than JPG and WEBP for large blocks of colors, such as the default coloring provided.

![example1](example1.png)

![example2](example2.png)

![example3](example3.png)

## Optional Coloring File

The formatting is very simple: every line contains an integer for the iteration count followed by three floats (in range 0.0-1.0) for the color. The iteration count must go from smallest to largest for correct functionality.

```
<iteration> R G B
<iteration> R G B
<iteration> R G B
...
```

Example:

```
2 0.0 0.0 0.0
5 1.0 0.0 0.0
35 0.5 0.5 1.0
10000 1 1 1
```

![example4](example4.png)

# License

MIT

# Acknowledgments

* StackOverflow
* the person I saw that did something very similar to this, but they printed colors in the terminal, and I wanted to do better by using ImageMagick
