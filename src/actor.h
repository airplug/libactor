/*
  Copyright (C) 2009 Chris Moos


  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SRC_ACTOR_H_
#define SRC_ACTOR_H_


/*------------------------------------------------------------------------------
                                    includes
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "./list.h"


/*------------------------------------------------------------------------------
                               preprocessor definitions
------------------------------------------------------------------------------*/

#ifdef ACTOR_THREAD_PRINT_DEBUG
#define ACTOR_THREAD_PRINT(msg) printf("[actor thread]: %s\n", msg);
#else
#define ACTOR_THREAD_PRINT(msg)
#endif

#define ACCESS_ACTORS_BEGIN pthread_mutex_lock(&actors_mutex)
#define ACCESS_ACTORS_END pthread_mutex_unlock(&actors_mutex)

#define ACTOR_INVALID -1


/*------------------------------------------------------------------------------
                                    types
------------------------------------------------------------------------------*/

struct alloc_info_struct {
  struct alloc_info_struct *next;
  void *block;
  unsigned int refcount;
};
typedef struct alloc_info_struct alloc_info_t;

/*
**
** Actor Function
**
*/

typedef void * (*actor_function_ptr_t)(void *);


/*
**
** Actor Types
**
*/

struct actor_state_struct;
typedef struct actor_state_struct actor_state_t;

/**
 * An integer that refers to a unique actor’s ID.
 */
typedef long actor_id;

struct actor_message_struct;
typedef struct actor_message_struct actor_msg_t;

/**
 * This structure contains information about a message.
 */
struct actor_message_struct {
  actor_msg_t *next;

  /**
   * The actor_id of the actor who sent the message.
   */
  actor_id sender;

  /**
   * The actor_id of the actor who should receive the message.
   */
  actor_id dest;

  /**
   * A `void *` to the message data.
   */
  void *data;

  /**
   * A integer that should identify the structure of the data.
   */
  long type;

  /**
   * The size of the data.
   */
  size_t size;
};

struct actor_alloc {
  struct actor_alloc *next;
  void *block;
};

struct actor_state_struct {
  actor_state_t *next;
  actor_id myid;
  actor_msg_t *messages;
  pthread_t thread;
  pthread_cond_t msg_cond;
  pthread_mutex_t msg_mutex;
  list_item_t *allocs;
};

enum {
  ACTOR_MSG_EXITED = 1
};


/*------------------------------------------------------------------------------
                                public functions
------------------------------------------------------------------------------*/

/**
 * Initialize global state
 */
void actor_init();


/**
 * Spawn a new actor.
 * 
 * @param func  the function that the thread should run
 * @param args  passed to the actor when it is spawned
 * @return      the `actor_id`
 */
actor_id spawn_actor(actor_function_ptr_t func, void *args);


/**
 * Destroy all actors
 */
void actor_destroy_all();


/**
 * Wait till all actors have exited
 */
void actor_wait_finish();


/**
 * Send a message to an actor.
 * The data is copied before being sent to the actor.
 * 
 * @param aid   the Actor to which the message is sent
 * @param type  a user defined value
 * @param data  a pointer to a block of data that will be sent to the Actor
 * @param size  the size of the data pointed at by `data`
 */
void actor_send_msg(actor_id aid, long type, void * data, size_t size);


/**
 * Broadcast a message to all actors.
 */
void actor_broadcast_msg(long type, void * data, size_t size);


/**
 * Reply to a received message.
 */
void actor_reply_msg(actor_msg_t *a, long type, void * data, size_t size);


/**
 * Receive a message from the actor’s mailbox.
 */
actor_msg_t * actor_receive();

/**
 * Same as actor_receive(), but allows a timeout (in milliseconds).
 */
actor_msg_t * actor_receive_timeout(long timeout);


/**
 * Gets the actor_id of the executing Actor.
 *
 * @return  the actor_id of the executing Actor.
 */
actor_id actor_self();

/* Memory management */
void *amalloc(size_t size);
void arelease(void *block);


#endif  // SRC_ACTOR_H_
