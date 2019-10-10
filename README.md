# cmus-visualizer

This code is an audio bar visualizer, pulling the raw audio from pulseaudio and rendering the visualzier using ncurses. Works in linux and many other unix distros.

All credit for visualizer goes to https://github.com/dpayne/cli-visualizer. I modified an old version of his program as part of a learning exercise to dissect his program and cmus and then integrate them together.

Before I forget to add a separate README, here are the instructions to only install this program:
* install cmus (or clone from repo)
* install using apt-get install:
* git, libfftw3-dev, libasound2-dev, libncursesw5-dev, libpulse-dev, automake, and libtool
* Clone the repo
* run the following:
* ./autogen.sh
* ./configure 
* make

note:
it's easy to call this directly from cmus, just compile from source and edit command_mode.c
add a line to call vis in the big array that contains commands.

