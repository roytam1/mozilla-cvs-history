// Test06.cpp

#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsCOMPtr.h"
#include "nsIPtr.h"

static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIWebShellWindowIID, NS_IWEBSHELL_WINDOW_IID);

NS_DEF_PTR(nsIScriptGlobalObject);
NS_DEF_PTR(nsIWebShell);
NS_DEF_PTR(nsIWebShellContainer);
NS_DEF_PTR(nsIWebShellWindow);

	/*
		Windows:
			nsCOMPtr_optimized					176
			nsCOMPtr_as_found						181
			nsCOMPtr_optimized*					182
			nsCOMPtr02*									184
			nsCOMPtr02									187
			nsCOMPtr02*									188
			nsCOMPtr03									189
			raw_optimized, nsCOMPtr00		191
			nsCOMPtr00*									199
			nsCOMPtr_as_found*					201
			raw													214
			nsIPtr_optimized						137 + 196
			nsIPtr01										168 + 196
			nsIPtr											220 + 196

		Macintosh:
			nsCOMPtr_optimized					300		(1.0000)
			nsCOMPtr02									320		(1.0667)	i.e., 6.67% bigger than nsCOMPtr_optimized
			nsCOMPtr00									328		(1.0933)
			raw_optimized, nsCOMPtr03		332		(1.1067)
			nsCOMPtr_as_found						344		(1.1467)
			raw													388		(1.2933)
			nsIPtr_optimized						400		(1.3333)
			nsIPtr01										464		(1.5467)
			nsIPtr											564		(1.8800)

	*/


void // nsresult
Test06_raw( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m388, w214
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsIScriptGlobalObject* scriptGlobalObject = 0;
		nsresult status = aDOMWindow->QueryInterface(kIScriptGlobalObjectIID, (void**)&scriptGlobalObject);

		nsIWebShell* webShell = 0;
		if ( scriptGlobalObject )
			scriptGlobalObject->GetWebShell(&webShell);

		nsIWebShell* rootWebShell = 0;
		if ( webShell )
			status = webShell->GetRootWebShellEvenIfChrome(rootWebShell);

		nsIWebShellContainer* webShellContainer = 0;
		if ( rootWebShell )
			status = rootWebShell->GetContainer(webShellContainer);

		if ( webShellContainer )
			status = webShellContainer->QueryInterface(kIWebShellWindowIID, (void**)aWebShellWindow);
		else
			(*aWebShellWindow) = 0;

		NS_IF_RELEASE(webShellContainer);
		NS_IF_RELEASE(rootWebShell);
		NS_IF_RELEASE(webShell);
		NS_IF_RELEASE(scriptGlobalObject);

//		return status;
	}


void // nsresult
Test06_raw_optimized( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m332, w191
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		(*aWebShellWindow) = 0;

		nsIScriptGlobalObject* scriptGlobalObject;
		nsresult status = aDOMWindow->QueryInterface(kIScriptGlobalObjectIID, (void**)&scriptGlobalObject);
		if ( NS_SUCCEEDED(status) )
			{
				nsIWebShell* webShell;
				scriptGlobalObject->GetWebShell(&webShell);
				if ( webShell )
					{
						nsIWebShell* rootWebShell;
						status = webShell->GetRootWebShellEvenIfChrome(rootWebShell);
						if ( NS_SUCCEEDED(status) )
							{
								nsIWebShellContainer* webShellContainer;
								status = rootWebShell->GetContainer(webShellContainer);
								if ( NS_SUCCEEDED(status) )
									{
										status = webShellContainer->QueryInterface(kIWebShellWindowIID, (void**)aWebShellWindow);
										NS_RELEASE(webShellContainer);
									}

								NS_RELEASE(rootWebShell);
							}

						NS_RELEASE(webShell);
					}

				NS_RELEASE(scriptGlobalObject);
			}

//		return status;
	}

void
Test06_nsCOMPtr_as_found( nsIDOMWindow* aDOMWindow, nsCOMPtr<nsIWebShellWindow>* aWebShellWindow )
		// m344, w181/201
	{
//		if ( !aDOMWindow )
//			return;

		nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject = do_QueryInterface(aDOMWindow);

		nsCOMPtr<nsIWebShell> webShell;
		if ( scriptGlobalObject )
			scriptGlobalObject->GetWebShell( getter_AddRefs(webShell) );

		nsCOMPtr<nsIWebShell> rootWebShell;
		if ( webShell )
			webShell->GetRootWebShellEvenIfChrome( *getter_AddRefs(rootWebShell) );

		nsCOMPtr<nsIWebShellContainer> webShellContainer;
		if ( rootWebShell )
			rootWebShell->GetContainer( *getter_AddRefs(webShellContainer) );

		(*aWebShellWindow) = do_QueryInterface(webShellContainer);
	}

void // nsresult
Test06_nsCOMPtr00( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m328, w191/199
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsresult status;
		nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject = do_QueryInterface(aDOMWindow, &status);

		nsIWebShell* temp0;
		if ( scriptGlobalObject )
			scriptGlobalObject->GetWebShell(&temp0);
		nsCOMPtr<nsIWebShell> webShell = dont_AddRef(temp0);

		if ( webShell )
			status = webShell->GetRootWebShellEvenIfChrome(temp0);
		nsCOMPtr<nsIWebShell> rootWebShell = dont_AddRef(temp0);

		nsIWebShellContainer* temp1;
		if ( rootWebShell )
			status = rootWebShell->GetContainer(temp1);
		nsCOMPtr<nsIWebShellContainer> webShellContainer = dont_AddRef(temp1);

		if ( webShellContainer )
			status = webShellContainer->QueryInterface(nsIWebShellWindow::GetIID(), (void**)aWebShellWindow);
		else
			(*aWebShellWindow) = 0;

//		return status;
	}

void // nsresult
Test06_nsCOMPtr_optimized( nsIDOMWindow* aDOMWindow, nsCOMPtr<nsIWebShellWindow>* aWebShellWindow )
		// m300, w176/182
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsresult status;
		nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject = do_QueryInterface(aDOMWindow, &status);

		nsIWebShell* temp0;
		if ( scriptGlobalObject )
			scriptGlobalObject->GetWebShell(&temp0);
		nsCOMPtr<nsIWebShell> webShell = dont_AddRef(temp0);

		if ( webShell )
			status = webShell->GetRootWebShellEvenIfChrome(temp0);
		nsCOMPtr<nsIWebShell> rootWebShell = dont_AddRef(temp0);

		nsIWebShellContainer* temp1;
		if ( rootWebShell )
			status = rootWebShell->GetContainer(temp1);
		nsCOMPtr<nsIWebShellContainer> webShellContainer = dont_AddRef(temp1);
		(*aWebShellWindow) = do_QueryInterface(webShellContainer, &status);

//		return status;
	}

void // nsresult
Test06_nsCOMPtr02( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m320, w187/184
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		(*aWebShellWindow) = 0;

		nsresult status;
		nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject = do_QueryInterface(aDOMWindow, &status);
		if ( scriptGlobalObject )
			{
				nsIWebShell* temp0;
				scriptGlobalObject->GetWebShell(&temp0);
				nsCOMPtr<nsIWebShell> webShell = dont_AddRef(temp0);

				if ( webShell )
					{
						status = webShell->GetRootWebShellEvenIfChrome(temp0);
						nsCOMPtr<nsIWebShell> rootWebShell = dont_AddRef(temp0);

						if ( rootWebShell )
							{
								nsIWebShellContainer* temp1;
								status = rootWebShell->GetContainer(temp1);
								nsCOMPtr<nsIWebShellContainer> webShellContainer = dont_AddRef(temp1);

								if ( webShellContainer )
									status = webShellContainer->QueryInterface(nsIWebShellWindow::GetIID(), (void**)aWebShellWindow);
							}
					}
			}

//		return status;
	}

void // nsresult
Test06_nsCOMPtr03( nsIDOMWindow* aDOMWindow, nsCOMPtr<nsIWebShellWindow>* aWebShellWindow )
		// m332, w189/188
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		(*aWebShellWindow) = 0;

		nsresult status;
		nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject = do_QueryInterface(aDOMWindow, &status);
		if ( scriptGlobalObject )
			{
				nsIWebShell* temp0;
				scriptGlobalObject->GetWebShell(&temp0);
				nsCOMPtr<nsIWebShell> webShell = dont_AddRef(temp0);

				if ( webShell )
					{
						status = webShell->GetRootWebShellEvenIfChrome(temp0);
						nsCOMPtr<nsIWebShell> rootWebShell = dont_AddRef(temp0);

						if ( rootWebShell )
							{
								nsIWebShellContainer* temp1;
								status = rootWebShell->GetContainer(temp1);
								nsCOMPtr<nsIWebShellContainer> webShellContainer = dont_AddRef(temp1);
								(*aWebShellWindow) = do_QueryInterface(webShellContainer, &status);
							}
					}
			}

//		return status;
	}

void // nsresult
Test06_nsIPtr( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m564, w220
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsIScriptGlobalObjectPtr scriptGlobalObject;
		nsresult status = aDOMWindow->QueryInterface(kIScriptGlobalObjectIID, scriptGlobalObject.Query());

		nsIWebShellPtr webShell;
		if ( scriptGlobalObject.IsNotNull() )
			scriptGlobalObject->GetWebShell( webShell.AssignPtr() );

		nsIWebShellPtr rootWebShell;
		if ( webShell.IsNotNull() )
			status = webShell->GetRootWebShellEvenIfChrome( rootWebShell.AssignRef() );

		nsIWebShellContainerPtr webShellContainer;
		if ( rootWebShell.IsNotNull() )
			status = rootWebShell->GetContainer( webShellContainer.AssignRef() );

		if ( webShellContainer.IsNotNull() )
			status = webShellContainer->QueryInterface(kIWebShellWindowIID, (void**)aWebShellWindow);
		else
			(*aWebShellWindow) = 0;

//		return status;
	}

void // nsresult
Test06_nsIPtr01( nsIDOMWindow* aDOMWindow, nsIWebShellWindowPtr* aWebShellWindow )
		// m464, w168
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsIScriptGlobalObject* temp0;
		nsresult status = aDOMWindow->QueryInterface(kIScriptGlobalObjectIID, (void**)&temp0);
		nsIScriptGlobalObjectPtr scriptGlobalObject = temp0;

		nsIWebShell* temp1;
		if ( scriptGlobalObject.IsNotNull() )
			scriptGlobalObject->GetWebShell(&temp1);
		nsIWebShellPtr webShell = temp1;

		if ( webShell.IsNotNull() )
			status = webShell->GetRootWebShellEvenIfChrome(temp1);
		nsIWebShellPtr rootWebShell;

		nsIWebShellContainer* temp2;
		if ( rootWebShell.IsNotNull() )
			status = rootWebShell->GetContainer(temp2);
		nsIWebShellContainerPtr webShellContainer = temp2;

		if ( webShellContainer.IsNotNull() )
			status = webShellContainer->QueryInterface(kIWebShellWindowIID, aWebShellWindow->Query());
		else
			(*aWebShellWindow) = 0;

//		return status;
	}
void // nsresult
Test06_nsIPtr_optimized( nsIDOMWindow* aDOMWindow, nsIWebShellWindow** aWebShellWindow )
		// m400, w137
	{
//		if ( !aDOMWindow )
//			return NS_ERROR_NULL_POINTER;

		nsIScriptGlobalObject* temp0;
		nsresult status = aDOMWindow->QueryInterface(kIScriptGlobalObjectIID, (void**)&temp0);
		nsIScriptGlobalObjectPtr scriptGlobalObject = temp0;

		nsIWebShell* temp1;
		if ( scriptGlobalObject.IsNotNull() )
			scriptGlobalObject->GetWebShell(&temp1);
		nsIWebShellPtr webShell = temp1;

		if ( webShell.IsNotNull() )
			status = webShell->GetRootWebShellEvenIfChrome(temp1);
		nsIWebShellPtr rootWebShell;

		nsIWebShellContainer* temp2;
		if ( rootWebShell.IsNotNull() )
			status = rootWebShell->GetContainer(temp2);
		nsIWebShellContainerPtr webShellContainer = temp2;

		if ( webShellContainer.IsNotNull() )
			status = webShellContainer->QueryInterface(kIWebShellWindowIID, (void**)aWebShellWindow);
		else
			(*aWebShellWindow) = 0;

//		return status;
	}
