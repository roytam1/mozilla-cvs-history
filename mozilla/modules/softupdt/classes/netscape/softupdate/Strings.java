/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
//
// Strings.java 
// Aleksandar Totic
//

/**
 * This class is a repository for all the dialog strings.  Currently,
 * it hard-codes the US-English strings, but it should eventually be
 * linked to the Netscape XP strings library.
 *
 * @version
 * @author	atotic
 */

package netscape.softupdate;

import java.util.ResourceBundle;

class Strings {
    // Resource bundle
    private static ResourceBundle gbundle;

    public static ResourceBundle bundle() {
        if (gbundle == null)
			try	{
				gbundle	= ResourceBundle.getBundle("netscape.softupdate.SoftUpdateResourceBundle");
			} catch	( Throwable	t)
			{
				System.err.println("Could not get localized	resources:");	// l10n, no	need to	localize
				t.printStackTrace();
				System.err.println("Using English Language Default.");
				gbundle	= new SoftUpdateResourceBundle();
			}
		return gbundle;
    }

    public static String getString(String key){
        try {
            return bundle().getString(key);
        } catch ( Throwable t){
            System.err.println("Could not get resource " + key + ":");
            t.printStackTrace();
            return key + "(!)";
        }
    }


    //
    // security resources for user targets
    //
    static String targetRiskLow() {
	return getString("s1");
    }
    static String targetRiskColorLow() {
	return "#aaffaa";
    }

    static String targetRiskMedium() {
	return getString("s2");
    }
    static String targetRiskColorMedium() {
	return "#ffffaa";
    }

    static String targetRiskHigh() {
	return getString("s3");
    }
    static String targetRiskColorHigh() {
	return "#ffaaaa";
    }

    static String targetDesc_LimitedInstall() {
	return getString("s4");
    }
    static String targetUrl_LimitedInstall() {
        // XXX: fix the URL.
	return "http://iapp16.mcom.com/java/signing/Games.html";
    }

    static String targetDesc_FullInstall() {
	return getString("s5");
    }
    static String targetUrl_FullInstall() {
        // XXX: fix the URL.
	return "http://iapp16.mcom.com/java/signing/FileRead.html";
    }

    static String targetDesc_SilentInstall() {
        // XXX: this should be ripped out
	return getString("s6");
    }
    static String targetUrl_SilentInstall() {
        // XXX: fix the URL.
	return "http://iapp16.mcom.com/java/signing/FileWrite.html";
    }

    static String targetDesc_WinIni() {
	return getString("s7");
    }
    static String targetUrl_WinIni() {
        // XXX: fix the URL.
	return "http://iapp16.mcom.com/java/signing/FileWrite.html";
    }

    static String targetDesc_WinReg() {
	return getString("s8");
    }
    static String targetUrl_WinReg() {
        // XXX: fix the URL.
	return "http://iapp16.mcom.com/java/signing/FileWrite.html";
    }

    // PROGRESS WINDOW

    static String progress_Title() {
    return getString("s9");
    }

    static String progress_GettingReady() {
    return getString("s10");
    }

    static String progress_ReadyToInstall1() {
    return getString("s11");
    }

    static String progress_ReadyToInstall2() {
    return getString("s12");
    }

    // DETAILS WINDOW

    static String details_Explain(String title) {
    return getString("s13") + title + getString("s14");
    }

    static String details_WinTitle() {
    return getString("s15");
    }

    static String details_ExecuteProgress() {
    return getString("s16");
    }

    // ERROR STRINGS

    static String error_Prefix()
    {
        return getString("s17");
    }
    static String error_NoCertificate() {
    return error_Prefix() + getString("s18");
    }

    static String error_TooManyCertificates() {
    return error_Prefix() + getString("s19");
    }

    static String error_SilentModeDenied() {
    return error_Prefix() + getString("s20");
    }

    static String error_WinProfileMustCallStart() {
    return error_Prefix() + getString("s21");
    }

    static String error_MismatchedCertificate() {
    return error_Prefix() + getString("s22");
    }

    static String error_BadPackageName() {
    return error_Prefix() + getString("s23");
    }

    static String error_Unexpected() {
    return error_Prefix() + getString("s24");
    }

    static String error_BadPackageNameAS() {
    return error_Prefix() + getString("s25");
    }

    static String error_IllegalPath() {
    return error_Prefix() + getString("s26");
    }

    static String error_InstallFileUnexpected() {
    return error_Prefix() + getString("s27");
    }

    static String error_BadJSArgument() {
    return error_Prefix() + getString("s28");
    }

    static String error_SmartUpdateDisabled() {
    return error_Prefix() + getString("s29");
    }

    static String error_NoInstallerFile() {
    return "";
    }

	static String error_VerificationFailed() {
    return error_Prefix() + getString("s30");
	}

	static String error_MissingInstaller() {
    return error_Prefix() + getString("s31");
	}

	static String error_ExtractFailed() {
    return error_Prefix() + getString("s32");
	}

}