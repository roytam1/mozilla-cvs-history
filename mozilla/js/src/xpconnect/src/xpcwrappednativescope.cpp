/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   John Bandhauer <jband@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* Class used to manage the wrapped native objects within a JS scope. */

#include "xpcprivate.h"

nsXPCWrappedNativeScope* nsXPCWrappedNativeScope::gScopes = nsnull;

NS_IMPL_ISUPPORTS1(nsXPCWrappedNativeScope, nsIXPCWrappedNativeScope)

nsXPCWrappedNativeScope::nsXPCWrappedNativeScope(XPCContext* xpcc,
                                                 nsXPCComponents* comp)
    :   mRuntime(xpcc->GetRuntime()),
        mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mComponents(comp),
        mNext(nsnull)
{
    NS_INIT_ISUPPORTS();
    NS_IF_ADDREF(mComponents);
    // add ourselves to the scopes list
    {   // scoped lock
        nsAutoLock lock(mRuntime->GetMapLock());  
        mNext = gScopes;
        gScopes = this;
    }
}        

nsXPCWrappedNativeScope::~nsXPCWrappedNativeScope()
{
    if(mWrappedNativeMap)
    {
        NS_ASSERTION(0 == mWrappedNativeMap->Count(), "scope has non-empty map");
        delete mWrappedNativeMap;    
    }

    // remove ourselves from the scopes list
    {   // scoped lock
        nsAutoLock lock(mRuntime->GetMapLock());  
        if(gScopes == this)
            gScopes = mNext;
        else
        {
            nsXPCWrappedNativeScope* cur = gScopes;
            while(cur && cur->mNext)
            {
                if(cur->mNext == this)
                {
                    cur->mNext = mNext;
                    break;                            
                }
                cur = cur->mNext;
            }
        }
    }
    NS_IF_RELEASE(mComponents);
}        

// static 
nsXPCWrappedNativeScope* 
nsXPCWrappedNativeScope::FindInJSObjectScope(XPCContext* xpcc, JSObject* obj)
{
    jsval prop;
    JSObject* compobj;
    JSClass* clazz;
    JSContext* cx = xpcc->GetJSContext();
    jsid id = xpcc->GetRuntime()->GetStringID(XPCJSRuntime::IDX_COMPONENTS);
    nsISupports* supports;
    nsresult rv;
 
    if(!obj)
        return nsnull;
    
    // XXX this parent walk is probably unnecessary
    JSObject* parent;
    while(nsnull != (parent = JS_GetParent(cx, obj)))
        obj = parent;

    if(!OBJ_GET_PROPERTY(cx, obj, id, &prop) ||
       JSVAL_IS_PRIMITIVE(prop) ||
       !(compobj = JSVAL_TO_OBJECT(prop)))
        return nsnull;

#ifdef JS_THREADSAFE
    clazz = JS_GetClass(cx, compobj);
#else
    clazz = JS_GetClass(compobj);
#endif

    if(!clazz ||
       !(clazz->flags & JSCLASS_HAS_PRIVATE) ||
       !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
       !(supports = (nsISupports*) JS_GetPrivate(cx, compobj)))
        return nsnull;

    nsCOMPtr<nsIXPConnectWrappedNative> wrapper_iface;
    rv = supports->QueryInterface(NS_GET_IID(nsIXPConnectWrappedNative),
                                  getter_AddRefs(wrapper_iface));
    if(NS_FAILED(rv))
        return nsnull;

    // We can fairly safely assume that this is really one of our
    // nsXPConnectWrappedNative objects. No other component in our
    // universe should be creating objects that implement the
    // nsIXPConnectWrappedNative interface!

    return ((nsXPCWrappedNative*)supports)->GetScope();
}        

// static 
void
nsXPCWrappedNativeScope::DebugDumpAllScopes(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;

    // get scope count.
    int count = 0;
    nsXPCWrappedNativeScope* cur;
    for(cur = gScopes; cur; cur = cur->mNext)
        count++ ;

    XPC_LOG_ALWAYS(("chain of %d nsXPCWrappedNativeScope(s)", count));
    XPC_LOG_INDENT();
        for(cur = gScopes; cur; cur = cur->mNext)
            cur->DebugDump(depth);
    XPC_LOG_OUTDENT();
#endif
}        

#ifdef DEBUG
JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeMapDumpEnumerator(JSHashEntry *he, intN i, void *arg)
{
    ((nsXPCWrappedNative*)he->value)->DebugDump(*(PRInt16*)arg);
    return HT_ENUMERATE_NEXT;
}
#endif

NS_IMETHODIMP 
nsXPCWrappedNativeScope::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("nsXPCWrappedNativeScope @ %x with mRefCnt = %d", this, mRefCnt));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mNext @ %x", mNext));

        XPC_LOG_ALWAYS(("mWrappedNativeMap @ %x with %d wrappers(s)", \
                         mWrappedNativeMap, \
                         mWrappedNativeMap ? mWrappedNativeMap->Count() : 0));
        // iterate contexts...
        if(depth && mWrappedNativeMap && mWrappedNativeMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedNativeMap->Enumerate(WrappedNativeMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}        

