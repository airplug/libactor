/*
libactor - A C Actor Library
actor.c
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

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#if defined(WIN32)
#	define PTHREAD_HANDLE(_t) _t.p
#else
#	include <sys/resource.h>
#	define PTHREAD_HANDLE(_t) _t
#endif // defined(WIN32)

#include "actor.h"
#include "list.h"

static pthread_mutex_t actors_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t actors_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t actors_alloc = PTHREAD_MUTEX_INITIALIZER;
static actors_ready = 0; 
static list_item_t *actor_list_real;
static list_item_t **actor_list = &actor_list_real;

static list_item_t *alloc_list_real;
static list_item_t **alloc_list = &alloc_list_real;

/* Private structs */
struct actor_spawn_info {
	actor_state_t *state;
	ACTOR_FUNCTION_PTR(fun);
	void *args;
};


/* Only use these functions if you know what you are doing (pthreads + concurrent memory access = death) */
actor_msg_t *_actor_create_msg(long type, void *data, size_t size, actor_id sender, actor_id dest, pthread_t thread);
void *_amalloc_thread(size_t size, pthread_t thread);
void _aretain_thread(void *block, pthread_t thread);
void _arelease(void *block, pthread_t thread);
void _actor_send_msg(actor_id aid, long type, void *data, size_t size);
void _actor_release_memory(actor_state_t *state);
void _actor_destroy_state(actor_state_t *state);
void _actor_init_state(actor_state_t **state);
actor_id _actor_find_by_thread();

/* Private */
void actor_init_state(actor_state_t **state);
void actor_destroy_state(actor_state_t *state);
void actor_release_memory(actor_state_t *state);
void aretain_thread(void *block, pthread_t thread);


/*
**
** Library Initilization and Management
**
*/

void actor_init() {
	assert(actor_list != NULL);
	if(actor_list == NULL) {
		pthread_mutex_lock(&actors_mutex);
		list_init(actor_list);
		list_init(alloc_list);
		actors_ready = 1;
		pthread_mutex_unlock(&actors_mutex);
		
		
	}
}

void actor_wait_finish() {
	int cont = 1;
	struct timespec ts;
	struct timeval tp;
	
	while(cont == 1) {
		pthread_mutex_lock(&actors_mutex);
		if(list_count(actor_list) == 0) goto end;
		else {
			gettimeofday(&tp, NULL);

			ts.tv_sec  = tp.tv_sec;
			ts.tv_nsec = tp.tv_usec * 1000;
			ts.tv_sec += 10;
			pthread_cond_timedwait(&actors_cond, &actors_mutex, &ts);
		}
		pthread_mutex_unlock(&actors_mutex);
	}
end:
	actors_ready = 0;
	pthread_mutex_unlock(&actors_mutex);
}

void actor_destroy_all() {
	void *temp;
	alloc_info_t *info;

	pthread_mutex_lock(&actors_mutex);
	
	/* Clean up actor list */
	while((temp = list_pop(actor_list)) != NULL) {
		free(temp);
	}
	
	pthread_mutex_unlock(&actors_mutex);
	pthread_mutex_destroy(&actors_mutex);
	pthread_mutex_destroy(&actors_alloc);
	pthread_cond_destroy(&actors_cond);
	
	/* Clean up memory */
	for(info = list_pop(alloc_list); info != NULL; info = list_pop(alloc_list)) {
		#ifdef DEBUG_MEMORY
		printf("Unfreed block found.\n");
		#endif
		free(info->block);
		free(info);
	}
}


/*
** 
**	Create/spawn actor
**
*/

void *spawn_actor_fun(void *arg) {
	struct actor_spawn_info *si = (struct actor_spawn_info*)arg;
	
	ACCESS_ACTORS_BEGIN
	si->state->thread = pthread_self();
	ACCESS_ACTORS_END
	
	(si->fun)(si->args);

	ACCESS_ACTORS_BEGIN
	
	if(si->state->trap_exit_to != 0) _actor_send_msg(si->state->trap_exit_to, ACTOR_MSG_EXITED, NULL, 0);
	
	_actor_release_memory(si->state);
	_actor_destroy_state(si->state);
	free(si);

	pthread_cond_signal(&actors_cond);
	ACCESS_ACTORS_END
	
	
	pthread_detach(pthread_self());
	
	pthread_exit((void*)NULL);
}

actor_id spawn_actor(ACTOR_FUNCTION_PTR(func), void *args) {
	actor_state_t *state;
	actor_id aid;
	struct actor_spawn_info *si;
	
	assert(func != NULL);
	
	ACCESS_ACTORS_BEGIN
	
	_actor_init_state(&state);
	
	assert(state != NULL);
	
	aid = state->myid;
	si = (struct actor_spawn_info*)malloc(sizeof(struct actor_spawn_info));
	assert(si != NULL);
	si->state = state;
	si->fun = func;
	si->args = args;
	

	pthread_create(&state->thread, NULL, spawn_actor_fun, si);
	
	ACCESS_ACTORS_END
	
	return aid;
}

/*
**
** Helper functions
**
*/

LIST_FILTER_FUNC(find_thread, item, arg) {
	int ret = -1;
	actor_state_t *st = (actor_state_t*)item;
	if(PTHREAD_HANDLE(st->thread) == arg) ret = 0;
	return ret;
}

LIST_FILTER_FUNC(find_by_id, item, arg) {
	int ret = -1;
	actor_state_t *st = ((actor_state_t*)item);
	if(st->myid == (actor_id)arg) ret = 0;
	return ret;
}

actor_id get_unique_actor_id() {
	actor_id aid = -1;
	actor_state_t *st;

find:
	aid = rand();
	for(st = (actor_state_t*)*actor_list; st != NULL; st = st->next) {
		if(st->myid == aid) goto find;
	}
	return aid;
}



actor_id _actor_find_by_thread() {
	actor_state_t *st = NULL;
	actor_id aid = -1;
	pthread_t thread = pthread_self();
	
	if((st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(thread))) != NULL) aid = st->myid;

	
	return aid;
}

actor_id actor_self() {
	actor_id aid = 0;
	ACCESS_ACTORS_BEGIN
	aid = _actor_find_by_thread();
	ACCESS_ACTORS_END
	return aid;
}

/*
**
** Actor State Management
**
*/

actor_id _actor_trapexit_to() {
	actor_state_t *st;
	if((st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(pthread_self()))) != NULL) {
		if(st->trap_exit == 1) {
			return st->myid;
		}
	}
	return 0;
}

void actor_trap_exit(int action) {
	actor_state_t *st;
	ACCESS_ACTORS_BEGIN
	if((st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(pthread_self()))) != NULL) {
		st->trap_exit = action == 0 ? 0 : 1;
	}
	ACCESS_ACTORS_END
}

void _actor_init_state(actor_state_t **state) {
	actor_state_t *t;
	assert(state != NULL);
	
	
	t = (actor_state_t*)malloc(sizeof(actor_state_t));
	t->trap_exit_to = _actor_trapexit_to();
	t->trap_exit = 0;
	t->myid = get_unique_actor_id();
	pthread_cond_init(&t->msg_cond, NULL);
	pthread_mutex_init(&t->msg_mutex, NULL);
	list_init((list_item_t**)&t->messages);
	list_init(&t->allocs);
	
	list_append(actor_list, t);

	
	*state = t;
}

void _actor_destroy_state(actor_state_t *state) {
	if(state == NULL) return;

	pthread_cond_destroy(&state->msg_cond);	
	pthread_mutex_destroy(&state->msg_mutex);
	list_remove(actor_list, state);
	free(state);

}

/*
**
** Actor Messaging
**
*/

actor_msg_t *_actor_create_msg(long type, void *data, size_t size, actor_id sender, actor_id dest, pthread_t thread) {
	void *newblock;
	actor_msg_t *msg = (actor_msg_t*)_amalloc_thread(sizeof(actor_msg_t), thread);
	newblock = _amalloc_thread(size, thread);


	memcpy(newblock, data, size);
	
	msg->type = type;
	msg->data = newblock;
	msg->size = size;
	msg->dest = dest;
	msg->sender = sender;
	
	return msg;
}

actor_msg_t *actor_receive() {
	return actor_receive_timeout(0);
}

actor_msg_t *actor_receive_timeout(long timeout) {
	actor_state_t *st = NULL;
	actor_msg_t *msg = NULL;
	pthread_t thread = pthread_self();
	struct timespec ts;
	struct timeval tp;

	memset(&ts, 0, sizeof(struct timespec));
	
	ACCESS_ACTORS_BEGIN
	ACTOR_THREAD_PRINT("actor_receive_msg()\n");
	
	st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(thread));
	
	

	if(st != NULL) {
		msg = list_pop((list_item_t**)&st->messages);
		
		pthread_mutex_lock(&st->msg_mutex);
		
		ACCESS_ACTORS_END
		
		if(msg == NULL) { /* no messages available, let's wait */
			
			if(timeout > 0) {
				gettimeofday(&tp, NULL);
				ts.tv_sec  = tp.tv_sec;
				ts.tv_nsec = (tp.tv_usec * 1000) + (timeout * 1000000);
				if(pthread_cond_timedwait(&st->msg_cond, &st->msg_mutex, &ts) == 0) {
					msg = list_pop((list_item_t**)&st->messages);
				}
			} else {
				pthread_cond_wait(&st->msg_cond, &st->msg_mutex);
				
			}
			msg = list_pop((list_item_t**)&st->messages);
		
		}
		pthread_mutex_unlock(&st->msg_mutex);
	} else {
		ACCESS_ACTORS_END
	}
	
	
	return msg;
}

void actor_reply_msg(actor_msg_t *a, long type, void *data, size_t size) {
	if(a == NULL) return;
	actor_send_msg(a->sender, type, data, size);
}

void actor_broadcast_msg(long type, void *data, size_t size) {
	actor_id *lst = NULL;
	actor_state_t *st;
	int count = 0;
	int x = 0;
	
	ACCESS_ACTORS_BEGIN
	count = list_count(actor_list);
	lst = _amalloc_thread(sizeof(actor_id) * count, pthread_self());
	for(st = (actor_state_t*)*actor_list; st != NULL; st = st->next) {
		lst[x] = st->myid;
		x++;
	}
	
	ACCESS_ACTORS_END
	
	for(x = 0; x < count; x++) {
		actor_send_msg(lst[x], type, data, size);	
	}
	
	arelease(lst);
}

void actor_send_msg(actor_id aid, long type, void *data, size_t size) {
	ACCESS_ACTORS_BEGIN
	_actor_send_msg(aid, type, data, size);
	ACCESS_ACTORS_END
}

void _actor_send_msg(actor_id aid, long type, void *data, size_t size) {
	actor_state_t *st = NULL;
	actor_msg_t *msg = NULL;
	actor_id myid = _actor_find_by_thread();
	pthread_t thread;
	PTHREAD_HANDLE(thread) = NULL;
	
	if(myid == -1) return;

	st = list_filter(actor_list, find_by_id, (void*)aid);

	if(st != NULL) {
		pthread_mutex_lock(&st->msg_mutex);
		msg = _actor_create_msg(type, data, size, myid, aid, st->thread);
		list_append((list_item_t**)&st->messages, msg);
		pthread_cond_signal(&st->msg_cond);
		pthread_mutex_unlock(&st->msg_mutex);
	}
}

/*
**
**	Memory Manager
**
*/

void *_amalloc_thread(size_t size, pthread_t thread) {
	alloc_info_t *info;
	void *block = NULL;
	actor_state_t *st = NULL;
	struct actor_alloc *al;
	
	
	if(size == 0) return NULL;
	pthread_mutex_lock(&actors_alloc);
	block = malloc(size);
	assert(block != NULL);
	info = (alloc_info_t*)malloc(sizeof(alloc_info_t));
	info->block = block;
	info->refcount = 1;
	
	st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(thread));
	if(st != NULL) {
		al = (struct actor_alloc*)malloc(sizeof(struct actor_alloc));
		assert(al != NULL);
		al->block = block;
		list_append(&st->allocs, al);
	}

	list_append(alloc_list, info);
	
	pthread_mutex_unlock(&actors_alloc);
	
	return block;
}

void *amalloc(size_t size) {
	pthread_t thread = pthread_self();
	void *block;
	ACCESS_ACTORS_BEGIN
	block = _amalloc_thread(size, thread);
	ACCESS_ACTORS_END
	return block;
}

LIST_FILTER_FUNC(find_memory, info, block) {
	if(((alloc_info_t*)info)->block == block) return 0;
	return -1;
}

LIST_FILTER_FUNC(find_actor_block, info, arg) {
	if(((struct actor_alloc*)info)->block == arg) return 0;
	else return -1;
}

void aretain(void *block) {
	pthread_t thread = pthread_self();
	ACCESS_ACTORS_BEGIN
	_aretain_thread(block, thread);
	ACCESS_ACTORS_END
}

void _aretain_thread(void *block, pthread_t thread) {
	alloc_info_t *info = NULL;
	actor_state_t *st = NULL;
	struct actor_alloc *al;
	
	if(block == NULL) return;
	
	if((info = list_filter(alloc_list, find_memory, block)) != NULL) {
		info->refcount++;
	}

	st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(thread));
	if(st != NULL) {
		al = (struct actor_alloc*)malloc(sizeof(struct actor_alloc));
		assert(al != NULL);
		al->block = block;
		list_append(&st->allocs, al);
	}	
}

void arelease(void *block) {
	pthread_t thread = pthread_self();
	ACCESS_ACTORS_BEGIN
	ACTOR_THREAD_PRINT("arelease()");
	_arelease(block, thread);
	ACCESS_ACTORS_END
}

void _arelease(void *block, pthread_t thread) {
	alloc_info_t *info = NULL;
	actor_state_t *st = NULL;
	struct actor_alloc *al;
	
	if(block == NULL) return;
	
	pthread_mutex_lock(&actors_alloc);
	
	if((info = list_filter(alloc_list, find_memory, block)) != NULL) {
		info->refcount--;
		if(info->refcount == 0) { /* time to destroy this block */
			free(info->block);
			list_remove(alloc_list, info);
			free(info);
		}
	}
	
	pthread_mutex_unlock(&actors_alloc);
	
	st = list_filter(actor_list, find_thread, (void*)PTHREAD_HANDLE(thread));
	if(st != NULL) {
		if((al = list_filter(&st->allocs, find_actor_block, block)) != NULL) {
			list_remove(&st->allocs, al);
			free(al);
		}
	}
}

LIST_FILTER_FUNC(find_memory_actor, owner, arg) {
	if(owner == arg) return 0;
	return -1;
}

void _actor_release_memory(actor_state_t *state) {
	struct actor_alloc *info, *tmp;
	#ifdef DEBUG_MEMORY
		int count = list_count(&state->allocs);
		if(count > 0) printf("_actor_release_memory(): automatically releasing %d allocations. (actor_id = %d)\n", count, (int)state->myid);
	#endif
	for(info = (struct actor_alloc*)state->allocs; info != NULL;) {
		tmp = info->next;
		_arelease(info->block, pthread_self());
		info = tmp;
	}
}
