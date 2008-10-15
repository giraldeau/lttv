/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
     ASSERT = 258,
     PRINT = 259,
     PRINTM = 260,
     C_CODE = 261,
     C_DECL = 262,
     C_EXPR = 263,
     C_STATE = 264,
     C_TRACK = 265,
     RUN = 266,
     LEN = 267,
     ENABLED = 268,
     EVAL = 269,
     PC_VAL = 270,
     TYPEDEF = 271,
     MTYPE = 272,
     INLINE = 273,
     LABEL = 274,
     OF = 275,
     GOTO = 276,
     BREAK = 277,
     ELSE = 278,
     SEMI = 279,
     IF = 280,
     FI = 281,
     DO = 282,
     OD = 283,
     SEP = 284,
     ATOMIC = 285,
     NON_ATOMIC = 286,
     D_STEP = 287,
     UNLESS = 288,
     TIMEOUT = 289,
     NONPROGRESS = 290,
     ACTIVE = 291,
     PROCTYPE = 292,
     D_PROCTYPE = 293,
     HIDDEN = 294,
     SHOW = 295,
     ISLOCAL = 296,
     PRIORITY = 297,
     PROVIDED = 298,
     FULL = 299,
     EMPTY = 300,
     NFULL = 301,
     NEMPTY = 302,
     CONST = 303,
     TYPE = 304,
     XU = 305,
     NAME = 306,
     UNAME = 307,
     PNAME = 308,
     INAME = 309,
     STRING = 310,
     CLAIM = 311,
     TRACE = 312,
     INIT = 313,
     ASGN = 314,
     R_RCV = 315,
     RCV = 316,
     O_SND = 317,
     SND = 318,
     OR = 319,
     AND = 320,
     NE = 321,
     EQ = 322,
     LE = 323,
     GE = 324,
     LT = 325,
     GT = 326,
     RSHIFT = 327,
     LSHIFT = 328,
     DECR = 329,
     INCR = 330,
     NEG = 331,
     UMIN = 332,
     DOT = 333
   };
#endif
/* Tokens.  */
#define ASSERT 258
#define PRINT 259
#define PRINTM 260
#define C_CODE 261
#define C_DECL 262
#define C_EXPR 263
#define C_STATE 264
#define C_TRACK 265
#define RUN 266
#define LEN 267
#define ENABLED 268
#define EVAL 269
#define PC_VAL 270
#define TYPEDEF 271
#define MTYPE 272
#define INLINE 273
#define LABEL 274
#define OF 275
#define GOTO 276
#define BREAK 277
#define ELSE 278
#define SEMI 279
#define IF 280
#define FI 281
#define DO 282
#define OD 283
#define SEP 284
#define ATOMIC 285
#define NON_ATOMIC 286
#define D_STEP 287
#define UNLESS 288
#define TIMEOUT 289
#define NONPROGRESS 290
#define ACTIVE 291
#define PROCTYPE 292
#define D_PROCTYPE 293
#define HIDDEN 294
#define SHOW 295
#define ISLOCAL 296
#define PRIORITY 297
#define PROVIDED 298
#define FULL 299
#define EMPTY 300
#define NFULL 301
#define NEMPTY 302
#define CONST 303
#define TYPE 304
#define XU 305
#define NAME 306
#define UNAME 307
#define PNAME 308
#define INAME 309
#define STRING 310
#define CLAIM 311
#define TRACE 312
#define INIT 313
#define ASGN 314
#define R_RCV 315
#define RCV 316
#define O_SND 317
#define SND 318
#define OR 319
#define AND 320
#define NE 321
#define EQ 322
#define LE 323
#define GE 324
#define LT 325
#define GT 326
#define RSHIFT 327
#define LSHIFT 328
#define DECR 329
#define INCR 330
#define NEG 331
#define UMIN 332
#define DOT 333




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

