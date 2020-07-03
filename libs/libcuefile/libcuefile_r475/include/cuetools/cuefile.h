/*
 * cuefile.h -- cue/toc public declarations
 *
 * Copyright (C) 2004, 2005, 2006 Svend Sorensen
 * For license terms, see the file COPYING in this distribution.
 */

#ifndef CUEFILE_H
#define CUEFILE_H

#include "cd.h"

enum {CUE, TOC, UNKNOWN};

typedef struct Cue Cue;

#ifdef __cplusplus
extern "C" {
#endif

Cd *cf_parse (char *fname, int *format);
int cf_print (char *fname, int *format, Cd *cue);
int cf_format_from_suffix (char *fname);

#ifdef __cplusplus
}
#endif

#endif
