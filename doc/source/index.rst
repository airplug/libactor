libactor documentation
====================================

==============
Introduction
==============

*libactor* is a simple C library that helps you build an application based on the Actor model. It is currently not supported on Windows. Any Linux/BSD should be fine, as long as it supports *pthreads*.

The library works great, but one caveat is that if one actor crashes -- they all fall down. In a future release there will be sandboxing of each actor to provide better reliability.

========
API
========

Once you have installed libactor, you just have to link it to your application with the flag *-lactor*.

You must include ``<libactor/actor.h>`` in your project as well.

Data Types
-----------

.. ctype:: typedef long actor_id

	An integer that refers to a unique actor's ID.

.. ctype:: actor_msg_t

	This structure contains information about a message. The following fields are available:

	.. cmember:: sender
	
		The :ctype:`actor_id` of the actor who sent the message.
		
	.. cmember:: data
	
		A :ctype:`void*` to the message data.
		
	.. cmember:: size
	
		The size of the data.

Macros
---------

.. cfunction:: DECLARE_ACTOR_MAIN(fun)

	This macro should be used in your main file. *fun* should be a function declared with the :cmacro:`ACTOR_FUNCTION`. This actor will be executed when the application starts.
	
.. cfunction:: ACTOR_FUNCTION(name, args)

	This macro is used to declare an actor. *name* is the name of the function, and *args* is any arguments passed to the actor as a :ctype:`void*`


Actor Management 
-------------------

.. cfunction:: actor_id spawn_actor(ACTOR_FUNCTION_PTR(func), void *args)

	This functions spawns a new actor, the first argument should be the name of an actor created with :cmacro:`ACTOR_FUNCTION`. The second argument is passed to the actor when it is spawned. The *actor_id* is returned to the caller.

.. cfunction:: actor_id actor_self()

	Returns the current actor's id.
	
.. cfunction:: void actor_trap_exit(int action)

	By setting *action* to 1, trap exit is enabled. This means that when you spawn an actor, when it exits, you will receive a :ctype:`ACTOR_MSG_EXITED` message. This is good if you want to monitor any actors that you have spawned.

Messaging
-----------

When sending a message, the *type* should be greater than 100, (anything below that may be used by the library).
	
.. cfunction:: void actor_send_msg(actor_id aid, long type, void *data, size_t size)

	Sends a message to an actor. *type* is a user defined value. *data* is a pointer to a block of data that will be sent to the actor. 
	
	**Note**: The data is copied before being sent to the actor. If you are passing a structure, make sure that it doesn't contain any pointers to memory, as this can cause a crash. *data* should be a complete message, see :ref:`memory-management`.
	
	
.. cfunction:: void actor_broadcast_msg(long type, void *data, size_t size)

	Broadcasts a message to all actors.
	
.. cfunction:: void actor_reply_msg(actor_msg_t *a, long type, void *data, size_t size)

	Reply to a received message.
	
.. cfunction:: 	actor_msg_t *actor_receive()

	Receives a message from the actor's mailbox.

.. cfunction:: actor_msg_t *actor_receive_timeout(long timeout)

	Same as :cfunc:`actor_receive`, but let's you specify a timeout (in milliseconds).
	
.. _memory-management:

Memory Management
------------------------

*libactor* uses pthreads for concurrency. If you allocate memory with :cfunc:`malloc` and pass a pointer or try to access the memory in a different actor, your application may crash. Therefore, if you plan to send a message to another actor, make sure that the message is complete(no pointers, only raw data). See :ref:`memory-example`.

*libactor* provides some convenience functions for managing memory. Please use these in your actors. Reference counting is used to manage memory. When an actor exits, any unfreed memory will be automatically freed. (But you should still release anything you are not using).

.. cfunction:: void *amalloc(size_t size)

	Allocates a block of memory for an actor.
	
.. cfunction:: 	void arelease(void *block)
	
	Call this function to release the memory. The reference count is decremented. When it reaches 0, the actual memory is freed.

.. cfunction:: void aretain(void *block)

	Retains a block of memory. Use this to hold on to a block of memory. The reference count is incremented.

.. _memory-example:

Example
^^^^^^^^^^

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
============================

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
