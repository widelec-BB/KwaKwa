/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __EMOTICONSTAB_H__
#define __EMOTICONSTAB_H__

#include <exec/types.h>

#define DEFAULT_QUESTION_EMOTICON_PATH EMOT_DIR "pytajnik.gif"
#define DEFAULT_EXCLAMATION_EMOTICON_PATH EMOT_DIR "wykrzyknik.gif"

struct EmoticonTabEntry {CONST STRPTR emo; CONST STRPTR def_file;};

GLOBAL CONST struct EmoticonTabEntry EmoticonsTab[];
GLOBAL CONST UBYTE EmoticonsTabSize;

#endif /* __EMOTICONSTAB_H__ */
