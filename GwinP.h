/* File: GwinP.h
 *
 * Graphics window widget private header file
 * (based on the Athena template widget)
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: GwinP.h,v 1.6 1994/08/10 10:27:28 risto Exp $
 */

#ifndef _GwinP_h
#define _GwinP_h

#include "Gwin.h"

/* include superclass private header file */
#include <X11/CoreP.h>

/* define unique representation types not found in <X11/StringDefs.h> */

#define XtRGwinResource "GwinResource"

typedef struct
  {
    int empty;
  }
GwinClassPart;

typedef struct _GwinClassRec
  {
    CoreClassPart core_class;
    GwinClassPart gwin_class;
  }
GwinClassRec;

extern GwinClassRec gwinClassRec;

typedef struct
  {
    /* resources */
    /* only redrawing and resizing callbacks are needed */
    XtCallbackList expose_callback;
    XtCallbackList resize_callback;
    /* private state */
    /* (none) */
  }
GwinPart;

typedef struct _GwinRec
  {
    CorePart core;
    GwinPart gwin;
  }
GwinRec;

#endif /* _GwinP_h */
