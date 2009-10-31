/*
libactor - A C Actor Library
example.c
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

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

//#define DEBUG_MEMORY

//#define USE_DMALLOC
//#define ACTOR_THREAD_PRINT_DEBUG 

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

#include "list.h"


#ifdef ACTOR_THREAD_PRINT_DEBUG
#define ACTOR_THREAD_PRINT(msg) printf("[actor thread]: %s\n", msg);
#else
#define ACTOR_THREAD_PRINT(msg)
#endif

#define ACCESS_ACTORS_BEGIN pthread_mutex_lock(&actors_mutex);
#define ACCESS_ACTORS_END pthread_mutex_unlock(&actors_mutex);

#define DECLARE_ACTOR_MAIN(fun) \
int main(int argc, char **argv) { \
	struct actor_main amain; \
	amain.argc = argc; \
	amain.argv = argv; \
	actor_init(); \
	spawn_actor(fun, (void*)&amain); \
	actor_wait_finish(); \
	actor_destroy_all(); \
	exit(0); \
}

#define ACTOR_INVALID -1

struct actor_main {
	int argc;
	char **argv;
};

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

#define ACTOR_FUNCTION(name, args) void *(name)(void *args)
#define ACTOR_FUNCTION_PTR(name) void *(*name)(void *)


/*
**
** Actor Types
**
*/

struct actor_state_struct;
typedef struct actor_state_struct actor_state_t;

typedef long actor_id;

struct actor_message_struct;
typedef struct actor_message_struct actor_msg_t;

struct actor_message_struct {
	actor_msg_t *next;
	actor_id sender, dest;
	long type;
	void *data;
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
	actor_id trap_exit_to;
	char trap_exit;
};

/*
**
** Mailbox Types
**
*/


struct actor_mailbox_struct {
	
};
typedef struct actor_mailbox_struct actor_mailbox_t;

#endif