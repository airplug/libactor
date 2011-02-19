libactor
--------

``libactor`` is a C library based on the `Actor model`_,
originally written by `Chris Moos`_.
See the doc directory for more information.


To install::

    ./configure
    make
    sudo make install


Try out the example::

    gcc -lactor -o example examples/example.c && ./example


You may have to run ldconfig to reload the library cache.


.. _Actor model: http://en.wikipedia.org/wiki/Actor_model
.. _Chris Moos:  http://www.chrismoos.com/
