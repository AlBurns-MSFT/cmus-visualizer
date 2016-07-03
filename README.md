# cmus-visualizer

Work toying around with an ncurses audio bar visualizer. Works in linux and many other unix distros.

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

It wasn't too difficult to handle this step if you want to do it again in the future
