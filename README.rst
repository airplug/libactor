libactor
--------

``libactor`` is a C library implementing the `Actor model`_ using `pthreads`_.

The library works great,
but one caveat is that if one actor crashes -- they all fall down.
In a future release there will be sandboxing of each actor.


To install:: shell

    ./configure
    make
    sudo make install


Try out the example::

    gcc -lactor -o example examples/example.c && ./example


You may have to run ldconfig to reload the library cache.


Usage
=====

To use ``libactor``,
include ``<libactor/actor.h>`` in your project
and compile with the flag ``-lactor``.


Declaring your Actors
"""""""""""""""""""""

Actors are just threads,
and as in ``pthreads``,
a thread is an executing function.
To define a new actor you can just define a new function
that takes a single ``void*`` as an argument
and returns a single ``void*``::

    void * foo(void * args) {
      /* your Actor's logic goes here */
    }
 

For clarity, you can use the macro ``ACTOR_FUNCTION(name, args)``, where
``name`` is the name of the function,
and ``args`` is any arguments passed to the actor as a ``void*``,
like so::

    ACTOR_FUNCTION(foo, args) {
      /* your Actor's logic goes here */
    }

``libactor`` distinguishes a *main* actor.
This actor will be executed when the application starts,
and passed a pointer to a struct containing the program arguments.
In your main file, declare your main actor like so::

    ACTOR_FUNCTION(main_actor, args) {
      struct actor_main * main = (struct actor_main *) args;
 
      /* Accessing the arguments passed to the application */
      printf("Number of arguments: %d\n", main->argc);
      for(int x = 0; x < main->argc; x++)
        printf("Argument: %s\n", main->argv[x]);
    }
 
    DECLARE_ACTOR_MAIN(main_actor)
 

Managing your Actors
""""""""""""""""""""

To spawn a new Actor,
use ``spawn_actor(function, args)``.
To spawn the ``foo`` Actor
with a ``NULL`` pointer as an argument::

    ACTOR_FUNCTION(main_actor, args) {
      actor_id foo_id = spawn_actor(foo, NULL);
    }

Notice the return type of ``spawn_actor()``:
an ``actor_id``.
This uniquely identifies the spawned Actor,
and allows communication between Actors to identify the sender and receiver.

After a ``foo`` Actor is spawned,
it can obtain its ID using ``actor_self()``::

    ACTOR_FUNCTION(foo, args) {
      actor_id my_id = actor_self();
    }



Data Types
""""""""""

.. ctype:: actor_msg_t

  This structure contains information about a message. The following fields are available:

  .. cmember:: sender
  
    The :ctype:`actor_id` of the actor who sent the message.
    
  .. cmember:: data
  
    A :ctype:`void*` to the message data.
    
  .. cmember:: size
  
    The size of the data.






Messaging
"""""""""

When sending a message, the *type* should be greater than 100, (anything below that may be used by the library).
  
.. cfunction:: void actor_send_msg(actor_id aid, long type, void *data, size_t size)

  Sends a message to an actor. *type* is a user defined value. *data* is a pointer to a block of data that will be sent to the actor. 
  
  **Note**: The data is copied before being sent to the actor. If you are passing a structure, make sure that it doesn't contain any pointers to memory, as this can cause a crash. *data* should be a complete message, see :ref:`memory-management`.
  
  
.. cfunction:: void actor_broadcast_msg(long type, void *data, size_t size)

  Broadcasts a message to all actors.
  
.. cfunction:: void actor_reply_msg(actor_msg_t *a, long type, void *data, size_t size)

  Reply to a received message.
  
.. cfunction::  actor_msg_t *actor_receive()

  Receives a message from the actor's mailbox.

.. cfunction:: actor_msg_t *actor_receive_timeout(long timeout)

  Same as :cfunc:`actor_receive`, but let's you specify a timeout (in milliseconds).
  
.. _memory-management:


Memory Management
"""""""""""""""""

*libactor* uses pthreads for concurrency. If you allocate memory with :cfunc:`malloc` and pass a pointer or try to access the memory in a different actor, your application may crash. Therefore, if you plan to send a message to another actor, make sure that the message is complete(no pointers, only raw data). See :ref:`memory-example`.

*libactor* provides some convenience functions for managing memory. Please use these in your actors. Reference counting is used to manage memory. When an actor exits, any unfreed memory will be automatically freed. (But you should still release anything you are not using).

.. cfunction:: void *amalloc(size_t size)

  Allocates a block of memory for an actor.
  
.. cfunction::  void arelease(void *block)
  
  Call this function to release the memory. The reference count is decremented. When it reaches 0, the actual memory is freed.

.. cfunction:: void aretain(void *block)

  Retains a block of memory. Use this to hold on to a block of memory. The reference count is incremented.

.. _memory-example:

Example
"""""""

This is okay::

  struct user {
    char *username;
    char *password;
    int status;
  };
  
  struct user usermsg;
  
  // initialize usermsg here
  
  actor_send_msg(5, 1, &usermsg, sizeof(struct user));

This is bad::

  struct usr_login_info {
    char *username;
    char *password;
  };

  struct user {
    struct usr_login_info *info;
    int status;
  };
  
  struct user usermsg;
  
  // initialize usermsg here
  
  actor_send_msg(5, 1, &usermsg, sizeof(struct user));

In the bad example, the user struct will be copied, but the pointer to *info* may then be accessed by multiple actors.



Ping/Pong Actor Example
"""""""""""""""""""""""

Below is a simple example of how to use the actor library. One actor will be spawned which will then spawn another actor, send it a ping message, and loop.

main.c::

  #include <stdio.h>
  #include <libactor/actor.h>
  
  ACTOR_FUNCTION(pong_func, args) {
    actor_msg_t *msg;

    while(1) {
      msg = actor_receive();
      if(msg->type == PING_MSG) {
        printf("PING! ");
        actor_reply_msg(msg, PONG_MSG, NULL, 0);
      }
      arelease(msg);
    }
  }

  ACTOR_FUNCTION(ping_func, args) {
    actor_msg_t *msg;
    actor_id aid = spawn_actor(pong_func, NULL);
    while(1) {
      actor_send_msg(aid, PING_MSG, NULL, 0);
      msg = actor_receive();
      if(msg->type == PONG_MSG) printf("PONG!\n");
      arelease(msg);
      sleep(5);
    }
  }
  

  ACTOR_FUNCTION(main_func, args) {
    struct actor_main *main = (struct actor_main*)args;
    int x;

    /* Accessing the arguments passed to the application */
    printf("Number of arguments: %d\n", main->argc);
    for(x = 0; x < main->argc; x++) printf("Argument: %s\n", main->argv[x]);

    /* PING/PONG example */
    spawn_actor(ping_func, NULL);
  }


.. _Actor model: http://en.wikipedia.org/wiki/Actor_model
.. _pthreads:    http://en.wikipedia.org/wiki/POSIX_Threads
.. _Chris Moos:  http://www.chrismoos.com/
