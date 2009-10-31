/*
libactor - A C Actor Library
actor.h
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

#ifndef ACTOR_H
#define ACTOR_H

#include "types.h"

enum {
	ACTOR_MSG_EXITED = 1
};


/* Initializes global state */
void actor_init();

/* Spawns an actor, returns the ID. */
actor_id spawn_actor(ACTOR_FUNCTION_PTR(func), void *args);

/* Destroy all actors */
void actor_destroy_all();

/* Waits till all actors have exited */
void actor_wait_finish();

/* Send message to an actor */
void actor_send_msg(actor_id aid, long type, void *data, size_t size);

/* Broadcast */
void actor_broadcast_msg(long type, void *data, size_t size);

/* Reply */
void actor_reply_msg(actor_msg_t *a, long type, void *data, size_t size);

/* Receive next message */
actor_msg_t *actor_receive();
actor_msg_t *actor_receive_timeout(long timeout);

/* Trap exit */
void actor_trap_exit(int action);

/* Get my id */
actor_id actor_self();

/* Memory management */
void *amalloc(size_t size);
void arelease(void *block);
void aretain(void *block);


#endif