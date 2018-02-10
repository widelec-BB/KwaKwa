/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/* URL matching rules are based on official RFC 3986 doc (http://www.faqs.org/rfcs/rfc3986.html) */
/* generate lexer with call: flex -i lexer.l */
%option noyywrap
%option never-interactive
%option case-insensitive
%option nounput
%option nodefault
%option warn

%{
#include <exec/types.h>
#include <string.h>
#include "lexer.h"

#define NORMALCHAR (1 << 0) /* internal, forbidden */

#define YY_NO_INPUT 1
#define YY_NO_UNISTD_H 1
#define YY_DECL ULONG yylex(void)
#undef YY_NEED_STRLEN
%}

 /* common rules */
AUTHORITY   ({USER}?(":"{PASSWORD})?"@")?{HOSTPORT}
HOSTPORT    {HOST}(":"{PORT})?
HOST        ({HOSTNAME}|{HOSTNUMBER})
HOSTNAME    ([[:alnum:]]|"-"|"."|"_"|"-")+
HOSTNUMBER  [[:digit:]]{1,3}"."[[:digit:]]{1,3}"."[[:digit:]]{1,3}"."[[:digit:]]{1,3}
PORT        [[:digit:]]+
USER        ({UCHAR}|";"|"?"|"&"|"=")*
PASSWORD    ({UCHAR}|";"|"?"|"&"|"=")*
URLPATH     {XCHAR}*
ROOTDOMAINS ("com"|"edu"|"gov"|"net"|"org"|"int"|"mil"|"biz"|"info"|"name"|"pro"|"eng"|"pl")

 /* FTP (see also RFC959) */
FTPURL      "ftp://"{AUTHORITY}("/"{FPATH}(";type="{FTPTYPE})?)?{NOTATEOL}
FPATH       {FSEGMENT}("/"{FSEGMENT})*
FSEGMENT    ({UCHAR}|"?"|":"|"@"|"&"|"=")*
FTPTYPE     [AIDaid]

 /* FILE */
FILEURL     "file://"{AUTHORITY}?"/"{FPATH}{NOTATEOL}

 /* HTTP/HTTPS/PLAINURL */
HTTPURL     "http://"{AUTHORITY}("/"{HPATH}("?"{SEARCH})?)?{NOTATEOL}
HTTPSURL    "https://"{AUTHORITY}("/"{HPATH}("?"{SEARCH})?)?{NOTATEOL}
PLAINURL    ("www"|"ftp")"."{HOSTNAME}"."[[:alpha:]]{1,4}("/"{HPATH}("?"{SEARCH})?)?{NOTATEOL}
HPATH       "~"?{HSEGMENT}("/""~"?{HSEGMENT})*
HSEGMENT    ({UCHAR}|";"|":"|"@"|"&"|"="|"#")*
SEARCH      ({UCHAR}|";"|":"|"@"|"&"|"="|"#"|"/")*
DOMAINURL   {HOSTNAME}"."{ROOTDOMAINS}|{HOSTNAME}"."{ROOTDOMAINS}("/"{HPATH}("?"{SEARCH})?)?{NOTATEOL}

 /* MAILTO (see also RFC822 */
MAILTOURL   "mailto:"{ENC822ADDR}("?"{SEARCH})?{NOTATEOL}
ENC822ADDR  ([[:alnum:]]|{SAFE})+"@"{HOST}

 /* Miscellaneous definitions */
SAFE        ("$"|"-"|"_"|"."|"+")
RESERVED    (";"|"/"|"?"|":"|"@"|"&"|"=")
ESCAPE      "%"[[:xdigit:]]{2}
UNRESERVED  ([^ \t\n\r]|{SAFE})
UCHAR       ({UNRESERVED}|{ESCAPE})
XCHAR       ({UNRESERVED}|{RESERVED}|{ESCAPE})
NOTATEOL    ([^[:punct:][:space:]]|{ESCAPE}|"/"|"=")
WHITECHAR   [ \n\t]
NORMALCHAR  .


 /* Emoticons (as in Gadu-Gadu) */
BIGEMOTICON    "<"([^ \t\n\r]*)">"
PUNCTUATION    ("?"){2,}|("!"){2,}
EYES           ((":")+|(";")+)
NOSE           (("-")*)
MOUTH          ((")")+|(">")+|("]")+|("(")+|("|")+|("D")+|("P")+|("[")+|("/")+|("\\")+|("*")+|("O")+|("E")+|("F")+|("C")+|("S")+)
SMILE          (({EYES}{NOSE}{MOUTH})|("]:->")|("^^")|("^_^"))
%%
 /* for simple word-by-word parsing */
{WHITECHAR}    {return WHITECHAR ;}
{NORMALCHAR}   {return NORMALCHAR ;}

 /* urls */
{HTTPURL}      {return URL_HTTP     |   SPECIALWORD ;}
{HTTPSURL}     {return URL_HTTPS    |   SPECIALWORD ;}
{FTPURL}       {return URL_FTP      |   SPECIALWORD ;}
{FILEURL}      {return URL_FILE     |   SPECIALWORD ;}
{MAILTOURL}    {return URL_MAILTO   |   SPECIALWORD ;}
{ENC822ADDR}   {return URL_MAIL     |   SPECIALWORD ;}
{PLAINURL}     {return URL_PLAIN    |   SPECIALWORD ;}
{DOMAINURL}    {return URL_DOMAIN   |   SPECIALWORD ;}
 /* emoticons */
{BIGEMOTICON}  {return BIG_EMOT     |   SPECIALWORD ;}
{SMILE}        {return SMILE        |   SPECIALWORD ;}
{PUNCTUATION}  {return PUNCTUATION  |   SPECIALWORD ;}
%%

ULONG MessageParse(STRPTR *str, STRPTR output, STRPTR spec)
{
	ULONG chars = 0, schars = 0, type = 0;
	YY_BUFFER_STATE buffer;

	if((buffer = yy_scan_string(*str)))
	{
		while((type = (ULONG)yylex()))
		{
			if(type & SPECIALWORD)
			{
				strcpy(spec, yytext);
				schars = yyleng;
				break;
			}
			else if(type == WHITECHAR)
			{
				break;
			}
			else  /* NORMALCHAR */
			{
				output[chars++] = *yytext;
			}
		}
		yy_delete_buffer(buffer);
	}

	if(chars > 0) type |= NORMALWORD;
	if(schars > 0) type |= SPECIALWORD;

	output[chars] = 0x00;
	*str = (*str + chars + schars);

	return type;
}
