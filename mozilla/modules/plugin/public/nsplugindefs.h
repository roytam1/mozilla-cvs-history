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

////////////////////////////////////////////////////////////////////////////////
/**
 * <B>INTERFACE TO NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).</B>
 *
 * <P>This superscedes the old plugin API (npapi.h, npupp.h), and 
 * eliminates the need for glue files: npunix.c, npwin.cpp and npmac.cpp that
 * get linked with the plugin. You will however need to link with the "backward
 * adapter" (badapter.cpp) in order to allow your plugin to run in pre-5.0
 * browsers. 
 *
 * <P>See nsplugin.h for an overview of how this fits with the 
 * overall plugin architecture.
 */
////////////////////////////////////////////////////////////////////////////////

#ifndef nsplugindefs_h___
#define nsplugindefs_h___

#ifndef prtypes_h___
#include "prtypes.h"
#endif

#ifdef __OS2__
#pragma pack(1)
#endif

#ifdef XP_MAC
#   include <Quickdraw.h>
#   include <Events.h>
#   include <MacWindows.h>
#endif

#if defined(XP_UNIX) && !defined(NO_X11)
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#endif

#ifdef XP_PC
#   include <windef.h>
#endif

////////////////////////////////////////////////////////////////////////////////

/* The OS/2 version of Netscape uses RC_DATA to define the
   mime types, file extentions, etc that are required.
   Use a vertical bar to seperate types, end types with \0.
   FileVersion and ProductVersion are 32bit ints, all other
   entries are strings the MUST be terminated wwith a \0.

AN EXAMPLE:

RCDATA NS_INFO_ProductVersion { 1,0,0,1,}

RCDATA NS_INFO_MIMEType    { "video/x-video|",
                             "video/x-flick\0" }
RCDATA NS_INFO_FileExtents { "avi|",
                             "flc\0" }
RCDATA NS_INFO_FileOpenName{ "MMOS2 video player(*.avi)|",
                             "MMOS2 Flc/Fli player(*.flc)\0" }

RCDATA NS_INFO_FileVersion       { 1,0,0,1 }
RCDATA NS_INFO_CompanyName       { "Netscape Communications\0" }
RCDATA NS_INFO_FileDescription   { "NPAVI32 Extension DLL\0"
RCDATA NS_INFO_InternalName      { "NPAVI32\0" )
RCDATA NS_INFO_LegalCopyright    { "Copyright Netscape Communications \251 1996\0"
RCDATA NS_INFO_OriginalFilename  { "NVAPI32.DLL" }
RCDATA NS_INFO_ProductName       { "NPAVI32 Dynamic Link Library\0" }

*/


/* RC_DATA types for version info - required */
#define NS_INFO_ProductVersion      1
#define NS_INFO_MIMEType            2
#define NS_INFO_FileOpenName        3
#define NS_INFO_FileExtents         4

/* RC_DATA types for version info - used if found */
#define NS_INFO_FileDescription     5
#define NS_INFO_ProductName         6

/* RC_DATA types for version info - optional */
#define NS_INFO_CompanyName         7
#define NS_INFO_FileVersion         8
#define NS_INFO_InternalName        9
#define NS_INFO_LegalCopyright      10
#define NS_INFO_OriginalFilename    11

#ifndef RC_INVOKED

////////////////////////////////////////////////////////////////////////////////
// Structures and definitions

#ifdef XP_MAC
#pragma options align=mac68k
#endif

typedef const char*     nsMIMEType;

struct nsByteRange {
    PRInt32             offset; 	/* negative offset means from the end */
    PRUint32            length;
    struct nsByteRange* next;
};

struct nsPluginRect {
    PRUint16            top;
    PRUint16            left;
    PRUint16            bottom;
    PRUint16            right;
};

////////////////////////////////////////////////////////////////////////////////
// Unix specific structures and definitions

#ifdef XP_UNIX

#include <stdio.h>

/*
 * Callback Structures.
 *
 * These are used to pass additional platform specific information.
 */
enum nsPluginCallbackType {
    nsPluginCallbackType_SetWindow = 1,
    nsPluginCallbackType_Print
};

struct nsPluginAnyCallbackStruct {
    PRInt32     type;
};

#ifndef NO_X11
struct nsPluginSetWindowCallbackStruct {
    PRInt32     type;
    Display*    display;
    Visual*     visual;
    Colormap    colormap;
    PRUint32    depth;
};
#else
struct nsPluginSetWindowCallbackStruct {
    PRInt32     type;
};
#endif


struct nsPluginPrintCallbackStruct {
    PRInt32     type;
    FILE*       fp;
};

#endif /* XP_UNIX */

////////////////////////////////////////////////////////////////////////////////

// List of variables which should be implmented by the plugin
enum nsPluginVariable {
    nsPluginVariable_NameString                      = 1,
    nsPluginVariable_DescriptionString               = 2
};

enum nsPluginManagerVariable {
    nsPluginManagerVariable_XDisplay                 = 1,
    nsPluginManagerVariable_XtAppContext             = 2
};

enum nsPluginInstancePeerVariable {
    nsPluginInstancePeerVariable_NetscapeWindow      = 3
//    nsPluginInstancePeerVariable_JavaClass              = 5,
//    nsPluginInstancePeerVariable_TimerInterval          = 7
};

enum nsPluginInstanceVariable {
    nsPluginInstanceVariable_WindowlessBool          = 3,
    nsPluginInstanceVariable_TransparentBool         = 4
};

////////////////////////////////////////////////////////////////////////////////

enum nsPluginMode {
    nsPluginMode_Embedded = 1,
    nsPluginMode_Full
};

// XXX this can go away now
enum nsPluginStreamType {
    nsPluginStreamType_Normal = 1,
    nsPluginStreamType_Seek,
    nsPluginStreamType_AsFile,
    nsPluginStreamType_AsFileOnly
};

/*
 * The type of a nsPluginWindow - it specifies the type of the data structure
 * returned in the window field.
 */
enum nsPluginWindowType {
    nsPluginWindowType_Window = 1,
    nsPluginWindowType_Drawable
};

#ifdef XP_MAC

struct nsPluginPort {
    CGrafPtr    port;   /* Grafport */
    PRInt32     portx;  /* position inside the topmost window */
    PRInt32     porty;
};
typedef RgnHandle       nsPluginRegion;
typedef WindowRef       nsPluginPlatformWindowRef;

#elif defined(XP_PC)

struct nsPluginPort;
typedef HRGN            nsPluginRegion;
typedef HWND            nsPluginPlatformWindowRef;

#elif defined(XP_UNIX) && !defined(NO_X11)

struct nsPluginPort;
typedef Region          nsPluginRegion;
typedef Drawable        nsPluginPlatformWindowRef;

#else

struct nsPluginPort;
typedef void*           nsPluginRegion;
typedef void*           nsPluginPlatformWindowRef;

#endif

struct nsPluginWindow {
    nsPluginPort* window;       /* Platform specific window handle */
                                /* OS/2: x - Position of bottom left corner  */
                                /* OS/2: y - relative to visible netscape window */
    PRUint32      x;            /* Position of top left corner relative */
    PRUint32      y;            /*	to a netscape page.					*/
    PRUint32      width;        /* Maximum window size */
    PRUint32      height;
    nsPluginRect  clipRect;     /* Clipping rectangle in port coordinates */
                                /* Used by MAC only.			  */
#ifdef XP_UNIX
    void*         ws_info;      /* Platform-dependent additonal data */
#endif /* XP_UNIX */
    nsPluginWindowType type;    /* Is this a window or a drawable? */
};

struct nsPluginFullPrint {
    PRBool      pluginPrinted;	/* Set TRUE if plugin handled fullscreen */
                                /*	printing							 */
    PRBool      printOne;       /* TRUE if plugin should print one copy  */
                                /*	to default printer					 */
    void*       platformPrint;  /* Platform-specific printing info */
};

struct nsPluginEmbedPrint {
    nsPluginWindow    window;
    void*             platformPrint;	/* Platform-specific printing info */
};

struct nsPluginPrint {
    nsPluginMode      mode;     /* nsPluginMode_Full or nsPluginMode_Embedded */
    union
    {
        nsPluginFullPrint     fullPrint;	/* if mode is nsPluginMode_Full */
        nsPluginEmbedPrint    embedPrint;	/* if mode is nsPluginMode_Embedded */
    } print;
};

struct nsPluginEvent {

#if defined(XP_MAC)
    EventRecord*                event;
    nsPluginPlatformWindowRef   window;

#elif defined(XP_PC)
    uint16      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_OS2)
    uint32      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_UNIX) && !defined(NO_X11)
    XEvent      event;
#else
    void        *event;
#endif
};

/*
 *  Non-standard event types that can be passed to HandleEvent
 */
enum nsPluginEventType {
#ifdef XP_MAC
    nsPluginEventType_GetFocusEvent        = (osEvt + 16),
    nsPluginEventType_LoseFocusEvent       = (osEvt + 17),
    nsPluginEventType_AdjustCursorEvent    = (osEvt + 18),
    nsPluginEventType_MenuCommandEvent     = (osEvt + 19),
#endif /* XP_MAC */
    nsPluginEventType_Idle                 = 0
};

////////////////////////////////////////////////////////////////////////////////

enum nsPluginReason {
    nsPluginReason_Base = 0,
    nsPluginReason_Done = 0,
    nsPluginReason_NetworkErr,
    nsPluginReason_UserBreak,
    nsPluginReason_NoReason
};

////////////////////////////////////////////////////////////////////////////////
// Version Numbers for Structs

// These version number are for structures whose fields may evolve over time.
// When fields are added to the end of the struct, the minor version will be
// incremented. When the struct changes in an incompatible way the major version
// will be incremented. 

#define nsMajorVersion(v)       (((PRInt32)(v) >> 16) & 0xffff)
#define nsMinorVersion(v)       ((PRInt32)(v) & 0xffff)

#define nsVersionOK(suppliedV, requiredV)                   \
    (nsMajorVersion(suppliedV) == nsMajorVersion(requiredV) \
     && nsMinorVersion(suppliedV) >= nsMinorVersion(requiredV))

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

// Classes that must be implemented by the plugin DLL:
struct nsIPlugin;                       // plugin class (MIME-type handler)
class nsILiveConnectPlugin;             // subclass of nsIPlugin
class nsIEventHandler;                  // event handler interface
class nsIPluginInstance;                // plugin instance
class nsIPluginStream;                  // stream to receive data from the browser

// Classes that are implemented by the browser:
class nsIPluginManager;                 // minimum browser requirements
class nsIFileUtilities;                 // file utilities (accessible from nsIPluginManager)
class nsIPluginInstancePeer;            // parts of nsIPluginInstance implemented by the browser
class nsIWindowlessPluginInstancePeer;  // subclass of nsIPluginInstancePeer for windowless plugins
class nsIPluginTagInfo;                 // describes html tag (accessible from nsIPluginInstancePeer)
class nsILiveConnectPluginInstancePeer; // subclass of nsIPluginInstancePeer
class nsIPluginStreamPeer;              // parts of nsIPluginStream implemented by the browser
class nsISeekablePluginStreamPeer;      // seekable subclass of nsIPluginStreamPeer

////////////////////////////////////////////////////////////////////////////////

#ifdef XP_MAC
#pragma options align=reset
#endif

#endif /* RC_INVOKED */
#ifdef __OS2__
#pragma pack()
#endif

#endif // nsplugindefs_h___
