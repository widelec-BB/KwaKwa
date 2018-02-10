/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "globaldefines.h"
#include "emoticonstab.h"

/* default files names as in Gadu-Gadu 7.x */
/* add new ones only on end of array! */

CONST struct EmoticonTabEntry EmoticonsTab[] =
{
	{":)", EMOT_DIR "usmiech.gif"},
	{";)", EMOT_DIR "oczko.gif"},

	{":(", EMOT_DIR "smutny2.gif"},
	{";(", EMOT_DIR "placze.gif"},

	{":D", EMOT_DIR "zeby.gif"},
	{";D", EMOT_DIR "zeby.gif"},

	{":P", EMOT_DIR "jezyk.gif"},
	{";P", EMOT_DIR "jezyk_oko.gif"},

	{":>", EMOT_DIR "ostr.gif"},
	{";>", EMOT_DIR "chytry.gif"},

	{":]", EMOT_DIR "kwadr.gif"},
	{";]", EMOT_DIR "krzywy.gif"},

	{":|", EMOT_DIR "yyyy.gif"},
	{";|", EMOT_DIR "ysz.gif"},

	{":[", EMOT_DIR "zly.gif"},
	{";[", EMOT_DIR "zly.gif"},

	{":/", EMOT_DIR "kwasny.gif"},
	{";/", EMOT_DIR "kwasny.gif"},

	{":*", EMOT_DIR "cmok.gif"},
	{";*", EMOT_DIR "cmok.gif"},

	{":O", EMOT_DIR "wow.gif"},
	{";O", EMOT_DIR "wow.gif"},

	{"^^", EMOT_DIR "manga_usmiech.gif"},

	{":-)", EMOT_DIR "usmiech2.gif"},
	{";-)", EMOT_DIR "oczko2.gif"},

	{":))", EMOT_DIR "wesoly.gif"},
	{";))", EMOT_DIR "mruga.gif"},

	{":((", EMOT_DIR "smutny3.gif"},
	{";((", EMOT_DIR "placze.gif"},

	{":-(", EMOT_DIR "smutny3.gif"},
	{";-(", EMOT_DIR "placze.gif"},

	{":-D", EMOT_DIR "zeby.gif"},
	{";-D", EMOT_DIR "zeby.gif"},

	{":DD", EMOT_DIR "zeby.gif"},
	{";DD", EMOT_DIR "zeby.gif"},

	{":-P", EMOT_DIR "jezyk1.gif"},
	{";-P", EMOT_DIR "jezyk2.gif"},

	{":PP", EMOT_DIR "jezyk1.gif"},
	{";PP", EMOT_DIR "jezyk2.gif"},

	{":->", EMOT_DIR "chytry.gif"},
	{";->", EMOT_DIR "chytry.gif"},

	{":>>", EMOT_DIR "chytry.gif"},
	{";>>", EMOT_DIR "chytry.gif"},

	{":-]", EMOT_DIR "krzywy.gif"},
	{";-]", EMOT_DIR "krzywy.gif"},

	{":]]", EMOT_DIR "krzywy.gif"},
	{";]]", EMOT_DIR "krzywy.gif"},

	{":-|", EMOT_DIR "ysz.gif"},
	{";-|", EMOT_DIR "ysz.gif"},

	{":||", EMOT_DIR "ysz.gif"},
	{";||", EMOT_DIR "ysz.gif"},

	{":-/", EMOT_DIR "kwasny.gif"},
	{";-/", EMOT_DIR "kwasny.gif"},

	{":-*", EMOT_DIR "cmok.gif"},
	{";-*", EMOT_DIR "cmok.gif"},

	{":-O", EMOT_DIR "wow.gif"},
	{";-O", EMOT_DIR "wow.gif"},

	{"^_^", EMOT_DIR "manga_usmiech.gif"},

	{":-))", EMOT_DIR "usmiech2.gif"},
	{";-))", EMOT_DIR "oczko2.gif"},

	{":-((", EMOT_DIR "placze.gif"},
	{";-((", EMOT_DIR "placze.gif"},

	{":-DD", EMOT_DIR "zeby.gif"},
	{";-DD", EMOT_DIR "zeby.gif"},

	{":-PP", EMOT_DIR "jezyk1.gif"},
	{";-PP", EMOT_DIR "jezyk2.gif"},

	{"]:->", EMOT_DIR "diabelek.gif"},

	/* not recognized by Gadu-Gadu 7.0 */

	{":\\", NULL},
	{";\\", NULL},

	{":-\\", NULL},
	{";-\\", NULL},

	{":\\\\", NULL},
	{";\\\\", NULL},

	{":-\\\\", NULL},
	{";-\\\\", NULL},

	{":E", NULL},
	{";E", NULL},

	{":-E", NULL},
	{";-E", NULL},

	{":EE", NULL},
	{";EE", NULL},

	{":-EE", NULL},
	{";-EE", NULL},

	{":F", NULL},
	{";F", NULL},

	{":-F", NULL},
	{";-F", NULL},

	{":FF", NULL},
	{";FF", NULL},

	{":-FF", NULL},
	{";-FF", NULL},

	{":C", NULL},
	{";C", NULL},

	{":-C", NULL},
	{";-C", NULL},

	{":CC", NULL},
	{";CC", NULL},

	{":-CC", NULL},
	{";-CC", NULL},

	{":S", NULL},
	{";S", NULL},

	{":-S", NULL},
	{";-S", NULL},

	{":SS", NULL},
	{";SS", NULL},

	{":-SS", NULL},
	{";-SS", NULL}
};

CONST UBYTE EmoticonsTabSize = sizeof(EmoticonsTab) / sizeof(*EmoticonsTab);
