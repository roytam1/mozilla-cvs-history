/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is TransforMiiX XSLT Processor.
 *
 * The Initial Developer of the Original Code is
 * Axel Hecht.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Axel Hecht <axel@pike.org>
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

#ifndef __TX_I_XPATH_CONTEXT
#define __TX_I_XPATH_CONTEXT

#include "Expr.h"
#include "txError.h"

enum txErrorLevel
{
    txLevelError,
    txLevelWarning,
    txLevelFatal
};

class txIParseContext
{
public:
    virtual ~txIParseContext() = 0;
    virtual nsresult resolveNamespacePrefix(txAtom* aPrefix, PRInt32& aID) = 0;
    virtual nsresult resolveFunctionCall(txAtom* aName, PRInt32 aID,
                                         FunctionCall*& aFunction) = 0;
    virtual void receiveError(const String& aMsg, txErrorLevel aLevel) = 0;
};

class txIMatchContext
{
public:
    virtual ~txIMatchContext() = 0;
    virtual nsresult getVariable(PRInt32 aNamespace, txAtom* aLName,
                                 ExprResult*& aResult) = 0;
    virtual MBool isStripSpaceAllowed(Node* aNode) = 0;
    virtual void receiveError(const String& aMsg, txErrorLevel aLevel) = 0;
};

#define TX_DECL_MATCH_CONTEXT \
    nsresult getVariable(PRInt32 aNamespace, txAtom* aLName, \
                         ExprResult*& aResult); \
    MBool isStripSpaceAllowed(Node* aNode); \
    void receiveError(const String& aMsg, txErrorLevel aLevel)

class txIEvalContext : public txIMatchContext
{
public:
    virtual ~txIEvalContext() = 0;
    virtual Node* getContextNode() = 0;
    virtual PRUint32 size() = 0;
    virtual PRUint32 position() = 0;
    virtual NodeSet* getContextNodeSet() = 0;
};

#define TX_DECL_EVAL_CONTEXT \
    TX_DECL_MATCH_CONTEXT; \
    Node* getContextNode(); \
    PRUint32 size(); \
    PRUint32 position(); \
    NodeSet* getContextNodeSet()

#endif // __TX_I_XPATH_CONTEXT
