/*
 * getoptwin.h
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef GETOPTWIN_H

#if defined(__GNUC__) || defined(HAVE_GETOPT_H)
    #include <getopt.h>
#else   /* !(defined(__GNUC__) || defined(HAVE_GETOPT_H)) */

#ifdef __cplusplus
extern "C" {
#endif

extern int optind;              /* index of first non-option in argv      */
extern int optopt;              /* single option character, as parsed     */
extern int opterr;              /* flag to enable built-in diagnostics... */
                                /* (user may set to zero, to suppress)    */

extern char *optarg;            /* pointer to argument of current option  */

int getopt(int, char * const [], const char *);

struct option           /* specification for a long form option...      */
{
    const char *name;           /* option name, without leading hyphens */
    int         has_arg;        /* does it take an argument?            */
    int        *flag;           /* where to save its status, or NULL    */
    int         val;            /* its associated status value          */
};

enum                    /* permitted values for its "has_arg" field...  */
{
    no_argument = 0,            /* option never takes an argument       */
    required_argument,          /* option always requires an argument   */
    optional_argument           /* option may take an argument          */
};

int getopt_long(int, char * const [], const char *, const struct option *, int *);
int getopt_long_only(int, char * const [], const char *, const struct option *, int *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* !(defined(__GNUC__) || defined(HAVE_GETOPT_H)) */
#endif  /* ndef GETOPTWIN_H */
