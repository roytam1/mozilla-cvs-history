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
 * The Original Code is mozilla.org code.
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
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsIStyleContext_h___
#define nsIStyleContext_h___

#include "nslayout.h"
#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsMargin.h"
#include "nsRect.h"
#include "nsFont.h"
#include "nsVoidArray.h"
#include "nsStyleCoord.h"
#include "nsStyleStruct.h"
#include "nsStyleConsts.h"
#include "nsIStyleSet.h"
#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsILanguageAtom.h"
#include "nsIFrame.h"

class nsISizeOfHandler;

class nsIFrame;
class nsISupportsArray;
class nsIStyleContext;
class nsIRuleNode;

//----------------------------------------------------------------------

#define NS_ISTYLECONTEXT_IID   \
 { 0x26a4d970, 0xa342, 0x11d1, \
   {0x89, 0x74, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81} }

class nsIStyleContext : public nsISupports {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISTYLECONTEXT_IID; return iid; }

  virtual PRBool    Equals(const nsIStyleContext* aOther) const = 0;
  
  virtual nsIStyleContext*  GetParent(void) const = 0;
  NS_IMETHOD GetFirstChild(nsIStyleContext** aContext)=0;

  NS_IMETHOD GetPseudoType(nsIAtom*& aPseudoTag) const = 0;

  NS_IMETHOD FindChildWithRules(const nsIAtom* aPseudoTag, 
                                nsIRuleNode* aRules,
                                nsIStyleContext*& aResult) = 0;

  virtual PRBool HasTextDecorations()=0;

   
  NS_IMETHOD GetBorderPaddingFor(nsStyleBorderPadding& aBorderPadding)=0;

  // compute the effective difference between two contexts
  NS_IMETHOD CalcStyleDifference(nsIStyleContext* aOther, PRInt32& aHint, PRBool aStopAtFirst = PR_FALSE) = 0;

  NS_IMETHOD GetRuleNode(nsIRuleNode** aResult)=0;
  NS_IMETHOD AddStyleBit(const PRUint32& aBit)=0;
  NS_IMETHOD GetStyleBits(PRUint32* aBits)=0;

  // debugging
  virtual void  List(FILE* out, PRInt32 aIndent) = 0;
  virtual void SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize) = 0;
#ifdef DEBUG
  virtual void DumpRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent)=0;
#endif

  // XXXdwh Make this function private.  It should not be possible to call this
  // function from layout.
  // Fill a style struct with data.  
  NS_IMETHOD SetStyle(nsStyleStructID aSID, const nsStyleStruct& aStruct) = 0;
  
  // Get the data for a style struct.
  NS_IMETHOD GetStyle(nsStyleStructID aSID, const nsStyleStruct** aStruct) = 0;

  virtual const nsStyleStruct* GetStyleData(nsStyleStructID aSID) = 0;

  // Avoid using this method at all costs.  It defeats sharing of style data, forcing the context
  // to have its own unique copy.  At the moment the only supported structs are text, background, and
  // display.  Don't add support for new structs or use this method without careful consideration! -dwh
  virtual nsStyleStruct* GetUniqueStyleData(nsIPresContext* aPresContext, const nsStyleStructID& aSID) = 0;

  // Used to clear away the style data for a given style context if it matches the specified |aRule|.
  // If |aRule| is null, then the style data is always blown away.
  virtual nsresult ClearStyleData(nsIPresContext* aPresContext, nsIStyleRule* aRule) = 0;
 
  // call this to prevent context from getting shared
  virtual void  ForceUnique(void) = 0;
};


// this is private to nsStyleSet, don't call it
extern NS_LAYOUT nsresult
  NS_NewStyleContext(nsIStyleContext** aInstancePtrResult,
                     nsIStyleContext* aParentContext,
                     nsIAtom* aPseudoType,
                     nsIRuleNode* aRuleNode,
                     nsIPresContext* aPresContext);


#endif /* nsIStyleContext_h___ */
