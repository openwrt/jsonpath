/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_ROOT = 258,
     T_THIS = 259,
     T_DOT = 260,
     T_BROPEN = 261,
     T_BRCLOSE = 262,
     T_OR = 263,
     T_AND = 264,
     T_LT = 265,
     T_LE = 266,
     T_GT = 267,
     T_GE = 268,
     T_EQ = 269,
     T_NE = 270,
     T_POPEN = 271,
     T_PCLOSE = 272,
     T_NOT = 273,
     T_BOOL = 274,
     T_NUMBER = 275,
     T_STRING = 276,
     T_LABEL = 277,
     T_WILDCARD = 278
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 60 "parser.y"

	struct jp_opcode *op;



/* Line 2068 of yacc.c  */
#line 79 "parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


/* "%code provides" blocks.  */

/* Line 2068 of yacc.c  */
#line 38 "parser.y"


#ifndef JP_OPCODE
# define JP_OPCODE
	struct jp_opcode {
		int type;
		struct jp_opcode *next;
		struct jp_opcode *down;
		struct jp_opcode *sibling;
		char *str;
		int num;
	};
#endif

struct jp_opcode *_jp_alloc_op(int type, int num, char *str, ...);
#define jp_alloc_op(type, num, str, ...) _jp_alloc_op(type, num, str, ##__VA_ARGS__, NULL)

struct jp_opcode *jp_parse(const char *expr, const char **error);
void jp_free(void);




/* Line 2068 of yacc.c  */
#line 117 "parser.h"
