/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Peter Van der Beken, peterv@netscape.com
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsNodeSet.h"
#include "nsIDOMClassInfo.h"

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,  NS_DOM_SCRIPT_OBJECT_FACTORY_CID);


// QueryInterface implementation for nsNodeSet
NS_INTERFACE_MAP_BEGIN(nsNodeSet)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNodeList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(NodeSet)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsNodeSet)
NS_IMPL_RELEASE(nsNodeSet)


nsNodeSet::nsNodeSet(NodeSet* aNodeSet) {
    NS_INIT_ISUPPORTS();
    mScriptObject = nsnull;

    if (aNodeSet) {
        for (int i=0; i < aNodeSet->size(); i++) {
            mNodes.AppendElement(aNodeSet->get(i)->getNSObj());
        }
    }
}

nsNodeSet::~nsNodeSet() {
}

NS_IMETHODIMP
nsNodeSet::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
    *aReturn = nsnull;
    mNodes.QueryElementAt(aIndex, NS_GET_IID(nsIDOMNode), (void**)aReturn);
    return NS_OK;
}

NS_IMETHODIMP
nsNodeSet::GetLength(PRUint32* aLength)
{
    mNodes.Count(aLength);
    return NS_OK;
}
