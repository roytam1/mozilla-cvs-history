/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
package netscape.ldap;

import java.util.*;
import java.io.*;
import java.net.*;

/**
 * Represents a socket connection that you can use to connect to an
 * LDAP server.  You can write a class that implements this interface
 * if you want to use a TLS socket to connect to a secure server.
 * (The <CODE>LDAPSSLSocketFactory class</CODE>, which is included
 * in the <CODE>netscape.ldap</CODE> package, implements this
 * interface for SSL connections.)
 * <P>
 *
 * When you construct a new <CODE>LDAPConnection</CODE>
 * object, you can specify that the connection uses his socket by
 * passing the constructor an object of the class that implements
 * this interface.
 * <P>
 *
 * @version 1.0
 * @see LDAPConnection#LDAPConnection(netscape.ldap.LDAPSocketFactory)
 * @see LDAPSSLSocketFactory
 */
public interface LDAPSocketFactory {
    /**
     * Returns a socket to the specified host name and port number.
     * <P>
     *
     * @param host Name of the host that you want to connect to.
     * @param port Port number that you want to connect to.
     * @exception LDAPException Failed to create the socket.
     * @see LDAPSSLSocketFactory#makeSocket(java.lang.String,int)
     */
    public Socket makeSocket(String host, int port)
        throws LDAPException;
}
