/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Xfe Widgets.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * In addition, as a special exception to the GNU GPL, the copyright holders
 * give permission to link the code of this program with the Motif and Open
 * Motif libraries (or with modified versions of these that use the same
 * license), and distribute linked combinations including the two. You
 * must obey the GNU General Public License in all respects for all of
 * the code used other than linking with Motif/Open Motif. If you modify
 * this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version.
 *
 * ***** END LICENSE BLOCK ***** */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		<Xfe/Logo.c>											*/
/* Description:	XfeLogo widget source.									*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/LogoP.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* Warnings and messages												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define MESSAGE1  "Cannot obtain geometry for pixmap number %d."
#define MESSAGE2  "Pixmap %d is invalid."
#define MESSAGE3  "Pixmap %d needs to have the same depth as the Logo widget."
#define MESSAGE4  "Pixmap %d needs to have the dimensions as pixmap 0."
#define MESSAGE5  "XmNpixmap & XmNanimationPixmaps must have same dimensions."
#define MESSAGE6  "XmNanimationRunning is a read-only resource."

/*----------------------------------------------------------------------*/
/*																		*/
/* Core Class methods													*/
/*																		*/
/*----------------------------------------------------------------------*/
static void 	Initialize		(Widget,Widget,ArgList,Cardinal *);
static void 	Destroy			(Widget);
static Boolean	SetValues		(Widget,Widget,Widget,ArgList,Cardinal *);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrimitive Class methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
static void	PreferredGeometry	(Widget,Dimension *,Dimension *);
static void	PrepareComponents	(Widget,int);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton class methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void	DrawPixmap			(Widget,XEvent *,Region,XRectangle *);

/*----------------------------------------------------------------------*/
/*																		*/
/* Misc XfeLogo functions												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void	AnimationPrepare	(Widget);

/*----------------------------------------------------------------------*/
/*																		*/
/* Misc animation functions												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void	AnimationTimeout	(XtPointer,XtIntervalId *);
static void	RemoveTimeout		(Widget);
static void	AddTimeout			(Widget);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLogo Resources													*/
/*																		*/
/*----------------------------------------------------------------------*/
static XtResource resources[] = 
{
    /* Callback resources */     	
    { 
		XmNanimationCallback,
		XmCCallback,
		XmRCallback,
		sizeof(XtCallbackList),
		XtOffsetOf(XfeLogoRec , xfe_logo . animation_callback),
		XmRImmediate, 
		(XtPointer) NULL
    },

	/* Resources */
    { 
		XmNanimationInterval,
		XmCAnimationInterval,
		XmRInt,
		sizeof(int),
		XtOffsetOf(XfeLogoRec , xfe_logo . animation_interval),
		XmRImmediate, 
		(XtPointer) 100
    },
    { 
		XmNanimationRunning,
		XmCReadOnly,
		XmRBoolean,
		sizeof(Boolean),
		XtOffsetOf(XfeLogoRec , xfe_logo . animation_running),
		XmRImmediate, 
		(XtPointer) False
    },
    { 
		XmNcurrentPixmapIndex,
		XmCCurrentPixmapIndex,
		XmRCardinal,
		sizeof(Cardinal),
		XtOffsetOf(XfeLogoRec , xfe_logo . current_pixmap_index),
		XmRImmediate, 
		(XtPointer) 0
    },
    { 
		XmNanimationPixmaps,
		XmCAnimationPixmaps,
		XmRPixmapTable,
		sizeof(XfePixmapTable),
		XtOffsetOf(XfeLogoRec , xfe_logo . animation_pixmaps),
		XmRImmediate, 
		(XtPointer) NULL
    },
    { 
		XmNnumAnimationPixmaps,
		XmCNumAnimationPixmaps,
		XmRCardinal,
		sizeof(Cardinal),
		XtOffsetOf(XfeLogoRec , xfe_logo . num_animation_pixmaps),
		XmRImmediate, 
		(XtPointer) 0
    },
    { 
		XmNresetWhenIdle,
		XmCResetWhenIdle,
		XmRBoolean,
		sizeof(Boolean),
		XtOffsetOf(XfeLogoRec , xfe_logo . reset_when_idle),
		XmRImmediate, 
		(XtPointer) True
    },

	/* Force XmNbuttonLayout to XmBUTTON_PIXMAP_ONLY */
    { 
		XmNbuttonLayout,
		XmCButtonLayout,
		XmRButtonLayout,
		sizeof(unsigned char),
		XtOffsetOf(XfeLogoRec , xfe_button . button_layout),
		XmRImmediate, 
		(XtPointer) XmBUTTON_PIXMAP_ONLY
    },
};

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLogo widget class record initialization							*/
/*																		*/
/*----------------------------------------------------------------------*/
_XFE_WIDGET_CLASS_RECORD(logo,Logo) =
{
    {
		/* Core Part */
		(WidgetClass) &xfeButtonClassRec,		/* superclass         	*/
		"XfeLogo",								/* class_name         	*/
		sizeof(XfeLogoRec),						/* widget_size        	*/
		NULL,									/* class_initialize   	*/
		NULL,									/* class_part_initiali	*/
		FALSE,                                  /* class_inited       	*/
		Initialize,                             /* initialize         	*/
		NULL,                                   /* initialize_hook    	*/
		XtInheritRealize,                       /* realize            	*/
		NULL,									/* actions            	*/
		0,										/* num_actions        	*/
		(XtResource *)resources,				/* resources          	*/
		XtNumber(resources),                    /* num_resources      	*/
		NULLQUARK,                              /* xrm_class          	*/
		TRUE,                                   /* compress_motion    	*/
		XtExposeCompressMaximal,                /* compress_exposure  	*/
		TRUE,                                   /* compress_enterleave	*/
		FALSE,                                  /* visible_interest   	*/
		Destroy,                                /* destroy            	*/
		XtInheritResize,						/* resize             	*/
		XtInheritExpose,						/* expose             	*/
		SetValues,                              /* set_values         	*/
		NULL,                                   /* set_values_hook    	*/
		XtInheritSetValuesAlmost,				/* set_values_almost  	*/
		NULL,									/* get_values_hook		*/
		NULL,                                   /* accept_focus       	*/
		XtVersion,                              /* version            	*/
		NULL,                                   /* callback_private   	*/
		XtInheritTranslations,					/* tm_table           	*/
		XtInheritQueryGeometry,					/* query_geometry     	*/
		XtInheritDisplayAccelerator,            /* display accel      	*/
		NULL,                                   /* extension          	*/
    },

    /* XmPrimitive Part */
    {
		XmInheritBorderHighlight,				/* border_highlight 	*/
		XmInheritBorderUnhighlight,				/* border_unhighlight 	*/
		XtInheritTranslations,                  /* translations       	*/
		XmInheritArmAndActivate,				/* arm_and_activate   	*/
		NULL,									/* syn resources      	*/
		0,										/* num syn_resources  	*/
		NULL,									/* extension          	*/
    },

    /* XfePrimitive Part */
    {
		XfeInheritBitGravity,					/* bit_gravity			*/
		PreferredGeometry,						/* preferred_geometry	*/
		XfeInheritUpdateBoundary,				/* update_boundary		*/
		PrepareComponents,						/* prepare_components	*/
		XfeInheritLayoutComponents,				/* layout_components	*/
		XfeInheritDrawBackground,				/* draw_background		*/
		XfeInheritDrawShadow,					/* draw_shadow			*/
		XfeInheritDrawComponents,				/* draw_components		*/
		NULL,									/* extension            */
    },

    /* XfeLabel Part */
    {
		XfeInheritLayoutString,					/* layout_string		*/
		XfeInheritDrawString,					/* draw_string			*/
		XfeInheritDrawSelection,				/* draw_selection		*/
		XfeInheritGetLabelGC,					/* get_label_gc			*/
		XfeInheritGetSelectionGC,				/* get_selection_gc		*/
		NULL,									/* extension            */
    },

    /* XfeButton Part */
    {
		XfeInheritLayoutPixmap,					/* layout_pixmap		*/
		DrawPixmap,								/* draw_pixmap			*/
		XfeInheritDrawAccentBorder,				/* draw_accent_border	*/
		XfeInheritDrawUnderline,				/* draw_underline		*/
		XfeInheritArmTimeout,					/* arm_timeout			*/
		NULL,									/* extension            */
    },

    /* XfeLogo Part */
    {
		NULL,									/* extension            */
    },
};

/*----------------------------------------------------------------------*/
/*																		*/
/* xfeLogoWidgetClass declaration.										*/
/*																		*/
/*----------------------------------------------------------------------*/
_XFE_WIDGET_CLASS(logo,Logo);

/*----------------------------------------------------------------------*/
/*																		*/
/* Core Class methods													*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
Initialize(Widget rw,Widget nw,ArgList args,Cardinal *nargs)
{
    XfeLogoPart * lp = _XfeLogoPart(nw);

    /* Allocate the copy GC */
    lp->copy_GC = XfeAllocateCopyGc(nw);

	/* Initialize private members */
	lp->timer_id = 0;
    
    /* Finish of initialization */
    _XfePrimitiveChainInitialize(rw,nw,xfeLogoWidgetClass);
}
/*----------------------------------------------------------------------*/
static void
Destroy(Widget w)
{
    XfeLogoPart * lp = _XfeLogoPart(w);

    /* Release GCs */
    XtReleaseGC(w,lp->copy_GC);

	/* Remove the timeout if needed */
	RemoveTimeout(w);
}
/*----------------------------------------------------------------------*/
static Boolean
SetValues(Widget ow,Widget rw,Widget nw,ArgList args,Cardinal *nargs)
{
    XfeLogoPart *	np = _XfeLogoPart(nw);
    XfeLogoPart *	op = _XfeLogoPart(ow);

    /* animation_running */
    if (np->animation_running != op->animation_running)
    {
		np->animation_running = op->animation_running;

		_XfeWarning(nw,MESSAGE6);
    }

    /* current_pixmap_index */
    if (np->current_pixmap_index != op->current_pixmap_index)
    {
		_XfeConfigFlags(nw) |= XfeConfigExpose;
    }

    /* animation_pixmaps */
    if (np->animation_pixmaps != op->animation_pixmaps)
    {
		_XfeConfigFlags(nw) |= XfeConfigGLE;

		_XfePrepareFlags(nw) |= _XFE_PREPARE_LOGO_ANIMATION;

		np->current_pixmap_index = 0;
	}

    /* num_animation_pixmaps */
    if (np->num_animation_pixmaps != op->num_animation_pixmaps)
    {
		if (!np->animation_running)
		{
			_XfeConfigFlags(nw) |= XfeConfigExpose;
		}

		np->current_pixmap_index = 0;
    }

    return _XfePrimitiveChainSetValues(ow,rw,nw,xfeLogoWidgetClass);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrimitive methods													*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
PreferredGeometry(Widget w,Dimension *width,Dimension *height)
{
    XfeButtonPart *	bp = _XfeButtonPart(w);
    XfeLogoPart *	lp = _XfeLogoPart(w);

	Dimension		pixmap_width_save = bp->pixmap_rect.width;
	Dimension		pixmap_height_save = bp->pixmap_rect.height;

	Dimension		pixmap_width;
	Dimension		pixmap_height;

	XfeButtonWidgetClass bwc = (XfeButtonWidgetClass) xfeButtonWidgetClass;

	/* The first pixmap determines the dimensions */
	if (lp->animation_pixmaps && lp->num_animation_pixmaps
		&& lp->animation_width && lp->animation_height)
	{
		pixmap_width = lp->animation_width;
		pixmap_height = lp->animation_height;
	}
	else
	{
		pixmap_width = bp->pixmap_rect.width;
		pixmap_height = bp->pixmap_rect.height;
	}

	bp->pixmap_rect.width = pixmap_width;
	bp->pixmap_rect.height = pixmap_height;

	/* Explicit invoke of XfeButton's preferred_geometry() method */
	(*bwc->xfe_primitive_class.preferred_geometry)(w,width,height);

	bp->pixmap_rect.width = pixmap_width_save;
	bp->pixmap_rect.height = pixmap_height_save;
}
/*----------------------------------------------------------------------*/
static void
PrepareComponents(Widget w,int flags)
{
    if (flags & _XFE_PREPARE_LOGO_ANIMATION)
    {
		AnimationPrepare(w);
    }
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton class methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
DrawPixmap(Widget w,XEvent * event,Region region,XRectangle * clip_rect)
{
    XfeButtonPart *	bp = _XfeButtonPart(w);
    XfeLogoPart *	lp = _XfeLogoPart(w);
	Pixmap			pixmap = XmUNSPECIFIED_PIXMAP;
	Pixmap			pixmap_save = bp->pixmap;

	XfeButtonWidgetClass bwc = (XfeButtonWidgetClass) xfeButtonWidgetClass;

	/* Determine which pixmap to use */


	/* If animation running, try to use one of the animation pixmaps */
	if (lp->animation_running || !lp->reset_when_idle)
	{
		if (lp->animation_pixmaps &&
			(lp->current_pixmap_index < lp->num_animation_pixmaps))
		{
			pixmap = lp->animation_pixmaps[lp->current_pixmap_index];
		}
		else
		{
			pixmap = bp->pixmap;
		}
	}
	/* If animation not running, try to use the XfeButton's pixmap */
	else
	{
		if (_XfePixmapGood(bp->pixmap))
		{
			pixmap = bp->pixmap;
		}
		else if (lp->animation_pixmaps &&
				 (lp->current_pixmap_index < lp->num_animation_pixmaps))
		{
			pixmap = lp->animation_pixmaps[lp->current_pixmap_index];
		}
	}

	/* temporary replacement to fool the superclass' draw_pixmap() */
	bp->pixmap = pixmap;

	/* Explicit invoke of XfeButton's draw_pixmap() method */
	(*bwc->xfe_button_class.draw_pixmap)(w,event,region,clip_rect);

	bp->pixmap = pixmap_save;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Misc XfeLogo functions												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
AnimationPrepare(Widget w)
{
    XfeLogoPart *	lp = _XfeLogoPart(w);
    XfeButtonPart *	bp = _XfeButtonPart(w);

	lp->animation_width = 0;
	lp->animation_height = 0;

	/* Check pixmaps only if they exist */
	if (lp->animation_pixmaps && lp->num_animation_pixmaps)
	{
		Cardinal	depth;
		Cardinal	i;
		Dimension	width;
		Dimension	height;

		for(i = 0; i < lp->num_animation_pixmaps; i++)
		{
			/* Check geometry of good pixmaps only */
			if (_XfePixmapGood(lp->animation_pixmaps[i]))
			{
				/* Obtain geometry info for the pixmap */
				if (XfePixmapExtent(XtDisplay(w),lp->animation_pixmaps[i],
									&width,&height,&depth))
				{
					/* Make sure all pixmaps have the same depth */
					if (depth != _XfeDepth(w))
					{
						_XfeArgWarning(w,MESSAGE3,i);
					}

					/* The first pixmap determines the animation geomerty */
					if (i == 0)
					{
						lp->animation_width = width;
						lp->animation_height = height;
					}
					/* All other pixmaps must have the same geometry */
					else
					{
						if ((width != lp->animation_width) ||
							(height != lp->animation_height))
						{
							_XfeArgWarning(w,MESSAGE4,i);
						}
					}


				}
				/* Mark the pixmap unusable since no info was obtainable */
				else
				{
					lp->animation_pixmaps[i] = XmUNSPECIFIED_PIXMAP;

					_XfeArgWarning(w,MESSAGE1,i);
				}
			}
			else
			{
				_XfeArgWarning(w,MESSAGE2,i);
			}
		}

		if (lp->animation_width && lp->animation_height)
		{
			if (_XfePixmapGood(bp->pixmap))
			{
				if ((bp->pixmap_rect.width != lp->animation_width) ||
					(bp->pixmap_rect.height != lp->animation_height))
				{
					bp->pixmap_rect.width = lp->animation_width;
					bp->pixmap_rect.height = lp->animation_height;

					_XfeWarning(w,MESSAGE5);
				}
			}
			else
			{
				bp->pixmap = lp->animation_pixmaps[0];

				bp->pixmap_rect.width = lp->animation_width;
				bp->pixmap_rect.height = lp->animation_height;
			}
		}
	}

}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Misc animation functions												*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
AnimationTimeout(XtPointer closure,XtIntervalId * id)
{
    Widget			w = (Widget) closure;
    XfeLogoPart *	lp = _XfeLogoPart(w);
	Cardinal		i;

	/*
	 *    Always clear timer_id, so we don't kill someone else's timer
	 *    by accident later.
	 */
	lp->timer_id = 0;

	/* Make sure the widget is still alive */
	if (!_XfeIsAlive(w))
	{
		return;
	}

	/* Make sure the animation is running */
	if (!lp->animation_running)
	{
		return;
	}

    i = (Cardinal) XfeGetValue(w,XmNcurrentPixmapIndex);

	i++;

	if (i == lp->num_animation_pixmaps)
	{
		i = 0;
	}

    XfeSetValue(w,XmNcurrentPixmapIndex,i);

    /* Invoke animation Callbacks */
    _XfeInvokeCallbacks(w,lp->animation_callback,XmCR_ANIMATION,NULL,True);

	/* Add the timeout again */
	AddTimeout(w);
}
/*----------------------------------------------------------------------*/
static void
AddTimeout(Widget w)
{
    XfeLogoPart *	lp = _XfeLogoPart(w);

	assert( _XfeIsAlive(w) );

	lp->timer_id = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
								   lp->animation_interval,
								   AnimationTimeout,
								   (XtPointer) w);
}
/*----------------------------------------------------------------------*/
static void
RemoveTimeout(Widget w)
{
    XfeLogoPart *	lp = _XfeLogoPart(w);

	if (lp->timer_id)
	{
		XtRemoveTimeOut(lp->timer_id);

		lp->timer_id = 0;
	}
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLogo Public Methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
Widget
XfeCreateLogo(Widget parent,char *name,Arg *args,Cardinal nargs)
{
    return (XtCreateWidget(name,xfeLogoWidgetClass,parent,args,nargs));
}
/*----------------------------------------------------------------------*/
void
XfeLogoAnimationStart(Widget w)
{
    XfeLogoPart *	lp = _XfeLogoPart(w);

	assert( _XfeIsAlive(w) );
	assert( XfeIsLogo(w) );

	/* Make sure the animation is not already running */
	if (lp->animation_running)
	{
		return;
	}

	lp->animation_running = True;

	/* Add the timeout */
	AddTimeout(w);
}
/*----------------------------------------------------------------------*/
void
XfeLogoAnimationStop(Widget w)
{
    XfeLogoPart *	lp = _XfeLogoPart(w);

	assert( _XfeIsAlive(w) );
	assert( XfeIsLogo(w) );

	/* Make sure the animation is running */
	if (!lp->animation_running)
	{
		return;
	}

	lp->animation_running = False;

	/* Remove the timeout if needed */
	RemoveTimeout(w);

	/* Redraw the widget */
	XfeExpose(w,NULL,NULL);
}
/*----------------------------------------------------------------------*/
void
XfeLogoAnimationReset(Widget w)
{
	assert( _XfeIsAlive(w) );
	assert( XfeIsLogo(w) );

    XfeSetValue(w,XmNcurrentPixmapIndex,0);
}
/*----------------------------------------------------------------------*/
