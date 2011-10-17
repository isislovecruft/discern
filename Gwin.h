/* File: Gwin.h
 *
 * Graphics window widget public declarations
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
 * $Id: Gwin.h,v 1.6 1994/08/10 10:27:28 risto Exp $
 */

#ifndef _Gwin_h
#define _Gwin_h

/* Resources:

   Name              Class              RepType         Default Value
   ----              -----              -------         -------------
   exposeCallback    Callback           Callback        NULL
   resizeCallback    Callback           Callback        NULL
   background        Background         Pixel           XtDefaultBackground
   border            BorderColor        Pixel           XtDefaultForeground
   borderWidth       BorderWidth        Dimension       1
   destroyCallback   Callback           Pointer         NULL
   height            Height             Dimension       0
   mappedWhenManaged MappedWhenManaged  Boolean         True
   sensitive         Sensitive          Boolean         True
   width             Width              Dimension       0
   x                 Position           Position        0
   y                 Position           Position        0

 */

/* define any special resource names here that are not in <X11/StringDefs.h> */
/* we need to be able to resize and redraw the window,
   otherwise not much else is needed */
#define XtNGwinResource 	"gwinResource"
#define XtCGwinResource 	"GwinResource"
#define XtNexposeCallback	"exposeCallback"
#define XtNresizeCallback	"resizeCallback"

/* declare specific GwinWidget class and instance datatypes */
typedef struct _GwinClassRec *GwinWidgetClass;
typedef struct _GwinRec *GwinWidget;

/* declare the class constant */
extern WidgetClass gwinWidgetClass;

#endif /* _Gwin_h */
