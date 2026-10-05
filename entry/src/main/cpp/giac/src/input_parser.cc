/* A Bison parser, made by GNU Bison 3.7.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30704

/* Bison version string.  */
#define YYBISON_VERSION "3.7.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         giac_yyparse
#define yylex           giac_yylex
#define yyerror         giac_yyerror
#define yydebug         giac_yydebug
#define yynerrs         giac_yynerrs

/* First part of user prologue.  */
#line 24 "input_parser.yy"

         #define YYPARSE_PARAM scanner
         #define YYLEX_PARAM   scanner
	 
#line 33 "input_parser.yy"

#include "giacPCH.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "first.h"
#include <stdexcept>
#include <cstdlib>
#include "giacPCH.h"
#include "index.h"
#include "gen.h"
#define YYSTYPE giac::gen
#define YY_EXTRA_TYPE  const giac::context *
#include "lexer.h"
#include "input_lexer.h"
#include "usual.h"
#include "derive.h"
#include "sym2poly.h"
#include "vecteur.h"
#include "modpoly.h"
#include "alg_ext.h"
#include "prog.h"
#include "rpn.h"
#include "intg.h"
#include "plot.h"
#include "maple.h"
using namespace std;

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

// It seems there is a bison bug when it reallocates space for the stack
// therefore I redefine YYINITDEPTH to 4000 (max size is YYMAXDEPTH)
// instead of 200
// Feel free to change if you need but then readjust YYMAXDEPTH
#if defined RTOS_THREADX || defined NSPIRE || defined NSPIRE_NEWLIB || defined NUMWORKS
#ifdef RTOS_THREADX
#define YYINITDEPTH 100
#define YYMAXDEPTH 101
#else
#define YYINITDEPTH 200
#define YYMAXDEPTH 201
#endif
#else // RTOS_THREADX
// Note that the compilation by bison with -v option generates a file y.output
// to debug the grammar, compile input_parser.yy with bison
// then add yydebug=1 in input_parser.cc at the beginning of yyparse (
#define YYDEBUG 1
#ifdef GNUWINCE
#define YYINITDEPTH 1000
#else 
#define YYINITDEPTH 4000
#define YYMAXDEPTH 20000
#define YYERROR_VERBOSE 1
#endif // GNUWINCE
#endif // RTOS_THREADX

#if 0
#define YYSTACK_USE_ALLOCA 1
#endif


gen polynome_or_sparse_poly1(const gen & coeff, const gen & index){
  if (index.type==_VECT){
    index_t i;
    const_iterateur it=index._VECTptr->begin(),itend=index._VECTptr->end();
    i.reserve(itend-it);
    for (;it!=itend;++it){
      if (it->type!=_INT_)
         return gentypeerr();
      i.push_back(it->val);
    }
    monomial<gen> m(coeff,i);
    return polynome(m);
  }
  else {
    sparse_poly1 res;
    res.push_back(monome(coeff,index));
    return res;
  }
}

#line 165 "input_parser.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "input_parser.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_T_NUMBER = 3,                   /* T_NUMBER  */
  YYSYMBOL_T_SYMBOL = 4,                   /* T_SYMBOL  */
  YYSYMBOL_T_LITERAL = 5,                  /* T_LITERAL  */
  YYSYMBOL_T_DIGITS = 6,                   /* T_DIGITS  */
  YYSYMBOL_T_STRING = 7,                   /* T_STRING  */
  YYSYMBOL_T_END_INPUT = 8,                /* T_END_INPUT  */
  YYSYMBOL_T_EXPRESSION = 9,               /* T_EXPRESSION  */
  YYSYMBOL_T_UNARY_OP = 10,                /* T_UNARY_OP  */
  YYSYMBOL_T_OF = 11,                      /* T_OF  */
  YYSYMBOL_T_NOT = 12,                     /* T_NOT  */
  YYSYMBOL_T_TYPE_ID = 13,                 /* T_TYPE_ID  */
  YYSYMBOL_T_VIRGULE = 14,                 /* T_VIRGULE  */
  YYSYMBOL_T_AFFECT = 15,                  /* T_AFFECT  */
  YYSYMBOL_T_MAPSTO = 16,                  /* T_MAPSTO  */
  YYSYMBOL_T_BEGIN_PAR = 17,               /* T_BEGIN_PAR  */
  YYSYMBOL_T_END_PAR = 18,                 /* T_END_PAR  */
  YYSYMBOL_T_PLUS = 19,                    /* T_PLUS  */
  YYSYMBOL_T_MOINS = 20,                   /* T_MOINS  */
  YYSYMBOL_T_FOIS = 21,                    /* T_FOIS  */
  YYSYMBOL_T_DIV = 22,                     /* T_DIV  */
  YYSYMBOL_T_MOD = 23,                     /* T_MOD  */
  YYSYMBOL_T_POW = 24,                     /* T_POW  */
  YYSYMBOL_T_QUOTED_BINARY = 25,           /* T_QUOTED_BINARY  */
  YYSYMBOL_T_QUOTE = 26,                   /* T_QUOTE  */
  YYSYMBOL_T_PRIME = 27,                   /* T_PRIME  */
  YYSYMBOL_T_TEST_EQUAL = 28,              /* T_TEST_EQUAL  */
  YYSYMBOL_T_EQUAL = 29,                   /* T_EQUAL  */
  YYSYMBOL_T_INTERVAL = 30,                /* T_INTERVAL  */
  YYSYMBOL_T_UNION = 31,                   /* T_UNION  */
  YYSYMBOL_T_INTERSECT = 32,               /* T_INTERSECT  */
  YYSYMBOL_T_MINUS = 33,                   /* T_MINUS  */
  YYSYMBOL_T_AND_OP = 34,                  /* T_AND_OP  */
  YYSYMBOL_T_COMPOSE = 35,                 /* T_COMPOSE  */
  YYSYMBOL_T_DOLLAR = 36,                  /* T_DOLLAR  */
  YYSYMBOL_T_DOLLAR_MAPLE = 37,            /* T_DOLLAR_MAPLE  */
  YYSYMBOL_T_INDEX_BEGIN = 38,             /* T_INDEX_BEGIN  */
  YYSYMBOL_T_VECT_BEGIN = 39,              /* T_VECT_BEGIN  */
  YYSYMBOL_T_VECT_DISPATCH = 40,           /* T_VECT_DISPATCH  */
  YYSYMBOL_T_VECT_END = 41,                /* T_VECT_END  */
  YYSYMBOL_T_SET_BEGIN = 42,               /* T_SET_BEGIN  */
  YYSYMBOL_T_SET_END = 43,                 /* T_SET_END  */
  YYSYMBOL_T_SEMI = 44,                    /* T_SEMI  */
  YYSYMBOL_T_DEUXPOINTS = 45,              /* T_DEUXPOINTS  */
  YYSYMBOL_T_DOUBLE_DEUX_POINTS = 46,      /* T_DOUBLE_DEUX_POINTS  */
  YYSYMBOL_T_IF = 47,                      /* T_IF  */
  YYSYMBOL_T_RPN_IF = 48,                  /* T_RPN_IF  */
  YYSYMBOL_T_ELIF = 49,                    /* T_ELIF  */
  YYSYMBOL_T_THEN = 50,                    /* T_THEN  */
  YYSYMBOL_T_ELSE = 51,                    /* T_ELSE  */
  YYSYMBOL_T_IFTE = 52,                    /* T_IFTE  */
  YYSYMBOL_T_SWITCH = 53,                  /* T_SWITCH  */
  YYSYMBOL_T_CASE = 54,                    /* T_CASE  */
  YYSYMBOL_T_DEFAULT = 55,                 /* T_DEFAULT  */
  YYSYMBOL_T_ENDCASE = 56,                 /* T_ENDCASE  */
  YYSYMBOL_T_FOR = 57,                     /* T_FOR  */
  YYSYMBOL_T_FROM = 58,                    /* T_FROM  */
  YYSYMBOL_T_TO = 59,                      /* T_TO  */
  YYSYMBOL_T_DO = 60,                      /* T_DO  */
  YYSYMBOL_T_BY = 61,                      /* T_BY  */
  YYSYMBOL_T_WHILE = 62,                   /* T_WHILE  */
  YYSYMBOL_T_MUPMAP_WHILE = 63,            /* T_MUPMAP_WHILE  */
  YYSYMBOL_T_RPN_WHILE = 64,               /* T_RPN_WHILE  */
  YYSYMBOL_T_REPEAT = 65,                  /* T_REPEAT  */
  YYSYMBOL_T_UNTIL = 66,                   /* T_UNTIL  */
  YYSYMBOL_T_IN = 67,                      /* T_IN  */
  YYSYMBOL_T_START = 68,                   /* T_START  */
  YYSYMBOL_T_BREAK = 69,                   /* T_BREAK  */
  YYSYMBOL_T_CONTINUE = 70,                /* T_CONTINUE  */
  YYSYMBOL_T_TRY = 71,                     /* T_TRY  */
  YYSYMBOL_T_CATCH = 72,                   /* T_CATCH  */
  YYSYMBOL_T_TRY_CATCH = 73,               /* T_TRY_CATCH  */
  YYSYMBOL_T_PROC = 74,                    /* T_PROC  */
  YYSYMBOL_T_BLOC = 75,                    /* T_BLOC  */
  YYSYMBOL_T_BLOC_BEGIN = 76,              /* T_BLOC_BEGIN  */
  YYSYMBOL_T_BLOC_END = 77,                /* T_BLOC_END  */
  YYSYMBOL_T_RETURN = 78,                  /* T_RETURN  */
  YYSYMBOL_T_LOCAL = 79,                   /* T_LOCAL  */
  YYSYMBOL_T_LOCALBLOC = 80,               /* T_LOCALBLOC  */
  YYSYMBOL_T_NAME = 81,                    /* T_NAME  */
  YYSYMBOL_T_PROGRAM = 82,                 /* T_PROGRAM  */
  YYSYMBOL_T_NULL = 83,                    /* T_NULL  */
  YYSYMBOL_T_ARGS = 84,                    /* T_ARGS  */
  YYSYMBOL_T_FACTORIAL = 85,               /* T_FACTORIAL  */
  YYSYMBOL_T_RPN_OP = 86,                  /* T_RPN_OP  */
  YYSYMBOL_T_RPN_BEGIN = 87,               /* T_RPN_BEGIN  */
  YYSYMBOL_T_RPN_END = 88,                 /* T_RPN_END  */
  YYSYMBOL_T_STACK = 89,                   /* T_STACK  */
  YYSYMBOL_T_GROUPE_BEGIN = 90,            /* T_GROUPE_BEGIN  */
  YYSYMBOL_T_GROUPE_END = 91,              /* T_GROUPE_END  */
  YYSYMBOL_T_LINE_BEGIN = 92,              /* T_LINE_BEGIN  */
  YYSYMBOL_T_LINE_END = 93,                /* T_LINE_END  */
  YYSYMBOL_T_VECTOR_BEGIN = 94,            /* T_VECTOR_BEGIN  */
  YYSYMBOL_T_VECTOR_END = 95,              /* T_VECTOR_END  */
  YYSYMBOL_T_CURVE_BEGIN = 96,             /* T_CURVE_BEGIN  */
  YYSYMBOL_T_CURVE_END = 97,               /* T_CURVE_END  */
  YYSYMBOL_T_ROOTOF_BEGIN = 98,            /* T_ROOTOF_BEGIN  */
  YYSYMBOL_T_ROOTOF_END = 99,              /* T_ROOTOF_END  */
  YYSYMBOL_T_SPOLY1_BEGIN = 100,           /* T_SPOLY1_BEGIN  */
  YYSYMBOL_T_SPOLY1_END = 101,             /* T_SPOLY1_END  */
  YYSYMBOL_T_POLY1_BEGIN = 102,            /* T_POLY1_BEGIN  */
  YYSYMBOL_T_POLY1_END = 103,              /* T_POLY1_END  */
  YYSYMBOL_T_MATRICE_BEGIN = 104,          /* T_MATRICE_BEGIN  */
  YYSYMBOL_T_MATRICE_END = 105,            /* T_MATRICE_END  */
  YYSYMBOL_T_ASSUME_BEGIN = 106,           /* T_ASSUME_BEGIN  */
  YYSYMBOL_T_ASSUME_END = 107,             /* T_ASSUME_END  */
  YYSYMBOL_T_HELP = 108,                   /* T_HELP  */
  YYSYMBOL_TI_DEUXPOINTS = 109,            /* TI_DEUXPOINTS  */
  YYSYMBOL_TI_LOCAL = 110,                 /* TI_LOCAL  */
  YYSYMBOL_TI_LOOP = 111,                  /* TI_LOOP  */
  YYSYMBOL_TI_FOR = 112,                   /* TI_FOR  */
  YYSYMBOL_TI_WHILE = 113,                 /* TI_WHILE  */
  YYSYMBOL_TI_STO = 114,                   /* TI_STO  */
  YYSYMBOL_TI_TRY = 115,                   /* TI_TRY  */
  YYSYMBOL_TI_DIALOG = 116,                /* TI_DIALOG  */
  YYSYMBOL_T_PIPE = 117,                   /* T_PIPE  */
  YYSYMBOL_TI_DEFINE = 118,                /* TI_DEFINE  */
  YYSYMBOL_TI_PRGM = 119,                  /* TI_PRGM  */
  YYSYMBOL_TI_SEMI = 120,                  /* TI_SEMI  */
  YYSYMBOL_TI_HASH = 121,                  /* TI_HASH  */
  YYSYMBOL_T_ACCENTGRAVE = 122,            /* T_ACCENTGRAVE  */
  YYSYMBOL_T_MAPLELIB = 123,               /* T_MAPLELIB  */
  YYSYMBOL_T_INTERROGATION = 124,          /* T_INTERROGATION  */
  YYSYMBOL_T_UNIT = 125,                   /* T_UNIT  */
  YYSYMBOL_T_BIDON = 126,                  /* T_BIDON  */
  YYSYMBOL_T_LOGO = 127,                   /* T_LOGO  */
  YYSYMBOL_T_SQ = 128,                     /* T_SQ  */
  YYSYMBOL_T_CASE38 = 129,                 /* T_CASE38  */
  YYSYMBOL_T_IFERR = 130,                  /* T_IFERR  */
  YYSYMBOL_T_MOINS38 = 131,                /* T_MOINS38  */
  YYSYMBOL_T_NEG38 = 132,                  /* T_NEG38  */
  YYSYMBOL_T_UNARY_OP_38 = 133,            /* T_UNARY_OP_38  */
  YYSYMBOL_T_FUNCTION = 134,               /* T_FUNCTION  */
  YYSYMBOL_T_IMPMULT = 135,                /* T_IMPMULT  */
  YYSYMBOL_YYACCEPT = 136,                 /* $accept  */
  YYSYMBOL_input = 137,                    /* input  */
  YYSYMBOL_correct_input = 138,            /* correct_input  */
  YYSYMBOL_exp = 139,                      /* exp  */
  YYSYMBOL_symbol_for = 140,               /* symbol_for  */
  YYSYMBOL_symbol = 141,                   /* symbol  */
  YYSYMBOL_symbol_or_literal = 142,        /* symbol_or_literal  */
  YYSYMBOL_entete = 143,                   /* entete  */
  YYSYMBOL_stack = 144,                    /* stack  */
  YYSYMBOL_local = 145,                    /* local  */
  YYSYMBOL_nom = 146,                      /* nom  */
  YYSYMBOL_suite_symbol = 147,             /* suite_symbol  */
  YYSYMBOL_affectable_symbol = 148,        /* affectable_symbol  */
  YYSYMBOL_exp_or_empty = 149,             /* exp_or_empty  */
  YYSYMBOL_suite = 150,                    /* suite  */
  YYSYMBOL_prg_suite = 151,                /* prg_suite  */
  YYSYMBOL_rpn_suite = 152,                /* rpn_suite  */
  YYSYMBOL_rpn_token = 153,                /* rpn_token  */
  YYSYMBOL_step = 154,                     /* step  */
  YYSYMBOL_from = 155,                     /* from  */
  YYSYMBOL_loop38_do = 156,                /* loop38_do  */
  YYSYMBOL_else = 157,                     /* else  */
  YYSYMBOL_bloc = 158,                     /* bloc  */
  YYSYMBOL_elif = 159,                     /* elif  */
  YYSYMBOL_ti_bloc_end = 160,              /* ti_bloc_end  */
  YYSYMBOL_ti_else = 161,                  /* ti_else  */
  YYSYMBOL_switch = 162,                   /* switch  */
  YYSYMBOL_case = 163,                     /* case  */
  YYSYMBOL_case38 = 164,                   /* case38  */
  YYSYMBOL_semi = 165                      /* semi  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  157
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   14221

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  136
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  30
/* YYNRULES -- Number of rules.  */
#define YYNRULES  260
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  607

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   390


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   199,   199,   207,   208,   209,   212,   213,   214,   215,
     216,   217,   218,   220,   221,   224,   225,   226,   227,   231,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   254,   255,
     261,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   275,   276,   277,   282,   287,   293,   294,   300,   301,
     302,   303,   304,   305,   306,   320,   325,   329,   336,   339,
     340,   342,   343,   344,   347,   348,   349,   350,   351,   355,
     362,   363,   365,   367,   368,   370,   371,   372,   397,   402,
     415,   428,   432,   436,   441,   446,   452,   456,   460,   461,
     465,   466,   467,   468,   469,   470,   471,   473,   474,   475,
     476,   477,   478,   481,   482,   487,   491,   495,   496,   522,
     531,   537,   538,   539,   540,   545,   549,   550,   559,   560,
     561,   562,   563,   567,   568,   571,   576,   577,   578,   579,
     584,   589,   594,   599,   604,   609,   614,   615,   616,   617,
     618,   619,   620,   621,   625,   628,   632,   636,   637,   638,
     639,   640,   641,   642,   643,   644,   645,   646,   647,   648,
     649,   650,   651,   652,   656,   660,   664,   667,   668,   669,
     670,   671,   675,   676,   695,   707,   708,   709,   712,   713,
     719,   720,   721,   722,   723,   724,   732,   738,   741,   742,
     745,   746,   747,   751,   752,   755,   758,   761,   762,   769,
     770,   771,   772,   773,   774,   775,   776,   784,   794,   795,
     798,   799,   802,   804,   809,   812,   813,   814,   817,   887,
     888,   891,   892,   893,   894,   897,   898,   901,   902,   903,
     907,   910,   917,   918,   922,   925,   930,   931,   934,   935,
     938,   939,   940,   943,   944,   945,   948,   949,   950,   951,
     954
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "T_NUMBER", "T_SYMBOL",
  "T_LITERAL", "T_DIGITS", "T_STRING", "T_END_INPUT", "T_EXPRESSION",
  "T_UNARY_OP", "T_OF", "T_NOT", "T_TYPE_ID", "T_VIRGULE", "T_AFFECT",
  "T_MAPSTO", "T_BEGIN_PAR", "T_END_PAR", "T_PLUS", "T_MOINS", "T_FOIS",
  "T_DIV", "T_MOD", "T_POW", "T_QUOTED_BINARY", "T_QUOTE", "T_PRIME",
  "T_TEST_EQUAL", "T_EQUAL", "T_INTERVAL", "T_UNION", "T_INTERSECT",
  "T_MINUS", "T_AND_OP", "T_COMPOSE", "T_DOLLAR", "T_DOLLAR_MAPLE",
  "T_INDEX_BEGIN", "T_VECT_BEGIN", "T_VECT_DISPATCH", "T_VECT_END",
  "T_SET_BEGIN", "T_SET_END", "T_SEMI", "T_DEUXPOINTS",
  "T_DOUBLE_DEUX_POINTS", "T_IF", "T_RPN_IF", "T_ELIF", "T_THEN", "T_ELSE",
  "T_IFTE", "T_SWITCH", "T_CASE", "T_DEFAULT", "T_ENDCASE", "T_FOR",
  "T_FROM", "T_TO", "T_DO", "T_BY", "T_WHILE", "T_MUPMAP_WHILE",
  "T_RPN_WHILE", "T_REPEAT", "T_UNTIL", "T_IN", "T_START", "T_BREAK",
  "T_CONTINUE", "T_TRY", "T_CATCH", "T_TRY_CATCH", "T_PROC", "T_BLOC",
  "T_BLOC_BEGIN", "T_BLOC_END", "T_RETURN", "T_LOCAL", "T_LOCALBLOC",
  "T_NAME", "T_PROGRAM", "T_NULL", "T_ARGS", "T_FACTORIAL", "T_RPN_OP",
  "T_RPN_BEGIN", "T_RPN_END", "T_STACK", "T_GROUPE_BEGIN", "T_GROUPE_END",
  "T_LINE_BEGIN", "T_LINE_END", "T_VECTOR_BEGIN", "T_VECTOR_END",
  "T_CURVE_BEGIN", "T_CURVE_END", "T_ROOTOF_BEGIN", "T_ROOTOF_END",
  "T_SPOLY1_BEGIN", "T_SPOLY1_END", "T_POLY1_BEGIN", "T_POLY1_END",
  "T_MATRICE_BEGIN", "T_MATRICE_END", "T_ASSUME_BEGIN", "T_ASSUME_END",
  "T_HELP", "TI_DEUXPOINTS", "TI_LOCAL", "TI_LOOP", "TI_FOR", "TI_WHILE",
  "TI_STO", "TI_TRY", "TI_DIALOG", "T_PIPE", "TI_DEFINE", "TI_PRGM",
  "TI_SEMI", "TI_HASH", "T_ACCENTGRAVE", "T_MAPLELIB", "T_INTERROGATION",
  "T_UNIT", "T_BIDON", "T_LOGO", "T_SQ", "T_CASE38", "T_IFERR",
  "T_MOINS38", "T_NEG38", "T_UNARY_OP_38", "T_FUNCTION", "T_IMPMULT",
  "$accept", "input", "correct_input", "exp", "symbol_for", "symbol",
  "symbol_or_literal", "entete", "stack", "local", "nom", "suite_symbol",
  "affectable_symbol", "exp_or_empty", "suite", "prg_suite", "rpn_suite",
  "rpn_token", "step", "from", "loop38_do", "else", "bloc", "elif",
  "ti_bloc_end", "ti_else", "switch", "case", "case38", "semi", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390
};
#endif

#define YYPACT_NINF (-521)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-258)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    8966,  -521,    98,    -4,  -521,   142,  -521,  -521,    23,  -521,
    8966,    81,  8966,  8966,  8966,  -521,  9099,  8966,  6572,  -521,
      91,  8966,  6705,   100,  9232,   102,   122,  9365,    22,  9498,
    8966,  8966,  -521,  -521,    64,   131,   218,   135,  1252,   169,
     192,  -521,    73,  -521,   143,   -13,  8966,  8966,  8966,  8966,
    8966,  8966,  8966,  8966,  6838,   194,  8966,   143,   132,  8966,
    1385,   -11,  8966,  8966,   196,   185,  -521, 10286,   199,  -521,
    -521,  -521,   203,   220,   -23,    62,  8966,  6971,  7104,  8966,
     407,  -521, 10763,   407,   407,    19,  1917, 10790, 13881,  8966,
   14055,  -521, 10401, 10908,   184,  -521,  8966, 10445,  8966,  8966,
    9631, 10327,   186,    46,  -521,    81,  7237,  -521,   170,     4,
    8966, 10967, 11026, 13350,  2981,  3114,   166,  8966,  7370,   226,
    8966,  1518, 13350,  8966,  8966,  8966,  8966,  -521,   157,   136,
    8966,  -521, 11085, 13409, 13350, 13350,   230,  3247, 13350,   140,
   11144,  3380,  3247,  -521,   233,    90,   129,  8966,   724,  7503,
   13881,  8966,  8966,   177,  3513,   407,  8966,  -521,  -521,   201,
    8966,  8966,  6838,  7370,  8966,  8966,  8966,  8966,  8966,  8966,
    -521,  8966,  8966,   853,  8966,  8966,  8966,  8966,  8966,  8966,
    8966,  9764,  7636,  8966,  8966,  -521,   274,  8966,  8966,  8966,
    8966,  -521,  8966,  7370,  8966,  -521,   119,  -521,  -521,  -521,
    -521,  8966,  -521, 13586,  -521, 11203,  -521, 11262, 11321,   242,
    -521,   986,  -521, 13763,    70,  -521, 11380,  6838,  8966, 11439,
   11498,    25,   258,  8966,   207, 11557,   222,  8966,  8966,  8966,
    8966,   153, 11616,  8966,  8966,  -521,  8966, 13350,  -521,  8966,
    7769,   188,  3646,   253, 11675,   257,  7370, 11734, 11793, 11852,
   11911, 11970,  -521,   143,  -521, 12029,  -521,  8966,  7370,  -521,
    7902,  -521,  8966,  8966,  8035,  8168,  -521,  7370,  -521, 12088,
    -521, 12147,  3779,  -521,  8966, 12206,  8966, 13763, 13586, 13940,
    -521,   261, 10363, 10363, 10522, 14077, 14093,   326, 10563, 10401,
   14055, 13967, 14018, 13996,   689,    90,   217, 10401,    17,  6705,
   12265,  -521,  -521, 13881, 10832,  -521,  -521,  -521,  -521,  -521,
    -521,  -521,  8966,    95, 13468,   630, 13822,   724, 10363,   263,
   12324,  -521,   268, 12391,  -521,  -521,  -521,  7370,   186,    86,
     227,    81,    70,  -521,    14,  -521,  1651,  2050,   231, 13350,
    -521,   209,  -521,   232,  3912,  -521,  -521,  7237, 12450, 13350,
   13350, 13350,  8966,  8966,    34,  1784,  4045,  4178, 12509, 12568,
      70,  -521,  4311,   211,  -521,  8966,  -521,   188,   273,  -521,
    -521,  -521,  -521,  -521,  -521,  -521, 13704,   276,  -521,  3247,
    3247,  3247,  -521,  8035,   279,  -521,  8966,  2183,  -521, 10832,
    -521,  1119,  8966, 10486,  -521,   724,  7370,  9897,    -7,   277,
     282,  -521,   285,  8966,  8966,  8966,  8966,   286,    70,  8966,
    7370, 12627,   -44,  8966,  -521,  2316,  -521,  -521,  8966,    64,
      -1,  8966, 13350,   262,  8966, 12701, 13350,  8966,  8966,  8966,
   12760,  -521,  -521,  -521,  -521,  -521,     6,  -521, 12819,  4444,
    2449,  -521,   -10,  -521,  -521,  -521,  3247,  -521,   281,  4577,
    8966,  -521, 10832,   270,   295,  6705, 12878,  8301,   188,   311,
    -521,  -521, 13350, 13645, 13527, 13645,  -521,  -521, 10604, 10763,
     -44,   265,  -521,  6838, 12937,  8966,  -521,  3247,  -521,   315,
     280,   244,  2582,  8434,  2715,    -3, 12996,  4710, 13055,  -521,
    -521,    64,  8966,  4843,   188,  7769,  4976, 10030,  -521,  8567,
     155,  5109,  -521,  -521, 10645,  -521,   202, 13586,  -521,  7769,
    -521,  -521,  8966,  -521, 13114,  -521,  8966, 13173,  -521,   283,
      64,  -521,   258,  -521,   304,  8966,  -521,  -521,  -521,  8966,
    8966,  -521,  8966,  -521,  5242,  -521,  7769,  5375,  -521,  8700,
    2848, 10163, 10401,   -11,  -521,  -521,   288,  7769,  5508, 13232,
    -521,  2050,  8966,    64,  -521,  6838,  5641,  5774,  5907,  6040,
    -521,  6173,  -521,  8966,  6306,  8966,  -521,  8833,  3247,  -521,
    -521,  6439,  -521,  -521,  -521,  2050,    -1, 13291,  -521,  -521,
    -521,  -521,  -521,  -521,   215,  8966,   221,  8966,  -521,  -521,
    -521,  -521,  -521,  8966,   223,  8966,   224,  3247,  8966,  3247,
    8966,  -521,  3247,  -521,  3247,  -521,  -521
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,   127,     6,   188,    31,    32,    13,    14,    68,    58,
       0,    99,     0,     0,     0,   113,     0,     0,     0,   107,
       0,     0,     0,     0,     0,    75,     0,     0,    93,     0,
       0,     0,    85,    86,     0,   159,     0,    81,     0,   133,
      77,   121,    63,   164,   225,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   137,     0,
       0,   256,     0,     0,     0,     0,     2,     0,    30,   128,
     198,   199,     0,     0,     7,     0,     0,     0,     0,     0,
      60,   196,     0,    55,    53,    99,     0,     0,    39,     0,
      49,   105,   101,   221,     0,   194,     0,     0,     0,     0,
       0,   253,     0,   188,   186,     0,     0,   187,     0,   231,
       0,     0,     0,   222,     0,     0,     0,     0,     0,     0,
       0,     0,    82,     0,     0,     0,     0,   228,     0,   225,
       0,   204,     0,     0,   122,   179,    30,     0,   221,     0,
       0,     0,     0,   178,     0,   197,     0,     0,   124,     0,
     129,     0,     0,     0,     0,    54,     0,     1,     3,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      69,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    70,     0,     0,     0,     0,
       0,   126,     0,     0,     0,   195,     0,    10,   191,   190,
     189,     0,   192,    33,    35,     0,    67,     0,     0,   118,
     100,     0,   114,    50,     0,   119,     0,     0,     0,     0,
       0,   188,     0,     0,     0,   219,     0,     0,     0,     0,
       0,   229,     0,     0,     0,   260,     0,   223,   224,     0,
       0,   200,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   136,   225,   226,     0,    57,     0,     0,   246,
       0,   166,     0,     0,     0,     0,   177,     0,   163,     0,
     131,     0,     0,    98,     0,     0,     0,   120,    59,    79,
      78,     0,    40,    41,    43,    44,    46,    45,    37,    38,
      47,   108,   110,   111,    51,   106,   104,   103,    30,     0,
       0,     4,     5,    52,   149,    36,    21,    25,    22,    23,
      24,    26,     0,    20,   112,   172,   123,   125,    42,     0,
       0,     8,     0,     0,    34,    64,    66,     0,   217,   188,
     214,   216,     0,   209,     0,   207,     0,     0,    72,   167,
      74,     0,   161,     0,     0,   162,   148,     0,     0,   232,
     233,   234,     0,     0,     0,     0,     0,     0,    94,     0,
       0,   201,     0,   202,   240,     0,   158,   200,     0,    80,
     132,    76,    61,    62,   227,   203,   120,     0,   247,     0,
       0,     0,   169,     0,     0,   138,     0,     0,    65,   150,
      29,     0,     0,     0,   115,    27,     0,     0,    28,    11,
       0,   193,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   237,     0,   248,     0,    73,   242,     0,     0,
     250,     0,   219,     0,     0,   229,   230,     0,     0,     0,
       0,   153,   155,   156,    95,   206,     0,   241,     0,     0,
       0,    56,    28,   183,   184,   168,     0,   171,     0,     0,
       0,    97,   102,     0,     0,     0,     0,     0,   200,     0,
       9,   117,   210,   211,   212,   215,   213,   208,     0,     0,
     237,     0,   134,     0,     0,     0,   249,     0,    71,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
     205,     0,     0,     0,   200,     0,     0,     0,   170,     0,
     256,     0,   116,    17,     0,    18,   200,    16,    15,     0,
      12,   151,     0,   135,     0,   239,     0,     0,   243,     0,
       0,   160,    58,   255,     0,     0,    87,   235,   236,     0,
       0,    91,     0,   157,     0,   139,     0,     0,   141,     0,
       0,     0,   180,   256,   258,    96,     0,     0,     0,     0,
     238,     0,     0,     0,   251,     0,     0,     0,     0,     0,
     143,     0,   140,     0,     0,     0,   176,     0,     0,   259,
      19,     0,   144,   152,   244,     0,   250,     0,   146,    88,
      89,    90,    92,   142,     0,     0,     0,     0,   182,   145,
     245,   252,   147,     0,     0,     0,     0,     0,     0,     0,
       0,   175,     0,   174,     0,   173,   181
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -521,  -521,   144,     0,  -521,   168,  -521,  -232,  -521,  -521,
    -521,   -29,  -326,  -345,    40,   255,  -126,   278,   -91,  -521,
    -521,  -130,   -17,  -520,    -8,  -401,  -235,  -140,  -485,  -521
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    65,    66,   113,   108,    68,    74,   240,    69,   361,
     241,   334,   335,   226,    94,   114,   128,   129,   354,   231,
     529,   472,   116,   416,   417,   418,   481,   224,   153,   238
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      67,   196,   423,   254,   130,   457,   407,   414,   457,   363,
      80,   473,    82,    83,    84,   544,    87,    88,    90,   228,
     408,    92,    93,    81,    97,   102,   103,   101,   408,   111,
     112,   574,   104,   229,   193,   105,   151,   143,   122,   106,
      78,   527,    75,   342,   152,   210,   132,   133,   134,   135,
     490,   138,   140,   479,   480,   590,   145,   528,   569,   148,
     150,    79,   230,   155,   458,   471,   198,   458,    23,   473,
     131,    75,   199,   328,   329,   200,   203,   205,   207,   208,
     330,   409,   467,   331,   392,    81,   122,   332,   201,   213,
     125,   139,    75,   427,   428,    91,   216,   429,   219,   220,
      82,   403,    70,    71,    95,   197,   225,   163,    72,   497,
     232,   126,   396,  -185,   237,   404,    23,   244,   138,    98,
     247,   135,   321,   248,   249,   250,   251,   374,   181,   261,
     255,   405,    75,   397,   266,   439,   322,   237,   524,    99,
     115,   237,   237,    56,    73,   280,   127,   269,   117,    82,
     253,   271,   120,   127,   237,   107,   275,    76,   245,    77,
     277,   278,   279,   138,   282,   283,   284,   285,   286,   287,
     147,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   300,    67,   303,   304,   157,   123,   314,   315,   316,
     317,    56,   318,   138,   320,   202,   109,   102,     3,   543,
     338,   323,   151,   281,   119,   102,     3,   105,   495,   124,
     152,    87,   352,   156,   353,   105,   193,   136,   339,   118,
     194,   102,     3,   144,   195,   215,   509,   348,   349,   350,
     351,   105,    73,   319,   163,   118,   358,   227,   243,   359,
      23,   169,   237,   246,   170,   252,   138,   258,    23,   262,
     267,   268,   178,  -258,   273,   181,   382,   376,   138,   327,
     135,   343,   536,   345,    23,   135,   347,   138,   276,   239,
     365,   400,   237,   406,   547,   367,   389,   102,     3,   390,
     305,   398,   419,   239,   306,   420,   368,   105,   307,   136,
     360,   440,   421,   308,   442,   309,   310,   448,   377,   393,
     460,   459,   185,   461,   466,   137,   483,   384,   141,   142,
     499,   502,   395,   503,   510,    56,   476,   154,   519,   412,
      23,   521,   555,    56,   593,   520,   302,   138,   553,   570,
     595,   436,   598,   600,   485,   146,   411,   237,   431,    56,
     513,   591,   523,   163,   237,   191,     0,   422,   298,     0,
     169,     0,   425,   426,   313,   430,   237,   237,     0,     0,
       0,   178,   237,     0,   181,   438,     0,   402,     0,     0,
     242,   443,   444,   445,     0,   447,     0,     0,     0,   237,
     237,   237,   333,     0,     0,     0,     0,   237,   311,     0,
       0,   286,   452,     0,     0,    56,   138,   456,     0,   312,
       0,     0,   478,   462,   463,   464,   465,   272,     0,   468,
     469,   185,     0,   474,     0,   135,     0,     0,     0,  -258,
       0,     0,     0,     0,   163,     0,     0,   486,   136,   488,
       0,   169,     0,   136,   170,     0,   454,     0,   498,     0,
     508,     0,   178,   179,     0,   181,   237,     0,     0,   237,
     402,     0,     0,     0,   191,   504,   515,   507,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   518,
       0,     0,   337,   514,   533,   517,     0,   237,   344,     0,
       0,     0,   237,   422,   237,     0,     0,   237,   356,   357,
       0,     0,   185,   237,     0,   362,   237,     0,     0,   542,
     333,   237,     0,   554,     0,     0,     0,     0,     0,     0,
       0,     0,   549,     0,     0,     0,     0,   379,   380,   381,
       0,     0,     0,     0,     0,     0,     0,     0,   333,   387,
       0,     0,   566,     0,   237,   191,   576,   237,   578,   135,
     237,     0,     0,     0,     0,     0,     0,     0,   237,     0,
       0,   237,     0,     0,     0,   577,   237,   237,   237,   237,
     588,   237,     0,   138,   135,   138,     0,   135,   237,     0,
       0,   237,     0,     0,     0,   237,   333,     0,     0,     0,
       0,     0,     0,   136,     0,   138,     0,   138,     0,   601,
       0,   603,     0,     0,   605,     0,   606,   237,     0,   237,
       0,     0,   237,   584,   237,   586,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   594,     0,   596,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   446,     0,
       0,   449,   159,     0,   160,     0,   162,   163,     0,   164,
     165,   166,   167,   168,   169,     0,     0,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,     0,     0,   477,   119,   183,   482,     0,     0,   484,
       0,     0,     0,   487,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   493,   496,     0,   184,     0,     0,
       0,   159,     0,     0,     0,   501,   163,   136,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,     0,   178,   179,   180,   181,     0,     0,
       0,     0,   136,     0,     0,   136,   159,     0,     0,     0,
       0,   163,     0,     0,     0,     0,     0,   534,   169,     0,
     537,   170,   540,     0,   189,   190,   184,     0,   191,   178,
     179,   192,   181,     0,   548,     0,     0,     0,     0,     0,
       0,   551,     0,     0,   185,     0,     0,     0,     0,     0,
     556,     0,     0,     0,   557,   558,     0,   559,     0,     0,
       0,   561,     0,     0,     0,     0,   568,     0,     0,     0,
       0,     0,   571,     0,     0,     0,     0,   575,     0,   185,
       0,     0,     0,     0,   190,     0,     0,   191,     0,     0,
     192,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   597,  -258,
     599,     0,   191,   602,     1,   604,     2,     3,     4,     5,
       6,   -48,     7,     8,     9,    10,    11,   -48,   -48,   -48,
      12,   -48,    13,    14,   -48,   -48,   -48,   -48,    15,    16,
     -48,   -48,   -48,   -48,   -48,   -48,   -48,   -48,    19,    20,
     -48,   -48,     0,    22,   -48,     0,     0,   -48,   -48,    23,
      24,     0,   -48,   -48,   -48,    25,    26,    27,   -48,   -48,
     -48,   -48,   -48,   -48,   -48,    29,    30,     0,    31,   -48,
     -48,     0,    32,    33,    34,     0,    35,    36,    37,     0,
     -48,   -48,     0,    39,     0,    40,    41,    42,   -48,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,   -48,    47,   -48,     0,     0,     0,     0,     0,
       0,    48,   -48,   -48,    50,    51,    52,   -48,    53,    54,
     -48,    55,     0,   -48,    56,    57,    58,   -48,    59,     0,
     -48,   -48,    61,    62,   -48,    63,    64,     1,     0,     2,
       3,     4,     5,     6,   -84,     7,     8,     9,    10,    85,
     -84,   -84,   -84,    12,   -84,    13,    14,   -84,   -84,   -84,
     -84,    15,    16,   -84,   -84,    17,    18,   -84,   -84,   -84,
     -84,    19,    20,    21,   -84,     0,    22,   -84,     0,     0,
     -84,   -84,    23,    24,     0,   -84,   -84,   -84,    25,    26,
      27,   -84,   -84,    28,   -84,   -84,   -84,   -84,    29,    30,
       0,    31,   -84,   -84,     0,    32,    33,    34,     0,    35,
      36,    37,     0,   -84,    86,     0,    39,     0,    40,    41,
      42,   -84,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,   -84,    47,   -84,     0,     0,
       0,     0,     0,     0,    48,    49,   -84,    50,    51,    52,
     -84,    53,    54,   -84,    55,     0,   -84,    56,    57,    58,
     -84,    59,     0,    60,   -84,    61,    62,   -84,    63,    64,
       1,     0,  -109,     3,     4,     5,     6,  -109,     7,     8,
       9,    10,    11,  -109,  -109,  -109,    12,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,    15,    16,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,    19,    20,  -109,  -109,     0,    22,
    -109,     0,     0,  -109,  -109,    23,    24,     0,  -109,  -109,
    -109,    25,    26,    27,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,    29,    30,     0,    31,  -109,  -109,     0,    32,    33,
      34,     0,    35,    36,    37,     0,  -109,  -109,     0,    39,
       0,    40,    41,    42,  -109,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,  -109,    47,
    -109,     0,     0,     0,     0,     0,     0,    48,  -109,  -109,
      50,    51,    52,  -109,    53,    54,  -109,    55,     0,  -109,
      56,    57,    58,  -109,    59,     0,  -109,  -109,    61,    62,
    -109,    63,    64,     1,     0,     2,     3,     4,     5,     6,
     -83,     7,     8,     9,    10,    11,   -83,   -83,   -83,    12,
     -83,    13,    14,   -83,   -83,   -83,   -83,    15,    16,   -83,
     -83,    17,    18,   -83,   -83,   -83,   -83,    19,    20,    21,
     -83,     0,    22,   -83,     0,     0,   -83,   -83,    23,    24,
       0,   -83,   -83,   -83,    25,    26,    27,   -83,   -83,    28,
     -83,   -83,   -83,   -83,    29,    30,     0,    31,   -83,   -83,
       0,    32,    33,    34,     0,    35,    36,    37,     0,   -83,
       0,     0,    39,     0,    40,    41,    42,   -83,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,   -83,    47,   -83,     0,     0,     0,     0,     0,     0,
      48,   -83,   -83,    50,    51,    52,   -83,    53,    54,   -83,
      55,     0,   -83,    56,    57,    58,   -83,    59,     0,    60,
     -83,    61,    62,   -83,    63,    64,     1,     0,     2,     3,
       4,     5,     6,  -130,     7,     8,     9,    10,    11,  -130,
    -130,  -130,   149,  -130,    13,    14,  -130,  -130,  -130,  -130,
      15,    16,  -130,  -130,    17,    18,  -130,  -130,  -130,  -130,
      19,    20,    21,  -130,     0,    22,  -130,     0,     0,  -130,
    -130,    23,    24,     0,  -130,  -130,  -130,    25,    26,    27,
    -130,  -130,  -130,  -130,  -130,  -130,  -130,    29,    30,     0,
      31,  -130,  -130,     0,    32,    33,    34,     0,    35,    36,
      37,     0,  -130,  -130,     0,    39,     0,    40,    41,    42,
    -130,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,  -130,    47,  -130,     0,     0,     0,
       0,     0,     0,    48,  -130,  -130,    50,    51,    52,  -130,
      53,    54,  -130,    55,     0,  -130,    56,    57,    58,  -130,
      59,     0,     0,  -130,    61,    62,  -130,    63,    64,     1,
       0,     2,     3,     4,     5,     6,  -165,     7,     8,     9,
      10,    11,  -165,  -165,  -165,    12,  -165,    13,    14,  -165,
    -165,  -165,  -165,    15,    16,  -165,  -165,    17,    18,  -165,
    -165,  -165,  -165,    19,    20,    21,  -165,     0,    22,  -165,
       0,     0,  -165,  -165,    23,    24,     0,  -165,  -165,  -165,
      25,    26,    27,  -165,  -165,    28,  -165,  -165,  -165,  -165,
      29,    30,     0,    31,  -165,  -165,     0,    32,    33,    34,
       0,    35,    36,    37,     0,  -165,    38,     0,    39,     0,
      40,    41,    42,  -165,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,  -165,    47,  -165,
       0,     0,     0,     0,     0,     0,    48,     0,  -165,    50,
      51,    52,  -165,    53,    54,  -165,    55,     0,  -165,    56,
      57,    58,  -165,    59,     0,    60,  -165,    61,    62,  -165,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,  -118,  -118,  -118,   410,     0,
      13,    14,  -118,  -118,  -118,  -118,    15,    16,  -118,  -118,
      17,    18,  -118,  -118,  -118,  -118,    19,    20,    21,  -118,
       0,    22,     0,     0,     0,     0,  -118,    23,    24,     0,
       0,  -118,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,  -118,     0,
      32,    33,    34,     0,    35,    36,    37,   115,     0,    38,
       0,    39,     0,    40,    41,    42,  -118,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,  -118,    53,    54,  -118,    55,
       0,  -118,    56,    57,    58,  -118,    59,     0,    60,  -118,
      61,    62,  -118,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,  -118,  -118,
    -118,   410,     0,    13,    14,  -118,  -118,  -118,  -118,    15,
      16,  -118,  -118,    17,    18,  -118,  -118,  -118,  -118,    19,
      20,    21,  -118,     0,    22,     0,     0,     0,     0,  -118,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,  -118,     0,    29,    30,     0,    31,
       0,  -118,     0,    32,    33,    34,     0,    35,    36,    37,
     115,     0,    38,     0,    39,     0,    40,    41,    42,  -118,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,  -118,    53,
      54,  -118,    55,     0,  -118,    56,    57,    58,  -118,    59,
       0,    60,  -118,    61,    62,  -118,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,   -83,   -83,   -83,    12,     0,    13,    14,   -83,   -83,
     -83,   -83,    15,   211,   -83,   -83,    17,    18,   -83,   -83,
     -83,   -83,    19,    20,    21,   -83,     0,    22,     0,     0,
       0,     0,   -83,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,   -83,     0,    32,    33,    34,     0,
      35,    36,    37,     0,     0,    38,     0,    39,     0,    40,
      41,    42,   -83,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,   121,     0,    50,    51,
      52,   -83,    53,    54,   -83,    55,     0,   -83,    56,    57,
      58,   -83,    59,     0,    60,   -83,    61,    62,   -83,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,   235,     0,    23,    24,     0,   413,
       0,   414,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,   259,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,   415,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,   235,     0,    23,
      24,     0,     0,     0,   450,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
     451,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
       0,     0,    23,    24,     0,   475,     0,   476,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,   378,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,     0,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    11,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,     0,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,   494,     0,    38,  -200,    39,
     239,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,   522,    10,    11,     0,     0,     0,    12,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,   235,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,   223,  -253,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,     0,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,    12,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,     0,     0,     0,   235,
       0,    23,    24,     0,     0,     0,   525,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,   526,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,    49,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,    12,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,   235,     0,    23,    24,     0,     0,     0,     0,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,     0,   259,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,   564,   565,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,     0,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,   235,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,   236,     0,     0,
      32,    33,    34,     0,    35,    36,    37,     0,     0,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,     0,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,    22,     0,     0,     0,     0,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
       0,     0,    38,  -200,    39,   239,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,    22,     0,     0,
       0,   235,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,   259,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,   260,     0,    50,    51,
      52,     0,    53,    54,     0,    55,     0,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,   235,     0,    23,    24,     0,     0,
       0,   264,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,     0,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,   265,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,   235,     0,    23,
      24,     0,     0,   274,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
       0,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
     235,     0,    23,    24,     0,     0,     0,     0,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,   364,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,     0,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    11,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,   235,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,     0,  -257,    38,     0,    39,
       0,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,     0,     0,     0,    12,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,   235,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,     0,  -254,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,     0,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,    12,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,     0,     0,     0,   235,
       0,    23,    24,     0,     0,     0,     0,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,   432,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,    49,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,    12,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,   235,     0,    23,    24,     0,     0,     0,     0,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,     0,   433,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,    49,     0,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,     0,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,   235,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,     0,     0,
      32,    33,    34,     0,    35,    36,    37,     0,   437,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,     0,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,    22,     0,     0,     0,     0,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
     492,     0,    38,   360,    39,     0,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,    22,     0,     0,
       0,   235,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,   500,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,    50,    51,
      52,     0,    53,    54,     0,    55,     0,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,   235,     0,    23,    24,     0,     0,
       0,     0,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,   531,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,    49,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,   235,     0,    23,
      24,     0,     0,     0,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
     535,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
     235,     0,    23,    24,     0,     0,     0,     0,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,   538,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,     0,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    11,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,   235,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,     0,   545,    38,     0,    39,
       0,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,     0,     0,     0,    12,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,   235,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,     0,     0,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,   560,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,    12,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,     0,     0,     0,   235,
       0,    23,    24,     0,     0,     0,     0,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,   562,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,    49,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,    12,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,   235,     0,    23,    24,     0,     0,     0,     0,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,     0,   572,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,    49,     0,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,     0,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,   235,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,     0,     0,
      32,    33,    34,     0,    35,    36,    37,     0,   579,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,     0,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,    22,     0,     0,     0,   235,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
       0,   580,    38,     0,    39,     0,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,    22,     0,     0,
       0,   235,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,   581,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,    50,    51,
      52,     0,    53,    54,     0,    55,     0,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,   235,     0,    23,    24,     0,     0,
       0,     0,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,   582,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,    49,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,   235,     0,    23,
      24,     0,     0,     0,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
     583,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
       0,     0,    23,    24,     0,     0,     0,     0,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,   378,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,   585,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    11,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,   235,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,     0,   589,    38,     0,    39,
       0,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,    89,     0,     0,    12,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,     0,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,     0,     0,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,     0,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,    12,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,  -220,     0,     0,     0,
       0,    23,    24,     0,     0,     0,     0,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,     0,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,    49,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,    12,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,     0,     0,    23,    24,     0,     0,     0,     0,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,   115,     0,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,    49,     0,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,   204,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,     0,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,     0,     0,
      32,    33,    34,     0,    35,    36,    37,     0,     0,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,   206,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,    22,     0,     0,     0,     0,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
       0,     0,    38,     0,    39,     0,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,    22,     0,     0,
       0,  -218,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,     0,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,    50,    51,
      52,     0,    53,    54,     0,    55,     0,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,  -220,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,     0,     0,    23,    24,     0,     0,
       0,     0,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,     0,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,    49,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,   270,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,     0,     0,    23,
      24,     0,     0,     0,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
       0,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,   301,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
       0,     0,    23,    24,     0,     0,     0,     0,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,     0,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,     0,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    11,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,     0,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,     0,     0,    38,   360,    39,
       0,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,     0,     0,     0,    12,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,     0,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,     0,     0,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,   378,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,    12,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,     0,     0,     0,     0,
       0,    23,    24,     0,     0,     0,     0,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,   259,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,   260,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,    12,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,     0,     0,    23,    24,     0,     0,     0,   383,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,     0,     0,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,    49,     0,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,     3,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,     0,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,     0,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,     0,     0,
      32,    33,    34,     0,    35,   506,    37,   115,     0,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,  -218,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,    22,     0,     0,     0,     0,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
       0,     0,    38,     0,    39,     0,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,    22,     0,     0,
       0,     0,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,     0,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,    50,    51,
      52,     0,    53,    54,     0,    55,   541,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,     0,     0,    23,    24,     0,     0,
       0,     0,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,     0,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,    49,
     563,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,     0,     0,    23,
      24,     0,     0,     0,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
       0,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,    49,   587,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,     0,    63,    64,     1,     0,     2,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
       0,     0,     0,    12,     0,    13,    14,     0,     0,     0,
       0,    15,    16,     0,     0,    17,    18,     0,     0,     0,
       0,    19,    20,    21,     0,     0,    22,     0,     0,     0,
       0,     0,    23,    24,     0,     0,     0,     0,    25,    26,
      27,     0,     0,    28,     0,     0,     0,     0,    29,    30,
       0,    31,     0,     0,     0,    32,    33,    34,     0,    35,
      36,    37,     0,     0,    38,     0,    39,     0,    40,    41,
      42,     0,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,    46,     0,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,     0,    50,    51,    52,
       0,    53,    54,     0,    55,     0,     0,    56,    57,    58,
       0,    59,     0,    60,     0,    61,    62,     0,    63,    64,
       1,     0,     2,     3,     4,     5,     6,     0,     7,     8,
       9,    10,    85,     0,     0,     0,    12,     0,    13,    14,
       0,     0,     0,     0,    15,    16,     0,     0,    17,    18,
       0,     0,     0,     0,    19,    20,    21,     0,     0,    22,
       0,     0,     0,     0,     0,    23,    24,     0,     0,     0,
       0,    25,    26,    27,     0,     0,    28,     0,     0,     0,
       0,    29,    30,     0,    31,     0,     0,     0,    32,    33,
      34,     0,    35,    36,    37,     0,     0,    86,     0,    39,
       0,    40,    41,    42,     0,    43,    44,     0,    45,     0,
       0,     0,     0,     0,     0,     0,     0,    46,     0,    47,
       0,     0,     0,     0,     0,     0,     0,    48,    49,     0,
      50,    51,    52,     0,    53,    54,     0,    55,     0,     0,
      56,    57,    58,     0,    59,     0,    60,     0,    61,    62,
       0,    63,    64,     1,     0,     2,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,     0,     0,     0,    96,
       0,    13,    14,     0,     0,     0,     0,    15,    16,     0,
       0,    17,    18,     0,     0,     0,     0,    19,    20,    21,
       0,     0,    22,     0,     0,     0,     0,     0,    23,    24,
       0,     0,     0,     0,    25,    26,    27,     0,     0,    28,
       0,     0,     0,     0,    29,    30,     0,    31,     0,     0,
       0,    32,    33,    34,     0,    35,    36,    37,     0,     0,
      38,     0,    39,     0,    40,    41,    42,     0,    43,    44,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
      46,     0,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,     0,    50,    51,    52,     0,    53,    54,     0,
      55,     0,     0,    56,    57,    58,     0,    59,     0,    60,
       0,    61,    62,     0,    63,    64,     1,     0,     2,     3,
       4,     5,     6,     0,     7,     8,     9,    10,    11,     0,
       0,     0,   100,     0,    13,    14,     0,     0,     0,     0,
      15,    16,     0,     0,    17,    18,     0,     0,     0,     0,
      19,    20,    21,     0,     0,    22,     0,     0,     0,     0,
       0,    23,    24,     0,     0,     0,     0,    25,    26,    27,
       0,     0,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,     0,     0,    32,    33,    34,     0,    35,    36,
      37,     0,     0,    38,     0,    39,     0,    40,    41,    42,
       0,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,    46,     0,    47,     0,     0,     0,     0,
       0,     0,     0,    48,    49,     0,    50,    51,    52,     0,
      53,    54,     0,    55,     0,     0,    56,    57,    58,     0,
      59,     0,    60,     0,    61,    62,     0,    63,    64,     1,
       0,     2,     3,     4,     5,     6,     0,     7,     8,     9,
      10,    11,     0,     0,     0,   110,     0,    13,    14,     0,
       0,     0,     0,    15,    16,     0,     0,    17,    18,     0,
       0,     0,     0,    19,    20,    21,     0,     0,    22,     0,
       0,     0,     0,     0,    23,    24,     0,     0,     0,     0,
      25,    26,    27,     0,     0,    28,     0,     0,     0,     0,
      29,    30,     0,    31,     0,     0,     0,    32,    33,    34,
       0,    35,    36,    37,     0,     0,    38,     0,    39,     0,
      40,    41,    42,     0,    43,    44,     0,    45,     0,     0,
       0,     0,     0,     0,     0,     0,    46,     0,    47,     0,
       0,     0,     0,     0,     0,     0,    48,    49,     0,    50,
      51,    52,     0,    53,    54,     0,    55,     0,     0,    56,
      57,    58,     0,    59,     0,    60,     0,    61,    62,     0,
      63,    64,     1,     0,     2,   221,     4,     5,     6,     0,
       7,     8,     9,    10,    11,     0,     0,     0,    12,     0,
      13,    14,     0,     0,     0,     0,    15,    16,     0,     0,
      17,    18,     0,     0,     0,     0,    19,    20,    21,     0,
       0,    22,     0,     0,     0,     0,     0,    23,    24,     0,
       0,     0,     0,    25,    26,    27,     0,     0,    28,     0,
       0,     0,     0,    29,    30,     0,    31,     0,     0,     0,
      32,    33,    34,     0,    35,    36,    37,     0,     0,    38,
       0,    39,     0,    40,    41,    42,     0,    43,    44,     0,
      45,     0,     0,     0,     0,     0,     0,     0,     0,    46,
       0,    47,     0,     0,     0,     0,     0,     0,     0,    48,
      49,     0,    50,    51,    52,     0,    53,    54,     0,    55,
       0,     0,    56,    57,    58,     0,    59,     0,    60,     0,
      61,    62,     0,    63,    64,     1,     0,     2,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,     0,     0,
       0,    12,     0,    13,    14,     0,     0,     0,     0,    15,
      16,     0,     0,    17,    18,     0,     0,     0,     0,    19,
      20,    21,     0,     0,   299,     0,     0,     0,     0,     0,
      23,    24,     0,     0,     0,     0,    25,    26,    27,     0,
       0,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,     0,     0,    32,    33,    34,     0,    35,    36,    37,
       0,     0,    38,     0,    39,     0,    40,    41,    42,     0,
      43,    44,     0,    45,     0,     0,     0,     0,     0,     0,
       0,     0,    46,     0,    47,     0,     0,     0,     0,     0,
       0,     0,    48,    49,     0,    50,    51,    52,     0,    53,
      54,     0,    55,     0,     0,    56,    57,    58,     0,    59,
       0,    60,     0,    61,    62,     0,    63,    64,     1,     0,
       2,     3,     4,     5,     6,     0,     7,     8,     9,    10,
      11,     0,     0,     0,    12,     0,    13,    14,     0,     0,
       0,     0,    15,    16,     0,     0,    17,    18,     0,     0,
       0,     0,    19,    20,    21,     0,     0,   455,     0,     0,
       0,     0,     0,    23,    24,     0,     0,     0,     0,    25,
      26,    27,     0,     0,    28,     0,     0,     0,     0,    29,
      30,     0,    31,     0,     0,     0,    32,    33,    34,     0,
      35,    36,    37,     0,     0,    38,     0,    39,     0,    40,
      41,    42,     0,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,    46,     0,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,    50,    51,
      52,     0,    53,    54,     0,    55,     0,     0,    56,    57,
      58,     0,    59,     0,    60,     0,    61,    62,     0,    63,
      64,     1,     0,     2,     3,     4,     5,     6,     0,     7,
       8,     9,    10,    11,     0,     0,     0,    12,     0,    13,
      14,     0,     0,     0,     0,    15,    16,     0,     0,    17,
      18,     0,     0,     0,     0,    19,    20,    21,     0,     0,
      22,     0,     0,     0,     0,     0,    23,    24,     0,     0,
       0,     0,    25,    26,    27,     0,     0,    28,     0,     0,
       0,     0,    29,    30,     0,    31,     0,     0,     0,    32,
      33,    34,     0,    35,    36,    37,     0,     0,    38,     0,
      39,     0,    40,    41,    42,     0,    43,    44,     0,    45,
       0,     0,     0,     0,     0,     0,     0,     0,    46,     0,
      47,     0,     0,     0,     0,     0,     0,     0,    48,   539,
       0,    50,    51,    52,     0,    53,    54,     0,    55,     0,
       0,    56,    57,    58,     0,    59,     0,    60,     0,    61,
      62,     0,    63,    64,     1,     0,     2,     3,     4,     5,
       6,     0,     7,     8,     9,    10,    11,     0,     0,     0,
      12,     0,    13,    14,     0,     0,     0,     0,    15,    16,
       0,     0,    17,    18,     0,     0,     0,     0,    19,    20,
      21,     0,     0,    22,     0,     0,     0,     0,     0,    23,
      24,     0,     0,     0,     0,    25,    26,    27,     0,     0,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
       0,     0,    32,    33,    34,     0,    35,    36,    37,     0,
       0,    38,     0,    39,     0,    40,    41,    42,     0,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,    46,     0,    47,     0,     0,     0,     0,     0,     0,
       0,    48,   567,     0,    50,    51,    52,     0,    53,    54,
       0,    55,     0,     0,    56,    57,    58,     0,    59,     0,
      60,     0,    61,    62,   158,    63,    64,     0,   159,     0,
     160,   161,   162,   163,     0,   164,   165,   166,   167,   168,
     169,     0,     0,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
     182,   183,     0,     0,     0,     0,     0,     0,   222,   159,
       0,   160,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   184,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,   185,   183,     0,     0,   159,     0,     0,     0,     0,
     163,     0,   223,     0,   166,   167,   168,   169,     0,     0,
     170,     0,     0,     0,   184,     0,     0,     0,   178,   179,
     186,   181,     0,   187,     0,     0,   188,     0,     0,     0,
     189,   190,   185,   159,   191,     0,     0,   192,   163,     0,
     164,   165,   166,   167,   168,   169,     0,     0,   170,   171,
     172,   173,   174,   175,   176,     0,   178,   179,     0,   181,
       0,   186,     0,     0,   187,     0,     0,   188,   185,     0,
       0,   189,   190,     0,     0,   191,     0,   159,   192,   160,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,     0,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,   185,     0,   190,     0,
     183,   191,     0,     0,     0,   217,     0,     0,   159,     0,
     160,   161,   162,   163,     0,   164,   165,   166,   167,   168,
     169,     0,   184,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,   190,   453,     0,   191,
     185,   183,   192,     0,   159,     0,     0,     0,     0,   163,
       0,     0,     0,   214,   167,   168,   169,     0,     0,   170,
       0,     0,     0,   184,   218,     0,     0,   178,   179,   186,
     181,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   185,     0,   191,     0,   159,   192,     0,     0,     0,
     163,     0,   164,   165,   166,   167,   168,   169,     0,     0,
     170,     0,     0,   173,   174,   175,   176,     0,   178,   179,
     186,   181,     0,   187,     0,     0,   188,   185,     0,     0,
     189,   190,     0,     0,   191,     0,   159,   192,   160,   161,
     162,   163,     0,   164,   165,   166,   167,   168,   169,     0,
       0,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,     0,   511,     0,   190,   185,   183,
     191,   512,     0,     0,     0,     0,     0,   159,     0,   160,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   184,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,   546,     0,   190,   185,
     183,   191,     0,     0,   192,     0,     0,     0,     0,     0,
       0,     0,   214,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,     0,     0,     0,     0,   186,     0,
       0,   187,     0,     0,   188,     0,     0,     0,   189,   190,
     185,     0,   191,     0,     0,   192,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,     0,     0,   191,     0,   159,   192,   160,   161,   162,
     163,   209,   164,   165,   166,   167,   168,   169,     0,     0,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   159,     0,   160,   161,   162,   163,   183,   164,
     165,   166,   167,   168,   169,     0,   212,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
     184,     0,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,     0,   159,     0,     0,     0,   185,   163,
       0,   164,   165,   166,   167,   168,   169,   184,     0,   170,
     171,   172,   173,   174,   175,   176,     0,   178,   179,   180,
     181,     0,     0,     0,     0,   185,     0,   186,     0,     0,
     187,     0,     0,   188,     0,     0,     0,   189,   190,     0,
       0,   191,     0,     0,   192,     0,     0,     0,     0,  -258,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,     0,   185,   191,     0,
     159,   192,   160,   161,   162,   163,     0,   164,   165,   166,
     167,   168,   169,     0,     0,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,     0,
       0,     0,     0,   183,     0,     0,     0,   190,     0,     0,
     191,     0,     0,   192,     0,   214,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   184,     0,     0,     0,   159,
       0,   160,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   186,     0,     0,   187,     0,   233,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,   159,   192,
     160,   161,   162,   163,     0,   164,   165,   166,   167,   168,
     169,     0,   185,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     0,     0,   187,     0,   234,   188,     0,     0,
       0,   189,   190,   184,     0,   191,     0,   159,   192,   160,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   185,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,     0,     0,     0,     0,
     183,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     186,     0,     0,   187,     0,     0,   188,     0,     0,     0,
     189,   190,   184,     0,   191,     0,   159,   192,   160,   161,
     162,   163,     0,   164,   165,   166,   167,   168,   169,     0,
     185,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,   256,     0,     0,     0,     0,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   184,     0,   191,     0,   159,   192,   160,   161,   162,
     163,   324,   164,   165,   166,   167,   168,   169,     0,   185,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,     0,     0,     0,     0,     0,     0,   183,     0,
       0,     0,     0,   263,     0,     0,     0,     0,   186,     0,
       0,   187,     0,     0,   188,     0,     0,     0,   189,   190,
     184,     0,   191,     0,   159,   192,   160,   161,   162,   163,
     325,   164,   165,   166,   167,   168,   169,     0,   185,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,     0,     0,     0,     0,     0,     0,   183,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
     187,     0,     0,   188,     0,     0,     0,   189,   190,   184,
       0,   191,     0,   159,   192,   160,   161,   162,   163,     0,
     164,   165,   166,   167,   168,   169,     0,   185,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,     0,   326,     0,     0,     0,   183,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   186,     0,     0,   187,
       0,     0,   188,     0,     0,     0,   189,   190,   184,     0,
     191,     0,   159,   192,   160,   161,   162,   163,   336,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,     0,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,     0,     0,   187,     0,
       0,   188,     0,     0,     0,   189,   190,   184,     0,   191,
       0,   159,   192,   160,   161,   162,   163,   340,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,     0,     0,
       0,     0,     0,     0,   183,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,   184,     0,   191,     0,
     159,   192,   160,   161,   162,   163,   341,   164,   165,   166,
     167,   168,   169,     0,   185,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,     0,
       0,     0,     0,   183,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   186,     0,     0,   187,     0,     0,   188,
       0,     0,     0,   189,   190,   184,     0,   191,     0,   159,
     192,   160,   161,   162,   163,   346,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   186,     0,     0,   187,     0,     0,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,   159,   192,
     160,   161,   162,   163,   355,   164,   165,   166,   167,   168,
     169,     0,   185,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     0,     0,   187,     0,     0,   188,     0,     0,
       0,   189,   190,   184,     0,   191,     0,   159,   192,   160,
     161,   162,   163,   366,   164,   165,   166,   167,   168,   169,
       0,   185,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,     0,     0,     0,     0,
     183,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     186,     0,     0,   187,     0,     0,   188,     0,     0,     0,
     189,   190,   184,     0,   191,     0,   159,   192,   160,   161,
     162,   163,   369,   164,   165,   166,   167,   168,   169,     0,
     185,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,     0,     0,     0,     0,     0,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   184,     0,   191,     0,   159,   192,   160,   161,   162,
     163,   370,   164,   165,   166,   167,   168,   169,     0,   185,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,     0,     0,     0,     0,     0,     0,   183,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   186,     0,
       0,   187,     0,     0,   188,     0,     0,     0,   189,   190,
     184,     0,   191,     0,   159,   192,   160,   161,   162,   163,
     371,   164,   165,   166,   167,   168,   169,     0,   185,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,     0,     0,     0,     0,     0,     0,   183,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
     187,     0,     0,   188,     0,     0,     0,   189,   190,   184,
       0,   191,     0,   159,   192,   160,   161,   162,   163,   372,
     164,   165,   166,   167,   168,   169,     0,   185,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,     0,     0,     0,     0,     0,   183,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   186,     0,     0,   187,
       0,     0,   188,     0,     0,     0,   189,   190,   184,     0,
     191,     0,   159,   192,   160,   161,   162,   163,     0,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,   373,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,     0,     0,   187,     0,
       0,   188,     0,     0,     0,   189,   190,   184,     0,   191,
       0,   159,   192,   160,   161,   162,   163,   375,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,     0,     0,
       0,     0,     0,     0,   183,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,   184,     0,   191,     0,
     159,   192,   160,   161,   162,   163,     0,   164,   165,   166,
     167,   168,   169,     0,   185,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,   385,
       0,     0,     0,   183,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   186,     0,     0,   187,     0,     0,   188,
       0,     0,     0,   189,   190,   184,     0,   191,     0,   159,
     192,   160,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,   386,     0,     0,
       0,     0,   186,     0,     0,   187,     0,     0,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,   159,   192,
     160,   161,   162,   163,   388,   164,   165,   166,   167,   168,
     169,     0,   185,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     0,     0,   187,     0,     0,   188,     0,     0,
       0,   189,   190,   184,     0,   191,     0,   159,   192,   160,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   185,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,   394,     0,     0,     0,
     183,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     186,     0,     0,   187,     0,     0,   188,     0,     0,     0,
     189,   190,   184,     0,   191,     0,   159,   192,   160,   161,
     162,   163,   399,   164,   165,   166,   167,   168,   169,     0,
     185,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,     0,     0,     0,     0,     0,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   184,     0,   191,     0,     0,   192,     0,     0,     0,
       0,     0,     0,   159,     0,   160,   161,   162,   163,   185,
     164,   165,   166,   167,   168,   169,     0,   401,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,     0,     0,     0,     0,     0,   183,     0,   186,     0,
       0,   187,     0,     0,   188,     0,     0,     0,   189,   190,
       0,     0,   191,     0,     0,   192,     0,     0,   184,     0,
       0,     0,   159,     0,   160,   161,   162,   163,     0,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,     0,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,     0,     0,   187,     0,
     424,   188,     0,     0,     0,   189,   190,   184,     0,   191,
       0,   159,   192,   160,   161,   162,   163,     0,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,     0,     0,
       0,     0,     0,     0,   183,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,   184,     0,   191,     0,
     159,   192,   160,   161,   162,   163,   434,   164,   165,   166,
     167,   168,   169,     0,   185,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,     0,
       0,     0,   435,   183,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   186,     0,     0,   187,     0,     0,   188,
       0,     0,     0,   189,   190,   184,     0,   191,     0,   159,
     192,   160,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,   470,   183,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   186,     0,     0,   187,     0,     0,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,     0,   192,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   185,   159,     0,   160,   161,   162,   163,     0,
     164,   165,   166,   167,   168,   169,     0,     0,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,   186,     0,     0,   187,     0,   183,   188,     0,     0,
       0,   189,   190,     0,     0,   191,     0,     0,   192,     0,
       0,     0,   353,     0,     0,     0,     0,     0,   184,     0,
       0,     0,   159,     0,   160,   161,   162,   163,     0,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,     0,     0,     0,   489,   183,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,     0,     0,   187,     0,
       0,   188,     0,     0,     0,   189,   190,   184,     0,   191,
       0,   159,   192,   160,   161,   162,   163,   491,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,     0,     0,
       0,     0,     0,     0,   183,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,   184,     0,   191,     0,
     159,   192,   160,   161,   162,   163,     0,   164,   165,   166,
     167,   168,   169,     0,   185,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,   505,
       0,     0,     0,   183,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   186,     0,     0,   187,     0,     0,   188,
       0,     0,     0,   189,   190,   184,     0,   191,     0,   159,
     192,   160,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,   516,     0,     0,
       0,     0,   186,     0,     0,   187,     0,     0,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,   159,   192,
     160,   161,   162,   163,     0,   164,   165,   166,   167,   168,
     169,     0,   185,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     0,     0,   187,     0,   530,   188,     0,     0,
       0,   189,   190,   184,     0,   191,     0,   159,   192,   160,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   185,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,     0,     0,     0,     0,
     183,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     186,     0,     0,   187,     0,   532,   188,     0,     0,     0,
     189,   190,   184,     0,   191,     0,   159,   192,   160,   161,
     162,   163,     0,   164,   165,   166,   167,   168,   169,     0,
     185,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,     0,     0,     0,     0,   550,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   184,     0,   191,     0,   159,   192,   160,   161,   162,
     163,     0,   164,   165,   166,   167,   168,   169,     0,   185,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,     0,     0,     0,     0,     0,     0,   183,     0,
       0,     0,     0,   552,     0,     0,     0,     0,   186,     0,
       0,   187,     0,     0,   188,     0,     0,     0,   189,   190,
     184,     0,   191,     0,   159,   192,   160,   161,   162,   163,
       0,   164,   165,   166,   167,   168,   169,     0,   185,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,     0,     0,   573,     0,     0,     0,   183,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
     187,     0,     0,   188,     0,     0,     0,   189,   190,   184,
       0,   191,     0,   159,   192,   160,   161,   162,   163,     0,
     164,   165,   166,   167,   168,   169,     0,   185,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,     0,     0,     0,     0,   592,   183,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   186,     0,     0,   187,
       0,     0,   188,     0,     0,     0,   189,   190,   184,     0,
     191,     0,   159,   192,   160,   161,   162,   163,     0,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,     0,
       0,     0,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   186,     0,     0,   187,     0,
       0,   188,     0,     0,     0,   189,   190,   184,     0,   191,
       0,   159,   192,   257,   161,   162,   163,     0,   164,   165,
     166,   167,   168,   169,     0,   185,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,     0,     0,
       0,     0,     0,     0,   183,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   186,     0,     0,   187,     0,     0,
     188,     0,     0,     0,   189,   190,   184,     0,   191,     0,
     159,   192,   160,   161,   162,   163,     0,   164,   165,   166,
     167,   168,   169,     0,   185,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,     0,     0,     0,
       0,     0,     0,   183,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   186,     0,     0,   187,     0,     0,   188,
       0,     0,     0,   189,   190,   184,     0,   191,     0,   159,
     192,     0,   161,   162,   163,     0,   164,   165,   166,   167,
     168,   169,     0,   185,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,     0,     0,     0,     0,
       0,     0,   183,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -258,     0,     0,   188,     0,
       0,     0,   189,   190,   184,     0,   191,     0,   159,   192,
     160,   161,   162,   163,     0,   164,   165,   166,   167,   168,
     169,     0,   185,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,     0,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     0,     0,   187,     0,     0,   188,     0,     0,
       0,   189,   190,   184,     0,   191,     0,   159,   192,     0,
     161,   162,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   185,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,     0,     0,     0,     0,
     183,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
     189,   190,     0,     0,   191,     0,   159,   192,     0,     0,
     162,   163,     0,   164,   165,   166,   167,   168,   169,     0,
     185,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,     0,     0,     0,     0,     0,     0,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   186,
       0,     0,   187,     0,     0,   188,     0,     0,     0,   189,
     190,   184,     0,   191,     0,   159,   192,     0,     0,   162,
     163,     0,   164,   165,   166,   167,   168,   169,     0,   185,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,     0,     0,     0,   441,     0,     0,   183,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   189,   190,
     184,     0,   191,     0,   159,   192,     0,     0,   162,   163,
       0,   164,   165,   166,   167,   168,   169,     0,   185,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,     0,     0,     0,     0,     0,     0,   183,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   189,   190,   184,
       0,   191,     0,   159,   192,     0,     0,   162,   163,     0,
     164,   165,   166,   167,   168,   169,     0,   185,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
       0,     0,     0,     0,     0,     0,   183,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -258,   190,   184,     0,
     191,     0,   159,   192,     0,     0,  -258,   163,     0,   164,
     165,   166,   167,   168,   169,     0,   185,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   159,
       0,     0,     0,     0,   163,     0,   164,   165,   166,   167,
     391,   169,     0,     0,   170,     0,     0,   173,     0,   175,
     176,     0,   178,   179,     0,   181,   190,   184,   159,   191,
       0,     0,   192,   163,     0,   164,   165,   166,   167,   168,
     169,     0,     0,   170,     0,   185,   173,     0,   175,  -258,
     159,   178,   179,     0,   181,   163,     0,   164,   165,   166,
     167,   168,   169,     0,     0,   170,     0,     0,   173,     0,
       0,     0,   185,   178,   179,     0,   181,     0,     0,     0,
       0,     0,     0,     0,     0,   190,     0,   159,   191,     0,
       0,   192,   163,     0,   164,   165,   166,   167,   168,   169,
       0,   185,   170,     0,     0,     0,     0,     0,     0,   159,
     178,   179,   190,   181,   163,   191,     0,     0,   192,     0,
     168,   169,     0,   185,   170,   159,     0,     0,     0,     0,
     163,     0,   178,   179,     0,   181,  -258,   169,     0,     0,
     170,   190,     0,     0,   191,     0,     0,   192,   178,   179,
       0,   181,     0,     0,     0,     0,     0,     0,     0,     0,
     185,     0,     0,   190,     0,     0,   191,     0,     0,   192,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   185,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   185,     0,
     190,     0,     0,   191,     0,     0,   192,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   190,     0,     0,   191,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   190,     0,
       0,   191
};

static const yytype_int16 yycheck[] =
{
       0,    24,   347,   129,    17,    15,   332,    51,    15,   241,
      10,   412,    12,    13,    14,   500,    16,    17,    18,    15,
      14,    21,    22,     4,    24,     3,     4,    27,    14,    29,
      30,   551,    10,    29,    17,    13,    47,    54,    38,    17,
      17,    44,    46,    18,    55,    26,    46,    47,    48,    49,
      44,    51,    52,    54,    55,   575,    56,    60,   543,    59,
      60,    38,    58,    63,    74,   109,     4,    74,    46,   470,
      83,    46,    10,     3,     4,    13,    76,    77,    78,    79,
      10,    67,   408,    13,    67,     4,    86,    17,    26,    89,
      17,    51,    46,    59,    60,     4,    96,    63,    98,    99,
     100,    15,     4,     5,     4,   128,   106,    17,    10,   119,
     110,    38,    17,    67,   114,    29,    46,   117,   118,    17,
     120,   121,     3,   123,   124,   125,   126,   253,    38,   137,
     130,    45,    46,    38,   142,   367,    17,   137,   483,    17,
      76,   141,   142,   121,    46,   162,    10,   147,    17,   149,
      14,   151,    17,    10,   154,   133,   156,    15,   118,    17,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
      38,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,     0,    17,   187,   188,   189,
     190,   121,   192,   193,   194,   133,    28,     3,     4,    44,
     217,   201,    47,   163,    36,     3,     4,    13,   440,    17,
      55,   211,    59,    17,    61,    13,    17,    49,   218,    17,
      17,     3,     4,    55,     4,    41,   458,   227,   228,   229,
     230,    13,    46,   193,    17,    17,   236,    67,    72,   239,
      46,    24,   242,    17,    27,    88,   246,    17,    46,   109,
      17,   122,    35,    36,    77,    38,   264,   257,   258,    17,
     260,     3,   494,    56,    46,   265,    44,   267,    67,    81,
      17,     3,   272,    46,   506,    18,   276,     3,     4,    18,
       6,    18,    51,    81,    10,    76,   246,    13,    14,   121,
      79,    18,    60,    19,    18,    21,    22,    18,   258,   299,
      18,    24,    85,    18,    18,    50,    44,   267,    53,    54,
      29,    41,   312,    18,     3,   121,    51,    62,     3,   336,
      46,    77,    18,   121,   109,    45,   182,   327,    45,    41,
     109,   360,   109,   109,   425,    57,   336,   337,   355,   121,
     470,   576,   482,    17,   344,   128,    -1,   347,   180,    -1,
      24,    -1,   352,   353,   186,   355,   356,   357,    -1,    -1,
      -1,    35,   362,    -1,    38,   365,    -1,   327,    -1,    -1,
     115,   379,   380,   381,    -1,   383,    -1,    -1,    -1,   379,
     380,   381,   214,    -1,    -1,    -1,    -1,   387,   114,    -1,
      -1,   391,   392,    -1,    -1,   121,   396,   397,    -1,   125,
      -1,    -1,   419,   403,   404,   405,   406,   152,    -1,   409,
     410,    85,    -1,   413,    -1,   415,    -1,    -1,    -1,    12,
      -1,    -1,    -1,    -1,    17,    -1,    -1,   427,   260,   429,
      -1,    24,    -1,   265,    27,    -1,   396,    -1,   446,    -1,
     457,    -1,    35,    36,    -1,    38,   446,    -1,    -1,   449,
     410,    -1,    -1,    -1,   128,   455,   473,   457,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   477,
      -1,    -1,   217,   473,   491,   475,    -1,   477,   223,    -1,
      -1,    -1,   482,   483,   484,    -1,    -1,   487,   233,   234,
      -1,    -1,    85,   493,    -1,   240,   496,    -1,    -1,   499,
     332,   501,    -1,   520,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   512,    -1,    -1,    -1,    -1,   262,   263,   264,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   360,   274,
      -1,    -1,   540,    -1,   534,   128,   553,   537,   555,   539,
     540,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   548,    -1,
      -1,   551,    -1,    -1,    -1,   555,   556,   557,   558,   559,
     568,   561,    -1,   563,   564,   565,    -1,   567,   568,    -1,
      -1,   571,    -1,    -1,    -1,   575,   408,    -1,    -1,    -1,
      -1,    -1,    -1,   415,    -1,   585,    -1,   587,    -1,   597,
      -1,   599,    -1,    -1,   602,    -1,   604,   597,    -1,   599,
      -1,    -1,   602,   563,   604,   565,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   585,    -1,   587,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   383,    -1,
      -1,   386,    12,    -1,    14,    -1,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    -1,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,   418,   506,    45,   421,    -1,    -1,   424,
      -1,    -1,    -1,   428,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   439,   440,    -1,    67,    -1,    -1,
      -1,    12,    -1,    -1,    -1,   450,    17,   539,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,   564,    -1,    -1,   567,    12,    -1,    -1,    -1,
      -1,    17,    -1,    -1,    -1,    -1,    -1,   492,    24,    -1,
     495,    27,   497,    -1,   124,   125,    67,    -1,   128,    35,
      36,   131,    38,    -1,   509,    -1,    -1,    -1,    -1,    -1,
      -1,   516,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,
     525,    -1,    -1,    -1,   529,   530,    -1,   532,    -1,    -1,
      -1,   536,    -1,    -1,    -1,    -1,   541,    -1,    -1,    -1,
      -1,    -1,   547,    -1,    -1,    -1,    -1,   552,    -1,    85,
      -1,    -1,    -1,    -1,   125,    -1,    -1,   128,    -1,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   593,   125,
     595,    -1,   128,   598,     1,   600,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    40,    41,    -1,    -1,    44,    45,    46,
      47,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    -1,    65,    66,
      67,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    80,    -1,    82,    83,    84,    85,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,    -1,   120,   121,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    40,    41,    -1,    -1,
      44,    45,    46,    47,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      -1,    65,    66,    67,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    80,    -1,    82,    83,
      84,    85,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    99,   100,   101,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,    -1,   120,   121,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,   131,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    40,
      41,    -1,    -1,    44,    45,    46,    47,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    -1,    65,    66,    67,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    77,    78,    -1,    80,
      -1,    82,    83,    84,    85,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    99,   100,
     101,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,    -1,   120,
     121,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    40,    41,    -1,    -1,    44,    45,    46,    47,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    -1,    65,    66,    67,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    77,
      -1,    -1,    80,    -1,    82,    83,    84,    85,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,    -1,   120,   121,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,   131,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    40,    41,    -1,    -1,    44,
      45,    46,    47,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    -1,
      65,    66,    67,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    80,    -1,    82,    83,    84,
      85,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    99,   100,   101,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,    -1,   120,   121,   122,   123,   124,
     125,    -1,    -1,   128,   129,   130,   131,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    40,    41,
      -1,    -1,    44,    45,    46,    47,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    -1,    65,    66,    67,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    77,    78,    -1,    80,    -1,
      82,    83,    84,    85,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    99,   100,   101,
      -1,    -1,    -1,    -1,    -1,    -1,   108,    -1,   110,   111,
     112,   113,   114,   115,   116,   117,   118,    -1,   120,   121,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    40,    -1,    -1,    -1,    -1,    45,    46,    47,    -1,
      -1,    50,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    67,    -1,
      69,    70,    71,    -1,    73,    74,    75,    76,    -1,    78,
      -1,    80,    -1,    82,    83,    84,    85,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,   114,   115,   116,   117,   118,
      -1,   120,   121,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    40,    -1,    -1,    -1,    -1,    45,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    60,    -1,    62,    63,    -1,    65,
      -1,    67,    -1,    69,    70,    71,    -1,    73,    74,    75,
      76,    -1,    78,    -1,    80,    -1,    82,    83,    84,    85,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,   114,   115,
     116,   117,   118,    -1,   120,   121,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,   131,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    40,    -1,    -1,
      -1,    -1,    45,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    67,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,
      83,    84,    85,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,   114,   115,   116,   117,   118,    -1,   120,   121,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,    49,
      -1,    51,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    77,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,
      47,    -1,    -1,    -1,    51,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    46,    47,    -1,    49,    -1,    51,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    76,    -1,    78,    79,    80,
      81,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    -1,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    44,
      -1,    46,    47,    -1,    -1,    -1,    51,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    77,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,   110,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    66,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    -1,    -1,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      -1,    -1,    78,    79,    80,    81,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    77,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,
      -1,    51,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    -1,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,
      47,    -1,    -1,    50,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    77,    78,    -1,    80,
      -1,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    56,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    -1,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    44,
      -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    77,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    -1,    77,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      76,    -1,    78,    79,    80,    -1,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    77,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,
      -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    77,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,
      47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    77,    78,    -1,    80,
      -1,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    77,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    44,
      -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    77,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    -1,    77,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      -1,    77,    78,    -1,    80,    -1,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    77,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,
      -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    77,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    46,
      47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      77,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    77,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,   110,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    44,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    77,    78,    -1,    80,
      -1,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    -1,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    41,    -1,    -1,    -1,
      -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    -1,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,    -1,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    76,    -1,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    18,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    -1,    -1,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    18,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      -1,    -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    44,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    18,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,
      -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    -1,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    18,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,
      47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    -1,    78,    79,    80,
      -1,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    77,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    77,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,    51,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    76,    -1,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    18,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      -1,    -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,   119,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,
      -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    -1,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
     110,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,
      47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,   110,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,    -1,   132,   133,     1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,
      -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,
      54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,
      -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,
      74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,    83,
      84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,
      -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,   123,
      -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,   133,
       1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,
      -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,
      71,    -1,    73,    74,    75,    -1,    -1,    78,    -1,    80,
      -1,    82,    83,    84,    -1,    86,    87,    -1,    89,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,
     111,   112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,
     121,   122,   123,    -1,   125,    -1,   127,    -1,   129,   130,
      -1,   132,   133,     1,    -1,     3,     4,     5,     6,     7,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,
      -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,
      -1,    69,    70,    71,    -1,    73,    74,    75,    -1,    -1,
      78,    -1,    80,    -1,    82,    83,    84,    -1,    86,    87,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,    -1,   111,   112,   113,    -1,   115,   116,    -1,
     118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,
      -1,   129,   130,    -1,   132,   133,     1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,
      25,    26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,
      65,    -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,
      75,    -1,    -1,    78,    -1,    80,    -1,    82,    83,    84,
      -1,    86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,
     115,   116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,
     125,    -1,   127,    -1,   129,   130,    -1,   132,   133,     1,
      -1,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,
      -1,    -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    -1,    -1,
      52,    53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      62,    63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,
      -1,    73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,
      82,    83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,
     112,   113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,
     122,   123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,
     132,   133,     1,    -1,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,
      19,    20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,
      -1,    -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,
      69,    70,    71,    -1,    73,    74,    75,    -1,    -1,    78,
      -1,    80,    -1,    82,    83,    84,    -1,    86,    87,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,
      -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,    -1,   111,   112,   113,    -1,   115,   116,    -1,   118,
      -1,    -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,
     129,   130,    -1,   132,   133,     1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,
      -1,    -1,    -1,    69,    70,    71,    -1,    73,    74,    75,
      -1,    -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,
      86,    87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,
      -1,   127,    -1,   129,   130,    -1,   132,   133,     1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,
      -1,    -1,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    52,
      53,    54,    -1,    -1,    57,    -1,    -1,    -1,    -1,    62,
      63,    -1,    65,    -1,    -1,    -1,    69,    70,    71,    -1,
      73,    74,    75,    -1,    -1,    78,    -1,    80,    -1,    82,
      83,    84,    -1,    86,    87,    -1,    89,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,   100,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   108,   109,    -1,   111,   112,
     113,    -1,   115,   116,    -1,   118,    -1,    -1,   121,   122,
     123,    -1,   125,    -1,   127,    -1,   129,   130,    -1,   132,
     133,     1,    -1,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    17,    -1,    19,
      20,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,
      -1,    -1,    52,    53,    54,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    62,    63,    -1,    65,    -1,    -1,    -1,    69,
      70,    71,    -1,    73,    74,    75,    -1,    -1,    78,    -1,
      80,    -1,    82,    83,    84,    -1,    86,    87,    -1,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    98,    -1,
     100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
      -1,   111,   112,   113,    -1,   115,   116,    -1,   118,    -1,
      -1,   121,   122,   123,    -1,   125,    -1,   127,    -1,   129,
     130,    -1,   132,   133,     1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      17,    -1,    19,    20,    -1,    -1,    -1,    -1,    25,    26,
      -1,    -1,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    46,
      47,    -1,    -1,    -1,    -1,    52,    53,    54,    -1,    -1,
      57,    -1,    -1,    -1,    -1,    62,    63,    -1,    65,    -1,
      -1,    -1,    69,    70,    71,    -1,    73,    74,    75,    -1,
      -1,    78,    -1,    80,    -1,    82,    83,    84,    -1,    86,
      87,    -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    98,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,    -1,   111,   112,   113,    -1,   115,   116,
      -1,   118,    -1,    -1,   121,   122,   123,    -1,   125,    -1,
     127,    -1,   129,   130,     8,   132,   133,    -1,    12,    -1,
      14,    15,    16,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    -1,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      44,    45,    -1,    -1,    -1,    -1,    -1,    -1,    11,    12,
      -1,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    67,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    85,    45,    -1,    -1,    12,    -1,    -1,    -1,    -1,
      17,    -1,    55,    -1,    21,    22,    23,    24,    -1,    -1,
      27,    -1,    -1,    -1,    67,    -1,    -1,    -1,    35,    36,
     114,    38,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,
     124,   125,    85,    12,   128,    -1,    -1,   131,    17,    -1,
      19,    20,    21,    22,    23,    24,    -1,    -1,    27,    28,
      29,    30,    31,    32,    33,    -1,    35,    36,    -1,    38,
      -1,   114,    -1,    -1,   117,    -1,    -1,   120,    85,    -1,
      -1,   124,   125,    -1,    -1,   128,    -1,    12,   131,    14,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    -1,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    85,    -1,   125,    -1,
      45,   128,    -1,    -1,    -1,    50,    -1,    -1,    12,    -1,
      14,    15,    16,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    67,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,   125,    41,    -1,   128,
      85,    45,   131,    -1,    12,    -1,    -1,    -1,    -1,    17,
      -1,    -1,    -1,    57,    22,    23,    24,    -1,    -1,    27,
      -1,    -1,    -1,    67,   109,    -1,    -1,    35,    36,   114,
      38,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    85,    -1,   128,    -1,    12,   131,    -1,    -1,    -1,
      17,    -1,    19,    20,    21,    22,    23,    24,    -1,    -1,
      27,    -1,    -1,    30,    31,    32,    33,    -1,    35,    36,
     114,    38,    -1,   117,    -1,    -1,   120,    85,    -1,    -1,
     124,   125,    -1,    -1,   128,    -1,    12,   131,    14,    15,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,    -1,
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    41,    -1,   125,    85,    45,
     128,    47,    -1,    -1,    -1,    -1,    -1,    12,    -1,    14,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    67,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    -1,   125,    85,
      45,   128,    -1,    -1,   131,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,   114,    -1,
      -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,
      85,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    -1,    -1,   128,    -1,    12,   131,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    -1,    -1,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    12,    -1,    14,    15,    16,    17,    45,    19,
      20,    21,    22,    23,    24,    -1,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      67,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    12,    -1,    -1,    -1,    85,    17,
      -1,    19,    20,    21,    22,    23,    24,    67,    -1,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    85,    -1,   114,    -1,    -1,
     117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,    -1,
      -1,   128,    -1,    -1,   131,    -1,    -1,    -1,    -1,    67,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    -1,    85,   128,    -1,
      12,   131,    14,    15,    16,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    -1,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    -1,   125,    -1,    -1,
     128,    -1,    -1,   131,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,    12,
      -1,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,    -1,   117,    -1,    60,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,   131,
      14,    15,    16,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    85,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   117,    -1,    60,   120,    -1,    -1,
      -1,   124,   125,    67,    -1,   128,    -1,    12,   131,    14,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,
     124,   125,    67,    -1,   128,    -1,    12,   131,    14,    15,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,    -1,
      85,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    99,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    67,    -1,   128,    -1,    12,   131,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    -1,    85,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    -1,    -1,   109,    -1,    -1,    -1,    -1,   114,    -1,
      -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,
      67,    -1,   128,    -1,    12,   131,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    -1,    85,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,
      -1,   128,    -1,    12,   131,    14,    15,    16,    17,    -1,
      19,    20,    21,    22,    23,    24,    -1,    85,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,
      -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,
     128,    -1,    12,   131,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,
      -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,
      -1,    12,   131,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,
      12,   131,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    -1,    85,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,
      -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,
     131,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,   131,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    -1,    85,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,
      -1,   124,   125,    67,    -1,   128,    -1,    12,   131,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,
     124,   125,    67,    -1,   128,    -1,    12,   131,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    -1,
      85,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    67,    -1,   128,    -1,    12,   131,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    -1,    85,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,
      -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,
      67,    -1,   128,    -1,    12,   131,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    -1,    85,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,
      -1,   128,    -1,    12,   131,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    -1,    85,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,
      -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,
     128,    -1,    12,   131,    14,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,
      -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,
      -1,    12,   131,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,
      12,   131,    14,    15,    16,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    85,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    41,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,
      -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,
     131,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,   131,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    -1,    85,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,
      -1,   124,   125,    67,    -1,   128,    -1,    12,   131,    14,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    41,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,
     124,   125,    67,    -1,   128,    -1,    12,   131,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    -1,
      85,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    67,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,
      -1,    -1,    -1,    12,    -1,    14,    15,    16,    17,    85,
      19,    20,    21,    22,    23,    24,    -1,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,   114,    -1,
      -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,    67,    -1,
      -1,    -1,    12,    -1,    14,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,
      60,   120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,
      -1,    12,   131,    14,    15,    16,    17,    -1,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,
      12,   131,    14,    15,    16,    17,    77,    19,    20,    21,
      22,    23,    24,    -1,    85,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,
      -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,
     131,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    12,    -1,    14,    15,    16,    17,    -1,
      19,    20,    21,    22,    23,    24,    -1,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,   114,    -1,    -1,   117,    -1,    45,   120,    -1,    -1,
      -1,   124,   125,    -1,    -1,   128,    -1,    -1,   131,    -1,
      -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,    67,    -1,
      -1,    -1,    12,    -1,    14,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,
      -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,
      -1,    12,   131,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,
      12,   131,    14,    15,    16,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    85,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    41,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,
      -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,
     131,    14,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,   131,
      14,    15,    16,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    85,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   117,    -1,    60,   120,    -1,    -1,
      -1,   124,   125,    67,    -1,   128,    -1,    12,   131,    14,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    -1,    -1,   117,    -1,    60,   120,    -1,    -1,    -1,
     124,   125,    67,    -1,   128,    -1,    12,   131,    14,    15,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,    -1,
      85,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    44,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    67,    -1,   128,    -1,    12,   131,    14,    15,    16,
      17,    -1,    19,    20,    21,    22,    23,    24,    -1,    85,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,   114,    -1,
      -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,
      67,    -1,   128,    -1,    12,   131,    14,    15,    16,    17,
      -1,    19,    20,    21,    22,    23,    24,    -1,    85,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     117,    -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,
      -1,   128,    -1,    12,   131,    14,    15,    16,    17,    -1,
      19,    20,    21,    22,    23,    24,    -1,    85,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    -1,    44,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,
      -1,    -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,
     128,    -1,    12,   131,    14,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,
      -1,   120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,
      -1,    12,   131,    14,    15,    16,    17,    -1,    19,    20,
      21,    22,    23,    24,    -1,    85,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,
     120,    -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,
      12,   131,    14,    15,    16,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    85,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   117,    -1,    -1,   120,
      -1,    -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,
     131,    -1,    15,    16,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    85,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,    -1,    -1,   120,    -1,
      -1,    -1,   124,   125,    67,    -1,   128,    -1,    12,   131,
      14,    15,    16,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    85,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   117,    -1,    -1,   120,    -1,    -1,
      -1,   124,   125,    67,    -1,   128,    -1,    12,   131,    -1,
      15,    16,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,    -1,
     124,   125,    -1,    -1,   128,    -1,    12,   131,    -1,    -1,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,    -1,
      85,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   117,    -1,    -1,   120,    -1,    -1,    -1,   124,
     125,    67,    -1,   128,    -1,    12,   131,    -1,    -1,    16,
      17,    -1,    19,    20,    21,    22,    23,    24,    -1,    85,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,   101,    -1,    -1,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   125,
      67,    -1,   128,    -1,    12,   131,    -1,    -1,    16,    17,
      -1,    19,    20,    21,    22,    23,    24,    -1,    85,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   125,    67,
      -1,   128,    -1,    12,   131,    -1,    -1,    16,    17,    -1,
      19,    20,    21,    22,    23,    24,    -1,    85,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   124,   125,    67,    -1,
     128,    -1,    12,   131,    -1,    -1,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    85,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    12,
      -1,    -1,    -1,    -1,    17,    -1,    19,    20,    21,    22,
      23,    24,    -1,    -1,    27,    -1,    -1,    30,    -1,    32,
      33,    -1,    35,    36,    -1,    38,   125,    67,    12,   128,
      -1,    -1,   131,    17,    -1,    19,    20,    21,    22,    23,
      24,    -1,    -1,    27,    -1,    85,    30,    -1,    32,    33,
      12,    35,    36,    -1,    38,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    -1,    27,    -1,    -1,    30,    -1,
      -1,    -1,    85,    35,    36,    -1,    38,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   125,    -1,    12,   128,    -1,
      -1,   131,    17,    -1,    19,    20,    21,    22,    23,    24,
      -1,    85,    27,    -1,    -1,    -1,    -1,    -1,    -1,    12,
      35,    36,   125,    38,    17,   128,    -1,    -1,   131,    -1,
      23,    24,    -1,    85,    27,    12,    -1,    -1,    -1,    -1,
      17,    -1,    35,    36,    -1,    38,    23,    24,    -1,    -1,
      27,   125,    -1,    -1,   128,    -1,    -1,   131,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      85,    -1,    -1,   125,    -1,    -1,   128,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,    -1,
     125,    -1,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   125,    -1,    -1,   128,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   125,    -1,
      -1,   128
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     3,     4,     5,     6,     7,     9,    10,    11,
      12,    13,    17,    19,    20,    25,    26,    29,    30,    35,
      36,    37,    40,    46,    47,    52,    53,    54,    57,    62,
      63,    65,    69,    70,    71,    73,    74,    75,    78,    80,
      82,    83,    84,    86,    87,    89,    98,   100,   108,   109,
     111,   112,   113,   115,   116,   118,   121,   122,   123,   125,
     127,   129,   130,   132,   133,   137,   138,   139,   141,   144,
       4,     5,    10,    46,   142,    46,    15,    17,    17,    38,
     139,     4,   139,   139,   139,    13,    78,   139,   139,    14,
     139,     4,   139,   139,   150,     4,    17,   139,    17,    17,
      17,   139,     3,     4,    10,    13,    17,   133,   140,   141,
      17,   139,   139,   139,   151,    76,   158,    17,    17,   141,
      17,   109,   139,    17,    17,    17,    38,    10,   152,   153,
      17,    83,   139,   139,   139,   139,   141,   151,   139,   150,
     139,   151,   151,   158,   141,   139,   153,    38,   139,    17,
     139,    47,    55,   164,   151,   139,    17,     0,     8,    12,
      14,    15,    16,    17,    19,    20,    21,    22,    23,    24,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    44,    45,    67,    85,   114,   117,   120,   124,
     125,   128,   131,    17,    17,     4,    24,   128,     4,    10,
      13,    26,   133,   139,    18,   139,    18,   139,   139,    18,
      26,    26,    26,   139,    57,    41,   139,    50,   109,   139,
     139,     4,    11,    55,   163,   139,   149,    67,    15,    29,
      58,   155,   139,    60,    60,    44,    66,   139,   165,    81,
     143,   146,   151,    72,   139,   150,    17,   139,   139,   139,
     139,   139,    88,    14,   152,   139,    99,    14,    17,    77,
     109,   160,   109,   109,    51,   109,   160,    17,   122,   139,
      18,   139,   151,    77,    50,   139,    67,   139,   139,   139,
     158,   150,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   141,    40,
     139,     8,   138,   139,   139,     6,    10,    14,    19,    21,
      22,   114,   125,   141,   139,   139,   139,   139,   139,   150,
     139,     3,    17,   139,    18,    18,    41,    17,     3,     4,
      10,    13,    17,   141,   147,   148,    18,   151,   158,   139,
      18,    18,    18,     3,   151,    56,    18,    44,   139,   139,
     139,   139,    59,    61,   154,    18,   151,   151,   139,   139,
      79,   145,   151,   143,    77,    17,    18,    18,   150,    18,
      18,    18,    18,    41,   152,    18,   139,   150,    77,   151,
     151,   151,   160,    51,   150,    41,    50,   151,    18,   139,
      18,    23,    67,   139,    41,   139,    17,    38,    18,    18,
       3,    26,   150,    15,    29,    45,    46,   148,    14,    67,
      17,   139,   158,    49,    51,   109,   159,   160,   161,    51,
      76,    60,   139,   149,    60,   139,   139,    59,    60,    63,
     139,   158,    77,    77,    77,    44,   147,    77,   139,   143,
      18,   101,    18,   160,   160,   160,   151,   160,    18,   151,
      51,    77,   139,    41,   150,    40,   139,    15,    74,    24,
      18,    18,   139,   139,   139,   139,    18,   148,   139,   139,
      44,   109,   157,   161,   139,    49,    51,   151,   158,    54,
      55,   162,   151,    44,   151,   154,   139,   151,   139,    44,
      44,    18,    76,   151,    76,   143,   151,   119,   160,    29,
      77,   151,    41,    18,   139,    41,    74,   139,   158,   143,
       3,    41,    47,   157,   139,   158,    50,   139,   160,     3,
      45,    77,    11,   163,   149,    51,    77,    44,    60,   156,
      60,    77,    60,   158,   151,    77,   143,   151,    77,   109,
     151,   119,   139,    44,   164,    77,    41,   143,   151,   139,
      44,   151,    50,    45,   158,    18,   151,   151,   151,   151,
      77,   151,    77,   110,   109,   110,   160,   109,   151,   164,
      41,   151,    77,    41,   159,   151,   158,   139,   158,    77,
      77,    77,    77,    77,   150,   110,   150,   110,   160,    77,
     159,   162,    44,   109,   150,   109,   150,   151,   109,   151,
     109,   160,   151,   160,   151,   160,   160
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   136,   137,   138,   138,   138,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   140,   140,   140,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   142,   142,
     143,   143,   143,   144,   144,   145,   146,   147,   147,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   149,   149,
     150,   150,   151,   151,   151,   152,   152,   152,   153,   154,
     154,   155,   155,   155,   155,   156,   156,   157,   157,   157,
     158,   158,   159,   159,   159,   159,   160,   160,   161,   161,
     162,   162,   162,   163,   163,   163,   164,   164,   164,   164,
     165
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     3,     3,     1,     2,     4,     6,
       3,     5,     7,     1,     1,     6,     6,     6,     6,     8,
       3,     3,     3,     3,     3,     3,     3,     4,     4,     4,
       1,     1,     1,     3,     4,     3,     3,     3,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       3,     3,     3,     2,     2,     2,     5,     3,     1,     3,
       2,     4,     4,     1,     4,     4,     4,     3,     1,     2,
       2,     6,     4,     5,     4,     1,     4,     1,     3,     3,
       4,     1,     2,     1,     3,     1,     1,     7,     9,     9,
       9,     7,     9,     1,     4,     5,     7,     5,     3,     1,
       3,     2,     5,     3,     3,     2,     3,     1,     3,     4,
       3,     3,     3,     1,     3,     4,     6,     6,     3,     3,
       3,     1,     2,     3,     2,     3,     2,     1,     1,     2,
       1,     3,     4,     1,     6,     7,     3,     1,     4,     7,
       8,     7,     9,     8,     8,     9,     9,    10,     4,     3,
       4,     7,     9,     5,     6,     5,     5,     7,     4,     1,
       7,     4,     4,     3,     1,     2,     3,     4,     5,     4,
       6,     5,     3,    13,    12,    12,     8,     3,     2,     2,
       7,    13,     9,     5,     5,     1,     1,     1,     1,     3,
       3,     3,     3,     5,     2,     3,     2,     2,     1,     1,
       0,     2,     2,     4,     2,     3,     3,     1,     3,     1,
       3,     3,     3,     3,     1,     3,     1,     1,     0,     1,
       0,     1,     1,     2,     2,     0,     2,     3,     1,     0,
       2,     0,     2,     2,     2,     1,     1,     0,     3,     2,
       3,     4,     1,     3,     5,     6,     1,     2,     1,     2,
       0,     3,     5,     0,     2,     5,     0,     2,     6,     7,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void * scanner)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (scanner);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void * scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, void * scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, void * scanner)
{
  YYUSE (yyvaluep);
  YYUSE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void * scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* input: correct_input  */
#line 199 "input_parser.yy"
                        {   const giac::context * contextptr = giac_yyget_extra(scanner);
			    if (yyvsp[0]._VECTptr->size()==1)
			     parsed_gen(yyvsp[0]._VECTptr->front(),contextptr);
                          else
			     parsed_gen(gen(*yyvsp[0]._VECTptr,_SEQ__VECT),contextptr);
			 }
#line 4473 "input_parser.cc"
    break;

  case 3: /* correct_input: exp T_END_INPUT  */
#line 207 "input_parser.yy"
                                { yyval=vecteur(1,yyvsp[-1]); }
#line 4479 "input_parser.cc"
    break;

  case 4: /* correct_input: exp T_SEMI T_END_INPUT  */
#line 208 "input_parser.yy"
                                       { if (yyvsp[-1].val==1) yyval=vecteur(1,symbolic(at_nodisp,yyvsp[-2])); else yyval=vecteur(1,yyvsp[-2]); }
#line 4485 "input_parser.cc"
    break;

  case 5: /* correct_input: exp T_SEMI correct_input  */
#line 209 "input_parser.yy"
                                         { if (yyvsp[-1].val==1) yyval=mergevecteur(makevecteur(symbolic(at_nodisp,yyvsp[-2])),*yyvsp[0]._VECTptr); else yyval=mergevecteur(makevecteur(yyvsp[-2]),*yyvsp[0]._VECTptr); }
#line 4491 "input_parser.cc"
    break;

  case 6: /* exp: T_NUMBER  */
#line 212 "input_parser.yy"
                                {yyval = yyvsp[0];}
#line 4497 "input_parser.cc"
    break;

  case 7: /* exp: T_NUMBER symbol_or_literal  */
#line 213 "input_parser.yy"
                                                        {if (is_one(yyvsp[-1])) yyval=yyvsp[0]; else yyval=symbolic(at_prod,gen(makevecteur(yyvsp[-1],yyvsp[0]),_SEQ__VECT));}
#line 4503 "input_parser.cc"
    break;

  case 8: /* exp: T_NUMBER symbol_or_literal T_POW T_NUMBER  */
#line 214 "input_parser.yy"
                                                                        {if (is_one(yyvsp[-3])) yyval=symb_pow(yyvsp[-2],yyvsp[0]); else yyval=symbolic(at_prod,gen(makevecteur(yyvsp[-3],symb_pow(yyvsp[-2],yyvsp[0])),_SEQ__VECT));}
#line 4509 "input_parser.cc"
    break;

  case 9: /* exp: T_NUMBER symbol_or_literal T_POW T_BEGIN_PAR T_NUMBER T_END_PAR  */
#line 215 "input_parser.yy"
                                                                                                {if (is_one(yyvsp[-5])) yyval=symb_pow(yyvsp[-4],yyvsp[-1]); else yyval=symbolic(at_prod,gen(makevecteur(yyvsp[-5],symb_pow(yyvsp[-4],yyvsp[-1])),_SEQ__VECT));}
#line 4515 "input_parser.cc"
    break;

  case 10: /* exp: T_NUMBER symbol_or_literal T_SQ  */
#line 216 "input_parser.yy"
                                                                {yyval=symbolic(at_prod,gen(makevecteur(yyvsp[-2],symb_pow(yyvsp[-1],yyvsp[0])) ,_SEQ__VECT));}
#line 4521 "input_parser.cc"
    break;

  case 11: /* exp: T_NUMBER T_UNARY_OP T_BEGIN_PAR exp T_END_PAR  */
#line 217 "input_parser.yy"
                                                        { yyval =yyvsp[-4]*symbolic(*yyvsp[-3]._FUNCptr,python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-1])):os_nary_workaround(yyvsp[-1])); }
#line 4527 "input_parser.cc"
    break;

  case 12: /* exp: T_NUMBER T_UNARY_OP T_BEGIN_PAR exp T_END_PAR T_POW T_NUMBER  */
#line 218 "input_parser.yy"
                                                                        { yyval =yyvsp[-6]*symb_pow(symbolic(*yyvsp[-5]._FUNCptr,python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-3])):os_nary_workaround(yyvsp[-3])),yyvsp[0]); }
#line 4533 "input_parser.cc"
    break;

  case 13: /* exp: T_STRING  */
#line 220 "input_parser.yy"
                                { yyval=yyvsp[0]; }
#line 4539 "input_parser.cc"
    break;

  case 14: /* exp: T_EXPRESSION  */
#line 221 "input_parser.yy"
                                { if (yyvsp[0].type==_FUNC) yyval=symbolic(*yyvsp[0]._FUNCptr,gen(vecteur(0),_SEQ__VECT)); else yyval=yyvsp[0]; }
#line 4545 "input_parser.cc"
    break;

  case 15: /* exp: symbol T_BEGIN_PAR suite T_END_PAR T_AFFECT bloc  */
#line 224 "input_parser.yy"
                                                           {yyval = symb_program_sto(yyvsp[-3],yyvsp[-3]*gen_zero,yyvsp[0],yyvsp[-5],false,giac_yyget_extra(scanner));}
#line 4551 "input_parser.cc"
    break;

  case 16: /* exp: symbol T_BEGIN_PAR suite T_END_PAR T_AFFECT exp  */
#line 225 "input_parser.yy"
                                                          {if (is_array_index(yyvsp[-5],yyvsp[-3],giac_yyget_extra(scanner)) || (abs_calc_mode(giac_yyget_extra(scanner))==38 && yyvsp[-5].type==_IDNT && strlen(yyvsp[-5]._IDNTptr->id_name)==2 && check_vect_38(yyvsp[-5]._IDNTptr->id_name))) yyval=symbolic(at_sto,gen(makevecteur(yyvsp[0],symbolic(at_of,gen(makevecteur(yyvsp[-5],yyvsp[-3]) ,_SEQ__VECT))) ,_SEQ__VECT)); else { yyval = symb_program_sto(yyvsp[-3],yyvsp[-3]*gen_zero,yyvsp[0],yyvsp[-5],true,giac_yyget_extra(scanner)); yyval._SYMBptr->feuille.subtype=_SORTED__VECT;  } }
#line 4557 "input_parser.cc"
    break;

  case 17: /* exp: exp TI_STO symbol T_BEGIN_PAR suite T_END_PAR  */
#line 226 "input_parser.yy"
                                                        {if (is_array_index(yyvsp[-3],yyvsp[-1],giac_yyget_extra(scanner)) || (abs_calc_mode(giac_yyget_extra(scanner))==38 && yyvsp[-3].type==_IDNT && check_vect_38(yyvsp[-3]._IDNTptr->id_name))) yyval=symbolic(at_sto,gen(makevecteur(yyvsp[-5],symbolic(at_of,gen(makevecteur(yyvsp[-3],yyvsp[-1]) ,_SEQ__VECT))) ,_SEQ__VECT)); else yyval = symb_program_sto(yyvsp[-1],yyvsp[-1]*gen_zero,yyvsp[-5],yyvsp[-3],false,giac_yyget_extra(scanner));}
#line 4563 "input_parser.cc"
    break;

  case 18: /* exp: exp TI_STO symbol T_INDEX_BEGIN exp T_VECT_END  */
#line 227 "input_parser.yy"
                                                         { 
         const giac::context * contextptr = giac_yyget_extra(scanner);
         gen g=symb_at(yyvsp[-3],yyvsp[-1],contextptr); yyval=parser_symb_sto(yyvsp[-5],g); 
        }
#line 4572 "input_parser.cc"
    break;

  case 19: /* exp: exp TI_STO symbol T_INDEX_BEGIN T_VECT_DISPATCH exp T_VECT_END T_VECT_END  */
#line 231 "input_parser.yy"
                                                                                    { 
         const giac::context * contextptr = giac_yyget_extra(scanner);
         gen g=symbolic(at_of,gen(makevecteur(yyvsp[-5],yyvsp[-2]) ,_SEQ__VECT)); yyval=parser_symb_sto(yyvsp[-7],g); 
        }
#line 4581 "input_parser.cc"
    break;

  case 20: /* exp: exp TI_STO symbol  */
#line 235 "input_parser.yy"
                            { if (yyvsp[0].type==_IDNT) { string s=yyvsp[0].print(context0); const char * ch=s.c_str(); if (ch[0]=='_' && unit_conversion_map().find(ch+1) != unit_conversion_map().end()) yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-2],symbolic(at_unit,makevecteur(1,yyvsp[0]))) ,_SEQ__VECT)); else yyval=parser_symb_sto(yyvsp[-2],yyvsp[0]); } else yyval=parser_symb_sto(yyvsp[-2],yyvsp[0]); }
#line 4587 "input_parser.cc"
    break;

  case 21: /* exp: exp TI_STO T_UNARY_OP  */
#line 236 "input_parser.yy"
                                { yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 4593 "input_parser.cc"
    break;

  case 22: /* exp: exp TI_STO T_PLUS  */
#line 237 "input_parser.yy"
                            { yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 4599 "input_parser.cc"
    break;

  case 23: /* exp: exp TI_STO T_FOIS  */
#line 238 "input_parser.yy"
                            { yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 4605 "input_parser.cc"
    break;

  case 24: /* exp: exp TI_STO T_DIV  */
#line 239 "input_parser.yy"
                           { yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 4611 "input_parser.cc"
    break;

  case 25: /* exp: exp TI_STO T_VIRGULE  */
#line 240 "input_parser.yy"
                               { yyval=symbolic(at_time,yyvsp[-2]);}
#line 4617 "input_parser.cc"
    break;

  case 26: /* exp: exp TI_STO TI_STO  */
#line 241 "input_parser.yy"
                            { if (yyvsp[-2]==16 || yyvsp[-2]==10 || yyvsp[-2]==8 || yyvsp[-2]==2) yyval=symbolic(at_integer_format,yyvsp[-2]); else yyval=symbolic(at_solve,symb_equal(yyvsp[-2],0));}
#line 4623 "input_parser.cc"
    break;

  case 27: /* exp: exp TI_STO T_UNIT exp  */
#line 242 "input_parser.yy"
                                { yyval=symbolic(at_convert,gen(makevecteur(yyvsp[-3],symb_unit(gen(1),yyvsp[0],giac_yyget_extra(scanner))),_SEQ__VECT)); opened_quote(giac_yyget_extra(scanner)) &= 0x7ffffffd;}
#line 4629 "input_parser.cc"
    break;

  case 28: /* exp: symbol T_BEGIN_PAR suite T_END_PAR  */
#line 243 "input_parser.yy"
                                             {yyval = check_symb_of(yyvsp[-3],python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-1])):os_nary_workaround(yyvsp[-1]),giac_yyget_extra(scanner));}
#line 4635 "input_parser.cc"
    break;

  case 29: /* exp: exp T_BEGIN_PAR suite T_END_PAR  */
#line 244 "input_parser.yy"
                                          {yyval = check_symb_of(yyvsp[-3],python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-1])):os_nary_workaround(yyvsp[-1]),giac_yyget_extra(scanner));}
#line 4641 "input_parser.cc"
    break;

  case 30: /* exp: symbol  */
#line 245 "input_parser.yy"
                                {yyval = yyvsp[0];}
#line 4647 "input_parser.cc"
    break;

  case 31: /* exp: T_LITERAL  */
#line 246 "input_parser.yy"
                                {yyval = yyvsp[0];}
#line 4653 "input_parser.cc"
    break;

  case 32: /* exp: T_DIGITS  */
#line 247 "input_parser.yy"
                                {yyval = yyvsp[0];}
#line 4659 "input_parser.cc"
    break;

  case 33: /* exp: T_DIGITS T_AFFECT exp  */
#line 248 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[-2]._FUNCptr,yyvsp[0]);}
#line 4665 "input_parser.cc"
    break;

  case 34: /* exp: T_DIGITS T_BEGIN_PAR exp T_END_PAR  */
#line 249 "input_parser.yy"
                                                {yyval = symbolic(*yyvsp[-3]._FUNCptr,yyvsp[-1]);}
#line 4671 "input_parser.cc"
    break;

  case 35: /* exp: T_DIGITS T_BEGIN_PAR T_END_PAR  */
#line 250 "input_parser.yy"
                                                {yyval = symbolic(*yyvsp[-2]._FUNCptr,gen(vecteur(0),_SEQ__VECT));}
#line 4677 "input_parser.cc"
    break;

  case 36: /* exp: exp TI_STO T_DIGITS  */
#line 251 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[0]._FUNCptr,yyvsp[-2]);}
#line 4683 "input_parser.cc"
    break;

  case 37: /* exp: exp T_TEST_EQUAL exp  */
#line 252 "input_parser.yy"
                                {yyval=symb_test_equal(yyvsp[-2],yyvsp[-1],yyvsp[0]);}
#line 4689 "input_parser.cc"
    break;

  case 38: /* exp: exp T_EQUAL exp  */
#line 254 "input_parser.yy"
                                        {yyval = symbolic(*yyvsp[-1]._FUNCptr,makesequence(yyvsp[-2],yyvsp[0])); }
#line 4695 "input_parser.cc"
    break;

  case 39: /* exp: T_EQUAL exp  */
#line 255 "input_parser.yy"
                                    { 
	if (yyvsp[0].type==_SYMB) yyval=yyvsp[0]; else yyval=symbolic(at_nop,yyvsp[0]); 
	yyval.change_subtype(_SPREAD__SYMB); 
        const giac::context * contextptr = giac_yyget_extra(scanner);
       spread_formula(false,contextptr); 
	}
#line 4706 "input_parser.cc"
    break;

  case 40: /* exp: exp T_PLUS exp  */
#line 261 "input_parser.yy"
                            { if (yyvsp[-2].is_symb_of_sommet(at_plus) && yyvsp[-2]._SYMBptr->feuille.type==_VECT){ yyvsp[-2]._SYMBptr->feuille._VECTptr->push_back(yyvsp[0]); yyval=yyvsp[-2]; } else
  yyval =symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4713 "input_parser.cc"
    break;

  case 41: /* exp: exp T_MOINS exp  */
#line 263 "input_parser.yy"
                                {yyval = symb_plus(yyvsp[-2],yyvsp[0].type<_IDNT?-yyvsp[0]:symbolic(at_neg,yyvsp[0]));}
#line 4719 "input_parser.cc"
    break;

  case 42: /* exp: exp T_MOINS38 exp  */
#line 264 "input_parser.yy"
                                {yyval = symb_plus(yyvsp[-2],yyvsp[0].type<_IDNT?-yyvsp[0]:symbolic(at_neg,yyvsp[0]));}
#line 4725 "input_parser.cc"
    break;

  case 43: /* exp: exp T_FOIS exp  */
#line 265 "input_parser.yy"
                                {yyval =symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4731 "input_parser.cc"
    break;

  case 44: /* exp: exp T_DIV exp  */
#line 266 "input_parser.yy"
                                {yyval =symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4737 "input_parser.cc"
    break;

  case 45: /* exp: exp T_POW exp  */
#line 267 "input_parser.yy"
                                {if (yyvsp[-2]==symbolic(at_exp,1) && yyvsp[-1]==at_pow) yyval=symbolic(at_exp,yyvsp[0]); else yyval =symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4743 "input_parser.cc"
    break;

  case 46: /* exp: exp T_MOD exp  */
#line 268 "input_parser.yy"
                                {if (yyvsp[-1].type==_FUNC) yyval=symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT)); else yyval = symbolic(at_normalmod,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4749 "input_parser.cc"
    break;

  case 47: /* exp: exp T_INTERVAL exp  */
#line 269 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 4755 "input_parser.cc"
    break;

  case 48: /* exp: exp T_INTERVAL  */
#line 270 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[0]._FUNCptr,gen(makevecteur(yyvsp[-1],RAND_MAX) ,_SEQ__VECT)); }
#line 4761 "input_parser.cc"
    break;

  case 49: /* exp: T_INTERVAL exp  */
#line 271 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(0,yyvsp[0]) ,_SEQ__VECT)); }
#line 4767 "input_parser.cc"
    break;

  case 50: /* exp: T_INTERVAL T_VIRGULE exp  */
#line 272 "input_parser.yy"
                                        {yyval = makesequence(symbolic(*yyvsp[-2]._FUNCptr,gen(makevecteur(0,RAND_MAX) ,_SEQ__VECT)),yyvsp[0]); }
#line 4773 "input_parser.cc"
    break;

  case 51: /* exp: exp T_AND_OP exp  */
#line 275 "input_parser.yy"
                                {yyval = symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]),_SEQ__VECT));}
#line 4779 "input_parser.cc"
    break;

  case 52: /* exp: exp T_DEUXPOINTS exp  */
#line 276 "input_parser.yy"
                                {yyval= symbolic(at_deuxpoints,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT));}
#line 4785 "input_parser.cc"
    break;

  case 53: /* exp: T_MOINS exp  */
#line 277 "input_parser.yy"
                                        { 
					if (yyvsp[0]==unsigned_inf)
						yyval = minus_inf;
					else { if (yyvsp[0].type==_INT_) yyval=(-yyvsp[0].val); else { if (yyvsp[0].type==_DOUBLE_) yyval=(-yyvsp[0]._DOUBLE_val); else yyval=symbolic(at_neg,yyvsp[0]); } }
				}
#line 4795 "input_parser.cc"
    break;

  case 54: /* exp: T_NEG38 exp  */
#line 282 "input_parser.yy"
                        { 
					if (yyvsp[0]==unsigned_inf)
						yyval = minus_inf;
					else { if (yyvsp[0].type==_INT_ || yyvsp[0].type==_DOUBLE_ || yyvsp[0].type==_FLOAT_) yyval=-yyvsp[0]; else yyval=symbolic(at_neg,yyvsp[0]); }
				}
#line 4805 "input_parser.cc"
    break;

  case 55: /* exp: T_PLUS exp  */
#line 287 "input_parser.yy"
                                        {
					if (yyvsp[0]==unsigned_inf)
						yyval = plus_inf;
					else
						yyval = yyvsp[0];
				}
#line 4816 "input_parser.cc"
    break;

  case 56: /* exp: T_SPOLY1_BEGIN exp T_VIRGULE exp T_SPOLY1_END  */
#line 293 "input_parser.yy"
                                                        {yyval = polynome_or_sparse_poly1(eval(yyvsp[-3],1, giac_yyget_extra(scanner)),yyvsp[-1]);}
#line 4822 "input_parser.cc"
    break;

  case 57: /* exp: T_ROOTOF_BEGIN exp T_ROOTOF_END  */
#line 294 "input_parser.yy"
                                          { 
           if ( (yyvsp[-1].type==_SYMB) && (yyvsp[-1]._SYMBptr->sommet==at_deuxpoints) )
             yyval = algebraic_EXTension(yyvsp[-1]._SYMBptr->feuille._VECTptr->front(),yyvsp[-1]._SYMBptr->feuille._VECTptr->back());
           else yyval=yyvsp[-1];
        }
#line 4832 "input_parser.cc"
    break;

  case 58: /* exp: T_OF  */
#line 300 "input_parser.yy"
               { yyval=gen(at_of,2); }
#line 4838 "input_parser.cc"
    break;

  case 59: /* exp: exp T_AFFECT exp  */
#line 301 "input_parser.yy"
                                        {if (yyvsp[-2].type==_FUNC) *logptr(giac_yyget_extra(scanner))<< ("Warning: "+yyvsp[-2].print(context0)+" is a reserved word")<<'\n'; if (yyvsp[-2].type==_INT_) yyval=symb_equal(yyvsp[-2],yyvsp[0]); else {yyval = parser_symb_sto(yyvsp[0],yyvsp[-2],yyvsp[-1]==at_array_sto); if (yyvsp[0].is_symb_of_sommet(at_program)) *logptr(giac_yyget_extra(scanner))<<"// End defining "<<yyvsp[-2]<<'\n';}}
#line 4844 "input_parser.cc"
    break;

  case 60: /* exp: T_NOT exp  */
#line 302 "input_parser.yy"
                        { yyval = symbolic(*yyvsp[-1]._FUNCptr,yyvsp[0]);}
#line 4850 "input_parser.cc"
    break;

  case 61: /* exp: T_ARGS T_BEGIN_PAR exp T_END_PAR  */
#line 303 "input_parser.yy"
                                                {yyval = symb_args(yyvsp[-1]);}
#line 4856 "input_parser.cc"
    break;

  case 62: /* exp: T_ARGS T_INDEX_BEGIN exp T_VECT_END  */
#line 304 "input_parser.yy"
                                                {yyval = symb_args(yyvsp[-1]);}
#line 4862 "input_parser.cc"
    break;

  case 63: /* exp: T_ARGS  */
#line 305 "input_parser.yy"
                 { yyval=symb_args(vecteur(0)); }
#line 4868 "input_parser.cc"
    break;

  case 64: /* exp: T_UNARY_OP T_BEGIN_PAR exp T_END_PAR  */
#line 306 "input_parser.yy"
                                                {
	gen tmp=python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-1])):os_nary_workaround(yyvsp[-1]);
	// CERR << python_compat(giac_yyget_extra(scanner)) << tmp << '\n';
	yyval = symbolic(*yyvsp[-3]._FUNCptr,tmp);
        const giac::context * contextptr = giac_yyget_extra(scanner);
	if (*yyvsp[-3]._FUNCptr==at_maple_mode ||*yyvsp[-3]._FUNCptr==at_xcas_mode ){
          xcas_mode(contextptr)=yyvsp[-1].val;
        }
        if (*yyvsp[-3]._FUNCptr==at_python_compat)
          python_compat(contextptr)=yyvsp[-1].val;
	if (*yyvsp[-3]._FUNCptr==at_user_operator){
          user_operator(yyvsp[-1],contextptr);
        }
	}
#line 4887 "input_parser.cc"
    break;

  case 65: /* exp: T_UNARY_OP_38 T_BEGIN_PAR exp T_END_PAR  */
#line 320 "input_parser.yy"
                                                        {
	if (yyvsp[-1].type==_VECT && yyvsp[-1]._VECTptr->empty())
          giac_yyerror(scanner,"void argument");
	yyval = symbolic(*yyvsp[-3]._FUNCptr,python_compat(giac_yyget_extra(scanner))?denest_sto(os_nary_workaround(yyvsp[-1])):os_nary_workaround(yyvsp[-1]));	
	}
#line 4897 "input_parser.cc"
    break;

  case 66: /* exp: T_UNARY_OP T_INDEX_BEGIN exp T_VECT_END  */
#line 325 "input_parser.yy"
                                                  { 
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_at(yyvsp[-3],yyvsp[-1],contextptr);
        }
#line 4906 "input_parser.cc"
    break;

  case 67: /* exp: T_UNARY_OP T_BEGIN_PAR T_END_PAR  */
#line 329 "input_parser.yy"
                                                {
	yyval = symbolic(*yyvsp[-2]._FUNCptr,gen(vecteur(0),_SEQ__VECT));
	if (*yyvsp[-2]._FUNCptr==at_rpn)
          rpn_mode(giac_yyget_extra(scanner))=1;
	if (*yyvsp[-2]._FUNCptr==at_alg)
          rpn_mode(giac_yyget_extra(scanner))=0;
	}
#line 4918 "input_parser.cc"
    break;

  case 68: /* exp: T_UNARY_OP  */
#line 336 "input_parser.yy"
                     {
	yyval = yyvsp[0];
	}
#line 4926 "input_parser.cc"
    break;

  case 69: /* exp: exp T_PRIME  */
#line 339 "input_parser.yy"
                        {yyval = symbolic(at_derive,yyvsp[-1]);}
#line 4932 "input_parser.cc"
    break;

  case 70: /* exp: exp T_FACTORIAL  */
#line 340 "input_parser.yy"
                          { yyval=symbolic(*yyvsp[0]._FUNCptr,yyvsp[-1]); }
#line 4938 "input_parser.cc"
    break;

  case 71: /* exp: T_IF exp T_THEN bloc T_ELSE bloc  */
#line 342 "input_parser.yy"
                                           {yyval = symbolic(*yyvsp[-5]._FUNCptr,makevecteur(equaltosame(yyvsp[-4]),symb_bloc(yyvsp[-2]),symb_bloc(yyvsp[0])));}
#line 4944 "input_parser.cc"
    break;

  case 72: /* exp: T_IF exp T_THEN bloc  */
#line 343 "input_parser.yy"
                               {yyval = symbolic(*yyvsp[-3]._FUNCptr,makevecteur(equaltosame(yyvsp[-2]),yyvsp[0],0));}
#line 4950 "input_parser.cc"
    break;

  case 73: /* exp: T_IF exp T_THEN prg_suite elif  */
#line 344 "input_parser.yy"
                                         {
	yyval = symbolic(*yyvsp[-4]._FUNCptr,makevecteur(equaltosame(yyvsp[-3]),symb_bloc(yyvsp[-1]),yyvsp[0]));
	}
#line 4958 "input_parser.cc"
    break;

  case 74: /* exp: T_IFTE T_BEGIN_PAR exp T_END_PAR  */
#line 347 "input_parser.yy"
                                                {yyval = symbolic(*yyvsp[-3]._FUNCptr,yyvsp[-1]);}
#line 4964 "input_parser.cc"
    break;

  case 75: /* exp: T_IFTE  */
#line 348 "input_parser.yy"
                 {yyval = yyvsp[0];}
#line 4970 "input_parser.cc"
    break;

  case 76: /* exp: T_PROGRAM T_BEGIN_PAR exp T_END_PAR  */
#line 349 "input_parser.yy"
                                                {yyval = symb_program(yyvsp[-1]);}
#line 4976 "input_parser.cc"
    break;

  case 77: /* exp: T_PROGRAM  */
#line 350 "input_parser.yy"
                    {yyval = gen(at_program,3);}
#line 4982 "input_parser.cc"
    break;

  case 78: /* exp: exp T_MAPSTO bloc  */
#line 351 "input_parser.yy"
                                {
          const giac::context * contextptr = giac_yyget_extra(scanner);
         yyval = symb_program(yyvsp[-2],gen_zero*yyvsp[-2],yyvsp[0],contextptr);
        }
#line 4991 "input_parser.cc"
    break;

  case 79: /* exp: exp T_MAPSTO exp  */
#line 355 "input_parser.yy"
                                {
          const giac::context * contextptr = giac_yyget_extra(scanner);
             if (yyvsp[0].type==_VECT) 
                yyval = symb_program(yyvsp[-2],gen_zero*yyvsp[-2],symb_bloc(makevecteur(at_nop,yyvsp[0])),contextptr); 
             else 
                yyval = symb_program(yyvsp[-2],gen_zero*yyvsp[-2],yyvsp[0],contextptr);
		}
#line 5003 "input_parser.cc"
    break;

  case 80: /* exp: T_BLOC T_BEGIN_PAR exp T_END_PAR  */
#line 362 "input_parser.yy"
                                                {yyval = symb_bloc(yyvsp[-1]);}
#line 5009 "input_parser.cc"
    break;

  case 81: /* exp: T_BLOC  */
#line 363 "input_parser.yy"
                 {yyval = at_bloc;}
#line 5015 "input_parser.cc"
    break;

  case 82: /* exp: T_RETURN exp  */
#line 365 "input_parser.yy"
                        { yyval=symbolic(*yyvsp[-1]._FUNCptr,yyvsp[0]); }
#line 5021 "input_parser.cc"
    break;

  case 83: /* exp: T_RETURN  */
#line 367 "input_parser.yy"
                   {yyval = gen(*yyvsp[0]._FUNCptr,0);}
#line 5027 "input_parser.cc"
    break;

  case 84: /* exp: T_QUOTE T_RETURN T_QUOTE  */
#line 368 "input_parser.yy"
                                   { yyval=yyvsp[-1];}
#line 5033 "input_parser.cc"
    break;

  case 85: /* exp: T_BREAK  */
#line 370 "input_parser.yy"
                        {yyval = symbolic(at_break,gen_zero);}
#line 5039 "input_parser.cc"
    break;

  case 86: /* exp: T_CONTINUE  */
#line 371 "input_parser.yy"
                        {yyval = symbolic(at_continue,gen_zero);}
#line 5045 "input_parser.cc"
    break;

  case 87: /* exp: T_FOR symbol_for T_IN exp T_DO prg_suite T_BLOC_END  */
#line 372 "input_parser.yy"
                                                              { 
	/*
	  gen kk(identificateur("index"));
	  vecteur v(*$6._VECTptr);
          const giac::context * contextptr = giac_yyget_extra(scanner);
	  v.insert(v.begin(),symb_sto(symb_at($4,kk,contextptr),$2));
	  $$=symbolic(*$1._FUNCptr,makevecteur(symb_sto(xcas_mode(contextptr)!=0,kk),symb_inferieur_strict(kk,symb_size($4)+(xcas_mode(contextptr)!=0)),symb_sto(symb_plus(kk,gen(1)),kk),symb_bloc(v))); 
          */
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9)
	    giac_yyerror(scanner,"missing loop end delimiter");
 	  bool rg=yyvsp[-3].is_symb_of_sommet(at_range);
          gen f=yyvsp[-3].type==_SYMB?yyvsp[-3]._SYMBptr->feuille:0,inc=1;
          if (rg){
            if (f.type!=_VECT) f=makesequence(0,f);
            vecteur v=*f._VECTptr;
            if (v.size()>=2) f=makesequence(v.front(),v[1]-1);
            if (v.size()==3) inc=v[2];
          }
          if (inc.type==_INT_  && inc.val!=0 && f.type==_VECT && f._VECTptr->size()==2 && (rg || (yyvsp[-3].is_symb_of_sommet(at_interval) 
	  // && f._VECTptr->front().type==_INT_ && f._VECTptr->back().type==_INT_ 
	  )))
            yyval=symbolic(*yyvsp[-6]._FUNCptr,makevecteur(symb_sto(f._VECTptr->front(),yyvsp[-5]),inc.val>0?symb_inferieur_egal(yyvsp[-5],f._VECTptr->back()):symb_superieur_egal(yyvsp[-5],f._VECTptr->back()),symb_sto(symb_plus(yyvsp[-5],inc),yyvsp[-5]),symb_bloc(yyvsp[-1])));
          else 
            yyval=symbolic(*yyvsp[-6]._FUNCptr,makevecteur(1,symbolic(*yyvsp[-6]._FUNCptr,makevecteur(yyvsp[-5],yyvsp[-3])),1,symb_bloc(yyvsp[-1])));
	  }
#line 5075 "input_parser.cc"
    break;

  case 88: /* exp: T_FOR symbol_for T_IN exp T_DO prg_suite T_ELSE prg_suite T_BLOC_END  */
#line 397 "input_parser.yy"
                                                                               { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9)
	    giac_yyerror(scanner,"missing loop end delimiter");
	  yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(1,symbolic(*yyvsp[-8]._FUNCptr,makevecteur(yyvsp[-7],yyvsp[-5],symb_bloc(yyvsp[-1]))),1,symb_bloc(yyvsp[-3])));
	  }
#line 5085 "input_parser.cc"
    break;

  case 89: /* exp: T_FOR symbol from T_TO exp step loop38_do prg_suite T_BLOC_END  */
#line 402 "input_parser.yy"
                                                                         { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9) giac_yyerror(scanner,"missing loop end delimiter");
          gen tmp,st=yyvsp[-3];  
       if (st==1 && yyvsp[-5]!=1) st=yyvsp[-5];
          const giac::context * contextptr = giac_yyget_extra(scanner);
	  if (!lidnt(st).empty())
            *logptr(contextptr) << "Warning, step is not numeric " << st << '\n';
          bool b=has_evalf(st,tmp,1,context0);
          if (!b || is_positive(tmp,context0)) 
             yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(symb_sto(yyvsp[-6],yyvsp[-7]),symb_inferieur_egal(yyvsp[-7],yyvsp[-4]),symb_sto(symb_plus(yyvsp[-7],b?abs(st,context0):symb_abs(st)),yyvsp[-7]),symb_bloc(yyvsp[-1]))); 
          else 
            yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(symb_sto(yyvsp[-6],yyvsp[-7]),symb_superieur_egal(yyvsp[-7],yyvsp[-4]),symb_sto(symb_plus(yyvsp[-7],st),yyvsp[-7]),symb_bloc(yyvsp[-1]))); 
        }
#line 5103 "input_parser.cc"
    break;

  case 90: /* exp: T_FOR symbol from step T_TO exp T_DO prg_suite T_BLOC_END  */
#line 415 "input_parser.yy"
                                                                    { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9) giac_yyerror(scanner,"missing loop end delimiter");
         gen tmp,st=yyvsp[-5]; 
        if (st==1 && yyvsp[-4]!=1) st=yyvsp[-4];
         const giac::context * contextptr = giac_yyget_extra(scanner);
	 if (!lidnt(st).empty())
            *logptr(contextptr) << "Warning, step is not numeric " << st << '\n';
         bool b=has_evalf(st,tmp,1,context0);
         if (!b || is_positive(tmp,context0)) 
           yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(symb_sto(yyvsp[-6],yyvsp[-7]),symb_inferieur_egal(yyvsp[-7],yyvsp[-3]),symb_sto(symb_plus(yyvsp[-7],b?abs(st,context0):symb_abs(st)),yyvsp[-7]),symb_bloc(yyvsp[-1]))); 
         else 
           yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(symb_sto(yyvsp[-6],yyvsp[-7]),symb_superieur_egal(yyvsp[-7],yyvsp[-3]),symb_sto(symb_plus(yyvsp[-7],st),yyvsp[-7]),symb_bloc(yyvsp[-1]))); 
        }
#line 5121 "input_parser.cc"
    break;

  case 91: /* exp: T_FOR symbol from step T_DO prg_suite T_BLOC_END  */
#line 428 "input_parser.yy"
                                                           { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9) giac_yyerror(scanner,"missing loop end delimiter");
          yyval=symbolic(*yyvsp[-6]._FUNCptr,makevecteur(symb_sto(yyvsp[-4],yyvsp[-5]),gen(1),symb_sto(symb_plus(yyvsp[-5],yyvsp[-3]),yyvsp[-5]),symb_bloc(yyvsp[-1]))); 
        }
#line 5130 "input_parser.cc"
    break;

  case 92: /* exp: T_FOR symbol from step T_MUPMAP_WHILE exp T_DO prg_suite T_BLOC_END  */
#line 432 "input_parser.yy"
                                                                              { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9 && yyvsp[0].val!=8) giac_yyerror(scanner,"missing loop end delimiter");
          yyval=symbolic(*yyvsp[-8]._FUNCptr,makevecteur(symb_sto(yyvsp[-6],yyvsp[-7]),yyvsp[-3],symb_sto(symb_plus(yyvsp[-7],yyvsp[-5]),yyvsp[-7]),symb_bloc(yyvsp[-1]))); 
        }
#line 5139 "input_parser.cc"
    break;

  case 93: /* exp: T_FOR  */
#line 436 "input_parser.yy"
                {yyval = gen(*yyvsp[0]._FUNCptr,4);}
#line 5145 "input_parser.cc"
    break;

  case 94: /* exp: T_REPEAT prg_suite T_UNTIL exp  */
#line 441 "input_parser.yy"
                                         { 
        vecteur v=gen2vecteur(yyvsp[-2]);
        v.push_back(symb_ifte(equaltosame(yyvsp[0]),symbolic(at_break,gen_zero),0));
	yyval=symbolic(*yyvsp[-3]._FUNCptr,makevecteur(gen_zero,1,gen_zero,symb_bloc(v))); 
	}
#line 5155 "input_parser.cc"
    break;

  case 95: /* exp: T_REPEAT prg_suite T_UNTIL exp T_BLOC_END  */
#line 446 "input_parser.yy"
                                                    { 
        if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=2 && yyvsp[0].val!=9) giac_yyerror(scanner,"missing loop end delimiter");
        vecteur v=gen2vecteur(yyvsp[-3]);
        v.push_back(symb_ifte(equaltosame(yyvsp[-1]),symbolic(at_break,gen_zero),0));
	yyval=symbolic(*yyvsp[-4]._FUNCptr,makevecteur(gen_zero,1,gen_zero,symb_bloc(v))); 
	}
#line 5166 "input_parser.cc"
    break;

  case 96: /* exp: T_IFERR prg_suite T_THEN prg_suite T_ELSE prg_suite T_BLOC_END  */
#line 452 "input_parser.yy"
                                                                         {
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=4) giac_yyerror(scanner,"missing iferr end delimiter");
           yyval=symbolic(at_try_catch,makevecteur(symb_bloc(yyvsp[-5]),0,symb_bloc(yyvsp[-3]),symb_bloc(yyvsp[-1])));
        }
#line 5175 "input_parser.cc"
    break;

  case 97: /* exp: T_IFERR prg_suite T_THEN prg_suite T_BLOC_END  */
#line 456 "input_parser.yy"
                                                        {
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=4) giac_yyerror(scanner,"missing iferr end delimiter");
           yyval=symbolic(at_try_catch,makevecteur(symb_bloc(yyvsp[-3]),0,symb_bloc(yyvsp[-1]),symb_bloc(0)));
        }
#line 5184 "input_parser.cc"
    break;

  case 98: /* exp: T_CASE38 case38 T_BLOC_END  */
#line 460 "input_parser.yy"
                                     {yyval=symbolic(at_piecewise,yyvsp[-1]); }
#line 5190 "input_parser.cc"
    break;

  case 99: /* exp: T_TYPE_ID  */
#line 461 "input_parser.yy"
                    { 
	yyval=yyvsp[0]; 
	// $$.subtype=1; 
	}
#line 5199 "input_parser.cc"
    break;

  case 100: /* exp: T_QUOTE T_TYPE_ID T_QUOTE  */
#line 465 "input_parser.yy"
                                    { yyval=yyvsp[-1]; /* $$.subtype=1; */ }
#line 5205 "input_parser.cc"
    break;

  case 101: /* exp: T_DOLLAR_MAPLE exp  */
#line 466 "input_parser.yy"
                             { yyval = symb_dollar(yyvsp[0]); }
#line 5211 "input_parser.cc"
    break;

  case 102: /* exp: exp T_DOLLAR_MAPLE symbol T_IN exp  */
#line 467 "input_parser.yy"
                                             {yyval=symb_dollar(gen(makevecteur(yyvsp[-4],yyvsp[-2],yyvsp[0]) ,_SEQ__VECT));}
#line 5217 "input_parser.cc"
    break;

  case 103: /* exp: exp T_DOLLAR_MAPLE exp  */
#line 468 "input_parser.yy"
                                 { yyval = symb_dollar(gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 5223 "input_parser.cc"
    break;

  case 104: /* exp: exp T_DOLLAR exp  */
#line 469 "input_parser.yy"
                           { yyval = symb_dollar(gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 5229 "input_parser.cc"
    break;

  case 105: /* exp: T_DOLLAR T_SYMBOL  */
#line 470 "input_parser.yy"
                            { yyval=symb_dollar(yyvsp[0]); }
#line 5235 "input_parser.cc"
    break;

  case 106: /* exp: exp T_COMPOSE exp  */
#line 471 "input_parser.yy"
                            {  //CERR << $1 << " compose " << $2 << $3 << '\n';
yyval = symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],python_compat(giac_yyget_extra(scanner))?denest_sto(yyvsp[0]):yyvsp[0]) ,_SEQ__VECT)); }
#line 5242 "input_parser.cc"
    break;

  case 107: /* exp: T_COMPOSE  */
#line 473 "input_parser.yy"
                    {yyval=symbolic(at_ans,-1);}
#line 5248 "input_parser.cc"
    break;

  case 108: /* exp: exp T_UNION exp  */
#line 474 "input_parser.yy"
                          { yyval = symbolic((yyvsp[-1].type==_FUNC?*yyvsp[-1]._FUNCptr:*at_union),gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 5254 "input_parser.cc"
    break;

  case 109: /* exp: exp T_UNION exp T_MOD  */
#line 475 "input_parser.yy"
                                { yyval = symbolic((yyvsp[-2].type==_FUNC?*yyvsp[-2]._FUNCptr:*at_union),gen(makevecteur(yyvsp[-3],yyvsp[-3]*yyvsp[-1]/100) ,_SEQ__VECT)); }
#line 5260 "input_parser.cc"
    break;

  case 110: /* exp: exp T_INTERSECT exp  */
#line 476 "input_parser.yy"
                              { yyval = symb_intersect(gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 5266 "input_parser.cc"
    break;

  case 111: /* exp: exp T_MINUS exp  */
#line 477 "input_parser.yy"
                          { yyval = symb_minus(gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); }
#line 5272 "input_parser.cc"
    break;

  case 112: /* exp: exp T_PIPE exp  */
#line 478 "input_parser.yy"
                         { 
	yyval=symbolic(*yyvsp[-1]._FUNCptr,gen(makevecteur(yyvsp[-2],yyvsp[0]) ,_SEQ__VECT)); 
	}
#line 5280 "input_parser.cc"
    break;

  case 113: /* exp: T_QUOTED_BINARY  */
#line 481 "input_parser.yy"
                          { yyval = yyvsp[0]; }
#line 5286 "input_parser.cc"
    break;

  case 114: /* exp: T_QUOTE exp T_QUOTE  */
#line 482 "input_parser.yy"
                                        {if (yyvsp[-1].type==_FUNC) yyval=yyvsp[-1]; else { 
          // const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_quote(yyvsp[-1]);
          } 
        }
#line 5296 "input_parser.cc"
    break;

  case 115: /* exp: exp T_INDEX_BEGIN exp T_VECT_END  */
#line 487 "input_parser.yy"
                                                {
          const giac::context * contextptr = giac_yyget_extra(scanner);
	  yyval = symb_at(yyvsp[-3],yyvsp[-1],contextptr);
        }
#line 5305 "input_parser.cc"
    break;

  case 116: /* exp: exp T_INDEX_BEGIN T_VECT_DISPATCH exp T_VECT_END T_VECT_END  */
#line 491 "input_parser.yy"
                                                                        {
          const giac::context * contextptr = giac_yyget_extra(scanner);
	  yyval = symbolic(at_of,gen(makevecteur(yyvsp[-5],yyvsp[-2]) ,_SEQ__VECT));
        }
#line 5314 "input_parser.cc"
    break;

  case 117: /* exp: T_BEGIN_PAR exp T_END_PAR T_BEGIN_PAR suite T_END_PAR  */
#line 495 "input_parser.yy"
                                                                {yyval = check_symb_of(yyvsp[-4],yyvsp[-1],giac_yyget_extra(scanner));}
#line 5320 "input_parser.cc"
    break;

  case 118: /* exp: T_BEGIN_PAR exp T_END_PAR  */
#line 496 "input_parser.yy"
                                                {
	if ( (yyvsp[-2]==_LIST__VECT && python_compat(giac_yyget_extra(scanner))) ||
              python_compat(giac_yyget_extra(scanner))==2){
           if (python_compat(giac_yyget_extra(scanner))==2)
             yyval=change_subtype(yyvsp[-1],_TUPLE__VECT);
           else
             yyval=symbolic(at_python_list,yyvsp[-1]);
        }
        else {
 	 if (abs_calc_mode(giac_yyget_extra(scanner))==38 && yyvsp[-1].type==_VECT && yyvsp[-1].subtype==_SEQ__VECT && yyvsp[-1]._VECTptr->size()==2 && (yyvsp[-1]._VECTptr->front().type<=_DOUBLE_ || yyvsp[-1]._VECTptr->front().type==_FLOAT_) && (yyvsp[-1]._VECTptr->back().type<=_DOUBLE_ || yyvsp[-1]._VECTptr->back().type==_FLOAT_)){ 
           const giac::context * contextptr = giac_yyget_extra(scanner);
	   gen a=evalf(yyvsp[-1]._VECTptr->front(),1,contextptr),
	       b=evalf(yyvsp[-1]._VECTptr->back(),1,contextptr);
	   if ( (a.type==_DOUBLE_ || a.type==_FLOAT_) &&
                (b.type==_DOUBLE_ || b.type==_FLOAT_))
             yyval= a+b*cst_i; 
           else yyval=yyvsp[-1];
  	 } else {
              if (calc_mode(giac_yyget_extra(scanner))==1 && yyvsp[-1].type==_VECT && yyvsp[-2]!=_LIST__VECT &&
	      yyvsp[-1].subtype==_SEQ__VECT && (yyvsp[-1]._VECTptr->size()==2 || yyvsp[-1]._VECTptr->size()==3) )
                yyval = gen(*yyvsp[-1]._VECTptr,_GGB__VECT);
              else
                yyval=yyvsp[-1];
          }
	 }
        }
#line 5351 "input_parser.cc"
    break;

  case 119: /* exp: T_VECT_DISPATCH suite T_VECT_END  */
#line 522 "input_parser.yy"
                                           { 
        //cerr << $1 << " " << $2 << '\n';
        yyval = gen(*(yyvsp[-1]._VECTptr),yyvsp[-2].val);
	if (yyvsp[-1]._VECTptr->size()==1 && yyvsp[-1]._VECTptr->front().is_symb_of_sommet(at_ti_semi) ) {
	  yyval=yyvsp[-1]._VECTptr->front();
        }
        // cerr << $$ << '\n';

        }
#line 5365 "input_parser.cc"
    break;

  case 120: /* exp: exp T_VIRGULE exp  */
#line 531 "input_parser.yy"
                                      { 
         if (yyvsp[-2].type==_VECT && yyvsp[-2].subtype==_SEQ__VECT && !(yyvsp[0].type==_VECT && yyvsp[-1].subtype==_SEQ__VECT)){ yyval=yyvsp[-2]; yyval._VECTptr->push_back(yyvsp[0]); }
	 else
           yyval = makesuite(yyvsp[-2],yyvsp[0]); 

        }
#line 5376 "input_parser.cc"
    break;

  case 121: /* exp: T_NULL  */
#line 537 "input_parser.yy"
                 { yyval=gen(vecteur(0),_SEQ__VECT); }
#line 5382 "input_parser.cc"
    break;

  case 122: /* exp: T_HELP exp  */
#line 538 "input_parser.yy"
                     {yyval=symb_findhelp(yyvsp[0]);}
#line 5388 "input_parser.cc"
    break;

  case 123: /* exp: exp T_INTERROGATION exp  */
#line 539 "input_parser.yy"
                                  { yyval=symb_interrogation(yyvsp[-2],yyvsp[0]); }
#line 5394 "input_parser.cc"
    break;

  case 124: /* exp: T_UNIT exp  */
#line 540 "input_parser.yy"
                     {
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_unit(gen(1),yyvsp[0],contextptr); 
          opened_quote(giac_yyget_extra(scanner)) &= 0x7ffffffd;	
        }
#line 5404 "input_parser.cc"
    break;

  case 125: /* exp: exp T_UNIT exp  */
#line 545 "input_parser.yy"
                         {
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_unit(yyvsp[-2],yyvsp[0],contextptr); 
          opened_quote(giac_yyget_extra(scanner)) &= 0x7ffffffd;        }
#line 5413 "input_parser.cc"
    break;

  case 126: /* exp: exp T_SQ  */
#line 549 "input_parser.yy"
                   { yyval=symb_pow(yyvsp[-1],yyvsp[0]); }
#line 5419 "input_parser.cc"
    break;

  case 127: /* exp: error  */
#line 550 "input_parser.yy"
                { 
        const giac::context * contextptr = giac_yyget_extra(scanner);
#ifdef HAVE_SIGNAL_H_OLD
	messages_to_print += parser_filename(contextptr) + parser_error(contextptr); 
	/* *logptr(giac_yyget_extra(scanner)) << messages_to_print; */
#endif
	yyval=undef;
        spread_formula(false,contextptr); 
	}
#line 5433 "input_parser.cc"
    break;

  case 128: /* exp: stack  */
#line 559 "input_parser.yy"
                { yyval=yyvsp[0]; }
#line 5439 "input_parser.cc"
    break;

  case 129: /* exp: T_LOGO exp  */
#line 560 "input_parser.yy"
                      { yyval=symbolic(*yyvsp[-1]._FUNCptr,yyvsp[0]); }
#line 5445 "input_parser.cc"
    break;

  case 130: /* exp: T_LOGO  */
#line 561 "input_parser.yy"
                 {yyval = symbolic(*yyvsp[0]._FUNCptr,gen(vecteur(0),_SEQ__VECT));}
#line 5451 "input_parser.cc"
    break;

  case 131: /* exp: T_LOGO T_BEGIN_PAR T_END_PAR  */
#line 562 "input_parser.yy"
                                        {yyval = symbolic(*yyvsp[-2]._FUNCptr,gen(vecteur(0),_SEQ__VECT));}
#line 5457 "input_parser.cc"
    break;

  case 132: /* exp: T_LOCALBLOC T_BEGIN_PAR exp T_END_PAR  */
#line 563 "input_parser.yy"
                                                {
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval = symb_local(yyvsp[-1],contextptr);
        }
#line 5466 "input_parser.cc"
    break;

  case 133: /* exp: T_LOCALBLOC  */
#line 567 "input_parser.yy"
                      {yyval = gen(at_local,2);}
#line 5472 "input_parser.cc"
    break;

  case 134: /* exp: T_IF T_BEGIN_PAR exp T_END_PAR bloc else  */
#line 568 "input_parser.yy"
                                                   {
	yyval = symbolic(*yyvsp[-5]._FUNCptr,makevecteur(equaltosame(yyvsp[-3]),symb_bloc(yyvsp[-1]),yyvsp[0]));
	}
#line 5480 "input_parser.cc"
    break;

  case 135: /* exp: T_IF T_BEGIN_PAR exp T_END_PAR exp T_SEMI else  */
#line 571 "input_parser.yy"
                                                         {
        vecteur v=makevecteur(equaltosame(yyvsp[-4]),yyvsp[-2],yyvsp[0]);
	// *logptr(giac_yyget_extra(scanner)) << v << '\n';
	yyval = symbolic(*yyvsp[-6]._FUNCptr,v);
	}
#line 5490 "input_parser.cc"
    break;

  case 136: /* exp: T_RPN_BEGIN rpn_suite T_RPN_END  */
#line 576 "input_parser.yy"
                                          { yyval=symb_rpn_prog(yyvsp[-1]); }
#line 5496 "input_parser.cc"
    break;

  case 137: /* exp: T_MAPLELIB  */
#line 577 "input_parser.yy"
                          { yyval=yyvsp[0]; }
#line 5502 "input_parser.cc"
    break;

  case 138: /* exp: T_MAPLELIB T_INDEX_BEGIN exp T_VECT_END  */
#line 578 "input_parser.yy"
                                                  { yyval=symbolic(at_maple_lib,makevecteur(yyvsp[-3],yyvsp[-1])); }
#line 5508 "input_parser.cc"
    break;

  case 139: /* exp: T_PROC T_BEGIN_PAR suite T_END_PAR entete prg_suite T_BLOC_END  */
#line 579 "input_parser.yy"
                                                                         { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program(yyvsp[-4],gen_zero*yyvsp[-4],symb_local(yyvsp[-2],yyvsp[-1],contextptr),contextptr); 
        }
#line 5518 "input_parser.cc"
    break;

  case 140: /* exp: T_PROC symbol T_BEGIN_PAR suite T_END_PAR entete prg_suite T_BLOC_END  */
#line 584 "input_parser.yy"
                                                                                { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program_sto(yyvsp[-4],gen_zero*yyvsp[-4],symb_local(yyvsp[-2],yyvsp[-1],contextptr),yyvsp[-6],false,contextptr); 
        }
#line 5528 "input_parser.cc"
    break;

  case 141: /* exp: T_PROC symbol T_BEGIN_PAR suite T_END_PAR prg_suite T_BLOC_END  */
#line 589 "input_parser.yy"
                                                                         { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program_sto(yyvsp[-3],gen_zero*yyvsp[-3],symb_bloc(yyvsp[-1]),yyvsp[-5],false,contextptr); 
        }
#line 5538 "input_parser.cc"
    break;

  case 142: /* exp: T_PROC symbol T_BEGIN_PAR suite T_END_PAR T_BLOC_BEGIN entete prg_suite T_BLOC_END  */
#line 594 "input_parser.yy"
                                                                                             { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program_sto(yyvsp[-5],gen_zero*yyvsp[-5],symb_local(yyvsp[-2],yyvsp[-1],contextptr),yyvsp[-7],false,contextptr); 
        }
#line 5548 "input_parser.cc"
    break;

  case 143: /* exp: T_PROC T_BEGIN_PAR suite T_END_PAR entete T_BLOC_BEGIN prg_suite T_BLOC_END  */
#line 599 "input_parser.yy"
                                                                                      { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
         yyval=symb_program(yyvsp[-5],gen_zero*yyvsp[-5],symb_local(yyvsp[-3],yyvsp[-1],contextptr),contextptr); 
        }
#line 5558 "input_parser.cc"
    break;

  case 144: /* exp: symbol T_BEGIN_PAR suite T_END_PAR T_PROC entete prg_suite T_BLOC_END  */
#line 604 "input_parser.yy"
                                                                                { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program_sto(yyvsp[-5],gen_zero*yyvsp[-5],symb_local(yyvsp[-2],yyvsp[-1],contextptr),yyvsp[-7],false,contextptr); 
        }
#line 5568 "input_parser.cc"
    break;

  case 145: /* exp: symbol T_BEGIN_PAR suite T_END_PAR T_AFFECT T_PROC entete prg_suite T_BLOC_END  */
#line 609 "input_parser.yy"
                                                                                         { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=3) giac_yyerror(scanner,"missing func/prog/proc end delimiter");
          const giac::context * contextptr = giac_yyget_extra(scanner);
           yyval=symb_program_sto(yyvsp[-6],gen_zero*yyvsp[-6],symb_local(yyvsp[-2],yyvsp[-1],contextptr),yyvsp[-8],false,contextptr); 
        }
#line 5578 "input_parser.cc"
    break;

  case 146: /* exp: T_FOR T_BEGIN_PAR exp_or_empty T_SEMI exp_or_empty T_SEMI exp_or_empty T_END_PAR bloc  */
#line 614 "input_parser.yy"
                                                                                                 {yyval = symbolic(*yyvsp[-8]._FUNCptr,makevecteur(yyvsp[-6],equaltosame(yyvsp[-4]),yyvsp[-2],symb_bloc(yyvsp[0])));}
#line 5584 "input_parser.cc"
    break;

  case 147: /* exp: T_FOR T_BEGIN_PAR exp_or_empty T_SEMI exp_or_empty T_SEMI exp_or_empty T_END_PAR exp T_SEMI  */
#line 615 "input_parser.yy"
                                                                                                       {yyval = symbolic(*yyvsp[-9]._FUNCptr,makevecteur(yyvsp[-7],equaltosame(yyvsp[-5]),yyvsp[-3],yyvsp[-1]));}
#line 5590 "input_parser.cc"
    break;

  case 148: /* exp: T_FOR T_BEGIN_PAR exp T_END_PAR  */
#line 616 "input_parser.yy"
                                                {yyval = symbolic(*yyvsp[-3]._FUNCptr,gen2vecteur(yyvsp[-1]));}
#line 5596 "input_parser.cc"
    break;

  case 149: /* exp: exp T_IN exp  */
#line 617 "input_parser.yy"
                       {yyval=symbolic(at_member,makesequence(yyvsp[-2],yyvsp[0])); if (yyvsp[-1]==at_not) yyval=symbolic(at_not,yyval);}
#line 5602 "input_parser.cc"
    break;

  case 150: /* exp: exp T_NOT T_IN exp  */
#line 618 "input_parser.yy"
                             {yyval=symbolic(at_not,symbolic(at_member,makesequence(yyvsp[-3],yyvsp[0])));}
#line 5608 "input_parser.cc"
    break;

  case 151: /* exp: T_VECT_DISPATCH exp T_FOR suite_symbol T_IN exp T_VECT_END  */
#line 619 "input_parser.yy"
                                                                     { yyval=symbolic(at_apply,makesequence(symbolic(at_program,makesequence(yyvsp[-3],0*yyvsp[-3],vecteur(1,yyvsp[-5]))),yyvsp[-1])); if (yyvsp[-6]==_TABLE__VECT) yyval=symbolic(at_table,yyval);}
#line 5614 "input_parser.cc"
    break;

  case 152: /* exp: T_VECT_DISPATCH exp T_FOR suite_symbol T_IN exp T_IF exp T_VECT_END  */
#line 620 "input_parser.yy"
                                                                              { yyval=symbolic(at_apply,symbolic(at_program,makesequence(yyvsp[-5],0*yyvsp[-5],vecteur(1,yyvsp[-7]))),symbolic(at_select,makesequence(symbolic(at_program,makesequence(yyvsp[-5],0*yyvsp[-5],yyvsp[-1])),yyvsp[-3]))); if (yyvsp[-8]==_TABLE__VECT) yyval=symbolic(at_table,yyval);}
#line 5620 "input_parser.cc"
    break;

  case 153: /* exp: T_WHILE T_BEGIN_PAR exp T_END_PAR bloc  */
#line 621 "input_parser.yy"
                                                 { 
	vecteur v=makevecteur(gen_zero,equaltosame(yyvsp[-2]),gen_zero,symb_bloc(yyvsp[0]));
	yyval=symbolic(*yyvsp[-4]._FUNCptr,v); 
	}
#line 5629 "input_parser.cc"
    break;

  case 154: /* exp: T_WHILE T_BEGIN_PAR exp T_END_PAR exp T_SEMI  */
#line 625 "input_parser.yy"
                                                       { 
	yyval=symbolic(*yyvsp[-5]._FUNCptr,makevecteur(gen_zero,equaltosame(yyvsp[-3]),gen_zero,yyvsp[-1])); 
	}
#line 5637 "input_parser.cc"
    break;

  case 155: /* exp: T_WHILE exp T_DO prg_suite T_BLOC_END  */
#line 628 "input_parser.yy"
                                                { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=9 && yyvsp[0].val!=8) giac_yyerror(scanner,"missing loop end delimiter");
	  yyval=symbolic(*yyvsp[-4]._FUNCptr,makevecteur(gen_zero,equaltosame(yyvsp[-3]),gen_zero,symb_bloc(yyvsp[-1]))); 
        }
#line 5646 "input_parser.cc"
    break;

  case 156: /* exp: T_MUPMAP_WHILE exp T_DO prg_suite T_BLOC_END  */
#line 632 "input_parser.yy"
                                                       { 
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=9 && yyvsp[0].val!=8) giac_yyerror(scanner,"missing loop end delimiter");
          yyval=symbolic(*yyvsp[-4]._FUNCptr,makevecteur(gen_zero,equaltosame(yyvsp[-3]),gen_zero,symb_bloc(yyvsp[-1]))); 
        }
#line 5655 "input_parser.cc"
    break;

  case 157: /* exp: T_TRY bloc T_CATCH T_BEGIN_PAR exp T_END_PAR bloc  */
#line 636 "input_parser.yy"
                                                            { yyval=symb_try_catch(makevecteur(symb_bloc(yyvsp[-5]),yyvsp[-2],symb_bloc(yyvsp[0])));}
#line 5661 "input_parser.cc"
    break;

  case 158: /* exp: T_TRY_CATCH T_BEGIN_PAR exp T_END_PAR  */
#line 637 "input_parser.yy"
                                                {yyval=symb_try_catch(gen2vecteur(yyvsp[-1]));}
#line 5667 "input_parser.cc"
    break;

  case 159: /* exp: T_TRY_CATCH  */
#line 638 "input_parser.yy"
                      {yyval=gen(at_try_catch,3);}
#line 5673 "input_parser.cc"
    break;

  case 160: /* exp: T_SWITCH T_BEGIN_PAR exp T_END_PAR T_BLOC_BEGIN switch T_BLOC_END  */
#line 639 "input_parser.yy"
                                                                            { yyval=symb_case(yyvsp[-4],yyvsp[-1]); }
#line 5679 "input_parser.cc"
    break;

  case 161: /* exp: T_CASE T_BEGIN_PAR T_SYMBOL T_END_PAR  */
#line 640 "input_parser.yy"
                                                { yyval = symb_case(yyvsp[-1]); }
#line 5685 "input_parser.cc"
    break;

  case 162: /* exp: T_CASE exp case T_ENDCASE  */
#line 641 "input_parser.yy"
                                    { yyval=symb_case(yyvsp[-2],yyvsp[-1]); }
#line 5691 "input_parser.cc"
    break;

  case 163: /* exp: T_ACCENTGRAVE rpn_token T_ACCENTGRAVE  */
#line 642 "input_parser.yy"
                                                { yyval=yyvsp[-1]; }
#line 5697 "input_parser.cc"
    break;

  case 164: /* exp: T_RPN_OP  */
#line 643 "input_parser.yy"
                   { yyval=yyvsp[0]; }
#line 5703 "input_parser.cc"
    break;

  case 165: /* exp: T_RETURN TI_DEUXPOINTS  */
#line 644 "input_parser.yy"
                                 {yyval = gen(*yyvsp[-1]._FUNCptr,0);}
#line 5709 "input_parser.cc"
    break;

  case 166: /* exp: TI_LOOP prg_suite ti_bloc_end  */
#line 645 "input_parser.yy"
                                        { yyval=symbolic(*yyvsp[-2]._FUNCptr,makevecteur(gen_zero,gen(1),gen_zero,symb_bloc(yyvsp[-1]))); }
#line 5715 "input_parser.cc"
    break;

  case 167: /* exp: T_IF exp TI_DEUXPOINTS exp  */
#line 646 "input_parser.yy"
                                     {yyval = symbolic(*yyvsp[-3]._FUNCptr,makevecteur(equaltosame(yyvsp[-2]),yyvsp[0],0));}
#line 5721 "input_parser.cc"
    break;

  case 168: /* exp: TI_TRY prg_suite T_ELSE prg_suite ti_bloc_end  */
#line 647 "input_parser.yy"
                                                        { yyval=symb_try_catch(makevecteur(symb_bloc(yyvsp[-3]),at_break,symb_bloc(yyvsp[-1]))); }
#line 5727 "input_parser.cc"
    break;

  case 169: /* exp: TI_TRY prg_suite T_ELSE ti_bloc_end  */
#line 648 "input_parser.yy"
                                              { yyval=symb_try_catch(makevecteur(symb_bloc(yyvsp[-2]),at_break,0)); }
#line 5733 "input_parser.cc"
    break;

  case 170: /* exp: TI_TRY prg_suite TI_DEUXPOINTS T_ELSE prg_suite ti_bloc_end  */
#line 649 "input_parser.yy"
                                                                      { yyval=symb_try_catch(makevecteur(symb_bloc(yyvsp[-4]),at_break,symb_bloc(yyvsp[-1]))); }
#line 5739 "input_parser.cc"
    break;

  case 171: /* exp: TI_TRY prg_suite TI_DEUXPOINTS T_ELSE ti_bloc_end  */
#line 650 "input_parser.yy"
                                                            { yyval=symb_try_catch(makevecteur(symb_bloc(yyvsp[-3]),at_break,0)); }
#line 5745 "input_parser.cc"
    break;

  case 172: /* exp: exp TI_SEMI exp  */
#line 651 "input_parser.yy"
                                    { vecteur v1(gen2vecteur(yyvsp[-2])),v3(gen2vecteur(yyvsp[0])); yyval=symbolic(at_ti_semi,makevecteur(v1,v3)); }
#line 5751 "input_parser.cc"
    break;

  case 173: /* exp: TI_DEUXPOINTS symbol T_BEGIN_PAR suite T_END_PAR TI_PRGM prg_suite TI_DEUXPOINTS TI_LOCAL suite TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 652 "input_parser.yy"
                                                                                                                                              { 
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_program_sto(yyvsp[-9],yyvsp[-9]*gen_zero,symb_local(yyvsp[-3],mergevecteur(*yyvsp[-6]._VECTptr,*yyvsp[-1]._VECTptr),contextptr),yyvsp[-11],false,contextptr); 
	}
#line 5760 "input_parser.cc"
    break;

  case 174: /* exp: TI_DEUXPOINTS symbol T_BEGIN_PAR suite T_END_PAR TI_PRGM prg_suite TI_LOCAL suite TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 656 "input_parser.yy"
                                                                                                                                { 
          const giac::context * contextptr = giac_yyget_extra(scanner);
	yyval=symb_program_sto(yyvsp[-8],yyvsp[-8]*gen_zero,symb_local(yyvsp[-3],mergevecteur(*yyvsp[-5]._VECTptr,*yyvsp[-1]._VECTptr),contextptr),yyvsp[-10],false,contextptr); 
	}
#line 5769 "input_parser.cc"
    break;

  case 175: /* exp: TI_DEUXPOINTS symbol T_BEGIN_PAR suite T_END_PAR TI_PRGM TI_DEUXPOINTS TI_LOCAL suite TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 660 "input_parser.yy"
                                                                                                                                    { 
          const giac::context * contextptr = giac_yyget_extra(scanner);
	yyval=symb_program_sto(yyvsp[-8],yyvsp[-8]*gen_zero,symb_local(yyvsp[-3],yyvsp[-1],contextptr),yyvsp[-10],false,contextptr); 
	}
#line 5778 "input_parser.cc"
    break;

  case 176: /* exp: TI_DEUXPOINTS symbol T_BEGIN_PAR suite T_END_PAR TI_PRGM prg_suite ti_bloc_end  */
#line 664 "input_parser.yy"
                                                                                         { 
	yyval=symb_program_sto(yyvsp[-4],yyvsp[-4]*gen_zero,symb_bloc(yyvsp[-1]),yyvsp[-6],false,giac_yyget_extra(scanner)); 
	}
#line 5786 "input_parser.cc"
    break;

  case 177: /* exp: TI_DIALOG prg_suite ti_bloc_end  */
#line 667 "input_parser.yy"
                                          { yyval=symbolic(*yyvsp[-2]._FUNCptr,yyvsp[-1]); }
#line 5792 "input_parser.cc"
    break;

  case 178: /* exp: TI_DIALOG bloc  */
#line 668 "input_parser.yy"
                         { yyval=symbolic(*yyvsp[-1]._FUNCptr,yyvsp[0]); }
#line 5798 "input_parser.cc"
    break;

  case 179: /* exp: TI_DEUXPOINTS exp  */
#line 669 "input_parser.yy"
                            { yyval=yyvsp[0]; }
#line 5804 "input_parser.cc"
    break;

  case 180: /* exp: TI_DEFINE symbol T_BEGIN_PAR suite T_END_PAR T_EQUAL exp  */
#line 670 "input_parser.yy"
                                                                   { yyval=symb_program_sto(yyvsp[-3],yyvsp[-3]*gen_zero,yyvsp[0],yyvsp[-5],false,giac_yyget_extra(scanner));}
#line 5810 "input_parser.cc"
    break;

  case 181: /* exp: TI_DEFINE symbol T_BEGIN_PAR suite T_END_PAR T_EQUAL TI_PRGM TI_DEUXPOINTS TI_LOCAL suite TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 671 "input_parser.yy"
                                                                                                                                        { 
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval=symb_program_sto(yyvsp[-9],yyvsp[-9]*gen_zero,symb_local(yyvsp[-3],yyvsp[-1],contextptr),yyvsp[-11],false,contextptr);
        }
#line 5819 "input_parser.cc"
    break;

  case 182: /* exp: TI_DEFINE symbol T_BEGIN_PAR suite T_END_PAR T_EQUAL TI_PRGM prg_suite ti_bloc_end  */
#line 675 "input_parser.yy"
                                                                                             { yyval=symb_program_sto(yyvsp[-5],yyvsp[-5]*gen_zero,symb_bloc(yyvsp[-1]),yyvsp[-7],false,giac_yyget_extra(scanner)); }
#line 5825 "input_parser.cc"
    break;

  case 183: /* exp: TI_FOR suite TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 676 "input_parser.yy"
                                                           {
           vecteur & v=*yyvsp[-3]._VECTptr;
           if ( (v.size()<3) || v[0].type!=_IDNT){
             *logptr(giac_yyget_extra(scanner)) << "Syntax For name,begin,end[,step]" << '\n';
             yyval=undef;
           }
           else {
             gen pas(gen(1));
             if (v.size()==4)
               pas=v[3];
             gen condition;
             if (is_positive(-pas,0))
               condition=symb_superieur_egal(v[0],v[2]);
            else
               condition=symb_inferieur_egal(v[0],v[2]);
            vecteur w=makevecteur(symb_sto(v[1],v[0]),condition,symb_sto(symb_plus(v[0],pas),v[0]),symb_bloc(yyvsp[-1]));
             yyval=symbolic(*yyvsp[-4]._FUNCptr,w);
           }
	}
#line 5849 "input_parser.cc"
    break;

  case 184: /* exp: TI_WHILE exp TI_DEUXPOINTS prg_suite ti_bloc_end  */
#line 695 "input_parser.yy"
                                                           { 
	vecteur v=makevecteur(gen_zero,equaltosame(yyvsp[-3]),gen_zero,symb_bloc(yyvsp[-1]));
	yyval=symbolic(*yyvsp[-4]._FUNCptr,v); 
	}
#line 5858 "input_parser.cc"
    break;

  case 185: /* symbol_for: T_SYMBOL  */
#line 707 "input_parser.yy"
                      { yyval=yyvsp[0]; }
#line 5864 "input_parser.cc"
    break;

  case 186: /* symbol_for: T_UNARY_OP  */
#line 708 "input_parser.yy"
                     { yyval=yyvsp[0]; }
#line 5870 "input_parser.cc"
    break;

  case 187: /* symbol_for: T_UNARY_OP_38  */
#line 709 "input_parser.yy"
                        { yyval=yyvsp[0]; }
#line 5876 "input_parser.cc"
    break;

  case 188: /* symbol: T_SYMBOL  */
#line 712 "input_parser.yy"
                   { yyval=yyvsp[0]; }
#line 5882 "input_parser.cc"
    break;

  case 189: /* symbol: T_SYMBOL T_DOUBLE_DEUX_POINTS T_TYPE_ID  */
#line 713 "input_parser.yy"
                                                  { 
	       gen tmp(yyvsp[0]); 
	       // tmp.subtype=1; 
	       //$$=symb_check_type(makevecteur(tmp,$1),context0); 
               yyval=symbolic(at_deuxpoints,makesequence(yyvsp[-2],yyvsp[0]));
          }
#line 5893 "input_parser.cc"
    break;

  case 190: /* symbol: T_SYMBOL T_DOUBLE_DEUX_POINTS T_UNARY_OP  */
#line 719 "input_parser.yy"
                                                   { yyval=symb_double_deux_points(makevecteur(yyvsp[-2],yyvsp[0])); }
#line 5899 "input_parser.cc"
    break;

  case 191: /* symbol: T_SYMBOL T_DOUBLE_DEUX_POINTS T_SYMBOL  */
#line 720 "input_parser.yy"
                                                 { yyval=symb_double_deux_points(makevecteur(yyvsp[-2],yyvsp[0])); }
#line 5905 "input_parser.cc"
    break;

  case 192: /* symbol: T_SYMBOL T_DOUBLE_DEUX_POINTS T_UNARY_OP_38  */
#line 721 "input_parser.yy"
                                                      { yyval=symb_double_deux_points(makevecteur(yyvsp[-2],yyvsp[0])); }
#line 5911 "input_parser.cc"
    break;

  case 193: /* symbol: T_SYMBOL T_DOUBLE_DEUX_POINTS T_QUOTE exp T_QUOTE  */
#line 722 "input_parser.yy"
                                                                          { yyval=symb_double_deux_points(makevecteur(yyvsp[-4],yyvsp[-1])); }
#line 5917 "input_parser.cc"
    break;

  case 194: /* symbol: T_DOUBLE_DEUX_POINTS T_SYMBOL  */
#line 723 "input_parser.yy"
                                        { yyval=symb_double_deux_points(makevecteur(0,yyvsp[0])); }
#line 5923 "input_parser.cc"
    break;

  case 195: /* symbol: T_NUMBER T_DOUBLE_DEUX_POINTS T_SYMBOL  */
#line 724 "input_parser.yy"
                                                 { yyval=symb_double_deux_points(makevecteur(yyvsp[-2],yyvsp[0])); }
#line 5929 "input_parser.cc"
    break;

  case 196: /* symbol: T_TYPE_ID T_SYMBOL  */
#line 732 "input_parser.yy"
                             { 
	  gen tmp(yyvsp[-1]); 
	  // tmp.subtype=1; 
	  // $$=symb_check_type(makevecteur(tmp,$2),context0); 
          yyval=symbolic(at_deuxpoints,makesequence(yyvsp[0],yyvsp[-1]));
	  }
#line 5940 "input_parser.cc"
    break;

  case 197: /* symbol: TI_HASH exp  */
#line 738 "input_parser.yy"
                      {yyval=symbolic(*yyvsp[-1]._FUNCptr,yyvsp[0]); }
#line 5946 "input_parser.cc"
    break;

  case 198: /* symbol_or_literal: T_SYMBOL  */
#line 741 "input_parser.yy"
                            { yyval=yyvsp[0]; }
#line 5952 "input_parser.cc"
    break;

  case 199: /* symbol_or_literal: T_LITERAL  */
#line 742 "input_parser.yy"
                    { yyval=yyvsp[0]; }
#line 5958 "input_parser.cc"
    break;

  case 200: /* entete: %empty  */
#line 745 "input_parser.yy"
                      { yyval=makevecteur(vecteur(0),vecteur(0)); }
#line 5964 "input_parser.cc"
    break;

  case 201: /* entete: entete local  */
#line 746 "input_parser.yy"
                        { vecteur v1 =gen2vecteur(yyvsp[-1]); vecteur v2=gen2vecteur(yyvsp[0]); yyval=makevecteur(mergevecteur(gen2vecteur(v1[0]),gen2vecteur(v2[0])),mergevecteur(gen2vecteur(v1[1]),gen2vecteur(v2[1]))); }
#line 5970 "input_parser.cc"
    break;

  case 202: /* entete: nom entete  */
#line 747 "input_parser.yy"
                     { yyval=yyvsp[0]; }
#line 5976 "input_parser.cc"
    break;

  case 203: /* stack: T_STACK T_BEGIN_PAR exp T_END_PAR  */
#line 751 "input_parser.yy"
                                          { if (yyvsp[-1].type==_VECT) yyval=gen(*yyvsp[-1]._VECTptr,_RPN_STACK__VECT); else yyval=gen(vecteur(1,yyvsp[-1]),_RPN_STACK__VECT); }
#line 5982 "input_parser.cc"
    break;

  case 204: /* stack: T_STACK T_NULL  */
#line 752 "input_parser.yy"
                         { yyval=gen(vecteur(0),_RPN_STACK__VECT); }
#line 5988 "input_parser.cc"
    break;

  case 205: /* local: T_LOCAL suite_symbol T_SEMI  */
#line 755 "input_parser.yy"
                                       { if (!yyvsp[-2].val) yyval=makevecteur(yyvsp[-1],vecteur(0)); else yyval=makevecteur(vecteur(0),yyvsp[-1]);}
#line 5994 "input_parser.cc"
    break;

  case 206: /* nom: T_NAME exp T_SEMI  */
#line 758 "input_parser.yy"
                            { yyval=yyvsp[-1]; }
#line 6000 "input_parser.cc"
    break;

  case 207: /* suite_symbol: affectable_symbol  */
#line 761 "input_parser.yy"
                                 { yyval=gen(vecteur(1,yyvsp[0]),_SEQ__VECT); }
#line 6006 "input_parser.cc"
    break;

  case 208: /* suite_symbol: suite_symbol T_VIRGULE affectable_symbol  */
#line 762 "input_parser.yy"
                                                        { 
	       vecteur v=*yyvsp[-2]._VECTptr;
	       v.push_back(yyvsp[0]);
	       yyval=gen(v,_SEQ__VECT);
	     }
#line 6016 "input_parser.cc"
    break;

  case 209: /* affectable_symbol: symbol  */
#line 769 "input_parser.yy"
                           { yyval=yyvsp[0]; }
#line 6022 "input_parser.cc"
    break;

  case 210: /* affectable_symbol: T_SYMBOL T_AFFECT exp  */
#line 770 "input_parser.yy"
                                     { yyval=parser_symb_sto(yyvsp[0],yyvsp[-2],yyvsp[-1]==at_array_sto); }
#line 6028 "input_parser.cc"
    break;

  case 211: /* affectable_symbol: T_SYMBOL T_EQUAL exp  */
#line 771 "input_parser.yy"
                                    { yyval=symb_equal(yyvsp[-2],yyvsp[0]); }
#line 6034 "input_parser.cc"
    break;

  case 212: /* affectable_symbol: T_SYMBOL T_DEUXPOINTS exp  */
#line 772 "input_parser.yy"
                                         { yyval=symbolic(at_deuxpoints,makesequence(yyvsp[-2],yyvsp[0]));  }
#line 6040 "input_parser.cc"
    break;

  case 213: /* affectable_symbol: T_BEGIN_PAR affectable_symbol T_END_PAR  */
#line 773 "input_parser.yy"
                                                       { yyval=yyvsp[-1]; }
#line 6046 "input_parser.cc"
    break;

  case 214: /* affectable_symbol: T_UNARY_OP  */
#line 774 "input_parser.yy"
                          { yyval=yyvsp[0]; *logptr(giac_yyget_extra(scanner)) << "Error: reserved word "<< yyvsp[0] <<'\n';}
#line 6052 "input_parser.cc"
    break;

  case 215: /* affectable_symbol: T_UNARY_OP T_DOUBLE_DEUX_POINTS exp  */
#line 775 "input_parser.yy"
                                                   { yyval=symb_double_deux_points(makevecteur(yyvsp[-2],yyvsp[0])); *logptr(giac_yyget_extra(scanner)) << "Error: reserved word "<< yyvsp[-2] <<'\n'; }
#line 6058 "input_parser.cc"
    break;

  case 216: /* affectable_symbol: T_TYPE_ID  */
#line 776 "input_parser.yy"
                         { 
  const giac::context * contextptr = giac_yyget_extra(scanner);
  yyval=string2gen("_"+yyvsp[0].print(contextptr),false); 
  if (!giac::first_error_line(contextptr)){
    giac::first_error_line(giac::lexer_line_number(contextptr),contextptr);
    giac:: error_token_name(yyvsp[0].print(contextptr)+ " (reserved word)",contextptr);
  }
}
#line 6071 "input_parser.cc"
    break;

  case 217: /* affectable_symbol: T_NUMBER  */
#line 784 "input_parser.yy"
                        { 
  const giac::context * contextptr = giac_yyget_extra(scanner);
  yyval=string2gen("_"+yyvsp[0].print(contextptr),false);
  if (!giac::first_error_line(contextptr)){
    giac::first_error_line(giac::lexer_line_number(contextptr),contextptr);
    giac:: error_token_name(yyvsp[0].print(contextptr)+ " reserved word",contextptr);
  }
}
#line 6084 "input_parser.cc"
    break;

  case 218: /* exp_or_empty: %empty  */
#line 794 "input_parser.yy"
                          { yyval=gen(1);}
#line 6090 "input_parser.cc"
    break;

  case 219: /* exp_or_empty: exp  */
#line 795 "input_parser.yy"
                { yyval=yyvsp[0]; }
#line 6096 "input_parser.cc"
    break;

  case 220: /* suite: %empty  */
#line 798 "input_parser.yy"
                   { yyval=gen(vecteur(0),_SEQ__VECT); }
#line 6102 "input_parser.cc"
    break;

  case 221: /* suite: exp  */
#line 799 "input_parser.yy"
             { yyval=makesuite(yyvsp[0]); }
#line 6108 "input_parser.cc"
    break;

  case 222: /* prg_suite: exp  */
#line 802 "input_parser.yy"
                { yyval = gen(makevecteur(yyvsp[0]),_PRG__VECT); }
#line 6114 "input_parser.cc"
    break;

  case 223: /* prg_suite: prg_suite exp  */
#line 804 "input_parser.yy"
                        { vecteur v(1,yyvsp[-1]); 
			  if (yyvsp[-1].type==_VECT) v=*(yyvsp[-1]._VECTptr); 
			  v.push_back(yyvsp[0]); 
			  yyval = gen(v,_PRG__VECT);
			}
#line 6124 "input_parser.cc"
    break;

  case 224: /* prg_suite: prg_suite semi  */
#line 809 "input_parser.yy"
                                        { yyval = yyvsp[-1];}
#line 6130 "input_parser.cc"
    break;

  case 225: /* rpn_suite: %empty  */
#line 812 "input_parser.yy"
                         { yyval=vecteur(0); }
#line 6136 "input_parser.cc"
    break;

  case 226: /* rpn_suite: rpn_token rpn_suite  */
#line 813 "input_parser.yy"
                                 { yyval=mergevecteur(vecteur(1,yyvsp[-1]),*(yyvsp[0]._VECTptr));}
#line 6142 "input_parser.cc"
    break;

  case 227: /* rpn_suite: rpn_token T_VIRGULE rpn_suite  */
#line 814 "input_parser.yy"
                                           { yyval=mergevecteur(vecteur(1,yyvsp[-2]),*(yyvsp[0]._VECTptr));}
#line 6148 "input_parser.cc"
    break;

  case 228: /* rpn_token: T_UNARY_OP  */
#line 817 "input_parser.yy"
                        { yyval=yyvsp[0]; }
#line 6154 "input_parser.cc"
    break;

  case 229: /* step: %empty  */
#line 887 "input_parser.yy"
                    { yyval=gen(1); }
#line 6160 "input_parser.cc"
    break;

  case 230: /* step: T_BY exp  */
#line 888 "input_parser.yy"
                   { yyval=yyvsp[0]; }
#line 6166 "input_parser.cc"
    break;

  case 231: /* from: %empty  */
#line 891 "input_parser.yy"
                    { yyval=gen(1); }
#line 6172 "input_parser.cc"
    break;

  case 232: /* from: T_AFFECT exp  */
#line 892 "input_parser.yy"
                       { yyval=yyvsp[0]; }
#line 6178 "input_parser.cc"
    break;

  case 233: /* from: T_EQUAL exp  */
#line 893 "input_parser.yy"
                      { yyval=yyvsp[0]; }
#line 6184 "input_parser.cc"
    break;

  case 234: /* from: T_FROM exp  */
#line 894 "input_parser.yy"
                     { yyval=yyvsp[0]; }
#line 6190 "input_parser.cc"
    break;

  case 235: /* loop38_do: T_SEMI  */
#line 897 "input_parser.yy"
                  { yyval=gen(1); }
#line 6196 "input_parser.cc"
    break;

  case 236: /* loop38_do: T_DO  */
#line 898 "input_parser.yy"
               { yyval=yyvsp[0]; }
#line 6202 "input_parser.cc"
    break;

  case 237: /* else: %empty  */
#line 901 "input_parser.yy"
                    { yyval=0; }
#line 6208 "input_parser.cc"
    break;

  case 238: /* else: ti_else exp T_SEMI  */
#line 902 "input_parser.yy"
                             { yyval=yyvsp[-1]; }
#line 6214 "input_parser.cc"
    break;

  case 239: /* else: ti_else bloc  */
#line 903 "input_parser.yy"
                       { yyval=symb_bloc(yyvsp[0]); }
#line 6220 "input_parser.cc"
    break;

  case 240: /* bloc: T_BLOC_BEGIN prg_suite T_BLOC_END  */
#line 907 "input_parser.yy"
                                            { 
	yyval = yyvsp[-1];
	}
#line 6228 "input_parser.cc"
    break;

  case 241: /* bloc: T_BLOC_BEGIN entete prg_suite T_BLOC_END  */
#line 910 "input_parser.yy"
                                                        {
          const giac::context * contextptr = giac_yyget_extra(scanner);
          yyval = symb_local(yyvsp[-2],yyvsp[-1],contextptr);
         }
#line 6237 "input_parser.cc"
    break;

  case 242: /* elif: ti_bloc_end  */
#line 917 "input_parser.yy"
                        { if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=4) giac_yyerror(scanner,"missing test end delimiter"); yyval=0; }
#line 6243 "input_parser.cc"
    break;

  case 243: /* elif: ti_else prg_suite ti_bloc_end  */
#line 918 "input_parser.yy"
                                        {
          if (yyvsp[0].type==_INT_ && yyvsp[0].val && yyvsp[0].val!=4) giac_yyerror(scanner,"missing test end delimiter");
	yyval=symb_bloc(yyvsp[-1]); 
	}
#line 6252 "input_parser.cc"
    break;

  case 244: /* elif: T_ELIF exp T_THEN prg_suite elif  */
#line 922 "input_parser.yy"
                                           { 
	  yyval=symb_ifte(equaltosame(yyvsp[-3]),symb_bloc(yyvsp[-1]),yyvsp[0]);
	  }
#line 6260 "input_parser.cc"
    break;

  case 245: /* elif: TI_DEUXPOINTS T_ELIF exp T_THEN prg_suite elif  */
#line 925 "input_parser.yy"
                                                         { 
	  yyval=symb_ifte(equaltosame(yyvsp[-3]),symb_bloc(yyvsp[-1]),yyvsp[0]);
	  }
#line 6268 "input_parser.cc"
    break;

  case 246: /* ti_bloc_end: T_BLOC_END  */
#line 930 "input_parser.yy"
                           { yyval=yyvsp[0]; }
#line 6274 "input_parser.cc"
    break;

  case 247: /* ti_bloc_end: TI_DEUXPOINTS T_BLOC_END  */
#line 931 "input_parser.yy"
                                         { yyval=yyvsp[0]; }
#line 6280 "input_parser.cc"
    break;

  case 248: /* ti_else: T_ELSE  */
#line 934 "input_parser.yy"
                    { yyval=0; }
#line 6286 "input_parser.cc"
    break;

  case 249: /* ti_else: TI_DEUXPOINTS T_ELSE  */
#line 935 "input_parser.yy"
                                    { yyval=0; }
#line 6292 "input_parser.cc"
    break;

  case 250: /* switch: %empty  */
#line 938 "input_parser.yy"
                      { yyval=vecteur(0); }
#line 6298 "input_parser.cc"
    break;

  case 251: /* switch: T_DEFAULT T_DEUXPOINTS bloc  */
#line 939 "input_parser.yy"
                                      { yyval=makevecteur(symb_bloc(yyvsp[0]));}
#line 6304 "input_parser.cc"
    break;

  case 252: /* switch: T_CASE T_NUMBER T_DEUXPOINTS bloc switch  */
#line 940 "input_parser.yy"
                                                   { yyval=mergevecteur(makevecteur(yyvsp[-3],symb_bloc(yyvsp[-1])),*(yyvsp[0]._VECTptr));}
#line 6310 "input_parser.cc"
    break;

  case 253: /* case: %empty  */
#line 943 "input_parser.yy"
                    { yyval=vecteur(0); }
#line 6316 "input_parser.cc"
    break;

  case 254: /* case: T_DEFAULT prg_suite  */
#line 944 "input_parser.yy"
                              { yyval=vecteur(1,symb_bloc(yyvsp[0])); }
#line 6322 "input_parser.cc"
    break;

  case 255: /* case: T_OF T_NUMBER T_DO prg_suite case  */
#line 945 "input_parser.yy"
                                            { yyval=mergevecteur(makevecteur(yyvsp[-3],symb_bloc(yyvsp[-1])),*(yyvsp[0]._VECTptr));}
#line 6328 "input_parser.cc"
    break;

  case 256: /* case38: %empty  */
#line 948 "input_parser.yy"
                    { yyval=vecteur(0); }
#line 6334 "input_parser.cc"
    break;

  case 257: /* case38: T_DEFAULT prg_suite  */
#line 949 "input_parser.yy"
                              { yyval=vecteur(1,symb_bloc(yyvsp[0])); }
#line 6340 "input_parser.cc"
    break;

  case 258: /* case38: T_IF exp T_THEN prg_suite T_BLOC_END case38  */
#line 950 "input_parser.yy"
                                                      { yyval=mergevecteur(makevecteur(yyvsp[-4],symb_bloc(yyvsp[-2])),gen2vecteur(yyvsp[0]));}
#line 6346 "input_parser.cc"
    break;

  case 259: /* case38: T_IF exp T_THEN prg_suite T_BLOC_END T_SEMI case38  */
#line 951 "input_parser.yy"
                                                             { yyval=mergevecteur(makevecteur(yyvsp[-5],symb_bloc(yyvsp[-3])),gen2vecteur(yyvsp[0]));}
#line 6352 "input_parser.cc"
    break;

  case 260: /* semi: T_SEMI  */
#line 954 "input_parser.yy"
               { yyval=yyvsp[0]; }
#line 6358 "input_parser.cc"
    break;


#line 6362 "input_parser.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (scanner, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 961 "input_parser.yy"


#ifndef NO_NAMESPACE_GIAC
} // namespace giac


#endif // ndef NO_NAMESPACE_GIAC
int giac_yyget_column  (yyscan_t yyscanner);

// Error print routine (store error string in parser_error)
#if 1
int giac_yyerror(yyscan_t scanner,const char *s) {
 const giac::context * contextptr = giac_yyget_extra(scanner);
 int col = giac_yyget_column(scanner);
 int line = giac::lexer_line_number(contextptr);
 const char * scanb=giac::currently_scanned(contextptr);
 std::string curline;
 if (scanb){
  for (int i=1;i<line;++i){
   for (;*scanb;++scanb){
     if (*scanb=='\n'){
       ++scanb;
       break;
     }
   }
  }
  const char * scane=scanb;
  for (;*scane;++scane){
    if (*scane=='\n') break;
  }
  curline=std::string (scanb,scane);
 }
 std::string token_name=string(giac_yyget_text(scanner));
 bool is_at_end = (token_name.size()==2 && (token_name[0]==char(0xC3)) && (token_name[1]==char(0xBF)));
 std::string suffix = " (reserved word)";
 if (token_name.size()>suffix.size() && token_name.compare(token_name.size()-suffix.size(),suffix.size(),suffix)) {
  if (col>=token_name.size()-suffix.size()) {
   col -= token_name.size()-suffix.size();
  }
 } else if (col>=token_name.size()) {
   col -= token_name.size();
 }
 giac::lexer_column_number(contextptr)=col;
 string sy("syntax error ");
 if (0 && strlen(s)){
   sy += ": ";
   sy += s;
   sy +=", ";
 }
 if (is_at_end) {
  parser_error(":" + giac::print_INT_(line) + ": " +sy + " at end of input\n",contextptr); // string(s) replaced with syntax error
  giac::parsed_gen(giac::undef,contextptr);
 } else {
 parser_error( ":" + giac::print_INT_(line) + ": " + sy + " line " + giac::print_INT_(line) + " col " + giac::print_INT_(col) + " at " + token_name +" in "+curline+" \n",contextptr); // string(s) replaced with syntax error
 giac::parsed_gen(giac::string2gen(token_name,false),contextptr);
 }
 if (!giac::first_error_line(contextptr)) {
  giac::first_error_line(line,contextptr);
  if (is_at_end) {
   token_name="end of input";
  }
  giac:: error_token_name(token_name,contextptr);
 }
 return line;
}

#else

int giac_yyerror(yyscan_t scanner,const char *s)
{
  const giac::context * contextptr = giac_yyget_extra(scanner);
  int col= giac_yyget_column(scanner);
  giac::lexer_column_number(contextptr)=col;
  if ( (*giac_yyget_text( scanner )) && (giac_yyget_text( scanner )[0]!=-61) && (giac_yyget_text( scanner )[1]!=-65)){
    std::string txt=giac_yyget_text( scanner );
    parser_error( ":" + giac::print_INT_(giac::lexer_line_number(contextptr)) + ": " + string(s) + " line " + giac::print_INT_(giac::lexer_line_number(contextptr)) + " col " + giac::print_INT_(col) + " at " + txt +"\n",contextptr);
     giac::parsed_gen(giac::string2gen(txt,false),contextptr);
  }
  else {
    parser_error(":" + giac::print_INT_(giac::lexer_line_number(contextptr)) + ": " +string(s) + " at end of input\n",contextptr);
    giac::parsed_gen(giac::undef,contextptr);
  }
  if (!giac::first_error_line(contextptr)){
    giac::first_error_line(giac::lexer_line_number(contextptr),contextptr);
    std::string s=string(giac_yyget_text( scanner ));
    if (s.size()==2 && s[0]==-61 && s[1]==-65)
      s="end of input";
    giac:: error_token_name(s,contextptr);
  }
  return giac::lexer_line_number(contextptr);
}
#endif
