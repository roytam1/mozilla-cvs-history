/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 * 
 * Contributor(s): 
 */


#ifndef nsQtEventProcessor_h__
#define nsQtEventProcessor_h__

#include <qobject.h>
#include <qsocketnotifier.h>

#include "nsIEventQueue.h"

class nsQtEventProcessor : public QSocketNotifier
{
	Q_OBJECT
public:
	nsQtEventProcessor( nsIEventQueue* eventQueue, QObject* parent );
	~nsQtEventProcessor( );

private slots:
	void handleActivated( int socket );

	nsIEventQueue*	m_eventQueue;
};
#endif // nsQtEventProcessor_h__
