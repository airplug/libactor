/*
libactor - A C Actor Library
list.c
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

#include "list.h"

void list_init(list_item_t **start) {
	*start = NULL;
}

void list_append(list_item_t **start, void *x) {
	list_item_t *lst = (list_item_t*)x, *temp = NULL;
	lst->next = NULL;
	if(*start == NULL) *start = lst;
	else {
		for(temp = *start; temp->next != NULL; temp = temp->next);
		temp->next = lst;
	}
}

size_t list_count(list_item_t **start) {
	list_item_t *temp;
	size_t x = 0;
	if(*start == NULL) return 0;
	for(temp = *start; temp != NULL; temp = temp->next) {
		x++;
	}
	return x;
}

void *list_pop(list_item_t **start) {
	list_item_t *temp;
	if(*start == NULL) return NULL;
	temp = *start;
	*start = temp->next;
	return temp;
}

void *list_filter(list_item_t **start, ANON_LIST_FILTER_FUNC(func), void *arg) {
	list_item_t *temp;
	if(*start == NULL) return NULL;
	for(temp = *start; temp != NULL; temp = temp->next) {
		if(func(temp, arg) == 0) return temp;
	}
	return NULL;
}

void list_remove(list_item_t **start, void *x) {
	list_item_t *a, *b;
	int removed = 0;
	if(*start == NULL) return;
	if(*start == x) {
		*start = ((list_item_t*)x)->next;
		return;
	}
	for(a = *start, b = *start; a != NULL; a = a->next) {
		if(a == x) {
			b->next = a->next;
			removed = 1;
			break;
		}
		b = a;
	}
}
