/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Bill Law    <law@netscape.com>
 */
#include "nsWindowsHooks.h"
#include <windows.h>

// Implementation utilities.
#include "nsWindowsHooksUtil.cpp"
#include "nsIDOMWindowInternal.h"
#include "nsIServiceManager.h"
#include "nsICommonDialogs.h"
#include "nsIStringBundle.h"
#include "nsIAllocator.h"
#include "nsICmdLineService.h"
#include "nsXPIDLString.h"

// Objects that describe the Windows registry entries that we need to tweak.
static ProtocolRegistryEntry
    http( "http" ),
    https( "https" ),
    ftp( "ftp" ),
    chrome( "chrome" ),
    gopher( "gopher" );

const char *jpgExts[] = { ".jpg", ".jpeg", 0 };
const char *gifExts[] = { ".gif", 0 };
const char *pngExts[] = { ".png", 0 };
const char *xmlExts[] = { ".xml", 0 };
const char *xulExts[] = { ".xul", 0 };
const char *htmExts[] = { ".htm", ".html", 0 };

static FileTypeRegistryEntry
    jpg( jpgExts, "MozillaJPEG", "Mozilla Joint Photographic Experts Group Image File" ),
    gif( gifExts, "MozillaGIF",  "Mozilla Graphics Interchange Format Image File" ),
    png( pngExts, "MozillaPNG",  "Mozilla Portable Network Graphic Image File" ),
    xml( xmlExts, "MozillaXML",  "Mozilla XML File Document" ),
    xul( xulExts, "MozillaXUL",  "Mozilla XUL File Document" );

static EditableFileTypeRegistryEntry
    mozillaMarkup( htmExts, "MozillaHTML", "Mozilla Hypertext Markup Language Document" );

// Implementation of the nsIWindowsHooksSettings interface.
// Use standard implementation of nsISupports stuff.
NS_IMPL_ISUPPORTS1( nsWindowsHooksSettings, nsIWindowsHooksSettings );

nsWindowsHooksSettings::nsWindowsHooksSettings() {
    NS_INIT_ISUPPORTS();
}

nsWindowsHooksSettings::~nsWindowsHooksSettings() {
}

// Generic getter.
NS_IMETHODIMP
nsWindowsHooksSettings::Get( PRBool *result, PRBool nsWindowsHooksSettings::*member ) {
    NS_ENSURE_ARG( result );
    NS_ENSURE_ARG( member );
    *result = this->*member;
    return NS_OK;
}

// Generic setter.
NS_IMETHODIMP
nsWindowsHooksSettings::Set( PRBool value, PRBool nsWindowsHooksSettings::*member ) {
    NS_ENSURE_ARG( member );
    this->*member = value;
    return NS_OK;
}

// Macros to define specific getter/setter methods.
#define DEFINE_GETTER_AND_SETTER( attr, member ) \
NS_IMETHODIMP \
nsWindowsHooksSettings::Get##attr ( PRBool *result ) { \
    return this->Get( result, &nsWindowsHooksSettings::member ); \
} \
NS_IMETHODIMP \
nsWindowsHooksSettings::Set##attr ( PRBool value ) { \
    return this->Set( value, &nsWindowsHooksSettings::member ); \
}

// Define all the getter/setter methods:
DEFINE_GETTER_AND_SETTER( IsHandlingHTML,   mHandleHTML   )
DEFINE_GETTER_AND_SETTER( IsHandlingJPEG,   mHandleJPEG   )
DEFINE_GETTER_AND_SETTER( IsHandlingGIF,    mHandleGIF    )
DEFINE_GETTER_AND_SETTER( IsHandlingPNG,    mHandlePNG    )
DEFINE_GETTER_AND_SETTER( IsHandlingXML,    mHandleXML    )
DEFINE_GETTER_AND_SETTER( IsHandlingXUL,    mHandleXUL    )
DEFINE_GETTER_AND_SETTER( IsHandlingHTTP,   mHandleHTTP   )
DEFINE_GETTER_AND_SETTER( IsHandlingHTTPS,  mHandleHTTPS  )
DEFINE_GETTER_AND_SETTER( IsHandlingFTP,    mHandleFTP    )
DEFINE_GETTER_AND_SETTER( IsHandlingCHROME, mHandleCHROME )
DEFINE_GETTER_AND_SETTER( IsHandlingGOPHER, mHandleGOPHER )
DEFINE_GETTER_AND_SETTER( ShowDialog,       mShowDialog   )
DEFINE_GETTER_AND_SETTER( HaveBeenSet,      mHaveBeenSet  )


// Implementation of the nsIWindowsHooks interface.
// Use standard implementation of nsISupports stuff.
NS_IMPL_ISUPPORTS1( nsWindowsHooks, nsIWindowsHooks );

nsWindowsHooks::nsWindowsHooks() {
  NS_INIT_ISUPPORTS();
}

nsWindowsHooks::~nsWindowsHooks() {
}

// Internal GetPreferences.
NS_IMETHODIMP
nsWindowsHooks::GetSettings( nsWindowsHooksSettings **result ) {
    nsresult rv = NS_OK;

    // Validate input arg.
    NS_ENSURE_ARG( result );

    // Allocate prefs object.
    nsWindowsHooksSettings *prefs = *result = new nsWindowsHooksSettings;
    NS_ENSURE_TRUE( prefs, NS_ERROR_OUT_OF_MEMORY );

    // Got it, increment ref count.
    NS_ADDREF( prefs );

    // Get each registry value and copy to prefs structure.
    prefs->mHandleHTTP   = (void*)( BoolRegistryEntry( "isHandlingHTTP"   ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleHTTPS  = (void*)( BoolRegistryEntry( "isHandlingHTTPS"  ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleFTP    = (void*)( BoolRegistryEntry( "isHandlingFTP"    ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleCHROME = (void*)( BoolRegistryEntry( "isHandlingCHROME" ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleGOPHER = (void*)( BoolRegistryEntry( "isHandlingGOPHER" ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleHTML   = (void*)( BoolRegistryEntry( "isHandlingHTML"   ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleJPEG   = (void*)( BoolRegistryEntry( "isHandlingJPEG"   ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleGIF    = (void*)( BoolRegistryEntry( "isHandlingGIF"    ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandlePNG    = (void*)( BoolRegistryEntry( "isHandlingPNG"    ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleXML    = (void*)( BoolRegistryEntry( "isHandlingXML"    ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHandleXUL    = (void*)( BoolRegistryEntry( "isHandlingXUL"    ) ) ? PR_TRUE : PR_FALSE;
    prefs->mShowDialog   = (void*)( BoolRegistryEntry( "showDialog"       ) ) ? PR_TRUE : PR_FALSE;
    prefs->mHaveBeenSet  = (void*)( BoolRegistryEntry( "haveBeenSet"      ) ) ? PR_TRUE : PR_FALSE;

#ifdef DEBUG_law
NS_WARN_IF_FALSE( NS_SUCCEEDED( rv ), "GetPreferences failed" );
#endif

    return rv;
}

// Public interface uses internal plus a QI to get to the proper result.
NS_IMETHODIMP
nsWindowsHooks::GetSettings( nsIWindowsHooksSettings **_retval ) {
    // Allocate prefs object.
    nsWindowsHooksSettings *prefs;
    nsresult rv = this->GetSettings( &prefs );

    if ( NS_SUCCEEDED( rv ) ) {
        // QI to proper interface.
        rv = prefs->QueryInterface( NS_GET_IID( nsIWindowsHooksSettings ), (void**)_retval );
        // Release (to undo our Get...).
        NS_RELEASE( prefs );
    }

    return rv;
}

static PRBool misMatch( const PRBool &flag, const ProtocolRegistryEntry &entry ) {
    PRBool result = PR_FALSE;
    // Check if we care.
    if ( flag ) { 
        // Compare registry entry setting to what it *should* be.
        if ( entry.currentSetting() != entry.setting ) {
            result = PR_TRUE;
        }
    }

    return result;
}

// Implementation of method that checks settings versus registry and prompts user
// if out of synch.
NS_IMETHODIMP
nsWindowsHooks::CheckSettings( nsIDOMWindowInternal *aParent ) {
    nsresult rv = NS_OK;

    // Only do this once!
    static PRBool alreadyChecked = PR_FALSE;
    if ( alreadyChecked ) {
        return NS_OK;
    } else {
        alreadyChecked = PR_TRUE;
    }

    // Get settings.
    nsWindowsHooksSettings *settings;
    rv = this->GetSettings( &settings );

    if ( NS_SUCCEEDED( rv ) && settings ) {
        // If not set previously, set to defaults so that they are
        // set properly when/if the user says to.
        if ( !settings->mHaveBeenSet ) {
            settings->mHandleHTTP   = PR_TRUE;
            settings->mHandleHTTPS  = PR_TRUE;
            settings->mHandleFTP    = PR_TRUE;
            settings->mHandleCHROME = PR_TRUE;
            settings->mHandleGOPHER = PR_TRUE;
            settings->mHandleHTML   = PR_TRUE;
            settings->mHandleJPEG   = PR_TRUE;
            settings->mHandleGIF    = PR_TRUE;
            settings->mHandlePNG    = PR_TRUE;
            settings->mHandleXML    = PR_TRUE;
            settings->mHandleXUL    = PR_TRUE;

            settings->mShowDialog       = PR_TRUE;
        }

        // If launched with "-installer" then override mShowDialog.
        PRBool installing = PR_FALSE;
        if ( !settings->mShowDialog ) {
            // Get command line service.
            nsCID cmdLineCID = NS_COMMANDLINE_SERVICE_CID;
            nsCOMPtr<nsICmdLineService> cmdLineArgs( do_GetService( cmdLineCID, &rv ) );
            if ( NS_SUCCEEDED( rv ) && cmdLineArgs ) {
                // See if "-installer" was specified.
                nsXPIDLCString installer;
                rv = cmdLineArgs->GetCmdLineValue( "-installer", getter_Copies( installer ) );
                if ( NS_SUCCEEDED( rv ) && installer ) {
                    installing = PR_TRUE;
                }
            }
        }

        // First, make sure the user cares.
        if ( settings->mShowDialog || installing ) {
            // Look at registry setting for all things that are set.
            if ( misMatch( settings->mHandleHTTP,   http )
                 ||
                 misMatch( settings->mHandleHTTPS,  https )
                 ||
                 misMatch( settings->mHandleFTP,    ftp )
                 ||
                 misMatch( settings->mHandleCHROME, chrome )
                 ||
                 misMatch( settings->mHandleGOPHER, gopher )
                 ||
                 misMatch( settings->mHandleHTML,   mozillaMarkup )
                 ||
                 misMatch( settings->mHandleJPEG,   jpg )
                 ||
                 misMatch( settings->mHandleGIF,    gif )
                 ||
                 misMatch( settings->mHandlePNG,    png )
                 ||
                 misMatch( settings->mHandleXML,    xml )
                 ||
                 misMatch( settings->mHandleXUL,    xul ) ) {
                // Need to prompt user.
                // First:
                //   o We need the common dialog service to show the dialog.
                //   o We need the string bundle service to fetch the appropriate
                //     dialog text.
                nsCID commonDlgCID = NS_CommonDialog_CID;
                nsCID bundleCID = NS_STRINGBUNDLESERVICE_CID;
                nsCOMPtr<nsICommonDialogs> commonDlgService( do_GetService( commonDlgCID, &rv ) );
                nsCOMPtr<nsIStringBundleService> bundleService( do_GetService( bundleCID, &rv ) );

                if ( commonDlgService && bundleService ) {
                    // Next, get bundle that provides text for dialog.
                    nsILocale *locale = 0;
                    nsIStringBundle *bundle;
                    rv = bundleService->CreateBundle( "chrome://global/locale/nsWindowsHooks.properties",
                                                      locale, 
                                                      getter_AddRefs( &bundle ) );
                    if ( NS_SUCCEEDED( rv ) && bundle ) {
                        // Get text for dialog and checkbox label.
                        //
                        // The window text depends on whether this is the first time
                        // the user is seeing this dialog.
                        const char *textKey  = "promptText";
                        if ( !settings->mHaveBeenSet ) {
                            textKey  = "initialPromptText";
                        }
                        nsXPIDLString text, label, title, yesButtonLabel, noButtonLabel, cancelButtonLabel;
                        if ( NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_ConvertASCIItoUCS2( textKey ).get(),
                                                                             getter_Copies( text ) ) ) )
                             &&
                             NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_LITERAL_STRING( "checkBoxLabel" ).get(),
                                                                             getter_Copies( label ) ) ) )
                             &&
                             NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_LITERAL_STRING( "title" ).get(),
                                                                             getter_Copies( title ) ) ) )
                             &&
                             NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_LITERAL_STRING( "yesButtonLabel" ).get(),
                                                                             getter_Copies( yesButtonLabel ) ) ) )
                             &&
                             NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_LITERAL_STRING( "noButtonLabel" ).get(),
                                                                             getter_Copies( noButtonLabel ) ) ) )
                             &&
                             NS_SUCCEEDED( ( rv = bundle->GetStringFromName( NS_LITERAL_STRING( "cancelButtonLabel" ).get(),
                                                                             getter_Copies( cancelButtonLabel ) ) ) ) ) {
                            // Got the text, now show dialog.
                            PRBool  showDialog = settings->mShowDialog;
                            PRInt32 dlgResult  = -1;
                            // No checkbox for initial display.
                            const PRUnichar *labelArg = 0;
                            if ( settings->mHaveBeenSet ) {
                                // Subsequent display uses label string.
                                labelArg = label;
                            }
                            // Note that the buttons need to be passed in this order:
                            //    o Yes
                            //    o Cancel
                            //    o No
                            // because UniversalDialog will move "Cancel" to the right.
                            rv = commonDlgService->UniversalDialog( aParent,
                                                                    0, // title          
                                                                    title,    // dlg title
                                                                    text,     // dlg text      
                                                                    labelArg, // Checkbox label
                                                                    yesButtonLabel,    // yes button
                                                                    cancelButtonLabel, // cancel button
                                                                    noButtonLabel,     // no button
                                                                    0, // button 3
                                                                    0, // edit 1 msg
                                                                    0, // edit 2 msg
                                                                    0, // edit 1 value
                                                                    0, // edit 2 value
                                                                    0, // icon (q-mark)
                                                                    &showDialog,
                                                                    3, // 3 buttons
                                                                    0, // no edit fields
                                                                    PR_FALSE, // no pw
                                                                    &dlgResult );
                            if ( NS_SUCCEEDED( rv ) ) {
                                // Did they say go ahead?
                                switch ( dlgResult ) {
                                    case 0:
                                        // User says: make the changes.
                                        // Remember "show dialog" choice.
                                        settings->mShowDialog = showDialog;
                                        // Apply settings; this single line of
                                        // code will do different things depending
                                        // on whether this is the first time (i.e.,
                                        // when "haveBeenSet" is false).  The first
                                        // time, this will set all prefs to true
                                        // (because that's how we initialized 'em
                                        // in GetSettings, above) and will update the
                                        // registry accordingly.  On subsequent passes,
                                        // this will only update the registry (because
                                        // the settings we got from GetSettings will
                                        // not have changed).
                                        //
                                        // BTW, the term "prefs" in this context does not
                                        // refer to conventional Mozilla "prefs."  Instead,
                                        // it refers to "Desktop Integration" prefs which
                                        // are stored in the windows registry.
                                        rv = SetSettings( settings );
                                        #ifdef DEBUG_law
                                            printf( "Yes, SetSettings returned 0x%08X\n", (int)rv );
                                        #endif
                                        break;

                                    case 2:
                                        // User says: Don't mess with Windows.
                                        // We update only the "showDialog" and
                                        // "haveBeenSet" keys.  Note that this will
                                        // have the effect of setting all the prefs
                                        // *off* if the user says no to the initial
                                        // prompt.
                                        BoolRegistryEntry( "haveBeenSet" ).set();
                                        if ( showDialog ) {
                                            BoolRegistryEntry( "showDialog" ).set();
                                        } else {
                                            BoolRegistryEntry( "showDialog" ).reset();
                                        }
                                        #ifdef DEBUG_law
                                            printf( "No, haveBeenSet=1 and showDialog=%d\n", (int)showDialog );
                                        #endif
                                        break;

                                    default:
                                        // User says: I dunno.  Make no changes (which
                                        // should produce the same dialog next time).
                                        #ifdef DEBUG_law
                                            printf( "Cancel\n" );
                                        #endif
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            #ifdef DEBUG_law
            else { printf( "Registry and prefs match\n" ); }
            #endif
        }
        #ifdef DEBUG_law
        else { printf( "showDialog is false and not installing\n" ); }
        #endif

        // Release the settings.
        settings->Release();
    }

    return rv;
}

// Utility to set PRBool registry value from getter method.
nsresult putPRBoolIntoRegistry( const char* valueName,
                                nsIWindowsHooksSettings *prefs,
                                nsWindowsHooksSettings::getter memFun ) {
    // Use getter method to extract attribute from prefs.
    PRBool boolValue;
    (void)(prefs->*memFun)( &boolValue );
    // Convert to DWORD.
    DWORD  dwordValue = boolValue;
    // Store into registry.
    BoolRegistryEntry pref( valueName );
    nsresult rv = boolValue ? pref.set() : pref.reset();

    return rv;
}

/* void setPreferences (in nsIWindowsHooksSettings prefs); */
NS_IMETHODIMP
nsWindowsHooks::SetSettings(nsIWindowsHooksSettings *prefs) {
    nsresult rv = NS_ERROR_FAILURE;

    putPRBoolIntoRegistry( "isHandlingHTTP",   prefs, &nsIWindowsHooksSettings::GetIsHandlingHTTP );
    putPRBoolIntoRegistry( "isHandlingHTTPS",  prefs, &nsIWindowsHooksSettings::GetIsHandlingHTTPS );
    putPRBoolIntoRegistry( "isHandlingFTP",    prefs, &nsIWindowsHooksSettings::GetIsHandlingFTP );
    putPRBoolIntoRegistry( "isHandlingCHROME", prefs, &nsIWindowsHooksSettings::GetIsHandlingCHROME );
    putPRBoolIntoRegistry( "isHandlingGOPHER", prefs, &nsIWindowsHooksSettings::GetIsHandlingGOPHER );
    putPRBoolIntoRegistry( "isHandlingHTML",   prefs, &nsIWindowsHooksSettings::GetIsHandlingHTML );
    putPRBoolIntoRegistry( "isHandlingJPEG",   prefs, &nsIWindowsHooksSettings::GetIsHandlingJPEG );
    putPRBoolIntoRegistry( "isHandlingGIF",    prefs, &nsIWindowsHooksSettings::GetIsHandlingGIF );
    putPRBoolIntoRegistry( "isHandlingPNG",    prefs, &nsIWindowsHooksSettings::GetIsHandlingPNG );
    putPRBoolIntoRegistry( "isHandlingXML",    prefs, &nsIWindowsHooksSettings::GetIsHandlingXML );
    putPRBoolIntoRegistry( "isHandlingXUL",    prefs, &nsIWindowsHooksSettings::GetIsHandlingXUL );
    putPRBoolIntoRegistry( "showDialog",       prefs, &nsIWindowsHooksSettings::GetShowDialog );

    // Indicate that these settings have indeed been set.
    BoolRegistryEntry( "haveBeenSet" ).set();

    rv = SetRegistry();

    return rv;
}

// Get preferences and start handling everything selected.
NS_IMETHODIMP
nsWindowsHooks::SetRegistry() {
    nsresult rv = NS_OK;

    // Get raw prefs object.
    nsWindowsHooksSettings *prefs;
    rv = this->GetSettings( &prefs );

    NS_ENSURE_TRUE( NS_SUCCEEDED( rv ), rv );

    if ( prefs->mHandleHTML ) {
        (void) mozillaMarkup.set();
    } else {
        (void) mozillaMarkup.reset();
    }
    if ( prefs->mHandleJPEG ) {
        (void) jpg.set();
    } else {
        (void) jpg.reset();
    }
    if ( prefs->mHandleGIF ) {
        (void) gif.set();
    } else {
        (void) gif.reset();
    }
    if ( prefs->mHandlePNG ) {
        (void) png.set();
    } else {
        (void) png.reset();
    }
    if ( prefs->mHandleXML ) {
        (void) xml.set();
    } else {
        (void) xml.reset();
    }
    if ( prefs->mHandleXUL ) {
        (void) xul.set();
    } else {
        (void) xul.reset();
    }
    if ( prefs->mHandleHTTP ) {
        (void) http.set();
    } else {
        (void) http.reset();
    }
    if ( prefs->mHandleHTTPS ) {
        (void) https.set();
    } else {
        (void) https.reset();
    }
    if ( prefs->mHandleFTP ) {
        (void) ftp.set();
    } else {
        (void) ftp.reset();
    }
    if ( prefs->mHandleCHROME ) {
        (void) chrome.set();
    } else {
        (void) chrome.reset();
    }
    if ( prefs->mHandleGOPHER ) {
        (void) gopher.set();
    } else {
        (void) gopher.reset();
    }

    return NS_OK;
}
