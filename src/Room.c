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
	Room.c	WJ99
*/

#include <config.h>

#include "debug.h"
#include "Room.h"
#include "cstring.h"
#include "CachedFile.h"
#include "util.h"
#include "cstring.h"
#include "mydirentry.h"
#include "strtoul.h"
#include "Param.h"
#include "Timer.h"
#include "Memory.h"
#include "OnlineUser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

Room *AllRooms = NULL, *HomeRooms = NULL, *Lobby_room = NULL;


Room *new_Room(void) {
Room *r;

	if ((r = (Room *)Malloc(sizeof(Room), TYPE_ROOM)) == NULL)
		return NULL;

	return r;
}

void destroy_Room(Room *r) {
	if (r == NULL)
		return;

	listdestroy_MsgIndex(r->msgs);
	listdestroy_StringList(r->info);
	listdestroy_StringList(r->room_aides);
	listdestroy_StringList(r->kicked);
	listdestroy_StringList(r->invited);
	listdestroy_StringList(r->chat_history);
	listdestroy_PList(r->inside);

	Free(r);
}

/* load the RoomData file */
Room *load_Room(unsigned int number) {
char filename[MAX_PATHLEN];

	sprintf(filename, "%s/%u/RoomData", PARAM_ROOMDIR, number);
	return load_RoomData(filename, number);
}

Room *load_Mail(char *username) {
Room *r;
char filename[MAX_PATHLEN];

	if (username == NULL || !*username || !user_exists(username))
		return NULL;

	Enter(load_Mail);

	sprintf(filename, "%s/%c/%s/MailData", PARAM_USERDIR, *username, username);

	if ((r = load_RoomData(filename, 1)) == NULL) {
		if ((r = new_Room()) == NULL) {
			Return NULL;
		}
		r->generation = (unsigned long)rtc;
	}
	r->number = 1;
	sprintf(r->name, "%s Mail", name_with_s(username));

	if (in_StringList(r->invited, username) == NULL)
		add_StringList(&r->invited, new_StringList(username));

	r->flags = ROOM_SUBJECTS | ROOM_NOZAP | ROOM_INVITE_ONLY;

	sprintf(filename, "%s/%c/%s/", PARAM_USERDIR, *username, username);
	room_readroomdir(r, filename);

	Return r;
}

Room *load_Home(char *username) {
Room *r;
char filename[MAX_PATHLEN];

	if (username == NULL || !*username || !user_exists(username))
		return NULL;

	Enter(load_Home);

	sprintf(filename, "%s/%c/%s/HomeData", PARAM_USERDIR, *username, username);

	if ((r = load_RoomData(filename, 2)) == NULL) {
		if ((r = new_Room()) == NULL) {
			Return NULL;
		}
		r->generation = (unsigned long)rtc;
	}
	r->number = 2;
	sprintf(r->name, "%s Home", name_with_s(username));

	if (in_StringList(r->room_aides, username) == NULL)
		add_StringList(&r->room_aides, new_StringList(username));

	if (in_StringList(r->invited, username) == NULL)
		add_StringList(&r->invited, new_StringList(username));

	r->flags = ROOM_CHATROOM | ROOM_NOZAP | ROOM_INVITE_ONLY | ROOM_HOME;
	Return r;
}

Room *load_RoomData(char *filename, unsigned int number) {
Room *r;
char buf[MAX_LINE];
File *f;
StringList *sl;

	if (filename == NULL || !*filename || (r = new_Room()) == NULL)
		return NULL;

	if ((f = Fopen(filename)) == NULL) {
		destroy_Room(r);
		return NULL;
	}
	r->number = number;

/* name */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_room;

	cstrip_line(buf);
	strncpy(r->name, buf, MAX_LINE);
	r->name[MAX_LINE-1] = 0;

/* generation/creation date */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_room;
	cstrip_line(buf);
	r->generation = strtoul(buf, NULL, 10);

/* flags */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_room;
	cstrip_line(buf);
	r->flags = (unsigned int)strtoul(buf, NULL, 16);
	r->flags &= ROOM_ALL;		/* reset non-existant flags */

/* roominfo_changed */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_room;
	cstrip_line(buf);
	r->roominfo_changed = (unsigned int)strtoul(buf, NULL, 10);

/* info */
	listdestroy_StringList(r->info);
	r->info = Fgetlist(f);

/* room aides */
	listdestroy_StringList(r->room_aides);
	r->room_aides = NULL;

	while(Fgets(f, buf, MAX_LINE) != NULL) {
		cstrip_line(buf);
		if (!*buf)
			break;

		if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
			r->room_aides = add_StringList(&r->room_aides, sl);
	}
	r->room_aides = rewind_StringList(r->room_aides);

/* invited */
	listdestroy_StringList(r->invited);
	r->invited = NULL;

	while(Fgets(f, buf, MAX_LINE) != NULL) {
		cstrip_line(buf);
		if (!*buf)
			break;

		if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
			r->invited = add_StringList(&r->invited, sl);
	}
	r->invited = rewind_StringList(r->invited);

/* kicked */
	listdestroy_StringList(r->kicked);
	r->kicked = NULL;

	while(Fgets(f, buf, MAX_LINE) != NULL) {
		cstrip_line(buf);
		if (!*buf)
			break;

		if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
			r->kicked = add_StringList(&r->kicked, sl);
	}
	r->kicked = rewind_StringList(r->kicked);

	if (r->flags & ROOM_CHATROOM)
		r->chat_history = Fgetlist(f);

	Fclose(f);
	return r;

err_load_room:
	destroy_Room(r);
	Fclose(f);
	return NULL;
}

/* save the RoomData file */
int save_Room(Room *r) {
char filename[MAX_LINE];
File *f;

	if (r == NULL)
		return -1;

	if (r->number == 1 || r->number == 2) {
		char name[MAX_LINE], *p;

		strcpy(name, r->name);
		if ((p = cstrchr(name, '\'')) == NULL)
			return -1;

		*p = 0;

		if (r->number == 1)
			sprintf(filename, "%s/%c/%s/MailData", PARAM_USERDIR, *name, name);
		else
			if (r->number == 2)
				sprintf(filename, "%s/%c/%s/HomeData", PARAM_USERDIR, *name, name);
	} else
		sprintf(filename, "%s/%u/RoomData", PARAM_ROOMDIR, r->number);

	if ((f = Fcreate(filename)) == NULL)
		return -1;

	Fputs(f, r->name);
	Fprintf(f, "%lu", r->generation);
	Fprintf(f, "%x", r->flags);
	Fprintf(f, "%u", r->roominfo_changed);

	Fputlist(f, r->info);
	Fputlist(f, r->room_aides);
	Fputlist(f, r->invited);
	Fputlist(f, r->kicked);

	if (r->flags & ROOM_CHATROOM)
		Fputlist(f, r->chat_history);

	Fclose(f);
	return 0;
}


/*
	this assumes msgs are sorted by number
	returns first new message in the room (usr->curr_msg should be set to this)
*/
MsgIndex *newMsgs(Room *r, unsigned long num) {
MsgIndex *m;

	if (r == NULL)
		return NULL;

	m = unwind_MsgIndex(r->msgs);
	if (m != NULL && m->number > num) {
		while(m->prev != NULL && m->prev->number > num)
			m = m->prev;
		return m;
	}
	return NULL;
}

void newMsg(Room *r, Message *m) {
	if (r == NULL || m == NULL)
		return;

	add_MsgIndex(&r->msgs, new_MsgIndex(m->number));
}


void room_readdir(Room *r) {
char dirname[MAX_PATHLEN];

	if (r == NULL || (r->flags & ROOM_CHATROOM))
		return;

	sprintf(dirname, "%s/%u/", PARAM_ROOMDIR, r->number);
	room_readroomdir(r, dirname);
}

void room_readmaildir(Room *r, char *username) {
char buf[MAX_PATHLEN];

	if (r == NULL || username == NULL || !*username)
		return;

	sprintf(buf, "%s/%c/%s/", PARAM_USERDIR, *username, username);
	room_readroomdir(r, buf);
}

void room_readroomdir(Room *r, char *dirname) {
DIR *dirp;
struct dirent *direntp;
char *bufp;
unsigned long num;

	if (r == NULL || dirname == NULL)
		return;

	listdestroy_MsgIndex(r->msgs);
	r->msgs = NULL;
	bufp = dirname+strlen(dirname);

	if ((dirp = opendir(dirname)) == NULL)
		return;

	while((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_name[0] >= '0' && direntp->d_name[0] <= '9') {
			strcpy(bufp, direntp->d_name);
			num = strtoul(bufp, NULL, 10);
			r->msgs = add_MsgIndex(&r->msgs, new_MsgIndex(num));
		}
	}
	closedir(dirp);

	r->msgs = rewind_MsgIndex(r->msgs);
	r->msgs = sort_MsgIndex(r->msgs, msgs_sort_func);
}

unsigned long room_top(Room *r) {
MsgIndex *m;

	if (r == NULL)
		return 0UL;

	if ((m = unwind_MsgIndex(r->msgs)) == NULL)
		return 0UL;
	return m->number;
}

Room *find_Room(User *usr, char *name) {
Room *r;

	if (name == NULL || !*name || usr == NULL)
		return NULL;

	if (*name >= '0' && *name <= '9')
		return find_Roombynumber(usr, (unsigned int)atoi(name));
	else {
		char *p, *quote;

		if (!strcmp(name, "Mail"))
			return usr->mail;

		if (!strcmp(name, "Home"))
			return find_Home(usr->name);

		if ((p = cstrchr(name, '\'')) != NULL) {
			quote = p;
			*quote = 0;
			p++;
			if (*p == 's')
				p++;
			if (*p == ' ') {
				p++;
				if (!strcmp(p, "Mail")) {
					User *u;

					if ((u = is_online(name)) == NULL) {
						r = load_Mail(name);
						*quote = '\'';
						add_Room(&HomeRooms, r);
						return r;
					}
					*quote = '\'';
					return u->mail;
				}
				if (!strcmp(p, "Home")) {
					r = find_Home(name);
					*quote = '\'';
					return r;
				}
			}
			*quote = '\'';
		}

/* find 'normal' room */

		for(r = AllRooms; r != NULL; r = r->next) {
			if (!strcmp(r->name, name)) {
				if (r->number <= 2)
					return find_Roombynumber(usr, r->number);
				return r;
			}
		}
	}
	return NULL;
}

/*
	find_Room() with abbreviated name
*/
Room *find_abbrevRoom(User *usr, char *name) {
Room *r;

	if (name == NULL || !*name || usr == NULL)
		return NULL;

	if ((r = find_Room(usr, name)) == NULL) {
		int l;

		l = strlen(name);
		for(r = AllRooms; r != NULL; r = r->next) {
			if (!(r->flags & ROOM_HIDDEN) && !strncmp(r->name, name, l)) {
				if (r->number <= 2)
					return find_Roombynumber(usr, r->number);
				return r;
			}
		}
/*
	didn't find any room, try a substring
*/
		for(r = AllRooms; r != NULL; r = r->next) {
			if (!(r->flags & ROOM_HIDDEN) && cstrstr(r->name, name) != NULL) {
				if (r->number <= 2)
					return find_Roombynumber(usr, r->number);
				return r;
			}
		}
	}
	return r;
}

Room *find_Roombynumber(User *usr, unsigned int u) {
Room *r;

	switch(u) {
		case 0:
			return Lobby_room;

		case 1:
			return usr->mail;

		case 2:
			return find_Home(usr->name);

		default:
			for(r = AllRooms; r != NULL; r = r->next)
				if (r->number == u)
					return r;
	}
	return NULL;
}

Room *find_Roombynumber_username(User *usr, char *username, unsigned int u) {
Room *r;

	switch(u) {
		case 0:
			return Lobby_room;

		case 1:
			return usr->mail;

		case 2:
			return find_Home(username);

		default:
			for(r = AllRooms; r != NULL; r = r->next)
				if (r->number == u)
					return r;
	}
	return NULL;
}

Room *find_Home(char *username) {
Room *r;
char buf[MAX_LINE];

	if (username == NULL || !*username)
		return NULL;

	sprintf(buf, "%s Home", name_with_s(username));

	for(r = HomeRooms; r != NULL; r = r->next)
		if (!strcmp(r->name, buf))
			return r;

	if ((r = load_Home(username)) == NULL)
		return NULL;

	add_Room(&HomeRooms, r);
	return r;
}

/* purely check if it exists at all */

int room_exists(char *name) {
char *p;

	if ((p = cstrchr(name, '\'')) != NULL) {
		if (!strcmp(p, "'s Home") || !strcmp(p, "'s Mail")
			|| !strcmp(p, "' Home") || !strcmp(p, "' Mail")) {
			*p = 0;
			if (user_exists(name)) {
				*p = '\'';
				return 1;
			}
			*p = '\'';
			return 0;
		}
	}
	if (*name >= '0' && *name <= '9')
		return roomnumber_exists((unsigned int)atoi(name));
	else {
		Room *r;

		for(r = AllRooms; r != NULL; r = r->next)
			if (!strcmp(r->name, name))
				return 1;
	}
	return 0;
}

int roomnumber_exists(unsigned int u) {
Room *r;

	if (u <= 2)
		return 1;

	for(r = AllRooms; r != NULL; r = r->next)
		if (r->number == u)
			return 1;
	return 0;
}

/*
	unload an 'on demand loaded' room

	Sometimes, the find_Room() functions load a room on demand
	This is cool, but there are cases in which the loaded room
	needs to be destroyed again -- this is done by unload_Room()
*/
void unload_Room(Room *r) {
	if (r == NULL)
		return;

	if (r->number == 2 && r->inside == NULL) {		/* demand loaded Home room */
		remove_Room(&HomeRooms, r);
		save_Room(r);
		destroy_Room(r);
		return;
	}
	if (r->number == 1) {			/* demand loaded Mail> room */
		Room *h;
/*
	Note: mail rooms are usually stored in the user as usr->mail
	However, if the user was not online, it was put on the HomeRooms list
	so if we can find it there, it should be unloaded
	if we can't find it there, the room should not be unloaded

	This procedure is more efficient than scanning all users for "(usr->mail == r)"
	because the HomeRooms list is usually very short or empty
*/
		for(h = HomeRooms; h != NULL; h = h->next) {
			if (h == r) {
				remove_Room(&HomeRooms, r);
				save_Room(r);
				destroy_Room(r);
				return;
			}
		}
	}
}

int room_sort_func(void *v1, void *v2) {
Room *r1, *r2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	r1 = *(Room **)v1;
	r2 = *(Room **)v2;

	if (r1 == NULL || r2 == NULL)
		return 0;

	if (r1->number < r2->number)
		return -1;

	if (r1->number > r2->number)
		return 1;

	return 0;
}

int msgs_sort_func(void *v1, void *v2) {
MsgIndex *m1, *m2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	m1 = *(MsgIndex **)v1;
	m2 = *(MsgIndex **)v2;

	if (m1 == NULL || m2 == NULL)
		return 0;

	if (m1->number < m2->number)
		return -1;

	if (m1->number > m2->number)
		return 1;
	return 0;
}


int completely_numeric(char *str) {
	if (str == NULL)
		return 0;

	while(*str) {
		if (*str >= '0' && *str <= '9')
			str++;
		else
			return 0;
	}
	return 1;
}

/*
	load all rooms definitions at startup
	Note that room #1 and room #2 are 'shadow' rooms for Mail> and Home>
	the BBS cannot work without room #0, #1, and #2
*/
int init_Room(void) {
char buf[MAX_PATHLEN], *bufp;
DIR *dirp;
struct dirent *direntp;
struct stat statbuf;
Room *newroom;
unsigned int u;

	printf("\n");

	listdestroy_Room(AllRooms);
	AllRooms = Lobby_room = NULL;

	sprintf(buf, "%s/", PARAM_ROOMDIR);
	bufp = buf+strlen(buf);

	if ((dirp = opendir(buf)) == NULL)
		return -1;

	while((direntp = readdir(dirp)) != NULL) {
		if (completely_numeric(direntp->d_name)) {		/* only do numeric directories */
			strcpy(bufp, direntp->d_name);
			if (stat(buf, &statbuf))
				continue;

			if ((statbuf.st_mode & S_IFDIR) == S_IFDIR) {
				u = (unsigned int)strtoul(bufp, NULL, 10);

				printf("loading room %3u ... ", u);
				fflush(stdout);

				if ((newroom = load_Room(u)) != NULL) {
					add_Room(&AllRooms, newroom);
					room_readdir(newroom);
					printf("%s>\n", newroom->name);
				} else {
					printf("FAILED!\n");
					closedir(dirp);

					listdestroy_Room(AllRooms);
					AllRooms = Lobby_room = NULL;
					return -1;
				}
			}
		}
	}
	closedir(dirp);

	AllRooms = sort_Room(AllRooms, room_sort_func);

/*
	find the Lobby>
	It should be first, but you never know...
*/
	if (Lobby_room == NULL) {
		Room *rm;

		for(rm = AllRooms; rm != NULL; rm = rm->next) {
			if (!rm->number) {
				Lobby_room = rm;
				break;
			}
		}
	}
	if (Lobby_room == NULL) {
		printf("Failed to find the Lobby> (room 0)\n"
			"Please create room definition file '%s/0/RoomData'\n", PARAM_ROOMDIR);

		listdestroy_Room(AllRooms);
		AllRooms = NULL;
		return -1;
	}
	printf("\n");
	return 0;
}

/* EOB */
