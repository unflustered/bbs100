/*
    bbs100 2.2 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

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
	OnlineUser.c	WJ103

	- rewritten to work with a Hash type
*/

#include "config.h"
#include "OnlineUser.h"
#include "Hash.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

static Hash *online_users = NULL;

int init_OnlineUser(void) {
	if ((online_users = new_Hash()) == NULL)
		return -1;

	online_users->hashaddr = hashaddr_ascii;
	return 0;
}

void deinit_OnlineUser(void) {
	destroy_Hash(online_users);
	online_users = NULL;
}


int add_OnlineUser(User *u) {
	if (u == NULL || !u->name[0] || u->socket <= 0)
		return -1;

	return add_Hash(online_users, u->name, u);
}

void remove_OnlineUser(User *u) {
	if (u == NULL || !u->name[0] || u->socket <= 0)
		return;

	remove_Hash(online_users, u->name);
}

User *is_online(char *name) {
User *u;

	if (name == NULL || !*name)
		return NULL;

	if ((u = (User *)in_Hash(online_users, name)) != NULL && u->socket > 0)
		return u;

	return NULL;
}

/* EOB */
