/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is mozilla.org code
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *   Robert Ginda, <rginda@netscape.com>
 *
 */

#ifndef JSDSERVICE_H___
#define JSDSERVICE_H___

#include "jsdIDebuggerService.h"
#include "jsdebug.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#if defined(DEBUG_rginda_l)
#define DEBUG_verbose
#endif

#ifdef DEBUG_verbose
extern PRUint32 gScriptCount;
extern PRUint32 gValueCount;
#endif

/*******************************************************************************
 * reflected jsd data structures
 *******************************************************************************/

class jsdPC : public jsdIPC
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIPC

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdPC (jsuword aPC) : mPC(aPC)
    {
        NS_INIT_ISUPPORTS();
    }

    static jsdIPC *FromPtr (jsuword aPC)
    {
        if (!aPC)
            return 0;
        
        jsdIPC *rv = new jsdPC (aPC);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdPC(); /* no implementation */
    jsdPC(const jsdPC&); /* no implementation */

    jsuword  mPC;
};

class jsdObject : public jsdIObject
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIOBJECT

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdObject (JSDContext *aCx, JSDObject *aObject) :
        mCx(aCx), mObject(aObject)
    {
        NS_INIT_ISUPPORTS();
    }

    static jsdIObject *FromPtr (JSDContext *aCx,
                                JSDObject *aObject)
    {
        if (!aObject)
            return 0;
        
        jsdIObject *rv = new jsdObject (aCx, aObject);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdObject(); /* no implementation */
    jsdObject(const jsdObject&); /* no implementation */

    JSDContext *mCx;
    JSDObject *mObject;
};


class jsdProperty : public jsdIProperty
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIPROPERTY

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdProperty (JSDContext *aCx, JSDProperty *aProperty) :
        mCx(aCx), mProperty(aProperty)
    {
        NS_INIT_ISUPPORTS();
    }

    static jsdIProperty *FromPtr (JSDContext *aCx,
                                  JSDProperty *aProperty)
    {
        if (!aProperty)
            return 0;
        
        jsdIProperty *rv = new jsdProperty (aCx, aProperty);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdProperty(); /* no implementation */
    jsdProperty(const jsdProperty&); /* no implementation */

    JSDContext  *mCx;
    JSDProperty *mProperty;
};

class jsdScript : public jsdIScript
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDISCRIPT

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdScript (JSDContext *aCx, JSDScript *aScript) : mValid(PR_FALSE), mCx(aCx),
                                                      mScript(aScript),
                                                      mFileName(0), 
                                                      mFunctionName(0),
                                                      mBaseLineNumber(0),
                                                      mLineExtent(0)
    {
        NS_INIT_ISUPPORTS();
#ifdef DEBUG_verbose
        printf ("++++++ jsdScript %i\n", ++gScriptCount);
#endif

        if (mScript)
        {
            /* copy the script's information now, so we have it later, when it
             * get's destroyed. */
            JSD_LockScriptSubsystem(mCx);
            mFileName = new nsCString(JSD_GetScriptFilename(mCx, mScript));
            mFunctionName =
                new nsCString(JSD_GetScriptFunctionName(mCx, mScript));
            mBaseLineNumber = JSD_GetScriptBaseLineNumber(mCx, mScript);
            mLineExtent = JSD_GetScriptLineExtent(mCx, mScript);
            JSD_UnlockScriptSubsystem(mCx);
            
            mValid = true;
        }
    }
    virtual ~jsdScript () 
    {
        if (mFileName)
            delete mFileName;
        if (mFunctionName)
            delete mFunctionName;
        
        /* Invalidate() needs to be called to release an owning reference to
         * ourselves, so if we got here without being invalidated, something
         * has gone wrong with our ref count */
        NS_ASSERTION (!mValid, "Script destroyed without being invalidated.");
        
#ifdef DEBUG_verbose
        printf ("------ jsdScript %i\n", --gScriptCount);
#endif
    }

    static jsdIScript *FromPtr (JSDContext *aCx, JSDScript *aScript)
    {
        if (!aScript)
            return 0;

        void *data = JSD_GetScriptPrivate (aScript);
        jsdIScript *rv;
        
        if (data) {
            rv = NS_STATIC_CAST(jsdIScript *, data);
        } else {
            rv = new jsdScript (aCx, aScript);
            NS_IF_ADDREF(rv);  /* addref for the SetScriptPrivate, released in
                                * Invalidate() */
            JSD_SetScriptPrivate (aScript, NS_STATIC_CAST(void *, rv));
        }
        
        NS_IF_ADDREF(rv); /* addref for return value */
        return rv;
    }

  private:
    jsdScript(); /* no implementation */
    jsdScript (const jsdScript&); /* no implementation */
    
    PRBool      mValid;
    JSDContext *mCx;
    JSDScript  *mScript;
    nsCString  *mFileName;
    nsCString  *mFunctionName;
    PRUint32    mBaseLineNumber, mLineExtent;
};

class jsdStackFrame : public jsdIStackFrame
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDISTACKFRAME

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdStackFrame (JSDContext *aCx, JSDThreadState *aThreadState,
                   JSDStackFrameInfo *aStackFrameInfo) :
        mCx(aCx), mThreadState(aThreadState), mStackFrameInfo(aStackFrameInfo)
    {
        NS_INIT_ISUPPORTS();
    }

    /* XXX These things are only valid for a short period of time, they reflect
     * state in the js engine that will go away after stepping past wherever
     * we were stopped at when this was created.  We could keep a list of every
     * instance of this we've created, and "invalidate" them before we let the
     * engine continue.  The next time we need a threadstate, we can search the
     * list to find an invalidated one, and just reuse it.
     */
    static jsdIStackFrame *FromPtr (JSDContext *aCx, 
                                    JSDThreadState *aThreadState,
                                    JSDStackFrameInfo *aStackFrameInfo)
    {
        if (!aStackFrameInfo)
            return 0;
        
        jsdIStackFrame *rv = new jsdStackFrame (aCx, aThreadState,
                                                aStackFrameInfo);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdStackFrame(); /* no implementation */
    jsdStackFrame(const jsdStackFrame&); /* no implementation */

    JSDContext        *mCx;
    JSDThreadState    *mThreadState;
    JSDStackFrameInfo *mStackFrameInfo;
};

class jsdValue : public jsdIValue
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIVALUE

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdValue (JSDContext *aCx, JSDValue *aValue) : mCx(aCx), mValue(aValue)
    {
#ifdef DEBUG_verbose
        printf ("++++++ jsdValue %i\n", ++gValueCount);
#endif
        NS_INIT_ISUPPORTS();
    }

    static jsdIValue *FromPtr (JSDContext *aCx, JSDValue *aValue)
    {
        if (!aValue)
            return 0;

        jsdIValue *rv;
        rv = new jsdValue (aCx, aValue);
        NS_IF_ADDREF(rv);
        return rv;
    }

    virtual ~jsdValue() 
    {
#ifdef DEBUG_verbose
        printf ("----- jsdValue %i\n", --gValueCount);
#endif
        JSD_DropValue (mCx, mValue);
    }
    
  private:
    jsdValue(); /* no implementation */
    jsdValue (const jsdScript&); /* no implementation */
    
    JSDContext *mCx;
    JSDValue  *mValue;
};

/******************************************************************************
 * debugger service
 ******************************************************************************/

class jsdService : public jsdIDebuggerService
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIDEBUGGERSERVICE

    jsdService() : mOn(PR_FALSE), mNestedLoopLevel(0), mCx(0), mRuntime(0),
                   mBreakpointHook(0), mErrorHook(0), mDebuggerHook(0),
                   mInterruptHook(0), mScriptHook(0), mThrowHook(0)
    {
        NS_INIT_ISUPPORTS();
    }

    virtual ~jsdService() { }
    
    static jsdService *GetService ();
    
  private:
    PRBool      mOn;
    PRUint32    mNestedLoopLevel;
    JSDContext *mCx;
    JSRuntime  *mRuntime;
    
    nsCOMPtr<jsdIExecutionHook> mBreakpointHook;
    nsCOMPtr<jsdIExecutionHook> mErrorHook;
    nsCOMPtr<jsdIExecutionHook> mDebuggerHook;
    nsCOMPtr<jsdIExecutionHook> mInterruptHook;
    nsCOMPtr<jsdIScriptHook>    mScriptHook;
    nsCOMPtr<jsdIExecutionHook> mThrowHook;
};

#endif /* JSDSERVICE_H___ */


/* graveyard */

#if 0

class jsdContext : public jsdIContext
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDICONTEXT

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdContext (JSDContext *aCx) : mCx(aCx)
    {
        NS_INIT_ISUPPORTS();
        printf ("++++++ jsdContext\n");
    }

    static jsdIContext *FromPtr (JSDContext *aCx)
    {
        if (!aCx)
            return 0;
        
        void *data = JSD_GetContextPrivate (aCx);
        jsdIContext *rv;
        
        if (data) {
            rv = NS_STATIC_CAST(jsdIContext *, data);
        } else {
            rv = new jsdContext (aCx);
            NS_IF_ADDREF(rv);  // addref for the SetContextPrivate
            JSD_SetContextPrivate (aCx, NS_STATIC_CAST(void *, rv));
        }
        
        NS_IF_ADDREF(rv); // addref for the return value
        return rv;
    }

    virtual ~jsdContext() { printf ("------ ~jsdContext\n"); }
  private:            
    jsdContext(); /* no implementation */
    jsdContext(const jsdContext&); /* no implementation */
    
    JSDContext *mCx;
};

class jsdThreadState : public jsdIThreadState
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDITHREADSTATE

    /* you'll normally use use FromPtr() instead of directly constructing one */
    jsdThreadState (JSDContext *aCx, JSDThreadState *aThreadState) :
        mCx(aCx), mThreadState(aThreadState)
    {
        NS_INIT_ISUPPORTS();
    }

    /* XXX These things are only valid for a short period of time, they reflect
     * state in the js engine that will go away after stepping past wherever
     * we were stopped at when this was created.  We could keep a list of every
     * instance of this we've created, and "invalidate" them before we let the
     * engine continue.  The next time we need a threadstate, we can search the
     * list to find an invalidated one, and just reuse it.
     */
    static jsdIThreadState *FromPtr (JSDContext *aCx,
                                     JSDThreadState *aThreadState)
    {
        if (!aThreadState)
            return 0;
        
        jsdIThreadState *rv = new jsdThreadState (aCx, aThreadState);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdThreadState(); /* no implementation */
    jsdThreadState(const jsdThreadState&); /* no implementation */

    JSDContext     *mCx;
    JSDThreadState *mThreadState;
};

#endif
