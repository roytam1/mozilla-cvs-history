/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef CXIcon_H
#define CXIcon_H
#include "cxdc.h"
void Icon_GetUrlExitRoutine(URL_Struct *pUrl, int iStatus, MWContext *pContext);  
class CXIcon;

class CCustomImageObject
{
private:
	CPtrList loadingImagesList; // A list of images that this window is still waiting for.

public:
	virtual void LoadComplete(HT_Resource r) = 0;

	virtual NSNavCenterImage* LookupImage(const char* url, HT_Resource r);

	virtual ~CCustomImageObject();

	virtual void AddLoadingImage(NSNavCenterImage* pImage);
	virtual void RemoveLoadingImage(NSNavCenterImage* pImage);
};

struct CIconCallbackInfo
{
	HT_Resource pResource;
	CCustomImageObject* pObject;

	CIconCallbackInfo(CCustomImageObject* pObj, HT_Resource pRes)
		:pObject(pObj), pResource(pRes) {}
};


class NSNavCenterImage {
public:

	char *pUrl;
	BOOL m_bCompletelyLoaded;
	BITMAPINFO *bmpInfo;
	void XP_HUGE *bits;
	void XP_HUGE *maskbits;
	HDC hSubDC;
	
	static HBITMAP m_hBadImageBitmap;
	static int refCount;
	CXIcon* iconContext;
	BOOL m_BadImage;

	CPtrList resourceList;
	HT_Resource m_HTResource;
	
	int m_nRefCount;
	int pairCount;

	NSNavCenterImage(const char * pUrl);
	virtual ~NSNavCenterImage();
	
	void ProcessIcon();
	void CompleteCallback();
	void CompleteCallbackWithoutDeletion();

	BOOL CompletelyLoaded();
	BOOL SuccessfullyLoaded();
	void DestroyContext();

	void RemoveListener(CCustomImageObject* pObject);
	void AddListener(CCustomImageObject* pObject, HT_Resource r);
};

class CXIcon : public CDCCX {

public:
	CXIcon(){}
	CXIcon(NSNavCenterImage* image);
	virtual ~CXIcon();

private:
	CPtrList imageList;
	HDC m_hDC;
	NI_Pixmap* m_image;
	NI_Pixmap* m_mask;

	NSNavCenterImage* m_icon;

public:
	virtual HDC GetContextDC() { return m_hDC; }
	void SubstituteDC(HDC hdc) { m_hDC = hdc; }
	virtual BOOL IsDeviceDC() { return TRUE; }
	virtual HDC GetAttribDC() { return m_hDC; }
	virtual BITMAPINFO *NewPixmap(NI_Pixmap *pImage, BOOL mask);
	void DeleteContextDC() { DeleteDC(m_hDC); }
	void ReleaseContextDC(HDC pDC) {}
	virtual void ImageComplete(NI_Pixmap* image);
	//	Don't display partial images.
	virtual void AllConnectionsComplete(MWContext *pContext);
	void NiceDestruction();
	virtual int DisplayPixmap(NI_Pixmap* image, NI_Pixmap* mask, int32 x, int32 y, int32 x_offset, int32 y_offset, int32 width, int32 height, LTRB& Rect);
};

#endif
