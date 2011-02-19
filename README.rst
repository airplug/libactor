http://www.chrismoos.com

A C library based on the Actor model. See the doc directory for more information.

Install:

./configure
make
sudo make install


Try out the example:

gcc -lactor -o example examples/example.c && ./example

You may have to run ldconfig to reload the library cache.