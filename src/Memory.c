/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
	Memory.c	WJ100
*/

#include "config.h"
#include "Memory.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MemFreeList *free_list = NULL;

unsigned long memory_total = 0UL;
unsigned long mem_stats[NUM_TYPES+1];


int init_Memory(void) {
	memory_total = 0UL;
	memset(mem_stats, 0, sizeof(unsigned long) * (NUM_TYPES+1));

	if ((free_list = (MemFreeList *)malloc(NUM_TYPES * sizeof(MemFreeList))) == NULL)
		return -1;
		
	memset(free_list, 0, NUM_TYPES * sizeof(MemFreeList));
	return 0;
}


/*
	get something from the freelist
*/
static char *get_freelist(int memtype) {
char *mem;
int n;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].free[n] != NULL) {
			mem = free_list[memtype].free[n];
			free_list[memtype].free[n] = NULL;
			return mem;
		}
	}
	return NULL;
}

/*
	put something on the freelist
*/
static int put_freelist(char *mem, int memtype) {
int n;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].free[n] == NULL) {
			memset(mem, 0, Types_table[memtype].size + 2*sizeof(unsigned long));	/* zero it out */
			free_list[memtype].free[n] = mem;
			return 0;
		}
	}
	return -1;
}

/*
	Malloc() puts the size and type before the pointer it returns
	as a sanity check, it also inserts the character 'A' before
	the memory type, so Free() can see if the pointer is valid at all
	(if we haven't already crashed by then, that is)

	Malloc() first checks the free_list for quick allocation
*/
void *Malloc(unsigned long size, int memtype) {
char *mem;

	if (size <= 0)
		return NULL;

	if (memtype < 0 || memtype >= (NUM_TYPES+1))
		return NULL;

/*
	asked for 1 item; search it in the free_list
*/
	if (size == Types_table[memtype].size) {
		if ((mem = get_freelist(memtype)) != NULL) {
			((unsigned long *)mem)[0] = 1UL;
			((unsigned long *)mem)[1] = ('A' << 16) | memtype;
			return (void *)((char *)mem + 2*sizeof(unsigned long));
		}
	}
	if ((mem = (char *)malloc(size + 2*sizeof(unsigned long))) == NULL)
		return NULL;

	memset(mem, 0, size + 2*sizeof(unsigned long));	/* malloc() sets it to 0, yeah right! :P */
	memory_total += (size + 2*sizeof(unsigned long));

	if (memtype < NUM_TYPES) {
		size /= Types_table[memtype].size;
		mem_stats[memtype] += size;
	} else
		mem_stats[NUM_TYPES]++;		/* unknown type */

	((unsigned long *)mem)[0] = size;
	((unsigned long *)mem)[1] = ('A' << 16) | memtype;

	return (void *)((char *)mem + 2*sizeof(unsigned long));
}

void Free(void *ptr) {
unsigned long *mem, size;

	if (ptr == NULL)
		return;

	mem = (unsigned long *)((char *)ptr - 2*sizeof(unsigned long));

	if ((mem[1] >> 16) != 'A')		/* crude sanity check */
		return;

	mem[1] &= 0xff;
	if (mem[1] >= 0 && mem[1] < NUM_TYPES) {
		if (mem[0] == 1UL && !put_freelist((char *)mem, mem[1]))
			return;

		size = mem[0] * Types_table[mem[1]].size;
		mem_stats[mem[1]] -= mem[0];
	} else {
		size = mem[0];
		mem_stats[NUM_TYPES]--;		/* unknown type */
	}
	mem[1] = 0UL;

	memory_total -= (size + 2*sizeof(unsigned long));
	free(mem);
}

/* EOB */
