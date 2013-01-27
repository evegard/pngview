pngview
=======

pngview is a standalone PNG viewer that decodes PNG files without
relying on libpng or zlib. pngview is written in C99 and uses Xlib to
display the images on the user's screen.

Compile it e.g. like this:

    $ cc -lm -lX11 -O3 -Wall -o pngview -std=c99 *.c

Then run it like this:

    $ ./pngview lena.png

pngview was written for the fun of the exercise, and is neither very
efficient nor bug-free. It supports indexed pixels, grayscale, truecolor
and alpha transparency, but probably not all permutations of the
aforementioned. It assumes well-formed PNG files and a bit depth of 8
for each color channel.
