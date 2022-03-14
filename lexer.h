/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __LEXER_H__
#define __LEXER_H__

/* don't forget: (1 << 0) is forbidden! */

/* general types */
#define WHITECHAR    (1 << 1)
#define NORMALWORD   (1 << 2)
#define SPECIALWORD  (1 << 3)

/* urls types */
#define URL_HTTP     (1 << 4)
#define URL_HTTPS    (1 << 5)
#define URL_FTP      (1 << 6)
#define URL_FILE     (1 << 7)
#define URL_MAILTO   (1 << 8)
#define URL_MAIL     (1 << 9)
#define URL_PLAIN    (1 << 10)
#define URL_DOMAIN   (1 << 11)

/* emoticons types */
#define BIG_EMOT     (1 << 12)
#define SMILE        (1 << 13)
#define PUNCTUATION  (1 << 14)

/* macros for checking types */

/* generals */
#define IS_WHITECHAR(x) ((x) & WHITECHAR)
#define IS_NORMALWORD(x) ((x) & NORMALWORD)
#define IS_SPECIALWORD(x) ((x) & SPECIALWORD)
#define IS_WORD(x) (IS_NORMALWORD(x) || IS_SPECIALWORD(x))

/* urls */
#define IS_WWW(x) (((x) & URL_HTTP) || ((x) & URL_HTTPS) || ((x) & URL_PLAIN) || ((x) & URL_DOMAIN))
#define IS_MAIL(x) (((x) & URL_MAILTO) || ((x) & URL_MAIL))
#define IS_FTP(x) ((x) & URL_FTP)
#define IS_FILE(x) ((x) & URL_FILE)
#define IS_URL(x) (IS_WWW(x) || IS_MAIL(x) || IS_FTP(x) || IS_FILE(x))

/* emoticons */
#define IS_BIG_EMOTICON(x) ((x) & BIG_EMOT)
#define IS_SIMILE(x) ((x) & SMILE)
#define IS_PUNCTUATION(x) ((x) & PUNCTUATION)
#define IS_EMOTICON(x) (IS_BIG_EMOTICON(x) || IS_SIMILE(x) || IS_PUNCTUATION(x))


ULONG MessageParse(STRPTR *str, STRPTR output, STRPTR spec);


#endif /* __LEXER_H__ */
