/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIXBLPrototypeBinding.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIDOMEventReceiver.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIXMLContent.h"
#include "nsIXULContent.h"
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsSupportsArray.h"
#include "nsINameSpace.h"
#include "nsXBLService.h"
#include "nsXBLBinding.h"
#include "nsIXBLInsertionPoint.h"
#include "nsXBLPrototypeBinding.h"
#include "nsFixedSizeAllocator.h"
#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsXBLAtoms.h"
#include "nsXBLProtoImpl.h"
#include "nsCRT.h"

#include "nsIScriptContext.h"

#include "nsICSSLoader.h"
#include "nsIStyleRuleProcessor.h"

// Helper Classes =====================================================================

// nsIXBLAttributeEntry and helpers.  This class is used to efficiently handle
// attribute changes in anonymous content.

// {A2892B81-CED9-11d3-97FB-00400553EEF0}
#define NS_IXBLATTR_IID \
{ 0xa2892b81, 0xced9, 0x11d3, { 0x97, 0xfb, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

class nsIXBLAttributeEntry : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXBLATTR_IID)

  NS_IMETHOD GetSrcAttribute(nsIAtom** aResult) = 0;
  NS_IMETHOD GetDstAttribute(nsIAtom** aResult) = 0;
  NS_IMETHOD GetElement(nsIContent** aResult) = 0;
  NS_IMETHOD GetNext(nsIXBLAttributeEntry** aResult) = 0;
  NS_IMETHOD SetNext(nsIXBLAttributeEntry* aEntry) = 0;
};
  
class nsXBLAttributeEntry : public nsIXBLAttributeEntry {
public:
  NS_IMETHOD GetSrcAttribute(nsIAtom** aResult) { *aResult = mSrcAttribute; NS_IF_ADDREF(*aResult); return NS_OK; };
  NS_IMETHOD GetDstAttribute(nsIAtom** aResult) { *aResult = mDstAttribute; NS_IF_ADDREF(*aResult); return NS_OK; };
  
  NS_IMETHOD GetElement(nsIContent** aResult) { *aResult = mElement; NS_IF_ADDREF(*aResult); return NS_OK; };

  NS_IMETHOD GetNext(nsIXBLAttributeEntry** aResult) { NS_IF_ADDREF(*aResult = mNext); return NS_OK; }
  NS_IMETHOD SetNext(nsIXBLAttributeEntry* aEntry) { mNext = aEntry; return NS_OK; }

  nsIContent* mElement;

  static nsXBLAttributeEntry*
  Create(nsIAtom* aSrcAtom, nsIAtom* aDstAtom, nsIContent* aContent) {
    void* place = nsXBLPrototypeBinding::kAttrPool->Alloc(sizeof(nsXBLAttributeEntry));
    return place ? ::new (place) nsXBLAttributeEntry(aSrcAtom, aDstAtom, aContent) : nsnull;
  }

  static void
  Destroy(nsXBLAttributeEntry* aSelf) {
    aSelf->~nsXBLAttributeEntry();
    nsXBLPrototypeBinding::kAttrPool->Free(aSelf, sizeof(*aSelf));
  }

  nsCOMPtr<nsIAtom> mSrcAttribute;
  nsCOMPtr<nsIAtom> mDstAttribute;
  nsCOMPtr<nsIXBLAttributeEntry> mNext;

  NS_DECL_ISUPPORTS

protected:
  nsXBLAttributeEntry(nsIAtom* aSrcAtom, nsIAtom* aDstAtom, nsIContent* aContent) {
    NS_INIT_ISUPPORTS(); mSrcAttribute = aSrcAtom; mDstAttribute = aDstAtom; mElement = aContent;
  }

  virtual ~nsXBLAttributeEntry() {}

private:
  // Hide so that only Create() and Destroy() can be used to
  // allocate and deallocate from the heap
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
};

NS_IMPL_ADDREF(nsXBLAttributeEntry)
NS_IMPL_RELEASE_WITH_DESTROY(nsXBLAttributeEntry, nsXBLAttributeEntry::Destroy(this))
NS_IMPL_QUERY_INTERFACE1(nsXBLAttributeEntry, nsIXBLAttributeEntry)

// nsIXBLInsertionPointEntry and helpers.  This class stores all the necessary
// info to figure out the position of an insertion point.

// {76F238AE-5ACB-49e6-B2DE-FD1940637753}
#define NS_IXBLINS_IID \
{ 0x76f238ae, 0x5acb, 0x49e6, { 0xb2, 0xde, 0xfd, 0x19, 0x40, 0x63, 0x77, 0x53 } }

class nsIXBLInsertionPointEntry : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXBLINS_IID)

  NS_IMETHOD GetInsertionParent(nsIContent** aResult)=0;

  NS_IMETHOD GetInsertionIndex(PRUint32* aResult)=0;
  NS_IMETHOD SetInsertionIndex(PRUint32 aIndex)=0;

  NS_IMETHOD GetDefaultContent(nsIContent** aResult)=0;
  NS_IMETHOD SetDefaultContent(nsIContent* aChildren)=0;
};
  
class nsXBLInsertionPointEntry : public nsIXBLInsertionPointEntry {
public:
  NS_IMETHOD GetInsertionParent(nsIContent** aResult)
  {
    *aResult = mInsertionParent;
    NS_IF_ADDREF(*aResult);
    return NS_OK;
  };
    
  NS_IMETHOD GetInsertionIndex(PRUint32* aResult) { *aResult = mInsertionIndex; return NS_OK; };
  NS_IMETHOD SetInsertionIndex(PRUint32 aIndex) { mInsertionIndex = aIndex;  return NS_OK; };

  NS_IMETHOD GetDefaultContent(nsIContent** aResult) { *aResult = mDefaultContent; NS_IF_ADDREF(*aResult); return NS_OK; };
  NS_IMETHOD SetDefaultContent(nsIContent* aChildren) { mDefaultContent = aChildren; return NS_OK; };

  nsCOMPtr<nsIContent> mInsertionParent;
  nsCOMPtr<nsIContent> mDefaultContent;
  PRUint32 mInsertionIndex;

  static nsXBLInsertionPointEntry*
  Create(nsIContent* aParent) {
    void* place = nsXBLPrototypeBinding::kInsPool->Alloc(sizeof(nsXBLInsertionPointEntry));
    return place ? ::new (place) nsXBLInsertionPointEntry(aParent) : nsnull;
  }

  static void
  Destroy(nsXBLInsertionPointEntry* aSelf) {
    aSelf->~nsXBLInsertionPointEntry();
    nsXBLPrototypeBinding::kInsPool->Free(aSelf, sizeof(*aSelf));
  }
  
  NS_DECL_ISUPPORTS

protected:
  nsXBLInsertionPointEntry(nsIContent* aParent) {
    NS_INIT_ISUPPORTS();
    mInsertionIndex = 0;
    mInsertionParent = aParent;
  };

  virtual ~nsXBLInsertionPointEntry() {}

private:
  // Hide so that only Create() and Destroy() can be used to
  // allocate and deallocate from the heap
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
};

NS_IMPL_ADDREF(nsXBLInsertionPointEntry)
NS_IMPL_RELEASE_WITH_DESTROY(nsXBLInsertionPointEntry, nsXBLInsertionPointEntry::Destroy(this))
NS_IMPL_QUERY_INTERFACE1(nsXBLInsertionPointEntry, nsIXBLInsertionPointEntry)

// =============================================================================

// Static initialization
PRUint32 nsXBLPrototypeBinding::gRefCnt = 0;

nsFixedSizeAllocator* nsXBLPrototypeBinding::kAttrPool;
nsFixedSizeAllocator* nsXBLPrototypeBinding::kInsPool;

static const PRInt32 kNumElements = 128;

static const size_t kAttrBucketSizes[] = {
  sizeof(nsXBLAttributeEntry)
};

static const PRInt32 kAttrNumBuckets = sizeof(kAttrBucketSizes)/sizeof(size_t);
static const PRInt32 kAttrInitialSize = (NS_SIZE_IN_HEAP(sizeof(nsXBLAttributeEntry))) * kNumElements;

static const size_t kInsBucketSizes[] = {
  sizeof(nsXBLInsertionPointEntry)
};

static const PRInt32 kInsNumBuckets = sizeof(kInsBucketSizes)/sizeof(size_t);
static const PRInt32 kInsInitialSize = (NS_SIZE_IN_HEAP(sizeof(nsXBLInsertionPointEntry))) * kNumElements;

// Implementation /////////////////////////////////////////////////////////////////

// Implement our nsISupports methods
NS_IMPL_ISUPPORTS2(nsXBLPrototypeBinding, nsIXBLPrototypeBinding, nsISupportsWeakReference)

// Constructors/Destructors
nsXBLPrototypeBinding::nsXBLPrototypeBinding(const nsACString& aID, nsIXBLDocumentInfo* aInfo,
                                             nsIContent* aElement)
: mImplementation(nsnull),
  mInheritStyle(PR_TRUE), 
  mHasBaseProto(PR_TRUE),
  mResources(nsnull),
  mAttributeTable(nsnull), 
  mInsertionPointTable(nsnull),
  mInterfaceTable(nsnull)
{
  NS_INIT_ISUPPORTS();
  
  mID = ToNewCString(aID);

  mXBLDocInfoWeak = getter_AddRefs(NS_GetWeakReference(aInfo));
  
  gRefCnt++;
  //  printf("REF COUNT UP: %d %s\n", gRefCnt, (const char*)mID);

  if (gRefCnt == 1) {
    kAttrPool = new nsFixedSizeAllocator();
    kAttrPool->Init("XBL Attribute Entries", kAttrBucketSizes, kAttrNumBuckets, kAttrInitialSize);
    kInsPool = new nsFixedSizeAllocator();
    kInsPool->Init("XBL Insertion Point Entries", kInsBucketSizes, kInsNumBuckets, kInsInitialSize);
  }

  SetBindingElement(aElement);
}

NS_IMETHODIMP
nsXBLPrototypeBinding::Initialize()
{
  nsCOMPtr<nsIContent> content;
  GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(content));
  if (content) {
    ConstructAttributeTable(content);
    ConstructInsertionTable(content);
  }

  return NS_OK;
}

nsXBLPrototypeBinding::~nsXBLPrototypeBinding(void)
{
  nsMemory::Free(mID);
  delete mResources;
  delete mAttributeTable;
  delete mInsertionPointTable;
  delete mInterfaceTable;
  delete mImplementation;
  gRefCnt--;
  if (gRefCnt == 0) {
    delete kAttrPool;
    delete kInsPool;
  }
}

// nsIXBLPrototypeBinding Interface ////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsXBLPrototypeBinding::GetBasePrototype(nsIXBLPrototypeBinding** aResult)
{
  *aResult = mBaseBinding;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetBasePrototype(nsIXBLPrototypeBinding* aBinding)
{
  if (mBaseBinding.get() == aBinding)
    return NS_OK;

  if (mBaseBinding) {
    NS_ERROR("Base XBL prototype binding is already defined!");
    return NS_OK;
  }

  mBaseBinding = aBinding; // Comptr handles rel/add
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetBindingElement(nsIContent** aResult)
{
  *aResult = mBinding;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetBindingElement(nsIContent* aElement)
{
  mBinding = aElement;
  nsAutoString inheritStyle;
  mBinding->GetAttr(kNameSpaceID_None, nsXBLAtoms::inheritstyle, inheritStyle);
  if (inheritStyle == NS_LITERAL_STRING("false"))
    mInheritStyle = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP 
nsXBLPrototypeBinding::GetBindingURI(nsCString& aResult)
{
  nsCOMPtr<nsIXBLDocumentInfo> info;
  GetXBLDocumentInfo(nsnull, getter_AddRefs(info));
  
  NS_ASSERTION(info, "The prototype binding has somehow lost its XBLDocInfo! Bad bad bad!!!\n");
  if (!info)
    return NS_ERROR_FAILURE;

  info->GetDocumentURI(aResult);
  aResult += "#";
  aResult += mID;
  return NS_OK;
}

NS_IMETHODIMP 
nsXBLPrototypeBinding::GetDocURI(nsCString& aResult)
{
  nsCOMPtr<nsIXBLDocumentInfo> info;
  GetXBLDocumentInfo(nsnull, getter_AddRefs(info));
  
  NS_ASSERTION(info, "The prototype binding has somehow lost its XBLDocInfo! Bad bad bad!!!\n");
  if (!info)
    return NS_ERROR_FAILURE;

  info->GetDocumentURI(aResult);
  return NS_OK;
}

NS_IMETHODIMP 
nsXBLPrototypeBinding::GetID(nsCString& aResult)
{
  aResult = mID;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetAllowScripts(PRBool* aResult)
{
  nsCOMPtr<nsIXBLDocumentInfo> info;
  GetXBLDocumentInfo(nsnull, getter_AddRefs(info));
  return info->GetScriptAccess(aResult);
}

NS_IMETHODIMP
nsXBLPrototypeBinding::LoadResources(PRBool* aResult)
{
  if (mResources)
    mResources->LoadResources(aResult);
  else
    *aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::AddResource(nsIAtom* aResourceType, const nsAString& aSrc)
{
  if (!mResources) {
    mResources = new nsXBLPrototypeResources(this);
    if (!mResources)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mResources->AddResource(aResourceType, aSrc);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::FlushSkinSheets()
{
  if (mResources)
    return mResources->FlushSkinSheets();
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::BindingAttached(nsIDOMEventReceiver* aReceiver)
{
  if (mImplementation && mImplementation->mConstructor)
    return mImplementation->mConstructor->BindingAttached(aReceiver);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::BindingDetached(nsIDOMEventReceiver* aReceiver)
{
  if (mImplementation && mImplementation->mDestructor)
    return mImplementation->mDestructor->BindingDetached(aReceiver);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::InheritsStyle(PRBool* aResult)
{
  *aResult = mInheritStyle;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetXBLDocumentInfo(nsIContent* aBoundElement, nsIXBLDocumentInfo** aResult)
{
  nsCOMPtr<nsIXBLDocumentInfo> info(do_QueryReferent(mXBLDocInfoWeak));
  if (info) {
    *aResult = info;
    NS_ADDREF(*aResult);
    return NS_OK;
  }
  else *aResult=nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::HasBasePrototype(PRBool* aResult)
{
  *aResult = mHasBaseProto;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetHasBasePrototype(PRBool aHasBase)
{
  mHasBaseProto = aHasBase;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetPrototypeHandlers(nsIXBLPrototypeHandler** aResult)
{
  *aResult = mPrototypeHandler;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetPrototypeHandlers(nsIXBLPrototypeHandler* aHandler)
{
  mPrototypeHandler = aHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetConstructor(nsIXBLPrototypeHandler** aResult)
{
  if (mImplementation && mImplementation->mConstructor) {
    *aResult = mImplementation->mConstructor; 
    NS_ADDREF(*aResult);
  }
  else
    *aResult = nsnull;
  return NS_OK; 
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetDestructor(nsIXBLPrototypeHandler** aResult)
{
  if (mImplementation && mImplementation->mDestructor) {
    *aResult = mImplementation->mDestructor; 
    NS_ADDREF(*aResult);
  }
  else
    *aResult = nsnull;
  return NS_OK; 
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetConstructor(nsIXBLPrototypeHandler* aHandler)
{
  if (!mImplementation)
    return NS_ERROR_FAILURE;
  mImplementation->mConstructor = aHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetDestructor(nsIXBLPrototypeHandler* aHandler)
{
  if (!mImplementation)
    return NS_ERROR_FAILURE;
  mImplementation->mDestructor = aHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::InstallImplementation(nsIContent* aBoundElement)
{
  if (mImplementation)
    return mImplementation->InstallImplementation(this, aBoundElement);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::AttributeChanged(nsIAtom* aAttribute,
                                        PRInt32 aNameSpaceID,
                                        PRBool aRemoveFlag, 
                                        nsIContent* aChangedElement,
                                        nsIContent* aAnonymousContent,
                                        PRBool aNotify)
{
  if (!mAttributeTable)
    return NS_OK;

  nsISupportsKey key(aAttribute);
  nsCOMPtr<nsISupports> supports = getter_AddRefs(NS_STATIC_CAST(nsISupports*, 
                                                                 mAttributeTable->Get(&key)));

  nsCOMPtr<nsIXBLAttributeEntry> xblAttr = do_QueryInterface(supports);
  if (!xblAttr)
    return NS_OK;

  // Iterate over the elements in the array.
  nsCOMPtr<nsIContent> content;
  GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(content));
  while (xblAttr) {
    nsCOMPtr<nsIContent> element;
    nsCOMPtr<nsIAtom> dstAttr;
    xblAttr->GetElement(getter_AddRefs(element));

    nsCOMPtr<nsIContent> realElement;
    LocateInstance(aChangedElement, content, aAnonymousContent, element, getter_AddRefs(realElement));

    if (realElement) {
      xblAttr->GetDstAttribute(getter_AddRefs(dstAttr));

      if (aRemoveFlag)
        realElement->UnsetAttr(aNameSpaceID, dstAttr, aNotify);
      else {
        PRBool attrPresent = PR_TRUE;
        nsAutoString value;
        // Check to see if the src attribute is xbl:text.  If so, then we need to obtain the 
        // children of the real element and get the text nodes' values.
        if (aAttribute == nsXBLAtoms::xbltext) {
          nsXBLBinding::GetTextData(aChangedElement, value);
          value.StripChar(PRUnichar('\n'));
          value.StripChar(PRUnichar('\r'));
          nsAutoString stripVal(value);
          stripVal.StripWhitespace();
          if (stripVal.IsEmpty()) 
            attrPresent = PR_FALSE;
        }    
        else {
          nsresult result = aChangedElement->GetAttr(aNameSpaceID, aAttribute, value);
          attrPresent = (result == NS_CONTENT_ATTR_NO_VALUE ||
                         result == NS_CONTENT_ATTR_HAS_VALUE);
        }

        if (attrPresent)
          realElement->SetAttr(aNameSpaceID, dstAttr, value, aNotify);
      }

      // See if we're the <html> tag in XUL, and see if value is being
      // set or unset on us.  We may also be a tag that is having
      // xbl:text set on us.
      nsCOMPtr<nsIAtom> tag;
      realElement->GetTag(*getter_AddRefs(tag));
      if (dstAttr == nsXBLAtoms::xbltext || (tag == nsHTMLAtoms::html) && (dstAttr == nsHTMLAtoms::value)) {
        // Flush out all our kids.
        PRInt32 childCount;
        realElement->ChildCount(childCount);
        for (PRInt32 i = 0; i < childCount; i++)
          realElement->RemoveChildAt(0, aNotify);
      
        if (!aRemoveFlag) {
          // Construct a new text node and insert it.
          nsAutoString value;
          aChangedElement->GetAttr(aNameSpaceID, aAttribute, value);
          if (!value.IsEmpty()) {
            nsCOMPtr<nsIDOMText> textNode;
            nsCOMPtr<nsIDocument> doc;
            aChangedElement->GetDocument(*getter_AddRefs(doc));
            nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
            domDoc->CreateTextNode(value, getter_AddRefs(textNode));
            nsCOMPtr<nsIDOMNode> dummy;
            nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(realElement));
            domElement->AppendChild(textNode, getter_AddRefs(dummy));
          }
        }
      }
    }

    nsCOMPtr<nsIXBLAttributeEntry> tmpAttr = xblAttr;
    tmpAttr->GetNext(getter_AddRefs(xblAttr));
  }

  return NS_OK;
}

struct InsertionData {
  nsIXBLBinding* mBinding;
  nsXBLPrototypeBinding* mPrototype;

  InsertionData(nsIXBLBinding* aBinding,
                nsXBLPrototypeBinding* aPrototype) 
    :mBinding(aBinding), mPrototype(aPrototype) {};
};

PRBool PR_CALLBACK InstantiateInsertionPoint(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsIXBLInsertionPointEntry* entry = (nsIXBLInsertionPointEntry*)aData;
  InsertionData* data = (InsertionData*)aClosure;
  nsIXBLBinding* binding = data->mBinding;
  nsXBLPrototypeBinding* proto = data->mPrototype;

  // Get the insertion parent.
  nsCOMPtr<nsIContent> content;
  entry->GetInsertionParent(getter_AddRefs(content));
  PRUint32 index;
  entry->GetInsertionIndex(&index);
  nsCOMPtr<nsIContent> defContent;
  entry->GetDefaultContent(getter_AddRefs(defContent));
    
  // Locate the real content.
  nsCOMPtr<nsIContent> realContent;
  nsCOMPtr<nsIContent> instanceRoot;
  binding->GetAnonymousContent(getter_AddRefs(instanceRoot));
  nsCOMPtr<nsIContent> templRoot;
  proto->GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(templRoot));
  proto->LocateInstance(nsnull, templRoot, instanceRoot, content, getter_AddRefs(realContent));
  if (!realContent)
    binding->GetBoundElement(getter_AddRefs(realContent));

  // Now that we have the real content, look it up in our table.
  nsCOMPtr<nsISupportsArray> points;
  binding->GetInsertionPointsFor(realContent, getter_AddRefs(points));
  nsCOMPtr<nsIXBLInsertionPoint> insertionPoint;
  PRUint32 count;
  points->Count(&count);
  PRUint32 i = 0;
  PRInt32 currIndex = 0;  
  
  for ( ; i < count; i++) {
    nsCOMPtr<nsIXBLInsertionPoint> currPoint = getter_AddRefs((nsIXBLInsertionPoint*)points->ElementAt(i));
    currPoint->GetInsertionIndex(&currIndex);
    if (currIndex == (PRInt32)index) {
      // This is a match. Break out of the loop and set our variable.
      insertionPoint = currPoint;
      break;
    }
    
    if (currIndex > (PRInt32)index)
      // There was no match. Break.
      break;
  }

  if (!insertionPoint) {
    // We need to make a new insertion point.
    NS_NewXBLInsertionPoint(realContent, index, defContent, getter_AddRefs(insertionPoint));
    points->InsertElementAt(insertionPoint, i);
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::InstantiateInsertionPoints(nsIXBLBinding* aBinding)
{
  InsertionData data(aBinding, this);
  if (mInsertionPointTable)
    mInsertionPointTable->Enumerate(InstantiateInsertionPoint, &data);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetInsertionPoint(nsIContent* aBoundElement, nsIContent* aCopyRoot,
                                         nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex,
                                         nsIContent** aDefaultContent)
{
  if (mInsertionPointTable) {
    nsCOMPtr<nsIAtom> tag;
    aChild->GetTag(*getter_AddRefs(tag));
    nsISupportsKey key(tag);
    nsCOMPtr<nsIXBLInsertionPointEntry> entry = getter_AddRefs(NS_STATIC_CAST(nsIXBLInsertionPointEntry*, 
                                                               mInsertionPointTable->Get(&key)));
    if (!entry) {
      nsISupportsKey key2(nsXBLAtoms::children);
      entry = getter_AddRefs(NS_STATIC_CAST(nsIXBLInsertionPointEntry*, mInsertionPointTable->Get(&key2)));
    }

    nsCOMPtr<nsIContent> realContent;
    if (entry) {
      nsCOMPtr<nsIContent> content;
      entry->GetInsertionParent(getter_AddRefs(content));
      entry->GetInsertionIndex(aIndex);
      entry->GetDefaultContent(aDefaultContent); // Addref happens here.
      nsCOMPtr<nsIContent> templContent;
      GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(templContent));
      LocateInstance(nsnull, templContent, aCopyRoot, content, getter_AddRefs(realContent));
    }
    else {
      // We got nothin'.  Bail.
      *aResult = nsnull;
      return NS_OK;
    }

    if (realContent)
      *aResult = realContent;
    else
      *aResult = aBoundElement;

    NS_IF_ADDREF(*aResult);
  }
  return NS_OK;  
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetSingleInsertionPoint(nsIContent* aBoundElement,
                                               nsIContent* aCopyRoot,
                                               nsIContent** aResult, PRUint32* aIndex,
                                               PRBool* aMultipleInsertionPoints,
                                               nsIContent** aDefaultContent)
{ 
  if (mInsertionPointTable) {
    if(mInsertionPointTable->Count() == 1) {
      nsISupportsKey key(nsXBLAtoms::children);
      nsCOMPtr<nsIXBLInsertionPointEntry> entry = getter_AddRefs(NS_STATIC_CAST(nsIXBLInsertionPointEntry*, 
                                                                 mInsertionPointTable->Get(&key)));
      nsCOMPtr<nsIContent> realContent;
      if (entry) {
        nsCOMPtr<nsIContent> content;
        entry->GetInsertionParent(getter_AddRefs(content));
        entry->GetInsertionIndex(aIndex);
        entry->GetDefaultContent(aDefaultContent); // Addref happens here.
        nsCOMPtr<nsIContent> templContent;
        GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(templContent));
        LocateInstance(nsnull, templContent, aCopyRoot, content, getter_AddRefs(realContent));
      }
      else {
        // The only insertion point specified was actually a filtered insertion point.
        // This means (strictly speaking) that we actually have multiple insertion
        // points: the filtered one and a generic insertion point (content that doesn't
        // match the filter will just go right underneath the bound element).
        *aMultipleInsertionPoints = PR_TRUE;
        *aResult = nsnull;
        *aIndex = 0;
        return NS_OK;
      }

      *aMultipleInsertionPoints = PR_FALSE;
      if (realContent)
        *aResult = realContent;
      else
        *aResult = aBoundElement;

      NS_IF_ADDREF(*aResult);
    }
    else 
      *aMultipleInsertionPoints = PR_TRUE;
  }
  return NS_OK;  
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag)
{
  mBaseNameSpaceID = aNamespaceID;
  mBaseTag = aTag;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetBaseTag(PRInt32* aNamespaceID, nsIAtom** aResult)
{
  if (mBaseTag) {
    *aResult = mBaseTag;
    NS_ADDREF(*aResult);
    *aNamespaceID = mBaseNameSpaceID;
  }
  else *aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::ImplementsInterface(REFNSIID aIID, PRBool* aResult)
{
  // Init the answer to FALSE.
  *aResult = PR_FALSE;

  // Now check our IID table.
  if (mInterfaceTable) {
    nsIIDKey key(aIID);
    nsCOMPtr<nsISupports> supports = getter_AddRefs(NS_STATIC_CAST(nsISupports*, 
                                                                   mInterfaceTable->Get(&key)));
    *aResult = supports != nsnull;
  }

  return NS_OK;
}

// Internal helpers ///////////////////////////////////////////////////////////////////////

void
nsXBLPrototypeBinding::GetImmediateChild(nsIAtom* aTag, nsIContent** aResult) 
{
  *aResult = nsnull;
  PRInt32 childCount;
  mBinding->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    mBinding->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsIAtom> tag;
    child->GetTag(*getter_AddRefs(tag));
    if (aTag == tag) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }

  return;
}
 
NS_IMETHODIMP
nsXBLPrototypeBinding::InitClass(const nsCString& aClassName,
                                 nsIScriptContext * aContext,
                                 void * aScriptObject, void ** aClassObject)
{
  NS_ENSURE_ARG_POINTER (aClassObject); 

  *aClassObject = nsnull;

  JSContext* cx = (JSContext*)aContext->GetNativeContext();
  JSObject* scriptObject = (JSObject*) aScriptObject;

  return nsXBLBinding::DoInitJSClass(cx, ::JS_GetGlobalObject(cx),
                                     scriptObject, aClassName, aClassObject);
}

void
nsXBLPrototypeBinding::LocateInstance(nsIContent* aBoundElement, nsIContent* aTemplRoot, nsIContent* aCopyRoot, 
                                      nsIContent* aTemplChild, nsIContent** aCopyResult)
{
  // XXX We will get in trouble if the binding instantiation deviates from the template
  // in the prototype.
  if (aTemplChild == aTemplRoot || !aTemplChild) {
    *aCopyResult = nsnull;
    return;
  }

  nsCOMPtr<nsIContent> templParent;
  nsCOMPtr<nsIContent> copyParent;
  nsCOMPtr<nsIContent> childPoint;
  aTemplChild->GetParent(*getter_AddRefs(templParent));
  
  if (aBoundElement) {
    nsCOMPtr<nsIAtom> tag;
    templParent->GetTag(*getter_AddRefs(tag));
    if (tag == nsXBLAtoms::children) {
      childPoint = templParent;
      childPoint->GetParent(*getter_AddRefs(templParent));
    }
  }

  if (!templParent)
    return;

  if (templParent == aTemplRoot)
    copyParent = aCopyRoot;
  else
    LocateInstance(aBoundElement, aTemplRoot, aCopyRoot, templParent, getter_AddRefs(copyParent));
  
  if (childPoint && aBoundElement) {
    // First we have to locate this insertion point and use its index and its
    // count to detemine our precise position within the template.
    nsCOMPtr<nsIDocument> doc;
    aBoundElement->GetDocument(*getter_AddRefs(doc));
    nsCOMPtr<nsIBindingManager> bm;
    doc->GetBindingManager(getter_AddRefs(bm));
    nsCOMPtr<nsIXBLBinding> binding;
    bm->GetBinding(aBoundElement, getter_AddRefs(binding));
    
    nsCOMPtr<nsIXBLBinding> currBinding = binding;
    nsCOMPtr<nsIContent> anonContent;
    while (currBinding) {
      currBinding->GetAnonymousContent(getter_AddRefs(anonContent));
      if (anonContent)
        break;
      nsCOMPtr<nsIXBLBinding> tempBinding = currBinding;
      tempBinding->GetBaseBinding(getter_AddRefs(currBinding));
    }

    nsCOMPtr<nsISupportsArray> points;
    if (anonContent == copyParent)
      currBinding->GetInsertionPointsFor(aBoundElement, getter_AddRefs(points));
    else
      currBinding->GetInsertionPointsFor(copyParent, getter_AddRefs(points));
    nsCOMPtr<nsIXBLInsertionPoint> insertionPoint;
    PRUint32 count;
    points->Count(&count);
    for (PRUint32 i = 0; i < count; i++) {
      // Next we have to find the real insertion point for this proto insertion
      // point.  If it does not contain any default content, then we should 
      // return null, since the content is not in the clone.
      nsCOMPtr<nsIXBLInsertionPoint> currPoint = getter_AddRefs((nsIXBLInsertionPoint*)points->ElementAt(i));
      nsCOMPtr<nsIContent> defContent;
      currPoint->GetDefaultContentTemplate(getter_AddRefs(defContent));
      if (defContent == childPoint) {
        // Now check to see if we even built default content at this
        // insertion point.
        currPoint->GetDefaultContent(getter_AddRefs(defContent));
        if (defContent) {
          // Find out the index of the template element within the <children> elt.
          PRInt32 index;
          childPoint->IndexOf(aTemplChild, index);
          
          // Now we just have to find the corresponding elt underneath the cloned
          // default content.
          defContent->ChildAt(index, *aCopyResult);
        } 
        break;
      }
    }
  }
  else if (copyParent)
  {
    PRInt32 index;
    templParent->IndexOf(aTemplChild, index);
    copyParent->ChildAt(index, *aCopyResult); // Addref happens here.
  }
}

struct nsXBLAttrChangeData
{
  nsXBLPrototypeBinding* mProto;
  nsIContent* mBoundElement;
  nsIContent* mContent;

  nsXBLAttrChangeData(nsXBLPrototypeBinding* aProto,
                      nsIContent* aElt, nsIContent* aContent) 
  :mProto(aProto), mBoundElement(aElt), mContent(aContent) {};
};

PRBool PR_CALLBACK SetAttrs(nsHashKey* aKey, void* aData, void* aClosure)
{
  // XXX How to deal with NAMESPACES!!!?
  nsIXBLAttributeEntry* entry = (nsIXBLAttributeEntry*)aData;
  nsXBLAttrChangeData* changeData = (nsXBLAttrChangeData*)aClosure;

  nsCOMPtr<nsIAtom> src;
  entry->GetSrcAttribute(getter_AddRefs(src));

  nsAutoString value;
  PRBool attrPresent = PR_TRUE;
  if (src == nsXBLAtoms::xbltext) {
    nsXBLBinding::GetTextData(changeData->mBoundElement, value);
    value.StripChar(PRUnichar('\n'));
    value.StripChar(PRUnichar('\r'));
    nsAutoString stripVal(value);
    stripVal.StripWhitespace();

    if (stripVal.IsEmpty()) 
      attrPresent = PR_FALSE;
  }
  else {
    nsresult result = changeData->mBoundElement->GetAttr(kNameSpaceID_None, src, value);
    attrPresent = (result == NS_CONTENT_ATTR_NO_VALUE ||
                   result == NS_CONTENT_ATTR_HAS_VALUE);
  }

  if (attrPresent) {
    nsCOMPtr<nsIContent> content;
    changeData->mProto->GetImmediateChild(nsXBLAtoms::content, getter_AddRefs(content));

    nsCOMPtr<nsIXBLAttributeEntry> curr = entry;
    while (curr) {
      nsCOMPtr<nsIAtom> dst;
      nsCOMPtr<nsIContent> element;
      curr->GetDstAttribute(getter_AddRefs(dst));
      curr->GetElement(getter_AddRefs(element));

      nsCOMPtr<nsIContent> realElement;
      changeData->mProto->LocateInstance(changeData->mBoundElement,
                                         content, changeData->mContent, element, getter_AddRefs(realElement));
      if (realElement) {
        realElement->SetAttr(kNameSpaceID_None, dst, value, PR_FALSE);
        nsCOMPtr<nsIAtom> tag;
        realElement->GetTag(*getter_AddRefs(tag));
        if (dst == nsXBLAtoms::xbltext ||
            (tag == nsHTMLAtoms::html) && (dst == nsHTMLAtoms::value) && !value.IsEmpty()) {
          nsCOMPtr<nsIDOMText> textNode;
          nsCOMPtr<nsIDocument> doc;
          changeData->mBoundElement->GetDocument(*getter_AddRefs(doc));
          nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
          domDoc->CreateTextNode(value, getter_AddRefs(textNode));
          nsCOMPtr<nsIDOMNode> dummy;
          nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(realElement));
          domElement->AppendChild(textNode, getter_AddRefs(dummy));
        }
      }

      nsCOMPtr<nsIXBLAttributeEntry> next = curr;
      curr->GetNext(getter_AddRefs(next));
      curr = next;
    }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent)
{
  if (mAttributeTable) {
    nsXBLAttrChangeData data(this, aBoundElement, aAnonymousContent);
    mAttributeTable->Enumerate(SetAttrs, (void*)&data);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetRuleProcessors(nsISupportsArray** aResult)
{
  if (mResources)
    *aResult = mResources->mRuleProcessors;
  else
    *aResult = nsnull;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::GetStyleSheets(nsISupportsArray** aResult)
{
  if (mResources)
    *aResult = mResources->mStyleSheetList;
  else
    *aResult = nsnull;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeBinding::ShouldBuildChildFrames(PRBool* aResult)
{
  *aResult = PR_TRUE;
  if (mAttributeTable) {
    nsISupportsKey key(nsXBLAtoms::xbltext);
    nsCOMPtr<nsISupports> supports = getter_AddRefs(NS_STATIC_CAST(nsISupports*, 
                                                                   mAttributeTable->Get(&key)));

    *aResult = !supports.get();
  }

  return NS_OK;
}

void
nsXBLPrototypeBinding::ConstructAttributeTable(nsIContent* aElement)
{
  nsAutoString inherits;
  aElement->GetAttr(kNameSpaceID_XBL, nsXBLAtoms::inherits, inherits);

  if (!inherits.IsEmpty()) {
    if (!mAttributeTable) {
      mAttributeTable = new nsSupportsHashtable(4);
    }

    // The user specified at least one attribute.
    char* str = ToNewCString(inherits);
    char* newStr;
    // XXX We should use a strtok function that tokenizes PRUnichars
    // so that we don't have to convert from Unicode to ASCII and then back

    char* token = nsCRT::strtok( str, ", ", &newStr );
    while( token != NULL ) {
      // Build an atom out of this attribute.
      nsCOMPtr<nsIAtom> atom;
      nsCOMPtr<nsIAtom> attribute;

      // Figure out if this token contains a :. 
      nsAutoString attrTok; attrTok.AssignWithConversion(token);
      PRInt32 index = attrTok.Find("=", PR_TRUE);
      if (index != -1) {
        // This attribute maps to something different.
        nsAutoString left, right;
        attrTok.Left(left, index);
        attrTok.Right(right, attrTok.Length()-index-1);

        atom = getter_AddRefs(NS_NewAtom(right.get()));
        attribute = getter_AddRefs(NS_NewAtom(left.get()));
      }
      else {
        nsAutoString tok; tok.AssignWithConversion(token);
        atom = getter_AddRefs(NS_NewAtom(tok.get()));
        attribute = atom;
      }
      
      // Create an XBL attribute entry.
      nsXBLAttributeEntry* xblAttr = nsXBLAttributeEntry::Create(atom, attribute, aElement);

      // Now we should see if some element within our anonymous
      // content is already observing this attribute.
      nsISupportsKey key(atom);
      nsCOMPtr<nsISupports> supports = getter_AddRefs(NS_STATIC_CAST(nsISupports*, 
                                                                     mAttributeTable->Get(&key)));
  
      nsCOMPtr<nsIXBLAttributeEntry> entry = do_QueryInterface(supports);
      if (!entry) {
        // Put it in the table.
        mAttributeTable->Put(&key, xblAttr);
      } else {
        nsCOMPtr<nsIXBLAttributeEntry> attr = entry;
        nsCOMPtr<nsIXBLAttributeEntry> tmpAttr = entry;
        do {
          attr = tmpAttr;
          attr->GetNext(getter_AddRefs(tmpAttr));
        } while (tmpAttr);
        attr->SetNext(xblAttr);
      }

      // Now remove the inherits attribute from the element so that it doesn't
      // show up on clones of the element.  It is used
      // by the template only, and we don't need it anymore.
      // XXXdwh Don't do this for XUL elements, since it faults them into heavyweight
      // elements. Should nuke from the prototype instead.
      // aElement->UnsetAttr(kNameSpaceID_XBL, nsXBLAtoms::inherits, PR_FALSE);

      token = nsCRT::strtok( newStr, ", ", &newStr );
    }

    nsMemory::Free(str);
  }

  // Recur into our children.
  PRInt32 childCount;
  aElement->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    aElement->ChildAt(i, *getter_AddRefs(child));
    ConstructAttributeTable(child);
  }
}

void 
nsXBLPrototypeBinding::ConstructInsertionTable(nsIContent* aContent)
{
  nsCOMPtr<nsISupportsArray> childrenElements;
  GetNestedChildren(nsXBLAtoms::children, aContent, getter_AddRefs(childrenElements));

  if (!childrenElements)
    return;

  mInsertionPointTable = new nsSupportsHashtable(4);

  PRUint32 count;
  childrenElements->Count(&count);
  PRUint32 i;
  for (i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> supp;
    childrenElements->GetElementAt(i, getter_AddRefs(supp));
    nsCOMPtr<nsIContent> child(do_QueryInterface(supp));
    if (child) {
      nsCOMPtr<nsIContent> parent; 
      child->GetParent(*getter_AddRefs(parent));

      // Create an XBL insertion point entry.
      nsXBLInsertionPointEntry* xblIns = nsXBLInsertionPointEntry::Create(parent);

      nsAutoString includes;
      child->GetAttr(kNameSpaceID_None, nsXBLAtoms::includes, includes);
      if (includes.IsEmpty()) {
        nsISupportsKey key(nsXBLAtoms::children);
        mInsertionPointTable->Put(&key, xblIns);
      }
      else {
        // The user specified at least one attribute.
        char* str = ToNewCString(includes);
        char* newStr;
        // XXX We should use a strtok function that tokenizes PRUnichar's
        // so that we don't have to convert from Unicode to ASCII and then back

        char* token = nsCRT::strtok( str, "| ", &newStr );
        while( token != NULL ) {
          // Build an atom out of this string.
          nsCOMPtr<nsIAtom> atom;
            
          nsAutoString tok; tok.AssignWithConversion(token);
          atom = getter_AddRefs(NS_NewAtom(tok.get()));
           
          nsISupportsKey key(atom);
          mInsertionPointTable->Put(&key, xblIns);
          
          token = nsCRT::strtok( newStr, "| ", &newStr );
        }

        nsMemory::Free(str);
      }

      // Compute the index of the <children> element.  This index is
      // equal to the index of the <children> in the template minus the #
      // of previous insertion point siblings removed.  Because our childrenElements
      // array was built in a DFS that went from left-to-right through siblings,
      // if we dynamically obtain our index each time, then the removals of previous
      // siblings will cause the index to adjust (and we won't have to take that into
      // account explicitly).
      PRInt32 index;
      parent->IndexOf(child, index);
      xblIns->SetInsertionIndex((PRUint32)index);

      // Now remove the <children> element from the template.  This ensures that the
      // binding instantiation will not contain a clone of the <children> element when
      // it clones the binding template.
      parent->RemoveChildAt(index, PR_FALSE);

      // See if the insertion point contains default content.  Default content must
      // be cached in our insertion point entry, since it will need to be cloned
      // in situations where no content ends up being placed at the insertion point.
      PRInt32 defaultCount;
      child->ChildCount(defaultCount);
      if (defaultCount > 0) {
        // Annotate the insertion point with our default content.
        xblIns->SetDefaultContent(child);

        // Reconnect back to our parent for access later.  This makes "inherits" easier
        // to work with on default content.
        child->SetParent(parent);
      }
    }
  }
}

NS_IMETHODIMP
nsXBLPrototypeBinding::ConstructInterfaceTable(const nsAString& aImpls)
{
  if (!aImpls.IsEmpty()) {
    // Obtain the interface info manager that can tell us the IID
    // for a given interface name.
    nsCOMPtr<nsIInterfaceInfoManager> infoManager = getter_AddRefs(XPTI_GetInterfaceInfoManager());
    if (!infoManager)
      return NS_ERROR_FAILURE;

    // Create the table.
    if (!mInterfaceTable)
      mInterfaceTable = new nsSupportsHashtable(4);

    // The user specified at least one attribute.
    char* str = ToNewCString(aImpls);
    char* newStr;
    // XXX We should use a strtok function that tokenizes PRUnichars
    // so that we don't have to convert from Unicode to ASCII and then back

    char* token = nsCRT::strtok( str, ", ", &newStr );
    while( token != NULL ) {
      // Take the name and try obtain an IID.
      nsIID* iid = nsnull;
      infoManager->GetIIDForName(token, &iid);
      if (iid) {
        // We found a valid iid.  Add it to our table.
        nsIIDKey key(*iid);
        mInterfaceTable->Put(&key, mBinding);
        nsMemory::Free(iid);
      }

      token = nsCRT::strtok( newStr, ", ", &newStr );
    }

    nsMemory::Free(str);
  }

  return NS_OK;
}

void
nsXBLPrototypeBinding::GetNestedChildren(nsIAtom* aTag, nsIContent* aContent, nsISupportsArray** aList)
{
  PRInt32 childCount;
  aContent->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    aContent->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsIAtom> tag;
    child->GetTag(*getter_AddRefs(tag));
    if (aTag == tag) {
      if (!*aList)
        NS_NewISupportsArray(aList); // Addref happens here.
      (*aList)->AppendElement(child);
    }
    else
      GetNestedChildren(aTag, child, aList);
  }
}

NS_IMETHODIMP
nsXBLPrototypeBinding::AddResourceListener(nsIContent* aBoundElement)
{
  if (!mResources)
    return NS_ERROR_FAILURE; // Makes no sense to add a listener when the binding
                             // has no resources.

  mResources->AddResourceListener(aBoundElement);
  return NS_OK;
}

// Creation Routine ///////////////////////////////////////////////////////////////////////

nsresult
NS_NewXBLPrototypeBinding(const nsACString& aRef, nsIContent* aElement, 
                          nsIXBLDocumentInfo* aInfo, nsIXBLPrototypeBinding** aResult)
{
  nsXBLPrototypeBinding * binding = new nsXBLPrototypeBinding(aRef, aInfo, aElement);
  if (!binding)
    return NS_ERROR_OUT_OF_MEMORY;
  *aResult = binding;
  NS_ADDREF(*aResult);
  return NS_OK;
}

