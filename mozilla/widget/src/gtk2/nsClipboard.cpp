/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
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
 * The Original Code mozilla.org code.
 *
 * The Initial Developer of the Original Code Christopher Blizzard
 * <blizzard@mozilla.org>.  Portions created by the Initial Developer
 * are Copyright (C) 2001 the Initial Developer. All Rights Reserved.
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
 * ***** END LICENSE BLOCK ***** */

#include "nsClipboard.h"
#include "nsSupportsPrimitives.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

#include <gtk/gtkclipboard.h>
#include <gtk/gtkinvisible.h>

// Callback when someone asks us for the selection
void
invisible_selection_get_cb (GtkWidget          *aWidget,
                            GtkSelectionData   *aSelectionData,
                            guint               aTime,
                            guint               aInfo,
                            nsClipboard        *aClipboard);

gboolean
selection_clear_event_cb   (GtkWidget          *aWidget,
                            GdkEventSelection  *aEvent,
                            nsClipboard        *aClipboard);

nsClipboard::nsClipboard()
{
    NS_INIT_ISUPPORTS();
    mWidget = nsnull;
}

nsClipboard::~nsClipboard()
{
    if (mWidget)
        gtk_widget_destroy(mWidget);
}

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)

nsresult
nsClipboard::Init(void)
{
    mWidget = gtk_invisible_new();
    if (!mWidget)
        return NS_ERROR_FAILURE;

    g_signal_connect(G_OBJECT(mWidget), "selection_get",
                     G_CALLBACK(invisible_selection_get_cb), this);

    g_signal_connect(G_OBJECT(mWidget), "selection_clear_event",
                     G_CALLBACK(selection_clear_event_cb), this);

    // XXX make sure to set up the selection_clear event

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *aOwner, PRInt32 aWhichClipboard)
{
    // See if we can short cut
    if ((aWhichClipboard == kGlobalClipboard &&
         aTransferable == mGlobalTransferable.get() &&
         aOwner == mGlobalOwner.get()) ||
        (aWhichClipboard == kSelectionClipboard &&
         aTransferable == mSelectionTransferable.get() &&
         aOwner == mSelectionOwner.get())) {
        return NS_OK;
    }

    // Clear out the clipboard in order to set the new data
    EmptyClipboard(aWhichClipboard);

    if (aWhichClipboard == kSelectionClipboard) {
        mSelectionOwner = aOwner;
        mSelectionTransferable = aTransferable;
    }
    else {
        mGlobalOwner = aOwner;
        mGlobalTransferable = aTransferable;
    }

    // Which selection are we about to claim, CLIPBOARD or PRIMARY?
    GdkAtom selectionAtom = GetSelectionAtom(aWhichClipboard);

    // Make ourselves the owner.  If we fail to, return.
    if (!gtk_selection_owner_set(mWidget, selectionAtom, GDK_CURRENT_TIME))
        return NS_ERROR_FAILURE;

    // Clear the old selection target list.
    gtk_selection_clear_targets(mWidget, selectionAtom);

    // Get the types of supported flavors
    nsresult rv;
    nsCOMPtr<nsISupportsArray> flavors;

    rv = aTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    // Add all the flavors to this widget's supported type.
    PRUint32 count;
    flavors->Count(&count);
    for (PRUint32 i=0; i < count; i++) {
        nsCOMPtr<nsISupports> tastesLike;
        flavors->GetElementAt(i, getter_AddRefs(tastesLike));
        nsCOMPtr<nsISupportsCString> flavor = do_QueryInterface(tastesLike);

        if (flavor) {
            nsXPIDLCString flavorStr;
            flavor->ToString(getter_Copies(flavorStr));

            // special case text/unicode since we can handle all of
            // the string types
            if (!strcmp(flavorStr, kUnicodeMime)) {
                AddTarget(gdk_atom_intern("UTF8_STRING", FALSE),
                          selectionAtom);
                AddTarget(gdk_atom_intern("COMPOUND_TEXT", FALSE),
                          selectionAtom);
                AddTarget(gdk_atom_intern("TEXT", FALSE), selectionAtom);
                AddTarget(GDK_SELECTION_TYPE_STRING, selectionAtom);
                // next loop iteration
                continue;
            }

            // Add this to our list of valid targets
            GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
            AddTarget(atom, selectionAtom);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
    if (!aTransferable)
        return NS_ERROR_FAILURE;

    GtkClipboard *clipboard;
    clipboard = gtk_clipboard_get(GetSelectionAtom(aWhichClipboard));

    guchar        *data = NULL;
    gint           length = 0;
    PRBool         foundData = PR_FALSE;
    nsCAutoString  foundFlavor;

    // Get a list of flavors this transferable can import
    nsCOMPtr<nsISupportsArray> flavors;
    nsresult rv;
    rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    PRUint32 count;
    flavors->Count(&count);
    for (PRUint32 i=0; i < count; i++) {
        nsCOMPtr<nsISupports> genericFlavor;
        flavors->GetElementAt(i, getter_AddRefs(genericFlavor));

        nsCOMPtr<nsISupportsCString> currentFlavor;
        currentFlavor = do_QueryInterface(genericFlavor);

        if (currentFlavor) {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            // Special case text/unicode since we can convert any
            // string into text/unicode
            if (!strcmp(flavorStr, kUnicodeMime)) {
                gchar *new_text = gtk_clipboard_wait_for_text(clipboard);
                if (new_text) {
                    // Convert utf-8 into our unicode format.
                    nsString ucs2string = NS_ConvertUTF8toUCS2(new_text);
                    data = (guchar *)ToNewUnicode(ucs2string);
                    length = ucs2string.Length() * 2;
                    g_free(new_text);
                    foundData = PR_TRUE;
                    foundFlavor = kUnicodeMime;
                    break;
                }
                // If the type was text/unicode and we couldn't get
                // text off the clipboard, run the next loop
                // iteration.
                continue;
            }

            // Get the atom for this type and try to request it off
            // the clipboard.
            GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
            GtkSelectionData *selectionData;
            selectionData = gtk_clipboard_wait_for_contents(clipboard,
                                                            atom);
            if (selectionData) {
                length = selectionData->length * selectionData->format / 8;
                data = (guchar *)nsMemory::Alloc(length);
                if (!data)
                    break;
                memcpy(data, selectionData->data, length);
                foundData = PR_TRUE;
                foundFlavor = flavorStr;
                break;
            }
        }
    }

    if (foundData) {
        nsCOMPtr<nsISupports> wrapper;
        nsPrimitiveHelpers::CreatePrimitiveForData(foundFlavor.get(),
                                                   data, length,
                                                   getter_AddRefs(wrapper));
        aTransferable->SetTransferData(foundFlavor.get(),
                                       wrapper, length);
    }

    if (data)
        nsMemory::Free(data);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
    if (aWhichClipboard == kSelectionClipboard) {
        if (mSelectionOwner) {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nsnull;
        }
        mSelectionTransferable = nsnull;
    }
    else {
        if (mGlobalOwner) {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nsnull;
        }
        mGlobalTransferable = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::ForceDataToClipboard(PRInt32 aWhichClipboard)
{
    // We don't have a way to force data to the clipboard since
    // there's no common place to put clipboard data.

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList,
                                    PRInt32 aWhichClipboard, PRBool *_retval)
{
    *_retval = PR_FALSE;

    PRUint32 length = 0;
    aFlavorList->Count(&length);
    if (!length)
        return NS_OK;

    GtkSelectionData *selection_data =
        GetTargets(GetSelectionAtom(aWhichClipboard));
    if (!selection_data)
        return NS_OK;

    gint n_targets = 0;
    GdkAtom *targets = NULL;

    if (!gtk_selection_data_get_targets(selection_data, 
                                        &targets, &n_targets) ||
        !n_targets)
        return NS_OK;

    // Walk through the provided types and try to match it to a
    // provided type.
    for (PRUint32 i = 0; i < length && !*_retval; i++) {
        nsCOMPtr<nsISupports> genericFlavor;
        aFlavorList->GetElementAt(i, getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> flavorWrapper;
        flavorWrapper = do_QueryInterface(genericFlavor);

        if (flavorWrapper) {
            nsXPIDLCString myStr;
            flavorWrapper->ToString(getter_Copies(myStr));

            // We special case text/unicode here.
            if (!strcmp(myStr, kUnicodeMime) && 
                gtk_selection_data_targets_include_text(selection_data)) {
                *_retval = PR_TRUE;
                break;
            }

            for (PRInt32 j = 0; j < n_targets; j++) {
                gchar *atom_name = gdk_atom_name(targets[i]);
                if (!strcmp(atom_name, (const char *)myStr))
                    *_retval = PR_TRUE;

                g_free(atom_name);

                if (*_retval)
                    break;
            }
        }
    }

    gtk_selection_data_free(selection_data);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
    *_retval = PR_TRUE; // yeah, unix supports the selection clipboard
    return NS_OK;
}

/* static */
GdkAtom
nsClipboard::GetSelectionAtom(PRInt32 aWhichClipboard)
{
    if (aWhichClipboard == kGlobalClipboard)
        return GDK_SELECTION_CLIPBOARD;

    return GDK_SELECTION_PRIMARY;
}

/* static */
GtkSelectionData *
nsClipboard::GetTargets(GdkAtom aWhichClipboard)
{
    GtkClipboard *clipboard = gtk_clipboard_get(aWhichClipboard);
    return gtk_clipboard_wait_for_contents (clipboard,
                                            gdk_atom_intern ("TARGETS",
                                                             FALSE));
}

nsITransferable *
nsClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
    nsITransferable *retval;

    if (aWhichClipboard == kSelectionClipboard)
        retval = mSelectionTransferable.get();
    else
        retval = mGlobalTransferable.get();
        
    NS_IF_ADDREF(retval);
    return retval;
}

void
nsClipboard::AddTarget(GdkAtom aName, GdkAtom aClipboard)
{
    gtk_selection_add_target(mWidget, aClipboard, aName, 0);
}

void
nsClipboard::SelectionGetEvent (GtkWidget        *aWidget,
                                GtkSelectionData *aSelectionData,
                                guint             aTime)
{
    // Someone has asked us to hand them something.  The first thing
    // that we want to do is see if that something includes text.  If
    // it does, try to give it text/unicode after converting it to
    // utf-8.

    PRInt32 whichClipboard;

    // which clipboard?
    if (aSelectionData->selection == GDK_SELECTION_PRIMARY)
        whichClipboard = kSelectionClipboard;
    else if (aSelectionData->selection == GDK_SELECTION_CLIPBOARD)
        whichClipboard = kGlobalClipboard;
    else
        return; // THAT AINT NO CLIPBOARD I EVER HEARD OF

    nsCOMPtr<nsITransferable> trans;
    trans = getter_AddRefs(GetTransferable(whichClipboard));
    
    nsresult rv;
    nsCOMPtr<nsISupports> item;
    PRUint32 len;

    // Check to see if the selection data includes any of the string
    // types that we support.
    if (aSelectionData->target == gdk_atom_intern ("STRING", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("TEXT", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("COMPOUND_TEXT", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("UTF8_STRING", FALSE)) {
        // Try to convert our internal type into a text string.  Get
        // the transferable for this clipboard and try to get the
        // text/unicode type for it.
        rv = trans->GetTransferData("text/unicode", getter_AddRefs(item),
                                    &len);
        if (!item || NS_FAILED(rv))
            return;
        
        nsCOMPtr<nsISupportsString> wideString;
        wideString = do_QueryInterface(item);
        if (!wideString)
            return;

        nsAutoString ucs2string;
        wideString->GetData(ucs2string);
        char *utf8string = ToNewUTF8String(ucs2string);
        if (!utf8string)
            return;
        
        gtk_selection_data_set_text (aSelectionData, utf8string,
                                     strlen(utf8string));

        nsMemory::Free(utf8string);
        return;
    }

    // Try to match up the selection data target to something our
    // transferable provides.
    gchar *target_name = gdk_atom_name(aSelectionData->target);
    if (!target_name)
        return;

    rv = trans->GetTransferData(target_name, getter_AddRefs(item), &len);
    // nothing found?
    if (!item || NS_FAILED(rv)) {
        g_free(target_name);
        return;
    }

    void *primitive_data = nsnull;
    nsPrimitiveHelpers::CreateDataFromPrimitive(target_name, item,
                                                &primitive_data, len);

    if (primitive_data) {
        gtk_selection_data_set(aSelectionData, aSelectionData->target,
                               8, /* 8 bits in a unit */
                               (const guchar *)primitive_data, len);
        nsMemory::Free(primitive_data);
    }

    g_free(target_name);
                           
}

void
nsClipboard::SelectionClearEvent (GtkWidget         *aWidget,
                                  GdkEventSelection *aEvent)
{
    PRInt32 whichClipboard;

    // which clipboard?
    if (aEvent->selection == GDK_SELECTION_PRIMARY)
        whichClipboard = kSelectionClipboard;
    else if (aEvent->selection == GDK_SELECTION_CLIPBOARD)
        whichClipboard = kGlobalClipboard;
    else
        return; // THAT AINT NO CLIPBOARD I EVER HEARD OF

    EmptyClipboard(whichClipboard);
}

void
invisible_selection_get_cb (GtkWidget          *aWidget,
                            GtkSelectionData   *aSelectionData,
                            guint               aTime,
                            guint               aInfo,
                            nsClipboard        *aClipboard)
{
    aClipboard->SelectionGetEvent(aWidget, aSelectionData, aTime);
}

gboolean
selection_clear_event_cb   (GtkWidget          *aWidget,
                            GdkEventSelection  *aEvent,
                            nsClipboard        *aClipboard)
{
    aClipboard->SelectionClearEvent(aWidget, aEvent);
    return TRUE;
}
