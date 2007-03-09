/*
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
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Ed Burns <edburns@acm.org>
 */

#ifndef __AjaxListener_h__
#define __AjaxListener_h__


#include "nsIObserver.h"
#include "nsIDOMEventListener.h"

#include "jni.h"

class EmbedProgress;
class nsIXMLHttpRequest;
class nsIHttpChannel;
class nsIRequest;

class AjaxListener : public nsIObserver, public nsIDOMEventListener
{
public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSIOBSERVER
   NS_DECL_NSIDOMEVENTLISTENER

   enum AJAX_STATE {
       AJAX_START,
       AJAX_END,
       AJAX_ERROR
   };
 
   AjaxListener(EmbedProgress *owner, 
		JNIEnv *env, jobject eventRegistration);
   virtual ~AjaxListener();

    NS_IMETHOD StartObserving(void);
    NS_IMETHOD StopObserving(void);

private:

    NS_IMETHOD ObserveAjax(nsIRequest *request,
			   nsIHttpChannel *channel,
			   nsIXMLHttpRequest *ajax,
			   AJAX_STATE state);

    EmbedProgress* mOwner;
    JNIEnv *mJNIEnv;
    jobject mEventRegistration;
    PRBool mIsObserving;
 };

#endif
