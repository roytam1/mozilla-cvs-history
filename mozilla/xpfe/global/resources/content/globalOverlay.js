function goQuitApplication()
{
  var ObserverService = Components.classes["@mozilla.org/observer-service;1"].getService();
  ObserverService = ObserverService.QueryInterface(Components.interfaces.nsIObserverService);
  if (ObserverService)
  {
    try
    {
      ObserverService.Notify(null, "quit-application", null);
    }
    catch (ex)
    {
      // dump("no observer found \n");
    }
  }

  var windowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
  var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
  var enumerator = windowManagerInterface.getEnumerator( null );
  var appShell = Components.classes['@mozilla.org/appshell/appShellService;1'].getService();
  appShell = appShell.QueryInterface( Components.interfaces.nsIAppShellService );

  // Get current server mode setting and turn it off while windows close.
  var serverMode = false;
  var nativeAppSupport = null;
  try 
  {
    nativeAppSupport = appShell.nativeAppSupport;
    if ( nativeAppSupport )
    {
      serverMode = nativeAppSupport.isServerMode;
      nativeAppSupport.isServerMode = false;
    }
  }
  catch ( ex )
  {
  }

  while ( enumerator.hasMoreElements()  )
  {
    var  windowToClose = enumerator.getNext();
    var domWindow = windowManagerInterface.convertISupportsToDOMWindow( windowToClose );
    if (!("tryToClose" in domWindow))
    {
      // dump(" window.close \n");
      domWindow.close();
    }
    else
    {
      // dump(" try to close \n" );
      if ( !domWindow.tryToClose() )
      {
        // Restore server mode if it was set on entry.
        if ( serverMode )
        {
          nativeAppSupport.isServerMode = true;
        }
        return false;
      }
    }
  };

  // call appshell exit
  appShell.Quit();
  return true;
}

//
// Command Updater functions
//
function goUpdateCommand(command)
{
  try {
    var controller = top.document.commandDispatcher.getControllerForCommand(command);

    var enabled = false;

    if ( controller )
      enabled = controller.isCommandEnabled(command);

    goSetCommandEnabled(command, enabled);
  }
  catch (e) {
    dump("An error occurred updating the "+command+" command\n");
  }
}

function goDoCommand(command)
{
  try {
    var controller = top.document.commandDispatcher.getControllerForCommand(command);
    if ( controller && controller.isCommandEnabled(command))
      controller.doCommand(command);
  }
  catch (e) {
    dump("An error occurred executing the "+command+" command\n");
  }
}


function goSetCommandEnabled(id, enabled)
{
  var node = document.getElementById(id);

  if ( node )
  {
    if ( enabled )
      node.removeAttribute("disabled");
    else
      node.setAttribute('disabled', 'true');
  }
}

function goSetMenuValue(command, labelAttribute)
{
  var commandNode = top.document.getElementById(command);
  if ( commandNode )
  {
    var label = commandNode.getAttribute(labelAttribute);
    if ( label )
      commandNode.setAttribute('label', label);
  }
}

function goSetAccessKey(command, valueAttribute)
{
  var commandNode = top.document.getElementById(command);
  if ( commandNode )
  {
    var value = commandNode.getAttribute(valueAttribute);
    if ( value )
      commandNode.setAttribute('accesskey', value);
  }
}

// this function is used to inform all the controllers attached to a node that an event has occurred
// (e.g. the tree controllers need to be informed of blur events so that they can change some of the
// menu items back to their default values)
function goOnEvent(node, event)
{
  var numControllers = node.controllers.getControllerCount();
  var controller;

  for ( var controllerIndex = 0; controllerIndex < numControllers; controllerIndex++ )
  {
    controller = node.controllers.getControllerAt(controllerIndex);
    if ( controller )
      controller.onEvent(event);
  }
}

function setTooltipText(aID, aTooltipText)
{
  var element = document.getElementById(aID);
  if (element)
    element.setAttribute("tooltiptext", aTooltipText);
}

function FillInTooltip ( tipElement )
{
  var retVal = false;
  var textNode = document.getElementById("TOOLTIP-tooltipText");
  while (textNode.hasChildNodes())
    textNode.removeChild(textNode.firstChild);
  if (textNode) {
    var tipText = tipElement.getAttribute("tooltiptext");
    if (tipText) {
      var node = document.createTextNode(tipText);
      textNode.appendChild(node);
      retVal = true;
    }
  }
  return retVal;
}
