/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>  (Original Author)
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
 * ***** END LICENSE BLOCK ***** */

#include "nsNativeThemeGTK.h"
#include "nsThemeConstants.h"
#include "nsDrawingSurfaceGTK.h"
#include "gtkdrawing.h"

#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIEventStateManager.h"
#include "nsIViewManager.h"
#include "nsINameSpaceManager.h"
#include "nsILookAndFeel.h"
#include "nsIDeviceContext.h"
#include "nsTransform2D.h"

#include <gdk/gdkprivate.h>

NS_IMPL_ISUPPORTS1(nsNativeThemeGTK, nsITheme)

GtkWidget* gButtonWidget;
GtkWidget* gCheckboxWidget;
GtkWidget* gScrollbarWidget;
GtkWidget* gGripperWidget;
GtkWidget* gEntryWidget;
GtkWidget* gDropdownButonWidget;
GtkWidget* gArrowWidget;
GtkWidget* gDropdownButtonWidget;
GtkWidget* gHandleBoxWidget;
GtkWidget* gFrameWidget;
GtkWidget* gProtoWindow;
GtkWidget* gProgressWidget;
GtkWidget* gTabWidget;
GtkTooltips* gTooltipWidget;

static int gLastXError;
const char* nsNativeThemeGTK::sDisabledEngines[] = {
  "xeno",   // xenophilia
  nsnull
};

typedef struct GtkThemeEnginePrivate {
  GtkThemeEngine engine;
  void* library;
  char* name;
} GtkThemeEnginePrivate;

nsNativeThemeGTK::nsNativeThemeGTK()
  : mProtoLayout(nsnull)
{
  NS_INIT_ISUPPORTS();
  mDisabledAtom = do_GetAtom("disabled");
  mCheckedAtom = do_GetAtom("checked");
  mSelectedAtom = do_GetAtom("selected");
  mInputCheckedAtom = do_GetAtom("_moz-input-checked");
  mInputAtom = do_GetAtom("input");
  mFocusedAtom = do_GetAtom("focused");
  mFirstTabAtom = do_GetAtom("first-tab");

  memset(mDisabledWidgetTypes, 0, sizeof(mDisabledWidgetTypes));

  // Check for a blacklisted theme engine.  These are known to crash.
  // If we're using a blacklisted engine, we need to disable all widget types.

  EnsureButtonWidget();  // arbitrary, but we need a widget's style
  GtkThemeEngine* gtkThemeEng = gButtonWidget->style->engine;
  if (!gtkThemeEng)  // gtk's built-in drawing routines are safe
    return;

  const char* curEngine = ((GtkThemeEnginePrivate*) gtkThemeEng)->name;
  const char* eng;
  int i = 0;
  while ((eng = sDisabledEngines[i++])) {
    if (!strcmp(eng, curEngine)) {
#ifdef DEBUG
      printf("Disabling GTK themed widgets due to blacklisted theme engine: %s\n", eng);
#endif
      memset(mDisabledWidgetTypes, 0xff, sizeof(mDisabledWidgetTypes));
      break;
    }
  }
}

nsNativeThemeGTK::~nsNativeThemeGTK() {
  // This will destroy all of our widgets
  if (gProtoWindow)
    gtk_widget_destroy(gProtoWindow);
  if (gTooltipWidget)
    gtk_object_unref(GTK_OBJECT(gTooltipWidget));
  moz_gtk_shutdown();
}

static void GetPrimaryPresShell(nsIFrame* aFrame, nsIPresShell** aResult)
{
  *aResult = nsnull;

  if (!aFrame)
    return;
 
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  content->GetDocument(*getter_AddRefs(doc));
  if (doc)
    doc->GetShellAt(0, aResult); // Addref happens here.
}

static void RefreshWidgetWindow(nsIFrame* aFrame)
{
  nsCOMPtr<nsIPresShell> shell;

  GetPrimaryPresShell(aFrame, getter_AddRefs(shell));
  if (!shell)
    return;

  nsCOMPtr<nsIViewManager> vm;
  shell->GetViewManager(getter_AddRefs(vm));
  if (!vm)
    return;
 
  vm->UpdateAllViews(NS_VMREFRESH_NO_SYNC);
}

static PRInt32 GetContentState(nsIFrame* aFrame)
{
  if (!aFrame)
    return 0;

  nsCOMPtr<nsIPresShell> shell;
  GetPrimaryPresShell(aFrame, getter_AddRefs(shell));
  if (!shell)
    return 0;

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  nsCOMPtr<nsIEventStateManager> esm;
  context->GetEventStateManager(getter_AddRefs(esm));
  PRInt32 flags = 0;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  esm->GetContentState(content, flags);
  return flags;
}

static PRBool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return PR_FALSE;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsAutoString attr;
  nsresult res = content->GetAttr(kNameSpaceID_None, aAtom, attr);
  if (res == NS_CONTENT_ATTR_NO_VALUE ||
      (res != NS_CONTENT_ATTR_NOT_THERE && attr.IsEmpty()))
    return PR_TRUE; // This handles the HTML case (an attr with no value is like a true val)
  return attr.EqualsIgnoreCase("true"); // This handles the XUL case.
}

PRBool nsNativeThemeGTK::IsDisabled(nsIFrame* aFrame)
{
  return CheckBooleanAttr(aFrame, mDisabledAtom);
}
  
nsresult
GetSystemColor(PRUint8 aWidgetType, nsILookAndFeel::nsColorID& aColorID)
{
  switch (aWidgetType) {
  case NS_THEME_BUTTON:
  case NS_THEME_TOOLBAR_BUTTON:
  case NS_THEME_TAB:
    aColorID = nsILookAndFeel::eColor_buttontext;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult 
GetSystemFont(PRUint8 aWidgetType, nsSystemFontID& aFont)
{
  switch (aWidgetType) {
  case NS_THEME_BUTTON:
  case NS_THEME_TOOLBAR_BUTTON:
  case NS_THEME_TAB:
      aFont = eSystemFont_Button;
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

static PRBool IsWidgetTypeDisabled(PRUint8* aDisabledVector, PRUint8 aWidgetType) {
  return aDisabledVector[aWidgetType >> 3] & (1 << (aWidgetType & 7));
}

static void SetWidgetTypeDisabled(PRUint8* aDisabledVector, PRUint8 aWidgetType) {
  aDisabledVector[aWidgetType >> 3] |= (1 << (aWidgetType & 7));
}

void
nsNativeThemeGTK::GetGtkWidgetState(PRUint8 aWidgetType,
                                    nsIFrame* aFrame, GtkWidgetState* aState)
{
  if (!aFrame) {
    aState->active = PR_FALSE;
    aState->focused = PR_FALSE;
    aState->inHover = PR_FALSE;
    aState->disabled = PR_FALSE;
    aState->isDefault = PR_FALSE;
    aState->canDefault = PR_FALSE;
  } else {
    PRInt32 eventState = GetContentState(aFrame);
    aState->active = (eventState & NS_EVENT_STATE_ACTIVE);
    if (aWidgetType == NS_THEME_TEXTFIELD ||
        aWidgetType == NS_THEME_RADIO_CONTAINER)
      aState->focused = CheckBooleanAttr(aFrame, mFocusedAtom);
    else
      aState->focused = (eventState & NS_EVENT_STATE_FOCUS);
    aState->inHover = (eventState & NS_EVENT_STATE_HOVER);
    aState->disabled = IsDisabled(aFrame);
    aState->isDefault = PR_FALSE; // XXX fix me
    aState->canDefault = PR_FALSE; // XXX fix me
  }
}

static int
NativeThemeErrorHandler(Display* dpy, XErrorEvent* error) {
  gLastXError = error->error_code;
  return 0;
}

NS_IMETHODIMP
nsNativeThemeGTK::DrawWidgetBackground(nsIRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aClipRect)
{
  nsDrawingSurfaceGTK* surface;
  aContext->GetDrawingSurface((nsDrawingSurface*)&surface);
  GdkWindow* window = (GdkWindow*) surface->GetDrawable();

  nsTransform2D* transformMatrix;
  aContext->GetCurrentTransform(transformMatrix);
  nsRect tr(aRect);
  transformMatrix->TransformCoord(&tr.x, &tr.y, &tr.width, &tr.height);
  GdkRectangle gdk_rect = {tr.x, tr.y, tr.width, tr.height};
  nsRect cr(aClipRect);
  transformMatrix->TransformCoord(&cr.x, &cr.y, &cr.width, &cr.height);
  GdkRectangle gdk_clip = {cr.x, cr.y, cr.width, cr.height};

  GtkWidgetState state;
  GetGtkWidgetState(aWidgetType, aFrame, &state);

  NS_ASSERTION(!IsWidgetTypeDisabled(mDisabledWidgetTypes, aWidgetType), "Trying to render an unsafe widget!");

  // The widget code on Solaris replaces gdk's X error handler with its own,
  // which means that gdk_error_trap_push/pop won't have any effect.
  // So, instead of using those, we just use our own error handler which
  // records the last X error to occur, then restore the old error handler
  // when we're done drawing.

  gLastXError = 0;
  XErrorHandler oldHandler = XSetErrorHandler(NativeThemeErrorHandler);

  switch (aWidgetType) {
    
  case NS_THEME_BUTTON:
  case NS_THEME_TOOLBAR_BUTTON:
    {
      GtkReliefStyle relief = (aWidgetType == NS_THEME_BUTTON) ? GTK_RELIEF_NORMAL : GTK_RELIEF_NONE;
      moz_gtk_button_paint(window, gButtonWidget->style, &gdk_rect, &gdk_clip,
                           &state, relief );
    }
    break;

  case NS_THEME_CHECKBOX:
  case NS_THEME_RADIO:
    {
      EnsureCheckBoxWidget();
      
      nsIAtom* atom = nsnull;

      if (aFrame) {
        // For XUL checkboxes and radio buttons, the state of the parent
        // determines our state.
        nsCOMPtr<nsIContent> content;
        aFrame->GetContent(getter_AddRefs(content));
        if (content->IsContentOfType(nsIContent::eXUL))
          aFrame->GetParent(&aFrame);
        else {
          nsCOMPtr<nsIAtom> tag;
          content->GetTag(*getter_AddRefs(tag));
          if (tag == mInputAtom)
            atom = mInputCheckedAtom;
        }
      }
      
      if (!atom)
        atom = (aWidgetType == NS_THEME_CHECKBOX) ? mCheckedAtom : mSelectedAtom;
      
      moz_gtk_checkbox_paint(window, gCheckboxWidget->style, &gdk_rect,
                             &gdk_clip, &state, CheckBooleanAttr(aFrame, atom),
                             (aWidgetType == NS_THEME_RADIO));
    }
    break;

  case NS_THEME_SCROLLBAR_BUTTON_UP:
  case NS_THEME_SCROLLBAR_BUTTON_DOWN:
  case NS_THEME_SCROLLBAR_BUTTON_LEFT:
  case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    {
      EnsureScrollbarWidget();
      EnsureArrowWidget();
      GtkArrowType arrowType = GtkArrowType(aWidgetType - NS_THEME_SCROLLBAR_BUTTON_UP);
      moz_gtk_scrollbar_button_paint(window, gScrollbarWidget->style,
                                     &gdk_rect, &gdk_clip, &state, arrowType);
    }
    break;

  case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
  case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    EnsureScrollbarWidget();
    moz_gtk_scrollbar_trough_paint(window, gScrollbarWidget->style, &gdk_rect,
                                   &gdk_clip, &state);
    break;

  case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
  case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    EnsureScrollbarWidget();
    moz_gtk_scrollbar_thumb_paint(window, gScrollbarWidget->style, &gdk_rect,
                                  &gdk_clip, &state);
    break;

  case NS_THEME_TOOLBAR_GRIPPER:
    EnsureGripperWidget();
    moz_gtk_gripper_paint(window, gGripperWidget->style, &gdk_rect, &gdk_clip,
                          &state);
    break;

  case NS_THEME_DROPDOWN_TEXTFIELD:
    if (aFrame)
      // look at the parent frame (the textbox)
      aFrame->GetParent(&aFrame);

    // fall through

  case NS_THEME_TEXTFIELD:
    EnsureEntryWidget();
    moz_gtk_entry_paint(window, gEntryWidget->style, &gdk_rect, &gdk_clip,
                        &state);
    break;

  case NS_THEME_DROPDOWN_BUTTON:
    EnsureArrowWidget();
    moz_gtk_dropdown_arrow_paint(window, gArrowWidget->style, &gdk_rect,
                                 &gdk_clip, &state);
    break;

  case NS_THEME_CHECKBOX_CONTAINER:
  case NS_THEME_RADIO_CONTAINER:
    EnsureCheckBoxWidget();
    moz_gtk_container_paint(window, gCheckboxWidget->style, &gdk_rect,
                            &gdk_clip, &state,
                            (aWidgetType == NS_THEME_RADIO_CONTAINER));
    break;

  case NS_THEME_TOOLBOX:
    EnsureHandleBoxWidget();
    moz_gtk_toolbar_paint(window, gHandleBoxWidget->style, &gdk_rect,
                          &gdk_clip);
    break;

  case NS_THEME_TOOLTIP:
    EnsureTooltipWidget();
    moz_gtk_tooltip_paint(window, gTooltipWidget->tip_window->style, &gdk_rect,
                          &gdk_clip);
    break;

  case NS_THEME_STATUSBAR_PANEL:
    EnsureFrameWidget();
    moz_gtk_frame_paint(window, gFrameWidget->style, &gdk_rect, &gdk_clip);
    break;

  case NS_THEME_PROGRESSBAR:
  case NS_THEME_PROGRESSBAR_VERTICAL:
    EnsureProgressBarWidget();
    moz_gtk_progressbar_paint(window, gProgressWidget->style, &gdk_rect,
                              &gdk_clip);
    break;

  case NS_THEME_PROGRESSBAR_CHUNK:
  case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    EnsureProgressBarWidget();
    moz_gtk_progress_chunk_paint(window, gProgressWidget->style, &gdk_rect,
                                 &gdk_clip);
    break;

  case NS_THEME_TAB_PANELS:
    EnsureTabWidget();
    moz_gtk_tabpanels_paint(window, gTabWidget->style, &gdk_rect, &gdk_clip);
    break;

  case NS_THEME_TAB:
  case NS_THEME_TAB_LEFT_EDGE:
  case NS_THEME_TAB_RIGHT_EDGE:
    {
      EnsureTabWidget();
      gint tab_flags = 0;

      if (aWidgetType == NS_THEME_TAB && CheckBooleanAttr(aFrame, mSelectedAtom))
        tab_flags |= MOZ_GTK_TAB_SELECTED;
      else if (aWidgetType == NS_THEME_TAB_LEFT_EDGE)
        tab_flags |= MOZ_GTK_TAB_BEFORE_SELECTED;

      nsCOMPtr<nsIContent> content;
      aFrame->GetContent(getter_AddRefs(content));
      if (content->HasAttr(kNameSpaceID_None, mFirstTabAtom))
        tab_flags |= MOZ_GTK_TAB_FIRST;

      moz_gtk_tab_paint(window, gTabWidget->style, &gdk_rect, &gdk_clip,
                        tab_flags);
      break;
    }
  }

  gdk_flush();
  XSetErrorHandler(oldHandler);
  if (gLastXError) {
#ifdef DEBUG
    printf("GTK theme failed for widget type %d, error was %d, state was [active=%d,focused=%d,inHover=%d,disabled=%d]\n",
           aWidgetType, gLastXError, state.active, state.focused, state.inHover, state.disabled);
#endif
    NS_WARNING("GTK theme failed; disabling unsafe widget");
    SetWidgetTypeDisabled(mDisabledWidgetTypes, aWidgetType);
    // force refresh of the window, because the widget was not successfully drawn
    // it must be redrawn using the default look
    RefreshWidgetWindow(aFrame);
  }

  return NS_OK;
}

inline void
WidgetBorderToMargin(GtkWidget* aWidget, nsMargin* aResult)
{
  aResult->left = aResult->right = aWidget->style->klass->xthickness;
  aResult->top = aResult->bottom = aWidget->style->klass->ythickness;
}

NS_IMETHODIMP
nsNativeThemeGTK::GetWidgetBorder(nsIDeviceContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType, nsMargin* aResult)
{
  aResult->top = aResult->bottom = aResult->left = aResult->right = 0;

  switch (aWidgetType) {
  case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
  case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    {
      gint trough_border;
      EnsureScrollbarWidget();
      moz_gtk_get_scrollbar_metrics(nsnull, &trough_border, nsnull, nsnull, nsnull);
      aResult->top = aResult->bottom = aResult->left = aResult->right = trough_border;
    }
    break;
  case NS_THEME_TEXTFIELD:
  case NS_THEME_DROPDOWN_TEXTFIELD:
    EnsureEntryWidget();
    WidgetBorderToMargin(gEntryWidget, aResult);
    break;
  case NS_THEME_BUTTON:
  case NS_THEME_TOOLBAR_BUTTON:
    WidgetBorderToMargin(gButtonWidget, aResult);
    break;
  case NS_THEME_TOOLBAR_GRIPPER:
    EnsureGripperWidget();
    WidgetBorderToMargin(gGripperWidget, aResult);
    break;
  case NS_THEME_DROPDOWN_BUTTON:
    EnsureArrowWidget();
    WidgetBorderToMargin(gDropdownButtonWidget, aResult);
    break;
  case NS_THEME_CHECKBOX_CONTAINER:
  case NS_THEME_RADIO_CONTAINER:
    aResult->top = aResult->bottom = aResult->left = aResult->right = 1;
    break;
  case NS_THEME_STATUSBAR_PANEL:
    EnsureFrameWidget();
    WidgetBorderToMargin(gFrameWidget, aResult);
    break;
  case NS_THEME_PROGRESSBAR:
  case NS_THEME_PROGRESSBAR_VERTICAL:
    EnsureProgressBarWidget();
    WidgetBorderToMargin(gProgressWidget, aResult);
    break;
  case NS_THEME_TAB_PANELS:
    EnsureTabWidget();
    WidgetBorderToMargin(gTabWidget, aResult);
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeGTK::GetMinimumWidgetSize(nsIRenderingContext* aContext,
                                       nsIFrame* aFrame, PRUint8 aWidgetType,
                                       nsSize* aResult, PRBool* aIsOverridable)
{
  aResult->width = aResult->height = 0;
  *aIsOverridable = PR_TRUE;

  switch (aWidgetType) {
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
      {
        EnsureScrollbarWidget();

        gint slider_width, stepper_size;
        moz_gtk_get_scrollbar_metrics(&slider_width, nsnull, &stepper_size,
                                      nsnull, nsnull);

        aResult->width = slider_width;
        aResult->height = stepper_size;
        *aIsOverridable = PR_FALSE;
      }
      break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
      {
        EnsureScrollbarWidget();

        gint slider_width, min_slider_size;
        moz_gtk_get_scrollbar_metrics(&slider_width, nsnull, nsnull, nsnull,
                                      &min_slider_size);

        if (aWidgetType == NS_THEME_SCROLLBAR_THUMB_VERTICAL) {
          aResult->width = slider_width;
          aResult->height = min_slider_size;
        } else {
          aResult->width = min_slider_size;
          aResult->height = slider_width;
        }

        *aIsOverridable = PR_FALSE;
      }
      break;
  case NS_THEME_DROPDOWN_BUTTON:
    {
      EnsureArrowWidget();

      // First, get the minimum size for the button itself.
      aResult->width = 2 * (1 + gDropdownButtonWidget->style->klass->xthickness);
      aResult->height = 2 * (1 + gDropdownButtonWidget->style->klass->ythickness);

      // Now, add in the requested size of the arrow.
      // Note: the minimum arrow size is fixed at 11 pixels.

      aResult->width += 11 + GTK_MISC(gArrowWidget)->xpad * 2;
      aResult->height += 11 + GTK_MISC(gArrowWidget)->ypad * 2;      
      *aIsOverridable = PR_FALSE;
    }
    break;
  case NS_THEME_CHECKBOX:
  case NS_THEME_RADIO:
  case NS_THEME_CHECKBOX_CONTAINER:
  case NS_THEME_RADIO_CONTAINER:
    {
      EnsureCheckBoxWidget();

      gint indicator_size, indicator_spacing;
      moz_gtk_checkbox_get_metrics(&indicator_size, &indicator_spacing);

      // Hack alert: several themes have indicators larger than the default
      // 10px size, but don't set the indicator size property.  So, leave
      // a little slop room by making the minimum size 14px.

      aResult->width = aResult->height = MAX(indicator_size, 14);
      *aIsOverridable = PR_FALSE;
    }
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeGTK::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                     nsIAtom* aAttribute, PRBool* aShouldRepaint)
{
  // Some widget types just never change state.
  if (aWidgetType == NS_THEME_TOOLBOX || aWidgetType == NS_THEME_TOOLBAR ||
      aWidgetType == NS_THEME_STATUSBAR || aWidgetType == NS_THEME_STATUSBAR_PANEL ||
      aWidgetType == NS_THEME_STATUSBAR_RESIZER_PANEL ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL ||
      aWidgetType == NS_THEME_PROGRESSBAR ||
      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL ||
      aWidgetType == NS_THEME_TOOLTIP) {
    *aShouldRepaint = PR_FALSE;
    return NS_OK;
  }

  // XXXdwh Not sure what can really be done here.  Can at least guess for
  // specific widgets that they're highly unlikely to have certain states.
  // For example, a toolbar doesn't care about any states.
  if (!aAttribute) {
    // Hover/focus/active changed.  Always repaint.
    *aShouldRepaint = PR_TRUE;
  }
  else {
    // Check the attribute to see if it's relevant.  
    // disabled, checked, dlgtype, default, etc.
    *aShouldRepaint = PR_FALSE;
    if (aAttribute == mDisabledAtom || aAttribute == mCheckedAtom ||
        aAttribute == mSelectedAtom)
      *aShouldRepaint = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeGTK::ThemeChanged()
{
  memset(mDisabledWidgetTypes, 0, sizeof(mDisabledWidgetTypes));
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsNativeThemeGTK::ThemeSupportsWidget(nsIPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  // Check for specific widgets to see if HTML has overridden the style.
  if (aFrame) {
    // For now don't support HTML.
    nsCOMPtr<nsIContent> content;
    aFrame->GetContent(getter_AddRefs(content));
    if (content->IsContentOfType(nsIContent::eHTML))
      return PR_FALSE;
  }

  if (IsWidgetTypeDisabled(mDisabledWidgetTypes, aWidgetType))
    return PR_FALSE;

  switch (aWidgetType) {
  case NS_THEME_BUTTON:
  case NS_THEME_RADIO:
  case NS_THEME_CHECKBOX:
  case NS_THEME_TOOLBOX:
    // case NS_THEME_TOOLBAR:  (not in skin)
  case NS_THEME_TOOLBAR_BUTTON:
  case NS_THEME_TOOLBAR_DUAL_BUTTON: // so we can override the border with 0
    // case NS_THEME_TOOLBAR_DUAL_BUTTON_DROPDOWN:
    // case NS_THEME_TOOLBAR_SEPARATOR:
  case NS_THEME_TOOLBAR_GRIPPER:
  case NS_THEME_STATUSBAR:
  case NS_THEME_STATUSBAR_PANEL:
    // case NS_THEME_RESIZER:  (n/a for gtk)
    // case NS_THEME_LISTBOX:
    // case NS_THEME_LISTBOX_LISTITEM:
    // case NS_THEME_TREEVIEW:
    // case NS_THEME_TREEVIEW_TREEITEM:
    // case NS_THEME_TREEVIEW_TWISTY:
    // case NS_THEME_TREEVIEW_LINE:
    // case NS_THEME_TREEVIEW_HEADER:
    // case NS_THEME_TREEVIEW_HEADER_CELL:
    // case NS_THEME_TREEVIEW_HEADER_SORTARROW:
    // case NS_THEME_TREEVIEW_TWISTY_OPEN:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TAB:
    // case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_LEFT_EDGE:
    case NS_THEME_TAB_RIGHT_EDGE:
    case NS_THEME_TAB_PANELS:
  case NS_THEME_TOOLTIP:
    // case NS_THEME_SPINNER:
    // case NS_THEME_SPINNER_UP_BUTTON:
    // case NS_THEME_SPINNER_DOWN_BUTTON:
    // case NS_THEME_SCROLLBAR:  (n/a for gtk)
  case NS_THEME_SCROLLBAR_BUTTON_UP:
  case NS_THEME_SCROLLBAR_BUTTON_DOWN:
  case NS_THEME_SCROLLBAR_BUTTON_LEFT:
  case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
  case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
  case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
  case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
  case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    // case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:  (n/a for gtk)
    // case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:  (n/a for gtk)
  case NS_THEME_TEXTFIELD:
    // case NS_THEME_TEXTFIELD_CARET:
    // case NS_THEME_DROPDOWN:
  case NS_THEME_DROPDOWN_BUTTON:
    // case NS_THEME_DROPDOWN_TEXT:
  case NS_THEME_DROPDOWN_TEXTFIELD:
    // case NS_THEME_SLIDER:
    // case NS_THEME_SLIDER_THUMB:
    // case NS_THEME_SLIDER_THUMB_START:
    // case NS_THEME_SLIDER_THUMB_END:
    // case NS_THEME_SLIDER_TICK:
  case NS_THEME_CHECKBOX_CONTAINER:
  case NS_THEME_RADIO_CONTAINER:
    // case NS_THEME_WINDOW:
    // case NS_THEME_DIALOG:
    // case NS_THEME_MENU:
    // case NS_THEME_MENUBAR:
    return PR_TRUE;
  }

  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsNativeThemeGTK::WidgetIsContainer(PRUint8 aWidgetType)
{
  // XXXdwh At some point flesh all of this out.
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON || 
      aWidgetType == NS_THEME_RADIO ||
      aWidgetType == NS_THEME_CHECKBOX)
    return PR_FALSE;
  return PR_TRUE;
}

void
nsNativeThemeGTK::SetupWidgetPrototype(GtkWidget* widget)
{
  if (!gProtoWindow) {
    gProtoWindow = gtk_window_new(GTK_WINDOW_POPUP);
    mProtoLayout = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(gProtoWindow), mProtoLayout);
  }

  gtk_container_add(GTK_CONTAINER(mProtoLayout), widget);
  gtk_widget_set_rc_style(widget);
  gtk_widget_realize(widget);
}

void
nsNativeThemeGTK::EnsureButtonWidget()
{
  if (!gButtonWidget) {
    gButtonWidget = gtk_button_new_with_label("M");
    SetupWidgetPrototype(gButtonWidget);
  }
}

void
nsNativeThemeGTK::EnsureCheckBoxWidget()
{
  if (!gCheckboxWidget) {
    gCheckboxWidget = gtk_check_button_new_with_label("M");
    SetupWidgetPrototype(gCheckboxWidget);
  }
}

void
nsNativeThemeGTK::EnsureScrollbarWidget()
{
  if (!gScrollbarWidget) {
    gScrollbarWidget = gtk_vscrollbar_new(NULL);
    SetupWidgetPrototype(gScrollbarWidget);
  }
}

void
nsNativeThemeGTK::EnsureGripperWidget()
{
  if (!gGripperWidget) {
    gGripperWidget = gtk_handle_box_new();
    SetupWidgetPrototype(gGripperWidget);
  }
}

void
nsNativeThemeGTK::EnsureEntryWidget()
{
  if (!gEntryWidget) {
    gEntryWidget = gtk_entry_new();
    SetupWidgetPrototype(gEntryWidget);
  }
}

void
nsNativeThemeGTK::EnsureArrowWidget()
{
  if (!gArrowWidget) {
    gDropdownButtonWidget = gtk_button_new();
    SetupWidgetPrototype(gDropdownButtonWidget);
    gArrowWidget = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
    gtk_container_add(GTK_CONTAINER(gDropdownButtonWidget), gArrowWidget);
    gtk_widget_set_rc_style(gArrowWidget);
    gtk_widget_realize(gArrowWidget);
  }
}   

void
nsNativeThemeGTK::EnsureHandleBoxWidget()
{
  if (!gHandleBoxWidget) {
    gHandleBoxWidget = gtk_handle_box_new();
    SetupWidgetPrototype(gHandleBoxWidget);
  }
}

void
nsNativeThemeGTK::EnsureTooltipWidget()
{
  if (!gTooltipWidget) {
    gTooltipWidget = gtk_tooltips_new();
    gtk_tooltips_force_window(gTooltipWidget);
    gtk_widget_set_rc_style(gTooltipWidget->tip_window);
    gtk_widget_realize(gTooltipWidget->tip_window);
  }
}

void
nsNativeThemeGTK::EnsureFrameWidget()
{
  if (!gFrameWidget) {
    gFrameWidget = gtk_frame_new(NULL);
    SetupWidgetPrototype(gFrameWidget);
  }
}

void
nsNativeThemeGTK::EnsureProgressBarWidget()
{
  if (!gProgressWidget) {
    gProgressWidget = gtk_progress_bar_new();
    SetupWidgetPrototype(gProgressWidget);
  }
}

void
nsNativeThemeGTK::EnsureTabWidget()
{
  if (!gTabWidget) {
    gTabWidget = gtk_notebook_new();
    SetupWidgetPrototype(gTabWidget);
  }
}

