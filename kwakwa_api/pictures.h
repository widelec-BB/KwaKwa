/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* KwaKwaAPI/pictures.h
 *
 *  NAME
 *    pictures.h
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    This file contains definitions of structures used to manipulate various pictures (for example avatars).
 *
 ********/

#ifndef __KWAKWA_PICTURES_H__
#define __KWAKWA_PICTURES_H__

/****d* pictures.h/Picture
 *
 *  NAME
 *    Picture
 *
 *  FUNCTION
 *    This structure is used to store pictures data in memory.
 *
 *  SOURCE
 */

struct Picture
{
	ULONG p_Width;
	ULONG p_Height;
	BYTE *p_Data;
};

/******AvatarData******/


#endif /* __KWAKWA_PICTURES_H__ */
