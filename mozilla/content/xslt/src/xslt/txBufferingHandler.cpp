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
 * The Original Code is TransforMiiX XSLT processor code.
 *
 * The Initial Developer of the Original Code is
 * Jonas Sicking.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <jonas@sicking.cc>
 *   Peter Van der Beken <peterv@propagandism.org>
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

#include "txBufferingHandler.h"

class txOutputTransaction
{
public:
    enum txTransactionType {
        eAttributeTransaction,
        eCharacterTransaction,
        eCharacterNoOETransaction,
        eCommentTransaction,
        eEndDocumentTransaction,
        eEndElementTransaction,
        ePITransaction,
        eStartDocumentTransaction,
        eStartElementTransaction
    };
    txOutputTransaction(txTransactionType aType)
        : mType(aType)
    {
    }
    virtual ~txOutputTransaction()
    {
    }
    txTransactionType mType;
};

class txCharacterTransaction : public txOutputTransaction
{
public:
    txCharacterTransaction(txTransactionType aType, PRUint32 aLength)
        : txOutputTransaction(aType),
          mLength(aLength)
    {
    }
    PRUint32 mLength;
};

class txCommentTransaction : public txOutputTransaction
{
public:
    txCommentTransaction(const nsAString& aValue)
        : txOutputTransaction(eCommentTransaction),
          mValue(aValue)
    {
    }
    nsString mValue;
};

class txPITransaction : public txOutputTransaction
{
public:
    txPITransaction(const nsAString& aTarget, const nsAString& aData)
        : txOutputTransaction(ePITransaction),
          mTarget(aTarget),
          mData(aData)
    {
    }
    nsString mTarget;
    nsString mData;
};

class txElementTransaction : public txOutputTransaction
{
public:
    txElementTransaction(txTransactionType aType, const nsAString& aName,
                         PRInt32 aNsID)
        : txOutputTransaction(aType),
          mName(aName),
          mNsID(aNsID)
    {
    }
    nsString mName;
    PRInt32 mNsID;
};

class txAttributeTransaction : public txOutputTransaction
{
public:
    txAttributeTransaction(const nsAString& aName, PRInt32 aNsID,
                           const nsAString& aValue)
        : txOutputTransaction(eAttributeTransaction),
          mName(aName),
          mNsID(aNsID),
          mValue(aValue)
    {
    }
    nsString mName;
    PRInt32 mNsID;
    nsString mValue;
};

txBufferingHandler::txBufferingHandler() : mCanAddAttribute(PR_FALSE)
{
    mBuffer = new txResultBuffer();
}

txBufferingHandler::~txBufferingHandler()
{
}

nsresult
txBufferingHandler::attribute(const nsAString& aName, const PRInt32 aNsID,
                              const nsAString& aValue)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    if (!mCanAddAttribute) {
        // XXX ErrorReport: Can't add attributes without element
        return NS_OK;
    }

    txOutputTransaction* transaction =
        new txAttributeTransaction(aName, aNsID, aValue);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::characters(const nsAString& aData, PRBool aDOE)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    mCanAddAttribute = PR_FALSE;

    txOutputTransaction::txTransactionType type =
         aDOE ? txOutputTransaction::eCharacterNoOETransaction
              : txOutputTransaction::eCharacterTransaction;

    txOutputTransaction* transaction = mBuffer->getLastTransaction();
    if (transaction && transaction->mType == type) {
        mBuffer->mStringValue.Append(aData);
        NS_STATIC_CAST(txCharacterTransaction*, transaction)->mLength +=
            aData.Length();
        return NS_OK;
    }

    transaction = new txCharacterTransaction(type, aData.Length());
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    mBuffer->mStringValue.Append(aData);
    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::comment(const nsAString& aData)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    mCanAddAttribute = PR_FALSE;

    txOutputTransaction* transaction = new txCommentTransaction(aData);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::endDocument(nsresult aResult)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    txOutputTransaction* transaction =
        new txOutputTransaction(txOutputTransaction::eEndDocumentTransaction);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::endElement(const nsAString& aName, const PRInt32 aNsID)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    mCanAddAttribute = PR_FALSE;

    txOutputTransaction* transaction =
        new txElementTransaction(txOutputTransaction::eEndElementTransaction,
                                 aName, aNsID);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::processingInstruction(const nsAString& aTarget,
                                          const nsAString& aData)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    mCanAddAttribute = PR_FALSE;

    txOutputTransaction* transaction =
        new txPITransaction(aTarget, aData);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::startDocument()
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    txOutputTransaction* transaction =
        new txOutputTransaction(txOutputTransaction::eStartDocumentTransaction);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

nsresult
txBufferingHandler::startElement(const nsAString& aName, const PRInt32 aNsID)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_OUT_OF_MEMORY);

    mCanAddAttribute = PR_TRUE;

    txOutputTransaction* transaction =
        new txElementTransaction(txOutputTransaction::eStartElementTransaction,
                                 aName, aNsID);
    NS_ENSURE_TRUE(transaction, NS_ERROR_OUT_OF_MEMORY);

    return mBuffer->addTransaction(transaction);
}

PR_STATIC_CALLBACK(PRBool)
deleteTransaction(void* aElement, void *aData)
{
    delete NS_STATIC_CAST(txOutputTransaction*, aElement);
    return PR_TRUE;
}

txResultBuffer::~txResultBuffer()
{
    mTransactions.EnumerateForwards(deleteTransaction, nsnull);
}

nsresult
txResultBuffer::addTransaction(txOutputTransaction* aTransaction)
{
    if (!mTransactions.AppendElement(aTransaction)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

struct Holder
{
    txAXMLEventHandler* mHandler;
    nsAFlatString::const_char_iterator mIter;
    nsresult mResult;
};

PR_STATIC_CALLBACK(PRBool)
flushTransaction(void* aElement, void *aData)
{
    Holder* holder = NS_STATIC_CAST(Holder*, aData);
    txAXMLEventHandler* handler = holder->mHandler;
    txOutputTransaction* transaction =
        NS_STATIC_CAST(txOutputTransaction*, aElement);

    nsresult rv;
    switch (transaction->mType) {
        case txOutputTransaction::eAttributeTransaction:
        {
            txAttributeTransaction* attrTransaction =
                NS_STATIC_CAST(txAttributeTransaction*, aElement);
            rv = handler->attribute(attrTransaction->mName,
                                    attrTransaction->mNsID,
                                    attrTransaction->mValue);
            break;
        }
        case txOutputTransaction::eCharacterTransaction:
        case txOutputTransaction::eCharacterNoOETransaction:
        {
            txCharacterTransaction* charTransaction =
                NS_STATIC_CAST(txCharacterTransaction*, aElement);
            nsAFlatString::const_char_iterator& start =
                holder->mIter;
            nsAFlatString::const_char_iterator end =
                start + charTransaction->mLength;
            rv = handler->characters(Substring(start, end),
                                     transaction->mType ==
                                     txOutputTransaction::eCharacterNoOETransaction);
            start = end;
            break;
        }
        case txOutputTransaction::eCommentTransaction:
        {
            txCommentTransaction* commentTransaction =
                NS_STATIC_CAST(txCommentTransaction*, aElement);
            rv = handler->comment(commentTransaction->mValue);
            break;
        }
        case txOutputTransaction::eEndElementTransaction:
        {
            txElementTransaction* elementTransaction =
                NS_STATIC_CAST(txElementTransaction*, aElement);
            rv = handler->endElement(elementTransaction->mName,
                                     elementTransaction->mNsID);
            break;
        }
        case txOutputTransaction::ePITransaction:
        {
            txPITransaction* piTransaction =
                NS_STATIC_CAST(txPITransaction*, aElement);
            rv = handler->processingInstruction(piTransaction->mTarget,
                                                piTransaction->mData);
            break;
        }
        case txOutputTransaction::eStartDocumentTransaction:
        {
            rv = handler->startDocument();
            break;
        }
        case txOutputTransaction::eStartElementTransaction:
        {
            txElementTransaction* elementTransaction =
                NS_STATIC_CAST(txElementTransaction*, aElement);
            rv = handler->startElement(elementTransaction->mName,
                                       elementTransaction->mNsID);
            break;
        }
    }

    holder->mResult = rv;

    return NS_SUCCEEDED(rv);
}

nsresult
txResultBuffer::flushToHandler(txAXMLEventHandler* aHandler)
{
    Holder data;
    data.mHandler = aHandler;
    mStringValue.BeginReading(data.mIter);
    data.mResult = NS_OK;

    mTransactions.EnumerateForwards(flushTransaction, &data);

    return data.mResult;
}

txOutputTransaction*
txResultBuffer::getLastTransaction()
{
    PRInt32 last = mTransactions.Count() - 1;
    if (last < 0) {
        return nsnull;
    }
    return NS_STATIC_CAST(txOutputTransaction*, mTransactions[last]);
}
