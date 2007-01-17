/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@pavlov.net>
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

#ifndef GFX_ASURFACE_H
#define GFX_ASURFACE_H

#include <cairo.h>

#include "gfxTypes.h"
#include "gfxRect.h"
#include "nsStringFwd.h"

/**
 * A surface is something you can draw on. Instantiate a subclass of this
 * abstract class, and use gfxContext to draw on this surface.
 */
class THEBES_API gfxASurface {
public:
    // Surfaces use refcounting that's tied to the cairo surface refcnt, to avoid
    // refcount mismatch issues.
    nsrefcnt AddRef(void) {
        NS_PRECONDITION(mSurface != nsnull, "gfxASurface::AddRef without mSurface");

        if (mHasFloatingRef) {
            // eat the floating ref
            mHasFloatingRef = PR_FALSE;
        } else {
            cairo_surface_reference(mSurface);
        }

        return (nsrefcnt) cairo_surface_get_reference_count(mSurface);
    }

    nsrefcnt Release(void) {
        NS_PRECONDITION(!mHasFloatingRef, "gfxASurface::Release while floating ref still outstanding!");
        NS_PRECONDITION(mSurface != nsnull, "gfxASurface::Release without mSurface");
        // Note that there is a destructor set on user data for mSurface,
        // which will delete this gfxASurface wrapper when the surface's refcount goes
        // out of scope.
        nsrefcnt refcnt = (nsrefcnt) cairo_surface_get_reference_count(mSurface);
        cairo_surface_destroy(mSurface);

        // |this| may not be valid any more, don't use it!

        return --refcnt;
    }

public:
    /**
     * The format for an image surface. For all formats with alpha data, 0
     * means transparent, 1 or 255 means fully opaque.
     */
    typedef enum {
        ImageFormatARGB32, ///< ARGB data in native endianness, using premultiplied alpha
        ImageFormatRGB24,  ///< xRGB data in native endianness
        ImageFormatA8,     ///< Only an alpha channel
        ImageFormatA1,     ///< Packed transparency information (one byte refers to 8 pixels)
        ImageFormatUnknown
    } gfxImageFormat;

    typedef enum {
        SurfaceTypeImage,
        SurfaceTypePDF,
        SurfaceTypePS,
        SurfaceTypeXlib,
        SurfaceTypeXcb,
        SurfaceTypeGlitz,
        SurfaceTypeQuartz,
        SurfaceTypeWin32,
        SurfaceTypeBeOS,
        SurfaceTypeDirectFB,
        SurfaceTypeSVG,
        SurfaceTypeQuartz2
    } gfxSurfaceType;

    typedef enum {
        CONTENT_COLOR = CAIRO_CONTENT_COLOR,
        CONTENT_ALPHA = CAIRO_CONTENT_ALPHA,
        CONTENT_COLOR_ALPHA = CAIRO_CONTENT_COLOR_ALPHA
    } gfxContentType;

    /* Wrap the given cairo surface and return a gfxASurface for it */
    static already_AddRefed<gfxASurface> Wrap(cairo_surface_t *csurf);

    /*** this DOES NOT addref the surface */
    cairo_surface_t *CairoSurface() { return mSurface; }

    gfxSurfaceType GetType() const { return (gfxSurfaceType)cairo_surface_get_type(mSurface); }

    gfxContentType GetContentType() const { return (gfxContentType)cairo_surface_get_content(mSurface); }

    void SetDeviceOffset (gfxFloat xOff, gfxFloat yOff) {
        cairo_surface_set_device_offset(mSurface,
                                        xOff, yOff);
    }

    void GetDeviceOffset (gfxFloat *xOff, gfxFloat *yOff) {
        cairo_surface_get_device_offset(mSurface, xOff, yOff);
    }

    void Flush() { cairo_surface_flush(mSurface); }
    void MarkDirty() { cairo_surface_mark_dirty(mSurface); }
    void MarkDirty(const gfxRect& r) {
        cairo_surface_mark_dirty_rectangle(mSurface,
                                           (int) r.pos.x, (int) r.pos.y,
                                           (int) r.size.width, (int) r.size.height);
    }

    /* Printing backend functions */
    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName) { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult EndPrinting() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult AbortPrinting() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult BeginPage() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult EndPage() { return NS_ERROR_NOT_IMPLEMENTED; }

    void SetData(const cairo_user_data_key_t *key,
                 void *user_data,
                 cairo_destroy_func_t destroy)
    {
        cairo_surface_set_user_data (mSurface, key, user_data, destroy);
    }

    void *GetData(const cairo_user_data_key_t *key)
    {
        return cairo_surface_get_user_data (mSurface, key);
    }

protected:
    static gfxASurface* GetSurfaceWrapper(cairo_surface_t *csurf);
    static void SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf);

    void Init(cairo_surface_t *surface, PRBool existingSurface = PR_FALSE);

    virtual ~gfxASurface() {
    }
private:
    cairo_surface_t *mSurface;
    PRPackedBool mHasFloatingRef;

    static void SurfaceDestroyFunc(void *data);
};

/**
 * An Unknown surface; used to wrap unknown cairo_surface_t returns from cairo
 */
class THEBES_API gfxUnknownSurface : public gfxASurface {
public:
    gfxUnknownSurface(cairo_surface_t *surf) {
        Init(surf, PR_TRUE);
    }

    virtual ~gfxUnknownSurface() { }
};

#endif /* GFX_ASURFACE_H */
