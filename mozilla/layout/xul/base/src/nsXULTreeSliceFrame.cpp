/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsXULTreeSliceFrame.h"

//
// NS_NewXULTreeSliceFrame
//
// Creates a new TreeSlice frame
//
nsresult
NS_NewXULTreeSliceFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRBool aIsRoot, 
                        nsIBoxLayout* aLayoutManager, PRBool aIsHorizontal)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsXULTreeSliceFrame* it = new (aPresShell) nsXULTreeSliceFrame(aPresShell, aIsRoot, aLayoutManager, aIsHorizontal);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewXULTreeSliceFrame


// Constructor
nsXULTreeSliceFrame::nsXULTreeSliceFrame(nsIPresShell* aPresShell, PRBool aIsRoot, nsIBoxLayout* aLayoutManager, PRBool aIsHorizontal)
:nsBoxFrame(aPresShell, aIsRoot, aLayoutManager, aIsHorizontal) 
{}

// Destructor
nsXULTreeSliceFrame::~nsXULTreeSliceFrame()
{
}
