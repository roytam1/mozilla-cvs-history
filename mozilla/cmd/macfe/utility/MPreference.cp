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

#include "MPreference.h"

#include "PascalString.h"

#include <LControl.h>
#include <LGAPopup.h>
#include <LGACaption.h>

#include "xp_mcom.h"
#include "prefapi.h"
#include "macutil.h" // for StringParamText
#include "prefwutil.h" // for CColorButton

#include "StSetBroadcasting.h"
#include "CTooltipAttachment.h"

Boolean MPreferenceBase::sWriteOnDestroy = false; 		// must be one for all instantiations of the template.
Boolean MPreferenceBase::sUseTempPrefPrefix = false; 	// must be one for all instantiations of the template.
char* 	MPreferenceBase::sReplacementString = nil;

const char *	kTempPrefPrefix = "temp_pref_mac";		// prepended to the prefs string

//========================================================================================
class CDebugPrefToolTipAttachment : public CToolTipAttachment
//========================================================================================
{
	public:
		enum { class_ID = 'X%W@' };

							CDebugPrefToolTipAttachment(MPreferenceBase* b);
	protected:
		virtual void		CalcTipText(
									LWindow*				inOwningWindow,
									LPane*					inOwningPane,
									const EventRecord&		inMacEvent,
									StringPtr				outTipText);
	MPreferenceBase*		mPreferenceBase;
}; // class CDebugPrefToolTipAttachment

//----------------------------------------------------------------------------------------
CDebugPrefToolTipAttachment::CDebugPrefToolTipAttachment(MPreferenceBase* b)
//----------------------------------------------------------------------------------------
	:	CToolTipAttachment(60, 11507)
	,	mPreferenceBase(b)
{
}

//----------------------------------------------------------------------------------------
void CDebugPrefToolTipAttachment::CalcTipText(
//----------------------------------------------------------------------------------------
	LWindow*	/* inOwningWindow */,
	LPane*		/* inOwningPane */,
	const EventRecord&	/* inMacEvent */,
	StringPtr	outTipText)
{
	*(CStr255*)outTipText = mPreferenceBase->mName;
}

#pragma mark -
//----------------------------------------------------------------------------------------
MPreferenceBase::MPreferenceBase(
	LPane*	inPane
,	LStream* inStream)
//----------------------------------------------------------------------------------------
:	mName(nil)
,	mSubstitutedName(nil)
,	mLocked(false)
,	mWritePref(true)
,	mPaneSelf(inPane)
{
	CStr255 text;
	inStream->ReadPString(text);
	SetPrefName((const char*)text, false);
	*inStream >> mOrdinal;
} // MPreferenceBase::MPreferenceBase

//----------------------------------------------------------------------------------------
MPreferenceBase::~MPreferenceBase()
//----------------------------------------------------------------------------------------
{
	XP_FREEIF(const_cast<char*>(mSubstitutedName));
	mSubstitutedName = nil;
	XP_FREEIF(const_cast<char*>(mName));
	mName = nil;
} // MPreferenceBase::~MPreferenceBase

//----------------------------------------------------------------------------------------
void MPreferenceBase::SetPrefName(const char* inNewName, Boolean inReread)
//----------------------------------------------------------------------------------------
{
	/*
	const char* oldName = mName; // so that inNewName == mName works
	CStr255 	text(inNewName);
	if (sReplacementString && *sReplacementString)
		::StringParamText(text, sReplacementString);
	
	if (sUseTempPrefPrefix)
	{
		mName = XP_STRDUP(kTempPrefPrefix);
		StrAllocCat(mName, text);
	}
	else
	mName = XP_STRDUP((const char*)text);
	*/
	XP_FREEIF(const_cast<char *>(mName));
	mName = XP_STRDUP(inNewName);
	
	if (inReread)
	{
		ReadLockState();
		ReadSelf();
	}
} // MPreferenceBase::ReadLockState

//----------------------------------------------------------------------------------------
/*static*/ void MPreferenceBase::SetReplacementString(const char* s)
//----------------------------------------------------------------------------------------
{
	XP_FREEIF(sReplacementString);
	sReplacementString = (s != nil) ? XP_STRDUP(s) : nil;
}

//----------------------------------------------------------------------------------------
/*static*/ void MPreferenceBase::ChangePrefName(LView* inSuperView, PaneIDT inPaneID, const char* inNewName)
//----------------------------------------------------------------------------------------
{
	LPane* p = inSuperView->FindPaneByID(inPaneID);
	MPreferenceBase* pb = dynamic_cast<MPreferenceBase*>(p);
	SignalIf_(!pb);
	if (pb)
		pb->SetPrefName(inNewName);
}

//----------------------------------------------------------------------------------------
/*static*/ void MPreferenceBase::SetPaneWritePref(LView* inSuperView, PaneIDT inPaneID, Boolean inWritePref)
//----------------------------------------------------------------------------------------
{
	LPane* p = inSuperView->FindPaneByID(inPaneID);
	MPreferenceBase* pb = dynamic_cast<MPreferenceBase*>(p);
	SignalIf_(!pb);
	if (pb)
		pb->SetWritePref(inWritePref);
}

//----------------------------------------------------------------------------------------
/*static*/ const char* MPreferenceBase::GetPanePrefName(LView* inSuperView, PaneIDT inPaneID)
//----------------------------------------------------------------------------------------
{
	LPane* p = inSuperView->FindPaneByID(inPaneID);
	MPreferenceBase* pb = dynamic_cast<MPreferenceBase*>(p);
	SignalIf_(!pb);
	if (pb)
		return XP_STRDUP(pb->GetPrefName());
	return nil;
}

//----------------------------------------------------------------------------------------
/*static*/ const char* MPreferenceBase::GetPaneUnsubstitutedPrefName(LView* inSuperView, PaneIDT inPaneID)
//----------------------------------------------------------------------------------------
{
	LPane* p = inSuperView->FindPaneByID(inPaneID);
	MPreferenceBase* pb = dynamic_cast<MPreferenceBase*>(p);
	SignalIf_(!pb);
	if (pb)
		return XP_STRDUP(pb->mName);
	return nil;
}

//----------------------------------------------------------------------------------------
const char* MPreferenceBase::GetPrefName()
//----------------------------------------------------------------------------------------
{
	CStr255 	subName(mName);
	
	if (sReplacementString && *sReplacementString)
		::StringParamText(subName, sReplacementString);
	
	XP_FREEIF(const_cast<char *>(mSubstitutedName));
	
	if (sUseTempPrefPrefix)
	{
		CStr255		newName(kTempPrefPrefix);
		newName += ".";
		newName += subName;
		
		mSubstitutedName = XP_STRDUP(newName);
	}
	else
		mSubstitutedName = XP_STRDUP(subName);
	
	return mSubstitutedName;
}

//----------------------------------------------------------------------------------------
const char* MPreferenceBase::GetValidPrefName()
// Get the pref name, with or without the prefix depending on whether the prefixed
// preference exists or not.
//----------------------------------------------------------------------------------------
{
	CStr255 	subName(mName);
	
	if (sReplacementString && *sReplacementString)
		::StringParamText(subName, sReplacementString);
	
	XP_FREEIF(const_cast<char *>(mSubstitutedName));
	
	if (sUseTempPrefPrefix)
	{
		CStr255		newName(kTempPrefPrefix);
		newName += ".";
		newName += subName;
				
		if (PREF_GetPrefType(newName) == PREF_ERROR)
			mSubstitutedName = XP_STRDUP(subName);	// the prefixed pref does not exist.
		else
			mSubstitutedName = XP_STRDUP(newName);
	}
	else
		mSubstitutedName = XP_STRDUP(subName);
	
	return mSubstitutedName;
}


//----------------------------------------------------------------------------------------
const char* MPreferenceBase::GetUnsubstitutedPrefName()
//----------------------------------------------------------------------------------------
{
	return XP_STRDUP(mName);
}

//----------------------------------------------------------------------------------------
void MPreferenceBase::ReadLockState()
//----------------------------------------------------------------------------------------
{
	mLocked = PREF_PrefIsLocked(GetPrefName());
	if (mLocked)
		mPaneSelf->Disable();
} // MPreferenceBase::ReadLockState

//----------------------------------------------------------------------------------------
void MPreferenceBase::FinishCreate()
//----------------------------------------------------------------------------------------
{
	ReadLockState();
	ReadSelf();
#ifdef DEBUG
	LAttachable::SetDefaultAttachable(mPaneSelf);
	CDebugPrefToolTipAttachment* a = new CDebugPrefToolTipAttachment(this);
	mPaneSelf->AddAttachment(a);
#endif
} // MPreferenceBase::FinishCreate


//----------------------------------------------------------------------------------------
Boolean MPreferenceBase::ShouldWrite() const
//----------------------------------------------------------------------------------------
{
	if (!sWriteOnDestroy || mLocked || !mWritePref)
		return false;

/*
	if (strstr(mName, "^0") != nil) // yow! unreplaced strings
	{
		// Check if a replacement has become possible
		Assert_(sReplacementString && *sReplacementString);
		if (!sReplacementString || !*sReplacementString)
			return false;
		const_cast<MPreferenceBase*>(this)->SetPrefName(mName, false); // don't read
	}
*/
	return true;
		// Note: don't worry about testing Changed(), since preflib does that.
} // MPreferenceBase::ShouldWrite


//----------------------------------------------------------------------------------------
/*static*/ void MPreferenceBase::InitTempPrefCache()
//----------------------------------------------------------------------------------------
{
	// delete the temp pref tree
	PREF_DeleteBranch(kTempPrefPrefix);
}

//----------------------------------------------------------------------------------------
/*static*/ void MPreferenceBase::CopyCachedPrefsToMainPrefs()
//----------------------------------------------------------------------------------------
{
	int result;
	
	result = PREF_CopyPrefsTree(kTempPrefPrefix, "");
	Assert_(result == PREF_NOERROR);
	
	result = PREF_DeleteBranch(kTempPrefPrefix);
	Assert_(result == PREF_NOERROR);
}

 
#pragma mark -
//----------------------------------------------------------------------------------------
template <class TPane, class TData> MPreference<TPane,TData>::MPreference(
						LPane* inPane,
						LStream* inStream)
//----------------------------------------------------------------------------------------
:	MPreferenceBase(inPane, inStream)
{
} // MPreference::MPreference

//----------------------------------------------------------------------------------------
template <class TPane, class TData> MPreference<TPane,TData>::~MPreference()
//----------------------------------------------------------------------------------------
{
} // MPreference::~MPreference

#pragma mark -

enum // what the ordinal means in this case:
{
	kOrdinalXORBit = 1<<0
,	kOrdinalIntBit = 1<<1
};

//----------------------------------------------------------------------------------------
PRBool MPreference<LControl,PRBool>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	return ((LControl*)mPaneSelf)->GetValue();
} // MPreference<LControl,PRBool>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LControl,PRBool>::SetPaneValue(PRBool inData)
//----------------------------------------------------------------------------------------
{
	((LControl*)mPaneSelf)->SetValue(inData);
} // MPreference<LControl,PRBool>::SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LControl, PRBool>::Changed() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue() != mInitialControlValue;
} // MPreference<LControl,PRBool>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LControl,PRBool>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	PRBool value;
	int	prefResult;
	if (mOrdinal & kOrdinalIntBit)
	{
		int32 intValue;
		typedef int	(*IntPrefReadFunc)(const char*, int32*);
		prefResult = ((IntPrefReadFunc)inFunc)(GetValidPrefName(), &intValue);
		value = intValue;
	}
	else
		prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
		SetPaneValue(value ^ (mOrdinal & kOrdinalXORBit));
} // MPreference<LControl,PRBool>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LControl,PRBool>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	if (mOrdinal & kOrdinalIntBit) // this bit indicates it's an int conversion
		InitializeUsing((PrefReadFunc)PREF_GetIntPref);
	else
		InitializeUsing(PREF_GetBoolPref);
	mInitialControlValue = GetPaneValue();
} // MPreference<LControl,PRBool>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LControl,PRBool>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		if (mOrdinal & kOrdinalIntBit) // this bit indicates it's an int conversion
			InitializeUsing((PrefReadFunc)PREF_GetDefaultIntPref);
		else
			InitializeUsing(PREF_GetDefaultBoolPref);
} // MPreference<LControl,PRBool>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LControl,PRBool>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
	{
		if (mOrdinal & kOrdinalIntBit) // this bit indicates it's an int conversion
			PREF_SetIntPref(GetPrefName(), GetPrefValue());
		else
			PREF_SetBoolPref(GetPrefName(), GetPrefValue());
	}
} // MPreference<LControl,PRBool>::WriteSelf

//----------------------------------------------------------------------------------------
PRBool MPreference<LControl,PRBool>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	// xor the boolean value with the low-order bit of the ordinal
	return (PRBool)(mPaneSelf->GetValue() ^ (mOrdinal & kOrdinalXORBit));
} // MPreference<LControl,PRBool>::GetPrefValue

template class MPreference<LControl,PRBool>;

#pragma mark -

// Why the heck would we want a prefcontrol that is just a caption?  Only for the use of the
// resource template to supply an extra string which initially holds the pref name.
// CSpecialFolderCaption is derived from this.

//----------------------------------------------------------------------------------------
PRBool MPreference<LGACaption,PRBool>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	return false;
} // MPreference<LGACaption,PRBool>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LGACaption,PRBool>::SetPaneValue(PRBool)
//----------------------------------------------------------------------------------------
{
} // MPreference<LGACaption,PRBool>::SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LGACaption, PRBool>::Changed() const
//----------------------------------------------------------------------------------------
{
	return false;
} // MPreference<LGACaption,PRBool>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LGACaption,PRBool>::InitializeUsing(PrefReadFunc)
//----------------------------------------------------------------------------------------
{
} // MPreference<LGACaption,PRBool>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LGACaption,PRBool>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_GetBoolPref);
	mInitialControlValue = false;
} // MPreference<LGACaption,PRBool>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LGACaption,PRBool>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_GetDefaultBoolPref);
} // MPreference<LGACaption,PRBool>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LGACaption,PRBool>::WriteSelf()
//----------------------------------------------------------------------------------------
{
} // MPreference<LGACaption,PRBool>::WriteSelf

//----------------------------------------------------------------------------------------
PRBool MPreference<LGACaption,PRBool>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return false;
} // MPreference<LGACaption,PRBool>::GetPrefValue

template class MPreference<LGACaption,PRBool>;

#pragma mark -

//----------------------------------------------------------------------------------------
int32 MPreference<LControl,int32>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	return ((LControl*)mPaneSelf)->GetValue();
} // MPreference<LControl,int32>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LControl,int32>::SetPaneValue(int32 inData)
//----------------------------------------------------------------------------------------
{
	((LControl*)mPaneSelf)->SetValue(inData);
} // MPreference<LControl,int32>::SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LControl, int32>::Changed() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue() != mInitialControlValue;
} // MPreference<LControl,int32>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LControl,int32>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	int32 value;
	int	prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
	{
		if (value == mOrdinal)
			SetPaneValue(1); // tab group will turn others off.
	}
} // MPreference<LControl,int32>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LControl,int32>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_GetIntPref);
	mInitialControlValue = GetPaneValue();
} // MPreference<LControl,int32>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LControl,int32>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_GetDefaultIntPref);
} // MPreference<LControl,int32>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LControl,int32>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
		PREF_SetIntPref(GetPrefName(), mOrdinal);
} // MPreference<int>::WriteSelf

//----------------------------------------------------------------------------------------
int32 MPreference<LControl,int32>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return mOrdinal;
} // MPreference<int>::GetPrefValue

template class MPreference<LControl,int32>;

#pragma mark -

//----------------------------------------------------------------------------------------
#pragma mark
MPreference<LTextEditView,char*>::~MPreference()
//----------------------------------------------------------------------------------------
{
	XP_FREEIF(mInitialControlValue);
} // MPreference<LTextEditView,char*>::CleanUpData

//----------------------------------------------------------------------------------------
char* MPreference<LTextEditView,char*>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	CStr255 value;
	mPaneSelf->GetDescriptor(value);
	return (char*)value;
} // MPreference<LTextEditView,char*>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,char*>::SetPaneValue(char* inData)
//----------------------------------------------------------------------------------------
{
	((LTextEditView*)mPaneSelf)->SetDescriptor(CStr255(inData));
} // MPreference<LTextEditView,char*>:SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LTextEditView,char*>::Changed() const
//----------------------------------------------------------------------------------------
{
	char* value = GetPaneValue();
	if (value && *value)
		return (strcmp(value, mInitialControlValue) != 0);
	return true;
} // MPreference<LTextEditView,char*>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,char*>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	char* value;
	int	prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
	{
		SetPaneValue(value);
		XP_FREEIF(value);
	}
} // MPreference<LTextEditView,char*>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,char*>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_CopyCharPref);
	mInitialControlValue = XP_STRDUP(GetPaneValue());
} // MPreference<LTextEditView,char*>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,char*>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_CopyDefaultCharPref);
} // MPreference<LTextEditView,char*>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,char*>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
		PREF_SetCharPref(GetPrefName(), GetPaneValue());
} // MPreference<LTextEditView,char*>::WriteSelf

//----------------------------------------------------------------------------------------
char* MPreference<LTextEditView,char*>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue();
} // MPreference<LTextEditView,char*>::GetPrefValue

template class MPreference<LTextEditView,char*>;

// This is used for captions, and for mixing in with another pref control (eg, to
// control the descriptor of a checkbox).

#pragma mark -

//----------------------------------------------------------------------------------------
#pragma mark
MPreference<LPane,char*>::~MPreference()
//----------------------------------------------------------------------------------------
{
	XP_FREEIF(mInitialControlValue);
} // MPreference<LPane,char*>::CleanUpData

//----------------------------------------------------------------------------------------
char* MPreference<LPane,char*>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	CStr255 value;
	mPaneSelf->GetDescriptor(value);
	return (char*)value;
} // MPreference<LPane,char*>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LPane,char*>::SetPaneValue(char* inData)
//----------------------------------------------------------------------------------------
{
	((LPane*)mPaneSelf)->SetDescriptor(CStr255(inData));
} // MPreference<LPane,char*>:SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LPane,char*>::Changed() const
//----------------------------------------------------------------------------------------
{
	char* value = GetPaneValue();
	if (value && *value)
		return (strcmp(value, mInitialControlValue) != 0);
	return true;
} // MPreference<LPane,char*>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LPane,char*>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	char* value;
	int	prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
	{
		SetPaneValue(value);
		XP_FREEIF(value);
	}
} // MPreference<LPane,char*>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LPane,char*>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_CopyCharPref);
	mInitialControlValue = XP_STRDUP(GetPaneValue());
} // MPreference<LPane,char*>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LPane,char*>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_CopyDefaultCharPref);
} // MPreference<LPane,char*>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LPane,char*>::WriteSelf()
//----------------------------------------------------------------------------------------
{
} // MPreference<LPane,char*>::WriteSelf

//----------------------------------------------------------------------------------------
char* MPreference<LPane,char*>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue();
} // MPreference<LPane,char*>::GetPrefValue

template class MPreference<LPane,char*>;

#pragma mark -

//----------------------------------------------------------------------------------------
#pragma mark
MPreference<LGAPopup,char*>::~MPreference()
//----------------------------------------------------------------------------------------
{
	XP_FREEIF(mInitialControlValue);
} // MPreference<LGAPopup,char*>::CleanUpData

//----------------------------------------------------------------------------------------
char* MPreference<LGAPopup,char*>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	Int32 itemNumber = ((LGAPopup*)mPaneSelf)->GetValue();
	CStr255 itemString;
	if (itemNumber > 0)
	{
		MenuHandle menuH = ((LGAPopup*)mPaneSelf)->GetMacMenuH();
		::GetMenuItemText(menuH, itemNumber, itemString);
	}
	return (char*)itemString;
} // MPreference<LGAPopup,char*>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LGAPopup,char*>::SetPaneValue(char* inData)
//----------------------------------------------------------------------------------------
{
	if (!inData || !*inData)
		return;
	MenuHandle menuH = ((LGAPopup*)mPaneSelf)->GetMacMenuH();
	short menuSize = ::CountMItems(menuH);
	CStr255	itemString;
	for (short menuItem = 1; menuItem <= menuSize; ++menuItem)
	{
		::GetMenuItemText(menuH, menuItem, itemString);
		if (itemString == inData)
		{
			StSetBroadcasting dontBroadcast((LGAPopup*)mPaneSelf, false);
			((LGAPopup*)mPaneSelf)->SetValue(menuItem);
			return;
		}
	}
} // MPreference<LGAPopup,char*>:SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LGAPopup,char*>::Changed() const
//----------------------------------------------------------------------------------------
{
	char* value = GetPaneValue();
	if (value && *value)
		return (strcmp(value, mInitialControlValue) != 0);
	return true;
} // MPreference<LGAPopup,char*>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LGAPopup,char*>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	char* value;
	int	prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
	{
		SetPaneValue(value);
		XP_FREEIF(value);
	}
} // MPreference<LGAPopup,char*>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LGAPopup,char*>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_CopyCharPref);
	mInitialControlValue = XP_STRDUP(GetPaneValue());
} // MPreference<LGAPopup,char*>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LGAPopup,char*>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_CopyDefaultCharPref);
} // MPreference<LGAPopup,char*>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LGAPopup,char*>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
		PREF_SetCharPref(GetPrefName(), GetPaneValue());
} // MPreference<LGAPopup,char*>::WriteSelf

//----------------------------------------------------------------------------------------
char* MPreference<LGAPopup,char*>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue();
} // MPreference<LGAPopup,char*>::GetPrefValue

template class MPreference<LGAPopup,char*>;

#pragma mark -

//----------------------------------------------------------------------------------------
MPreference<LTextEditView,int32>::~MPreference()
//----------------------------------------------------------------------------------------
{
} // MPreference<LTextEditView,int32>::CleanUpData

//----------------------------------------------------------------------------------------
int32 MPreference<LTextEditView,int32>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	return ((LTextEditView*)mPaneSelf)->GetValue();
} // MPreference<LTextEditView,int32>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,int32>::SetPaneValue(int32 inData)
//----------------------------------------------------------------------------------------
{
	((LTextEditView*)mPaneSelf)->SetValue(inData);
} // MPreference<LTextEditView,int32>:SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<LTextEditView,int32>::Changed() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue() != mInitialControlValue;
} // MPreference<LTextEditView,int32>::Changed

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,int32>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	int32 value;
	int	prefResult = inFunc(GetValidPrefName(), &value);
	if (prefResult == PREF_NOERROR)
		SetPaneValue(value);
} // MPreference<LTextEditView,int32>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,int32>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing(PREF_GetIntPref);
	mInitialControlValue = GetPaneValue();
} // MPreference<LTextEditView,int32>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,int32>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing(PREF_GetDefaultIntPref);
} // MPreference<LTextEditView,int32>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<LTextEditView,int32>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
		PREF_SetIntPref(GetPrefName(), GetPaneValue());
} // MPreference<LTextEditView,int32>::WriteSelf

//----------------------------------------------------------------------------------------
int32 MPreference<LTextEditView,int32>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue();
} // MPreference<LTextEditView,int32>::GetPrefValue

template class MPreference<LTextEditView,int32>;

#pragma mark -

// The function signature for reading colors is not like the other types.  Here is
// its prototype, which we use to cast back and forth.
typedef int (*ColorReadFunc)(const char*, uint8*, uint8*, uint8*);

//----------------------------------------------------------------------------------------
MPreference<CColorButton,RGBColor>::~MPreference()
//----------------------------------------------------------------------------------------
{
} // MPreference<CColorButton,RGBColor>::CleanUpData

//----------------------------------------------------------------------------------------
RGBColor MPreference<CColorButton,RGBColor>::GetPaneValue() const
//----------------------------------------------------------------------------------------
{
	return ((CColorButton*)mPaneSelf)->GetColor();
} // MPreference<CColorButton,RGBColor>::GetPaneValue

//----------------------------------------------------------------------------------------
void MPreference<CColorButton,RGBColor>::SetPaneValue(RGBColor inData)
//----------------------------------------------------------------------------------------
{
	((CColorButton*)mPaneSelf)->SetColor(inData);
	mPaneSelf->Refresh();
} // MPreference<CColorButton,RGBColor>:SetPaneValue

//----------------------------------------------------------------------------------------
Boolean MPreference<CColorButton,RGBColor>::Changed() const
//----------------------------------------------------------------------------------------
{
	return memcmp(&GetPaneValue(), &mInitialControlValue, sizeof(RGBColor)) == 0;
} // MPreference<CColorButton,RGBColor>::Changed

//----------------------------------------------------------------------------------------
void MPreference<CColorButton,RGBColor>::InitializeUsing(PrefReadFunc inFunc)
//----------------------------------------------------------------------------------------
{
	RGBColor value;
	uint8 red = 0, green = 0, blue = 0;
	int	prefResult = ((ColorReadFunc)inFunc)(GetValidPrefName(), &red, &green, &blue);
	if (prefResult == PREF_NOERROR)
	{
		value.red = red << 8;
		value.green = green << 8;
		value.blue = blue << 8;
		SetPaneValue(value);
	}
} // MPreference<CColorButton,RGBColor>::InitializeUsing

//----------------------------------------------------------------------------------------
void MPreference<CColorButton,RGBColor>::ReadSelf()
//----------------------------------------------------------------------------------------
{
	InitializeUsing((PrefReadFunc)PREF_GetColorPref);
	mInitialControlValue = GetPaneValue();
} // MPreference<CColorButton,RGBColor>::ReadSelf

//----------------------------------------------------------------------------------------
void MPreference<CColorButton,RGBColor>::ReadDefaultSelf()
//----------------------------------------------------------------------------------------
{
	if (!IsLocked())
		InitializeUsing((PrefReadFunc)PREF_GetDefaultColorPref);
} // MPreference<CColorButton,RGBColor>::ReadDefaultSelf

//----------------------------------------------------------------------------------------
void MPreference<CColorButton,RGBColor>::WriteSelf()
//----------------------------------------------------------------------------------------
{
	if (ShouldWrite())
	{
		RGBColor value = GetPaneValue();
		PREF_SetColorPref(GetPrefName(), value.red >> 8, value.green >> 8, value.blue >> 8);
	}
} // MPreference<CColorButton,RGBColor>::WriteSelf

//----------------------------------------------------------------------------------------
RGBColor MPreference<CColorButton,RGBColor>::GetPrefValue() const
//----------------------------------------------------------------------------------------
{
	return GetPaneValue();
} // MPreference<CColorButton,RGBColor>::GetPrefValue

template class MPreference<CColorButton,RGBColor>;

