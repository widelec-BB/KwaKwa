/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __KWAKWA_CONTACT_H__
#define __KWAKWA_CONTACT_H__

#include "defs.h"
#include "pictures.h"

struct ContactEntry
{
	STRPTR entryid;
	ULONG pluginid;
	STRPTR name;
	STRPTR nickname;
	STRPTR firstname;
	STRPTR lastname;
	STRPTR groupname;
	STRPTR birthyear;
	STRPTR city;
	ULONG status;
	STRPTR statusdesc;
	SHORT gender;
	BOOL unread;
	struct Picture *avatar;
};

#define ContactName(x) ((x)->name ? (x)->name : (x)->nickname ? (x)->nickname : (x)->firstname ? (x)->firstname : (x)->lastname ? (x)->lastname : (x)->entryid)
#define ContactNameLoc(x) ((x).name ? (x).name : (x).nickname ? (x).nickname : (x).firstname ? (x).firstname : (x).lastname ? (x).lastname : (x).entryid)

#endif /* __KWAKWA_CONTACT_H__ */
