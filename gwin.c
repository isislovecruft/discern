/* File: gwin.c
 *
 * Graphics window widget code
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
 * $Id: gwin.c,v 1.6 1994/08/10 10:27:28 risto Exp $
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "GwinP.h"

/* make space for the resource data here */
static XtResource resources[] =
{
#define offset(field) XtOffset(GwinWidget, gwin.field)
  /* {name, class, type, size, offset, default_type, default_addr}, */
  {XtNexposeCallback, XtCCallback, XtRCallback, sizeof (XtCallbackList),
   offset (expose_callback), XtRCallback, NULL},
  {XtNresizeCallback, XtCCallback, XtRCallback, sizeof (XtCallbackList),
   offset (resize_callback), XtRCallback, NULL},
#undef offset
};

/* redraw callback call */
/* ARGSUSED */
static void
Redisplay (w, event, region)
     Widget w;
     XEvent *event;		/* unused */
     Region region;
{
  XtCallCallbacks (w, XtNexposeCallback, (XtPointer) region);
}

/* resize callback call */
/* ARGSUSED */
static void
Resize (w)
     Widget w;
{
  XtCallCallbacks (w, XtNresizeCallback, NULL);
}

/* widget class definition */
GwinClassRec gwinClassRec =
{
  {				/* core fields */
    /* superclass               */ (WidgetClass) &widgetClassRec,
    /* class_name               */ "Gwin",
    /* widget_size              */ sizeof (GwinRec),
    /* class_initialize         */ NULL,
    /* class_part_initialize    */ NULL,
    /* class_inited             */ FALSE,
    /* initialize               */ NULL,
    /* initialize_hook          */ NULL,
    /* realize                  */ XtInheritRealize,
    /* actions                  */ NULL,
    /* num_actions              */ 0,
    /* resources                */ resources,
    /* num_resources            */ XtNumber (resources),
    /* xrm_class                */ NULLQUARK,
    /* compress_motion          */ TRUE,
    /* compress_exposure        */ TRUE,
    /* compress_enterleave      */ TRUE,
    /* visible_interest         */ FALSE,
    /* destroy                  */ NULL,
    /* resize                   */ Resize,
    /* expose                   */ Redisplay,
    /* set_values               */ NULL,
    /* set_values_hook          */ NULL,
    /* set_values_almost        */ XtInheritSetValuesAlmost,
    /* get_values_hook          */ NULL,
    /* accept_focus             */ NULL,
    /* version                  */ XtVersion,
    /* callback_private         */ NULL,
    /* tm_table                 */ NULL,
    /* query_geometry           */ XtInheritQueryGeometry,
    /* display_accelerator      */ XtInheritDisplayAccelerator,
    /* extension                */ NULL
  },
  {				/* template fields */
    /* empty                    */ 0
  }
};

WidgetClass gwinWidgetClass = (WidgetClass) &gwinClassRec;
