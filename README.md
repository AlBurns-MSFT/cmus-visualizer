# cmus-visualizer

This code is an audio bar visualizer, pulling the raw audio from pulseaudio and rendering the visualzier using ncurses. Works in linux and many other unix distros.

all credit goes to the below repo:
https://github.com/karlstav/cava

This was a project to fully grok his code and I then integrated it with cmus. You should use his repo not this. If there is interest in adding cava to cmus let me know, I would consider updating this with a current version of cava+cmus+ an easy way to compile on many systems....perhaps with docker.

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

