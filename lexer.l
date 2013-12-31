%{
/*
 * Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>

#include "parser.h"

int yylex(struct jp_state *s);

#define YY_DECL int yylex(struct jp_state *s)

static void
str_put(struct jp_state *s, char c)
{
	if ((s->str_ptr - s->str_buf + 1) < sizeof(s->str_buf))
		*s->str_ptr++ = c;
}

static void
str_decode(struct jp_state *s, const char *input, int base)
{
	int code;
	char *end;

	code = strtoul(input, &end, base);

	if (end == input || *end)
		return;

	if (code > 0 && code <= 0x7F)
	{
		str_put(s, code);
	}
	else if (code > 0 && code <= 0x7FF)
	{
		str_put(s, ((code >>  6) & 0x1F) | 0xC0);
		str_put(s, ( code        & 0x3F) | 0x80);
	}
	else if (code > 0 && code <= 0xFFFF)
	{
		str_put(s, ((code >> 12) & 0x0F) | 0xE0);
		str_put(s, ((code >>  6) & 0x3F) | 0x80);
		str_put(s, ( code        & 0x3F) | 0x80);
	}
	else if (code > 0 && code <= 0x10FFFF)
	{
		str_put(s, ((code >> 18) & 0x07) | 0xF0);
		str_put(s, ((code >> 12) & 0x3F) | 0x80);
		str_put(s, ((code >>  6) & 0x3F) | 0x80);
		str_put(s, ( code        & 0x3F) | 0x80);
	}
}

%}

%option outfile="lexer.c" header-file="lexer.h"
%option noyywrap nounput noinput

DOT			"."
LABEL		[a-zA-Z_][a-zA-Z0-9_]*

BROPEN		"["
BRCLOSE		"]"
POPEN		"("
PCLOSE		")"

ROOT		"$"
THIS		"@"

LT			"<"
LE			"<="
GT			">"
GE			">="
NE			"!="
EQ			"="
NOT			"!"
AND			"&&"
OR			"||"

NUMBER		-?[0-9]+
WILDCARD	"*"
BOOL		(true|false)

WS			[ \t\n]*

%x			STRING

%%

["'] {
	s->str_ptr = s->str_buf;
	s->str_quote = *yytext;
	memset(s->str_buf, 0, sizeof(s->str_buf));
	BEGIN(STRING);
}

<STRING>{
	["'] {
		if (*yytext == s->str_quote)
		{
			BEGIN(INITIAL);
			yylval.op = jp_alloc_op(T_STRING, 0, s->str_buf);
			return T_STRING;
		}

		str_put(s, *yytext);
	}

	\\([0-3][0-7]{1,2}|[0-7]{0,2})	{ str_decode(s, yytext + 1, 8); }
	\\x[A-Fa-f0-9]{2}				{ str_decode(s, yytext + 2, 16); }
	\\u[A-Fa-f0-9]{4}				{ str_decode(s, yytext + 2, 16); }
	\\a								{ str_put(s, '\a'); }
	\\b								{ str_put(s, '\b'); }
	\\e								{ str_put(s, '\e'); }
	\\f								{ str_put(s, '\f'); }
	\\n								{ str_put(s, '\n'); }
	\\r								{ str_put(s, '\r'); }
	\\t								{ str_put(s, '\t'); }
	\\v								{ str_put(s, '\v'); }
	\\.								{ str_put(s, *yytext); }
	[^\\"']+						{ while (*yytext) str_put(s, *yytext++); }
}

{BOOL} {
	yylval.op = jp_alloc_op(T_BOOL, (*yytext == 't'), NULL);
	return T_BOOL;
}

{NUMBER} {
	yylval.op = jp_alloc_op(T_NUMBER, atoi(yytext), NULL);
	return T_NUMBER;
}

{LABEL} {
	yylval.op = jp_alloc_op(T_LABEL, 0, yytext);
	return T_LABEL;
}

{WILDCARD} {
	yylval.op = jp_alloc_op(T_WILDCARD, 0, NULL);
	return T_WILDCARD;
}

{DOT}		{ return T_DOT; }
{BROPEN}	{ return T_BROPEN; }
{BRCLOSE}	{ return T_BRCLOSE; }
{POPEN}		{ return T_POPEN; }
{PCLOSE}	{ return T_PCLOSE; }

{ROOT}		{ return T_ROOT; }
{THIS}		{ return T_THIS; }

{LT}		{ return T_LT; }
{LE}		{ return T_LE; }
{GT}		{ return T_GT; }
{GE}		{ return T_GE; }
{EQ}		{ return T_EQ; }
{NE}		{ return T_NE; }
{NOT}		{ return T_NOT; }
{AND}		{ return T_AND; }
{OR}		{ return T_OR; }

{WS}		{ }

%%
