/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* Implementation of nsIInterfaceInfo. */

#include "nscore.h"

#include "nsISupports.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIAllocator.h"

#include "nsInterfaceInfo.h"
#include "nsInterfaceInfoManager.h"
#include "xptinfo.h"

NS_IMPL_ISUPPORTS1(nsInterfaceInfo, nsIInterfaceInfo)

nsInterfaceInfo::nsInterfaceInfo(nsInterfaceRecord *record,
                                 nsInterfaceInfo *parent)
    :   mInterfaceRecord(record),
        mParent(parent)
{
    NS_INIT_REFCNT();
    NS_ADDREF_THIS();

    if(mParent != NULL) {
        NS_ADDREF(mParent);
        mMethodBaseIndex =
            mParent->mMethodBaseIndex + mParent->mMethodCount;
        mConstantBaseIndex =
            mParent->mConstantBaseIndex + mParent->mConstantCount;
    } else {
        mMethodBaseIndex = mConstantBaseIndex = 0;
    }

    mMethodCount   =
        mInterfaceRecord->interfaceDescriptor->num_methods;
    mConstantCount =
        mInterfaceRecord->interfaceDescriptor->num_constants;
}

nsInterfaceInfo::~nsInterfaceInfo()
{
    if (this->mParent != NULL)
        NS_RELEASE(mParent);

    // remove interface record's notion of my existence
    mInterfaceRecord->info = NULL;
}

NS_IMETHODIMP
nsInterfaceInfo::GetName(char **name)
{
    return this->mInterfaceRecord->GetName(name);
}

NS_IMETHODIMP
nsInterfaceInfo::GetIID(nsIID **iid)
{
    return this->mInterfaceRecord->GetIID(iid);
}

NS_IMETHODIMP
nsInterfaceInfo::IsScriptable(PRBool* result)
{
    if(!result)
        return NS_ERROR_NULL_POINTER;
    *result = XPT_ID_IS_SCRIPTABLE(this->mInterfaceRecord->
                                                interfaceDescriptor->flags);
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetParent(nsIInterfaceInfo** parent)
{
    NS_PRECONDITION(parent, "bad param");
    if(mParent) {
        NS_ADDREF(mParent);
        *parent = mParent;
        return NS_OK;
    }
    *parent = NULL;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInterfaceInfo::GetMethodCount(uint16* count)
{
    NS_PRECONDITION(count, "bad param");

    *count = mMethodBaseIndex + mMethodCount;
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetConstantCount(uint16* count)
{
    NS_PRECONDITION(count, "bad param");
    *count = mConstantBaseIndex + mConstantCount;
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetMethodInfo(uint16 index, const nsXPTMethodInfo** info)
{
    NS_PRECONDITION(info, "bad param");
    if (index < mMethodBaseIndex)
        return mParent->GetMethodInfo(index, info);

    if (index >= mMethodBaseIndex + mMethodCount) {
        NS_PRECONDITION(0, "bad param");
        *info = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    // else...
    *info = NS_REINTERPRET_CAST(nsXPTMethodInfo*,
                                &mInterfaceRecord->interfaceDescriptor->
                                method_descriptors[index - mMethodBaseIndex]);
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetMethodInfoForName(const char* methodName, uint16 *index,
                                      const nsXPTMethodInfo** result)
{
    NS_PRECONDITION(methodName, "bad param");
    NS_PRECONDITION(index, "bad param");
    NS_PRECONDITION(result, "bad param");

    // XXX probably want to speed this up with a hashtable, or by at least interning
    // the names to avoid the strcmp
    for (uint16 i = mMethodBaseIndex; i < mMethodCount; i++) {
        const nsXPTMethodInfo* info;
        info = NS_REINTERPRET_CAST(nsXPTMethodInfo*,
                                   &mInterfaceRecord->interfaceDescriptor->
                                   method_descriptors[i - mMethodBaseIndex]);
        if (PL_strcmp(methodName, info->name) == 0) {
#ifdef NS_DEBUG
            // XXX this isn't really the best place for this since it
            // does not deal with duplicates accross inheritence boundaries
            //
            // make sure there aren't duplicate names
            for (; i < mMethodCount; i++) {
                const nsXPTMethodInfo* info2;
                info2 = NS_REINTERPRET_CAST(nsXPTMethodInfo*,
                                           &mInterfaceRecord->interfaceDescriptor->
                                           method_descriptors[i - mMethodBaseIndex]);
                NS_ASSERTION(PL_strcmp(methodName, info2->name)!= 0, "duplicate names");
            }
#endif
            *index = i;
            *result = info;
            return NS_OK;
        }
    }
    if(mParent)
        return mParent->GetMethodInfoForName(methodName, index, result);
    else
        return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsInterfaceInfo::GetConstant(uint16 index, const nsXPTConstant** constant)
{
    NS_PRECONDITION(constant, "bad param");

    if (index < mConstantBaseIndex)
        return mParent->GetConstant(index, constant);

    if (index >= mConstantBaseIndex + mConstantCount) {
        NS_PRECONDITION(0, "bad param");
        *constant = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    // else...
    *constant =
        NS_REINTERPRET_CAST(nsXPTConstant*,
                            &mInterfaceRecord->interfaceDescriptor->
                            const_descriptors[index-mConstantBaseIndex]);
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetInfoForParam(uint16 methodIndex,
                                 const nsXPTParamInfo *param,
                                 nsIInterfaceInfo** info)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(info, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetInfoForParam(methodIndex, param, info);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_PRECONDITION(0, "bad param");
        *info = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td = &param->type;

    while (XPT_TDP_TAG(td->prefix) == TD_ARRAY) {
        td = &this->mInterfaceRecord->interfaceDescriptor->
                                additional_types[td->type.additional_type];
    }

    if (XPT_TDP_TAG(td->prefix) != TD_INTERFACE_TYPE) {
        NS_ASSERTION(0, "not an interface");
        return NS_ERROR_INVALID_ARG;
    }

    nsInterfaceRecord *paramRecord =
        *(this->mInterfaceRecord->typelibRecord->
          interfaceRecords + (td->type.interface - 1));

    return paramRecord->GetInfo((nsInterfaceInfo **)info);
}

NS_IMETHODIMP
nsInterfaceInfo::GetIIDForParam(uint16 methodIndex,
                                const nsXPTParamInfo* param, nsIID** iid)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(iid, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetIIDForParam(methodIndex, param, iid);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_PRECONDITION(0, "bad param");
        *iid = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td = &param->type;

    while (XPT_TDP_TAG(td->prefix) == TD_ARRAY) {
        td = &this->mInterfaceRecord->interfaceDescriptor->
                                additional_types[td->type.additional_type];
    }

    if (XPT_TDP_TAG(td->prefix) != TD_INTERFACE_TYPE) {
        NS_ASSERTION(0, "not an interface");
        return NS_ERROR_INVALID_ARG;
    }

    nsInterfaceRecord *paramRecord =
        *(this->mInterfaceRecord->typelibRecord->
          interfaceRecords + (td->type.interface - 1));

    return paramRecord->GetIID(iid);
}

// this is a private helper
NS_IMETHODIMP
nsInterfaceInfo::GetTypeInArray(const nsXPTParamInfo* param,
                                uint16 dimension,
                                const XPTTypeDescriptor** type)
{
    const XPTTypeDescriptor *td = &param->type;
    const XPTTypeDescriptor *additional_types =
                this->mInterfaceRecord->interfaceDescriptor->additional_types;

    for (uint16 i = 0; i < dimension; i++) {
        if (XPT_TDP_TAG(td->prefix) != TD_ARRAY) {
            NS_ASSERTION(0, "bad dimension");
            return NS_ERROR_INVALID_ARG;
        }
        td = &additional_types[td->type.additional_type];
    }

    *type = td;
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetTypeForParam(uint16 methodIndex,
                                 const nsXPTParamInfo* param,
                                 uint16 dimension,
                                 nsXPTType* type)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(type, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetTypeForParam(methodIndex, param, dimension, type);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_ASSERTION(0, "bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if (dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if (NS_FAILED(rv))
            return rv;
    }
    else
        td = &param->type;

    *type = nsXPTType(td->prefix);
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetSizeIsArgNumberForParam(uint16 methodIndex,
                                            const nsXPTParamInfo* param,
                                            uint16 dimension,
                                            uint8* argnum)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(argnum, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetSizeIsArgNumberForParam(methodIndex, param,
                                                   dimension, argnum);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_ASSERTION(0, "bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if (dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if (NS_FAILED(rv))
            return rv;
    }
    else
        td = &param->type;

    // verify that this is a type that has size_is
    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_ARRAY:
      case TD_PSTRING_SIZE_IS:
      case TD_PWSTRING_SIZE_IS:
        break;
      default:
        NS_ASSERTION(0, "not a size_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum;
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetLengthIsArgNumberForParam(uint16 methodIndex,
                                              const nsXPTParamInfo* param,
                                              uint16 dimension,
                                              uint8* argnum)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(argnum, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetLengthIsArgNumberForParam(methodIndex, param,
                                                     dimension, argnum);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_ASSERTION(0, "bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if (dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if (NS_FAILED(rv)) {
            return rv;
        }
    }
    else
        td = &param->type;

    // verify that this is a type that has length_is
    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_ARRAY:
      case TD_PSTRING_SIZE_IS:
      case TD_PWSTRING_SIZE_IS:
        break;
      default:
        NS_ASSERTION(0, "not a length_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum2;
    return NS_OK;
}

NS_IMETHODIMP
nsInterfaceInfo::GetInterfaceIsArgNumberForParam(uint16 methodIndex,
                                                 const nsXPTParamInfo* param,
                                                 uint8* argnum)
{
    NS_PRECONDITION(param, "bad pointer");
    NS_PRECONDITION(argnum, "bad pointer");

    if (methodIndex < mMethodBaseIndex)
        return mParent->GetInterfaceIsArgNumberForParam(methodIndex, param,
                                                        argnum);

    if (methodIndex >= mMethodBaseIndex + mMethodCount) {
        NS_ASSERTION(0, "bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td = &param->type;

    while (XPT_TDP_TAG(td->prefix) == TD_ARRAY) {
        td = &this->mInterfaceRecord->interfaceDescriptor->
                                additional_types[td->type.additional_type];
    }

    if (XPT_TDP_TAG(td->prefix) != TD_INTERFACE_IS_TYPE) {
        NS_ASSERTION(0, "not an iid_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum;
    return NS_OK;
}


#ifdef DEBUG
#include <stdio.h>
void
nsInterfaceInfo::print(FILE *fd)
{
    if (this == NULL) {
        fprintf(fd, "No record!!\n");
        return;
    }

    const char *name_space = this->mInterfaceRecord->name_space;

    fprintf(fd, "iid: %s name: %s name_space: %s\n",
            this->mInterfaceRecord->iid.ToString(),
            this->mInterfaceRecord->name,
            (name_space) ? name_space : "(none)");
    if (mParent != NULL) {
        fprintf(fd, "parent:\n\t");
        this->mParent->print(fd);
    }
}
#endif
