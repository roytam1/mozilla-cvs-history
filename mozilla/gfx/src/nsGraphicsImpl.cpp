/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Patrick C. Beard <beard@netscape.com>
 */

#include "nsGraphicsImpl.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsCRT.h"

NS_IMPL_ISUPPORTS1(nsGraphicsImpl, nsIGraphics)

nsGraphicsImpl::nsGraphicsImpl(nsIRenderingContext* aRenderer)
	:	mRenderer(aRenderer)
{
	NS_INIT_ISUPPORTS();
	
	// hack:  back out the coordinate transformation to use pixels.
	nsCOMPtr<nsIDeviceContext> dc;
	mRenderer->GetDeviceContext(*getter_AddRefs(dc));
	dc->GetDevUnitsToAppUnits(mDev2App);
	mRenderer->Scale(mDev2App, mDev2App);
}

NS_IMETHODIMP nsGraphicsImpl::GetColor(nscolor *aColor)
{
	return mRenderer->GetColor(*aColor);
}

NS_IMETHODIMP nsGraphicsImpl::SetColor(nscolor aColor)
{
	return mRenderer->SetColor(aColor);
}

NS_IMETHODIMP nsGraphicsImpl::ClipRect(nscoord x, nscoord y, nscoord width, nscoord height)
{
	nsRect r(x, y, width, height);
	PRBool clipEmpty;
	return mRenderer->SetClipRect(r, nsClipCombine_kIntersect, clipEmpty);
}

NS_IMETHODIMP nsGraphicsImpl::DrawLine(nscoord x1, nscoord y1, nscoord x2, nscoord y2)
{
	return mRenderer->DrawLine(x1, y1, x2, y2);
}

NS_IMETHODIMP nsGraphicsImpl::DrawRect(nscoord x, nscoord y, nscoord width, nscoord height)
{
	return mRenderer->DrawRect(x, y, width, height);
}

NS_IMETHODIMP nsGraphicsImpl::FillRect(nscoord x, nscoord y, nscoord width, nscoord height)
{
	return mRenderer->FillRect(x, y, width, height);
}

NS_IMETHODIMP nsGraphicsImpl::InvertRect(nscoord x, nscoord y, nscoord width, nscoord height)
{
	return mRenderer->InvertRect(x, y, width, height);
}

NS_IMETHODIMP nsGraphicsImpl::DrawEllipse(nscoord x, nscoord y, nscoord width, nscoord height)
{
	return mRenderer->DrawEllipse(x, y, width, height);
}

NS_IMETHODIMP nsGraphicsImpl::FillEllipse(nscoord x, nscoord y, nscoord width, nscoord height)
{
	return mRenderer->FillEllipse(x, y, width, height);
}

NS_IMETHODIMP nsGraphicsImpl::InvertEllipse(nscoord x, nscoord y, nscoord width, nscoord height)
{
	// return mRenderer->InvertEllipse(x, y, width, height);
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsGraphicsImpl::DrawArc(nscoord x, nscoord y, nscoord width, nscoord height, float startAngle, float endAngle)
{
	return mRenderer->DrawArc(x, y, width, height, startAngle, endAngle);
}

NS_IMETHODIMP nsGraphicsImpl::FillArc(nscoord x, nscoord y, nscoord width, nscoord height, float startAngle, float endAngle)
{
	return mRenderer->FillArc(x, y, width, height, startAngle, endAngle);
}

NS_IMETHODIMP nsGraphicsImpl::InvertArc(nscoord x, nscoord y, nscoord width, nscoord height, float startAngle, float endAngle)
{
	// return mRenderer->InvertArc(x, y, width, height, startAngle, endAngle);
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsGraphicsImpl::DrawPolygon(PRUint32 count, PRInt32 *points)
{
	return mRenderer->DrawPolygon((nsPoint*)points, count / 2);
}

NS_IMETHODIMP nsGraphicsImpl::FillPolygon(PRUint32 count, PRInt32 *points)
{
	return mRenderer->FillPolygon((nsPoint*)points, count / 2);
}

NS_IMETHODIMP nsGraphicsImpl::InvertPolygon(PRUint32 count, PRInt32 *points)
{
	// return mRenderer->InvertPolygon(points);
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsGraphicsImpl::DrawString(const PRUnichar *text, nscoord x, nscoord y)
{
	// this works around a bug in the way the ascent calculation is done.
	nsCOMPtr<nsIFontMetrics> metrics;
	if (mRenderer->GetFontMetrics(*getter_AddRefs(metrics)) == NS_OK) {
		nscoord ascent = 0;
		metrics->GetMaxAscent(ascent);
		y -= ascent;
		return mRenderer->DrawString(text, nsCRT::strlen(text), x, y);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsGraphicsImpl::SetFont(const PRUnichar *name, nscoord size)
{
	size *= mDev2App;
	nsFont font(name, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE, size);
	return mRenderer->SetFont(font);
}

NS_IMETHODIMP nsGraphicsImpl::Gsave(void)
{
	return mRenderer->PushState();
}

NS_IMETHODIMP nsGraphicsImpl::Grestore(void)
{
	PRBool unused;
	return mRenderer->PopState(unused);
}
