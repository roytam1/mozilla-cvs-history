/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Extension Manager.
 *
 * The Initial Developer of the Original Code is Ben Goodger.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ben Goodger <ben@bengoodger.com>
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

package org.mozilla.update.extensions;

import java.sql.*;
import java.util.*;

public class VersionCheck
{
  public VersionCheck()
  {
  }

  /*
  public static void main(String[] args) throws Exception 
  {
    VersionCheck impl = new VersionCheck();
    int id = impl.getNewestExtension("{bb8ee064-ccb9-47fc-94ae-ec335af3fe2d}", "3.0", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}", "0.8.0+");
    System.out.println("result = " + impl.getProperty(id, "xpiurl"));
  }
  */

  protected Connection getConnection() throws Exception
  {
    Class.forName("com.mysql.jdbc.Driver");
    return DriverManager.getConnection("jdbc:mysql://localhost/umo_extensions", "root", "");
  }

  public Extension getExtension(String aExtensionGUID, String aInstalledVersion, String aTargetApp, String aTargetAppVersion)
  {
    int id = getNewestExtension(aExtensionGUID, aInstalledVersion, aTargetApp, aTargetAppVersion);
    Extension e = new Extension();

    e.setRow(id);
    e.setId(getProperty(id, "id"));
    e.setVersion(getProperty(id, "version"));
    e.setName(getProperty(id, "name"));
    e.setXpiURL(getProperty(id, "xpiurl"));

    return e;
  }

  public String getProperty(int aRowID, String aProperty)
  {
    String result = null;
    try
    {
      Connection c = getConnection();
      Statement s = c.createStatement();

      String sql = "SELECT * FROM extensions WHERE id = '" + aRowID + "'";
      ResultSet rs = s.executeQuery(sql);

      result = rs.next() ? rs.getString(aProperty) : null;
      if (result == null)
        result = "query succeeded, but null!";
    }
    catch (Exception e)
    {
    }
    return result;
  }

  public int getNewestExtension(String aExtensionGUID, 
    String aInstalledVersion, 
    String aTargetApp, 
    String aTargetAppVersion)
  {
    int id = -1;
    int extensionVersionParts = getPartCount(aInstalledVersion);
    int targetAppVersionParts = getPartCount(aTargetAppVersion);

    int extensionVersion = parseVersion(aInstalledVersion, extensionVersionParts);
    int targetAppVersion = parseVersion(aTargetAppVersion, targetAppVersionParts);

    Connection c;
    Statement s;
    ResultSet rs;
    try 
    {
      c = getConnection();
      s = c.createStatement();
  
      // We need to find all rows matching aExtensionGUID, and filter like so:
      // 1) version > extensionVersion
      // 2) targetapp == aTargetApp
      // 3) mintargetappversion <= targetAppVersion <= maxtargetappversion
      String sql = "SELECT * FROM extensions WHERE guid = '" + aExtensionGUID + "' AND targetapp = '" + aTargetApp + "'";

      boolean goat = s.execute(sql);
      rs = s.getResultSet();

      int newestExtensionVersion = extensionVersion;

      while (rs.next()) 
      {
        int minTargetAppVersion = parseVersion(rs.getString("mintargetappversion"), targetAppVersionParts);
        int maxTargetAppVersion = parseVersion(rs.getString("maxtargetappversion"), targetAppVersionParts);
        
        int version = parseVersion(rs.getString("version"), extensionVersionParts);
        if (version > extensionVersion && 
          version > newestExtensionVersion &&
          minTargetAppVersion <= targetAppVersion && 
          targetAppVersion < maxTargetAppVersion) 
        {
          newestExtensionVersion = version;
          id = rs.getInt("id");
        }
      }
      rs.close();
      s.close();
      c.close();
    }
    catch (Exception e) 
    {
    }

    return id;
  }

  protected int parseVersion(String aVersionString, int aPower)
  {
    int version = 0;
    StringTokenizer tokenizer = new StringTokenizer(aVersionString, ".");

    if (aPower == 0)
      aPower = tokenizer.countTokens();
    
    for (int i = 0; tokenizer.hasMoreTokens(); ++i) 
    {
      String token = tokenizer.nextToken();
      if (token.endsWith("+")) 
      {
        token = token.substring(0, token.lastIndexOf("+"));
        version += 1;
        if (token.length() == 0)
          continue;
      }

      version += Integer.parseInt(token) * Math.pow(10, aPower - i);
    }
    return version;
  }

  protected int getPartCount(String aVersionString)
  {
    return (new StringTokenizer(aVersionString, ".")).countTokens();
  }
}

