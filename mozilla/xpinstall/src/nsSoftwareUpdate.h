
#ifndef nsSoftwareUpdate_h___
#define nsSoftwareUpdate_h___

#include "nsSoftwareUpdateIIDs.h"
#include "nsISoftwareUpdate.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "prlock.h"
//#include "mozreg.h"
#include "NSReg.h"

class nsInstallInfo;

#include "nsIScriptExternalNameSet.h"
#include "nsIAppShellComponent.h"
#include "nsIXPINotifier.h"
#include "nsPIXPIStubHook.h"
#include "nsTopProgressNotifier.h"

class nsSoftwareUpdate: public nsIAppShellComponent, 
                        public nsISoftwareUpdate, 
                        public nsPIXPIStubHook
{
    public:
        
        NS_DEFINE_STATIC_CID_ACCESSOR( NS_SoftwareUpdate_CID );

        static nsSoftwareUpdate *GetInstance();

        /** GetProgramDirectory
         *  information used within the XPI module -- not
         *  available through any interface
         */
        static nsIFileSpec* GetProgramDirectory() { return mProgramDir; }

        NS_DECL_ISUPPORTS
        NS_DECL_NSIAPPSHELLCOMPONENT
        
        NS_IMETHOD InstallJar( nsIFileSpec* localFile,
                               const PRUnichar* URL,
                               const PRUnichar* arguments,
                               long flags = 0,
                               nsIXPINotifier* notifier = 0);  

        NS_IMETHOD RegisterNotifier(nsIXPINotifier *notifier);
        
        NS_IMETHOD InstallJarCallBack();
        NS_IMETHOD GetMasterNotifier(nsIXPINotifier **notifier);
        NS_IMETHOD SetActiveNotifier(nsIXPINotifier *notifier);
        NS_IMETHOD StartupTasks( PRBool* outAutoreg );

        /** StubInitialize() is private for the Install Wizard.
         *  The mStubLockout property makes sure this is only called
         *  once, and is also set by the AppShellComponent initialize
         *  so it can't be called during a normal Mozilla run
         */
        NS_IMETHOD StubInitialize(nsIFileSpec *dir);

        nsSoftwareUpdate();
        virtual ~nsSoftwareUpdate();


    private:
        static   nsSoftwareUpdate* mInstance;
        static   nsIFileSpec*      mProgramDir;

        nsresult RunNextInstall();
        nsresult RegisterNameset();
        
        PRLock*           mLock;
        PRBool            mInstalling;
        PRBool            mStubLockout;
        nsVoidArray       mJarInstallQueue;
        nsTopProgressNotifier   mMasterNotifier;

        HREG              mReg;
};


class nsSoftwareUpdateNameSet : public nsIScriptExternalNameSet 
{
    public:
        nsSoftwareUpdateNameSet();
        virtual ~nsSoftwareUpdateNameSet();

        // nsISupports
        NS_DECL_ISUPPORTS

        // nsIScriptExternalNameSet
        NS_IMETHOD InitializeClasses(nsIScriptContext* aScriptContext);
        NS_IMETHOD AddNameSet(nsIScriptContext* aScriptContext);
};
#endif
