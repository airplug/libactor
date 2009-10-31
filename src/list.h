/*
libactor - A C Actor Library
list.h
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

#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define ANON_LIST_FILTER_FUNC(name) int (*name)(void *, void *)
#define LIST_FILTER_FUNC(name,item,arg) int (name)(void *item, void *arg)

struct list_item_struct;
typedef struct list_item_struct list_item_t;

struct list_item_struct {
	list_item_t *next;
};


void list_init(list_item_t **start);

void list_append(list_item_t **start, void *x);
void list_remove(list_item_t **start, void *x);
void *list_pop(list_item_t **start);

size_t list_count(list_item_t **start);

void *list_filter(list_item_t **start, ANON_LIST_FILTER_FUNC(func), void *arg);

#endif