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

#ifndef nsRuleNode_h___
#define nsRuleNode_h___

#include "nsCOMPtr.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsFixedSizeAllocator.h"
#include "nsIPresContext.h"
#include "nsCSSDeclaration.h"
#include "nsILanguageAtomService.h"

class nsIStyleContext;
struct nsRuleList;
struct PLDHashTable;

typedef void (*nsPostResolveFunc)(nsStyleStruct* aStyleStruct, nsRuleData* aData);

struct nsInheritedStyleData
{
  nsStyleVisibility* mVisibilityData;
  nsStyleFont* mFontData;
  nsStyleList* mListData;
  nsStyleTableBorder* mTableData;
  nsStyleColor* mColorData;
  nsStyleQuotes* mQuotesData;
  nsStyleText* mTextData;
  nsStyleUserInterface* mUIData;
#ifdef MOZ_SVG
  nsStyleSVG* mSVGData;
#endif

  void* operator new(size_t sz, nsIPresContext* aContext) {
    void* result = nsnull;
    aContext->AllocateFromShell(sz, &result);
    return result;
  };

  void ClearInheritedData(PRUint32 aBits) {
    if (mVisibilityData && (aBits & NS_STYLE_INHERIT_VISIBILITY))
      mVisibilityData = nsnull;
    if (mFontData && (aBits & NS_STYLE_INHERIT_FONT))
      mFontData = nsnull;
    if (mListData && (aBits & NS_STYLE_INHERIT_LIST))
      mListData = nsnull;
    if (mTableData && (aBits & NS_STYLE_INHERIT_TABLE_BORDER))
      mTableData = nsnull;
    if (mColorData && (aBits & NS_STYLE_INHERIT_COLOR))
      mColorData = nsnull;
    if (mQuotesData && (aBits & NS_STYLE_INHERIT_QUOTES))
      mQuotesData = nsnull;
    if (mTextData && (aBits & NS_STYLE_INHERIT_TEXT))
      mTextData = nsnull;
    if (mUIData && (aBits & NS_STYLE_INHERIT_UI))
      mUIData = nsnull;
#ifdef MOZ_SVG
    if (mSVGData && (aBits & NS_STYLE_INHERIT_SVG))
      mSVGData = nsnull;
#endif 
  };

  void Destroy(PRUint32 aBits, nsIPresContext* aContext) {
    if (mVisibilityData && !(aBits & NS_STYLE_INHERIT_VISIBILITY))
      mVisibilityData->Destroy(aContext);
    if (mFontData && !(aBits & NS_STYLE_INHERIT_FONT))
      mFontData->Destroy(aContext);
    if (mListData && !(aBits & NS_STYLE_INHERIT_LIST))
      mListData->Destroy(aContext);
    if (mTableData && !(aBits & NS_STYLE_INHERIT_TABLE_BORDER))
      mTableData->Destroy(aContext);
    if (mColorData && !(aBits & NS_STYLE_INHERIT_COLOR))
      mColorData->Destroy(aContext);
    if (mQuotesData && !(aBits & NS_STYLE_INHERIT_QUOTES))
      mQuotesData->Destroy(aContext);
    if (mTextData && !(aBits & NS_STYLE_INHERIT_TEXT))
      mTextData->Destroy(aContext);
    if (mUIData && !(aBits & NS_STYLE_INHERIT_UI))
      mUIData->Destroy(aContext);
#ifdef MOZ_SVG
    if (mSVGData && !(aBits & NS_STYLE_INHERIT_SVG))
      mSVGData->Destroy(aContext);
#endif
    aContext->FreeToShell(sizeof(nsInheritedStyleData), this);
  };

  nsInheritedStyleData() 
    :mVisibilityData(nsnull), mFontData(nsnull), mListData(nsnull), 
     mTableData(nsnull), mColorData(nsnull), mQuotesData(nsnull), mTextData(nsnull), mUIData(nsnull)
#ifdef MOZ_SVG
    , mSVGData(nsnull)
#endif
  {};
};

struct nsResetStyleData
{
  nsResetStyleData()
    :mDisplayData(nsnull), mMarginData(nsnull), mBorderData(nsnull), mPaddingData(nsnull), 
     mOutlineData(nsnull), mPositionData(nsnull), mTableData(nsnull), mBackgroundData(nsnull),
     mContentData(nsnull), mTextData(nsnull), mUIData(nsnull)
  {
#ifdef INCLUDE_XUL
    mXULData = nsnull;
#endif
  };

  void* operator new(size_t sz, nsIPresContext* aContext) {
    void* result = nsnull;
    aContext->AllocateFromShell(sz, &result);
    return result;
  }

  void ClearInheritedData(PRUint32 aBits) {
    if (mDisplayData && (aBits & NS_STYLE_INHERIT_DISPLAY))
      mDisplayData = nsnull;
    if (mMarginData && (aBits & NS_STYLE_INHERIT_MARGIN))
      mMarginData = nsnull;
    if (mBorderData && (aBits & NS_STYLE_INHERIT_BORDER))
      mBorderData = nsnull;
    if (mPaddingData && (aBits & NS_STYLE_INHERIT_PADDING))
      mPaddingData = nsnull;
    if (mOutlineData && (aBits & NS_STYLE_INHERIT_OUTLINE))
      mOutlineData = nsnull;
    if (mPositionData && (aBits & NS_STYLE_INHERIT_POSITION))
      mPositionData = nsnull;
    if (mTableData && (aBits & NS_STYLE_INHERIT_TABLE))
      mTableData = nsnull;
    if (mBackgroundData && (aBits & NS_STYLE_INHERIT_BACKGROUND))
      mBackgroundData = nsnull;
    if (mContentData && (aBits & NS_STYLE_INHERIT_CONTENT))
      mContentData = nsnull;
    if (mTextData && (aBits & NS_STYLE_INHERIT_TEXT_RESET))
      mTextData = nsnull;
    if (mUIData && (aBits & NS_STYLE_INHERIT_UI_RESET))
      mUIData = nsnull;
#ifdef INCLUDE_XUL
    if (mXULData && (aBits & NS_STYLE_INHERIT_XUL))
      mXULData = nsnull;
#endif
  };

  void Destroy(PRUint32 aBits, nsIPresContext* aContext) {
    if (mDisplayData && !(aBits & NS_STYLE_INHERIT_DISPLAY))
      mDisplayData->Destroy(aContext);
    if (mMarginData && !(aBits & NS_STYLE_INHERIT_MARGIN))
      mMarginData->Destroy(aContext);
    if (mBorderData && !(aBits & NS_STYLE_INHERIT_BORDER))
      mBorderData->Destroy(aContext);
    if (mPaddingData && !(aBits & NS_STYLE_INHERIT_PADDING))
      mPaddingData->Destroy(aContext);
    if (mOutlineData && !(aBits & NS_STYLE_INHERIT_OUTLINE))
      mOutlineData->Destroy(aContext);
    if (mPositionData && !(aBits & NS_STYLE_INHERIT_POSITION))
      mPositionData->Destroy(aContext);
    if (mTableData && !(aBits & NS_STYLE_INHERIT_TABLE))
      mTableData->Destroy(aContext);
    if (mBackgroundData && !(aBits & NS_STYLE_INHERIT_BACKGROUND))
      mBackgroundData->Destroy(aContext);
    if (mContentData && !(aBits & NS_STYLE_INHERIT_CONTENT))
      mContentData->Destroy(aContext);
    if (mTextData && !(aBits & NS_STYLE_INHERIT_TEXT_RESET))
      mTextData->Destroy(aContext);
    if (mUIData && !(aBits & NS_STYLE_INHERIT_UI_RESET))
      mUIData->Destroy(aContext);
#ifdef INCLUDE_XUL
    if (mXULData && !(aBits & NS_STYLE_INHERIT_XUL))
      mXULData->Destroy(aContext);
#endif
    aContext->FreeToShell(sizeof(nsResetStyleData), this);
  };

  nsStyleDisplay* mDisplayData;
  nsStyleMargin* mMarginData;
  nsStyleBorder* mBorderData;
  nsStylePadding* mPaddingData;
  nsStyleOutline* mOutlineData;
  nsStylePosition* mPositionData;
  nsStyleTable* mTableData;
  nsStyleBackground* mBackgroundData;
  nsStyleContent* mContentData;
  nsStyleTextReset* mTextData;
  nsStyleUIReset* mUIData;
#ifdef INCLUDE_XUL
  nsStyleXUL* mXULData;
#endif
};

struct nsCachedStyleData
{
  struct StyleStructInfo {
    ptrdiff_t mCachedStyleDataOffset;
    ptrdiff_t mInheritResetOffset;
    PRBool    mIsReset;
  };

  static StyleStructInfo gInfo[];

  nsInheritedStyleData* mInheritedData;
  nsResetStyleData* mResetData;

  static PRBool IsReset(const nsStyleStructID& aSID) {
    return gInfo[aSID].mIsReset;
  };

  static PRUint32 GetBitForSID(const nsStyleStructID& aSID) {
    return 1 << (aSID - 1);
  };

  nsStyleStruct* GetStyleData(const nsStyleStructID& aSID) {
    const StyleStructInfo& info = gInfo[aSID];
    char* resetOrInheritSlot = NS_REINTERPRET_CAST(char*, this) + info.mCachedStyleDataOffset;
    char* resetOrInherit = NS_REINTERPRET_CAST(char*, *NS_REINTERPRET_CAST(void**, resetOrInheritSlot));
    nsStyleStruct* data = nsnull;
    if (resetOrInherit) {
      char* dataSlot = resetOrInherit + info.mInheritResetOffset;
      data = *NS_REINTERPRET_CAST(nsStyleStruct**, dataSlot);
    }
    return data;
  };

  void ClearInheritedData(PRUint32 aBits) {
    if (mResetData)
      mResetData->ClearInheritedData(aBits);
    if (mInheritedData)
      mInheritedData->ClearInheritedData(aBits);
  }

  void Destroy(PRUint32 aBits, nsIPresContext* aContext) {
    if (mResetData)
      mResetData->Destroy(aBits, aContext);
    if (mInheritedData)
      mInheritedData->Destroy(aBits, aContext);
    mResetData = nsnull;
    mInheritedData = nsnull;
  }

  nsCachedStyleData() :mInheritedData(nsnull), mResetData(nsnull) {};
  ~nsCachedStyleData() {};
};

struct nsRuleData
{
  nsStyleStructID mSID;
  PRPackedBool mCanStoreInRuleTree;
  nsIPresContext* mPresContext;
  nsIStyleContext* mStyleContext;
  nsPostResolveFunc mPostResolveCallback;
  nsCSSFont* mFontData; // Should always be stack-allocated! We don't own these structures!
  nsCSSDisplay* mDisplayData;
  nsCSSMargin* mMarginData;
  nsCSSList* mListData;
  nsCSSPosition* mPositionData;
  nsCSSTable* mTableData;
  nsCSSColor* mColorData;
  nsCSSContent* mContentData;
  nsCSSText* mTextData;
  nsCSSUserInterface* mUIData;

#ifdef INCLUDE_XUL
  nsCSSXUL* mXULData;
#endif

#ifdef MOZ_SVG
  nsCSSSVG* mSVGData;
#endif

  nsRuleData(const nsStyleStructID& aSID, nsIPresContext* aContext, nsIStyleContext* aStyleContext) 
    :mSID(aSID), mPresContext(aContext), mStyleContext(aStyleContext), mPostResolveCallback(nsnull),
     mFontData(nsnull), mDisplayData(nsnull), mMarginData(nsnull), mListData(nsnull), 
     mPositionData(nsnull), mTableData(nsnull), mColorData(nsnull), mContentData(nsnull), mTextData(nsnull),
     mUIData(nsnull)
  {
    mCanStoreInRuleTree = PR_TRUE;

#ifdef INCLUDE_XUL
    mXULData = nsnull;
#endif
#ifdef MOZ_SVG
    mSVGData = nsnull;
#endif
  };
  ~nsRuleData() {};
};

class nsRuleNode {
public:
    // for purposes of the RuleDetail (and related code),
    //  * a non-inherited value is one that is specified as a
    //    non-"inherit" value or as an "inherit" value that is reflected
    //    in the struct and to the user of the style system with an
    //    eCSSUnit_Inherit value
    //  * an inherited value is one that is specified as "inherit" and
    //    where the inheritance is computed by the style system
  enum RuleDetail {
    eRuleNone, // No props have been specified at all.
    eRulePartialReset, // At least one prop with a non-inherited value
                       // has been specified.  No props have been
                       // specified with an inherited value.  At least
                       // one prop remains unspecified.
    eRulePartialMixed, // At least one prop with a non-inherited value
                       // has been specified.  Some props may also have
                       // been specified with an inherited value.  At
                       // least one prop remains unspecified.
    eRulePartialInherited, // Only props with inherited values have
                           // have been specified.  At least one prop
                           // remains unspecified.
    eRuleFullReset, // All props have been specified.  None has an
                    // inherited value.
    eRuleFullMixed, // All props have been specified.  At least one has
                    // a non-inherited value.
    eRuleFullInherited, // All props have been specified with inherited
                        // values.
    eRuleUnknown // Information unknown (used as a result from a check
                 // callback to trigger the normal checking codepath)
  };

  enum { // Types of RuleBits
    eNoneBits,
    eDependentBits
  };

private:
  nsIPresContext* mPresContext; // Our pres context.

  nsRuleNode* mParent; // A pointer to the parent node in the tree.
                       // This enables us to walk backwards from the
                       // most specific rule matched to the least
                       // specific rule (which is the optimal order to
                       // use for lookups of style properties.
  nsCOMPtr<nsIStyleRule> mRule; // A pointer to our specific rule.

  // The children of this node are stored in either a hashtable or list
  // that maps from rules to our nsRuleNode children.  When matching
  // rules, we use this mapping to transition from node to node
  // (constructing new nodes as needed to flesh out the tree).

  void *mChildrenTaggedPtr; // Accessed only through the methods below.

  enum {
    kTypeMask = 0x1,
    kListType = 0x0,
    kHashType = 0x1
  };
  enum {
    // Maximum to have in a list before converting to a hashtable.
    // XXX Need to optimize this.
    kMaxChildrenInList = 32
  };

  PRBool HaveChildren() {
    return mChildrenTaggedPtr != nsnull;
  }
  PRBool ChildrenAreHashed() {
    return (PRWord(mChildrenTaggedPtr) & kTypeMask) == kHashType;
  }
  nsRuleList* ChildrenList() {
    return NS_STATIC_CAST(nsRuleList*, mChildrenTaggedPtr);
  }
  PLDHashTable* ChildrenHash() {
    return (PLDHashTable*) (PRWord(mChildrenTaggedPtr) & ~PRWord(kTypeMask));
  }
  void SetChildrenList(nsRuleList *aList) {
    NS_ASSERTION(!(PRWord(aList) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildrenTaggedPtr = aList;
  }
  void SetChildrenHash(PLDHashTable *aHashtable) {
    NS_ASSERTION(!(PRWord(aHashtable) & kTypeMask),
                 "pointer not 2-byte aligned");
    mChildrenTaggedPtr = (void*)(PRWord(aHashtable) | kHashType);
  }
  void ConvertChildrenToHash();

  nsCachedStyleData mStyleData;   // Any data we cached on the rule node.

  PRUint32 mDependentBits; // Used to cache the fact that we can look up
                           // cached data under a parent rule.

  PRUint32 mNoneBits; // Used to cache the fact that the branch to this
                      // node specifies no non-inherited data for a
                      // given struct type.  (This usually implies that
                      // the entire branch specifies no non-inherited
                      // data, although not necessarily, if a
                      // non-inherited value is overridden by an
                      // explicit 'inherit' value.)  For example, if an
                      // entire rule branch specifies no color
                      // information, then a bit will be set along every
                      // rule node on that branch, so that you can break
                      // out of the rule tree early and just inherit
                      // from the parent style context.  The presence of
                      // this bit means we should just get inherited
                      // data from the parent style context, and it is
                      // never used for reset structs since their
                      // Compute*Data functions don't initialize from
                      // inherited data.

friend struct nsRuleList;

public:
  // Overloaded new operator. Initializes the memory to 0 and relies on an arena
  // (which comes from the presShell) to perform the allocation.
  void* operator new(size_t sz, nsIPresContext* aContext);
  void Destroy();
  static nsILanguageAtomService *gLangService;

protected:
  void PropagateDependentBit(PRUint32 aBit, nsRuleNode* aHighestNode);
  void PropagateNoneBit(PRUint32 aBit, nsRuleNode* aHighestNode);
  
  const nsStyleStruct* SetDefaultOnRoot(const nsStyleStructID aSID, nsIStyleContext* aContext);

  const nsStyleStruct* WalkRuleTree(const nsStyleStructID aSID, nsIStyleContext* aContext, 
                                    nsRuleData* aRuleData,
                                    nsCSSStruct* aSpecificData,
                                    PRBool aComputeData);

  const nsStyleStruct* ComputeDisplayData(nsStyleStruct* aStartDisplay, const nsCSSStruct& aDisplayData, 
                                          nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                          const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeVisibilityData(nsStyleStruct* aStartVisibility, const nsCSSStruct& aDisplayData, 
                                             nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                             const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeFontData(nsStyleStruct* aStartFont, const nsCSSStruct& aFontData, 
                                       nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                       const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeColorData(nsStyleStruct* aStartColor, const nsCSSStruct& aColorData, 
                                        nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                        const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeBackgroundData(nsStyleStruct* aStartBackground, const nsCSSStruct& aColorData, 
                                             nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                             const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeMarginData(nsStyleStruct* aStartMargin, const nsCSSStruct& aMarginData, 
                                         nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                         const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeBorderData(nsStyleStruct* aStartBorder, const nsCSSStruct& aMarginData, 
                                         nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                         const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputePaddingData(nsStyleStruct* aStartPadding, const nsCSSStruct& aMarginData, 
                                          nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                          const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeOutlineData(nsStyleStruct* aStartOutline, const nsCSSStruct& aMarginData, 
                                          nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                          const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeListData(nsStyleStruct* aStartList, const nsCSSStruct& aListData, 
                                       nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                       const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputePositionData(nsStyleStruct* aStartPosition, const nsCSSStruct& aPositionData, 
                                           nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                           const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeTableData(nsStyleStruct* aStartTable, const nsCSSStruct& aTableData, 
                                        nsIStyleContext* aContext, nsRuleNode* aHighestNode,
                                        const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeTableBorderData(nsStyleStruct* aStartTable, const nsCSSStruct& aTableData, 
                                              nsIStyleContext* aContext,  
                                              nsRuleNode* aHighestNode,
                                              const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeContentData(nsStyleStruct* aStartContent, const nsCSSStruct& aData, 
                                          nsIStyleContext* aContext,  
                                          nsRuleNode* aHighestNode,
                                          const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeQuotesData(nsStyleStruct* aStartQuotes, const nsCSSStruct& aData, 
                                         nsIStyleContext* aContext,  
                                         nsRuleNode* aHighestNode,
                                         const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeTextData(nsStyleStruct* aStartData, const nsCSSStruct& aData, 
                                       nsIStyleContext* aContext,  
                                       nsRuleNode* aHighestNode,
                                       const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeTextResetData(nsStyleStruct* aStartData, const nsCSSStruct& aData, 
                                            nsIStyleContext* aContext,  
                                            nsRuleNode* aHighestNode,
                                            const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeUIData(nsStyleStruct* aStartData, const nsCSSStruct& aData, 
                                     nsIStyleContext* aContext,  
                                     nsRuleNode* aHighestNode,
                                     const RuleDetail& aRuleDetail, PRBool aInherited);
  const nsStyleStruct* ComputeUIResetData(nsStyleStruct* aStartData, const nsCSSStruct& aData, 
                                          nsIStyleContext* aContext,  
                                          nsRuleNode* aHighestNode,
                                          const RuleDetail& aRuleDetail, PRBool aInherited);
#ifdef INCLUDE_XUL
  const nsStyleStruct* ComputeXULData(nsStyleStruct* aStartXUL, const nsCSSStruct& aXULData, 
                                      nsIStyleContext* aContext,  
                                      nsRuleNode* aHighestNode,
                                      const RuleDetail& aRuleDetail, PRBool aInherited);
#endif

#ifdef MOZ_SVG
  const nsStyleStruct* ComputeSVGData(nsStyleStruct* aStartSVG, const nsCSSStruct& aSVGData, 
                                      nsIStyleContext* aContext,  
                                      nsRuleNode* aHighestNode,
                                      const RuleDetail& aRuleDetail, PRBool aInherited);
#endif
  
  typedef const nsStyleStruct*
  (nsRuleNode::*ComputeStyleDataFn)(nsStyleStruct* aStartStruct,
                                    const nsCSSStruct& aStartData,
                                    nsIStyleContext* aContext,
                                    nsRuleNode* aHighestNode,
                                    const RuleDetail& aRuleDetail,
                                    PRBool aInherited);

  static ComputeStyleDataFn gComputeStyleDataFn[];

  inline RuleDetail CheckSpecifiedProperties(const nsStyleStructID aSID, const nsCSSStruct& aCSSStruct);

  const nsStyleStruct* GetParentData(const nsStyleStructID aSID);
  const nsStyleStruct* GetDisplayData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetVisibilityData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetFontData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetColorData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetBackgroundData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetMarginData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetBorderData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetPaddingData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetOutlineData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetListData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetPositionData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetTableData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetTableBorderData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetContentData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetQuotesData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetTextData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetTextResetData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetUIData(nsIStyleContext* aContext, PRBool aComputeData);
  const nsStyleStruct* GetUIResetData(nsIStyleContext* aContext, PRBool aComputeData);
#ifdef INCLUDE_XUL
  const nsStyleStruct* GetXULData(nsIStyleContext* aContext, PRBool aComputeData);
#endif
#ifdef MOZ_SVG
  const nsStyleStruct* GetSVGData(nsIStyleContext* aContext, PRBool aComputeData);
#endif

  typedef const nsStyleStruct* (nsRuleNode::*GetStyleDataFn)(nsIStyleContext*, PRBool);
  static GetStyleDataFn gGetStyleDataFn[];

public:
  nsRuleNode(nsIPresContext* aPresContext, nsIStyleRule* aRule=nsnull, nsRuleNode* aParent=nsnull);
  virtual ~nsRuleNode();

  static void CreateRootNode(nsIPresContext* aPresContext, nsRuleNode** aResult);

  nsresult GetBits(PRInt32 aType, PRUint32* aResult);
  nsresult Transition(nsIStyleRule* aRule, nsRuleNode** aResult);
  nsRuleNode* GetParent() { return mParent; }
  PRBool IsRoot() { return mParent == nsnull; }
  nsresult GetRule(nsIStyleRule** aResult)
  {
    *aResult = mRule;
    NS_IF_ADDREF(*aResult);
    return NS_OK;
  }
  nsIStyleRule* Rule() {
    // NOTE:  Does not |AddRef|.
    return mRule;
  }

  nsresult ClearCachedData(nsIStyleRule* aRule);
  nsresult ClearCachedDataInSubtree(nsIStyleRule* aRule);
  nsresult GetPresContext(nsIPresContext** aResult);
  nsresult PathContainsRule(nsIStyleRule* aRule, PRBool* aMatched);
  const nsStyleStruct* GetStyleData(nsStyleStructID aSID, 
                                    nsIStyleContext* aContext,
                                    PRBool aComputeData);
};

#endif
