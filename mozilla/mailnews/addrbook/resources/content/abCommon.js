var dirTree = 0;
var resultsTree = 0;

// functions needed from abMainWindow and abSelectAddresses

// Controller object for Results Pane
var ResultsPaneController =
{
  supportsCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
      case "cmd_delete":
      case "button_delete":
        return true;
      default:
        return false;
    }
  },

  isCommandEnabled: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        return true;
      
      case "cmd_delete":
      case "button_delete":
        var numSelected = 0;
        if (resultsTree && resultsTree.selectedItems)
          numSelected = resultsTree.selectedItems.length;
        if (command == "cmd_delete") {
          if (numSelected < 2)
            goSetMenuValue(command, "valueCard");
          else
            goSetMenuValue(command, "valueCards");
        }
        return (numSelected > 0);
      
      default:
        return false;
    }
  },

  doCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        if (resultsTree)
          resultsTree.selectAll();
        break;
      case "cmd_delete":
      case "button_delete":
        if (resultsTree) {
          var cardList = resultsTree.selectedItems;
          top.addressbook.deleteCards(resultsTree, resultsTree, cardList);
        }
        break;
    }
  },
  
  onEvent: function(event)
  {
    // on blur events set the menu item texts back to the normal values
    if (event == "blur")
      goSetMenuValue("cmd_delete", "valueDefault");
  }
};


// Controller object for Dir Pane
var DirPaneController =
{
  supportsCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
      case "cmd_delete":
      case "button_delete":
        return true;
      default:
        return false;
    }
  },

  isCommandEnabled: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        return true;
      case "cmd_delete":
      case "button_delete":
        if (command == "cmd_delete")
          goSetMenuValue(command, "valueAddressBook");
        if (dirTree && dirTree.selectedItems)
          return true;
        else
          return false;

      default:
        return false;
    }
  },

  doCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        if (dirTree)
          dirTree.selectAll();
        break;

      case "cmd_delete":
      case "button_delete":
        if (dirTree)
          AbDeleteDirectory();
//        top.addressbook.deleteAddressBooks(dirTree.database, dirTree, dirTree.selectedItems);
        break;
    }
  },
  
  onEvent: function(event)
  {
    // on blur events set the menu item texts back to the normal values
    if (event == "blur")
      goSetMenuValue("cmd_delete", "valueDefault");
  }
};

function InitCommonJS()
{
  dirTree = document.getElementById("dirTree");
  resultsTree = document.getElementById("resultsTree");
}

function SetupCommandUpdateHandlers()
{
  // dir pane
  if (dirTree)
    dirTree.controllers.appendController(DirPaneController);
  
  // results pane
  if (resultsTree)
    resultsTree.controllers.appendController(ResultsPaneController);
}


function AbNewCard()
{
  var selectedAB = 0;
  if (dirTree && dirTree.selectedItems && (dirTree.selectedItems.length == 1))
    selectedAB = dirTree.selectedItems[0].getAttribute("id");
    
  goNewCardDialog(selectedAB);
}

function AbEditCard()
{
  var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService();
  rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);

  if (resultsTree.selectedItems && resultsTree.selectedItems.length == 1) {
    var uri = resultsTree.selectedItems[0].getAttribute("id");
    var card = rdf.GetResource(uri);
    card = card.QueryInterface(Components.interfaces.nsIAbCard);
    if (card.isMailList) {
      var dirUri = dirTree.selectedItems[0].getAttribute("id");
      listUri = dirUri + "/MailList" + card.dbRowID;
      goEditListDialog(resultsTree.getAttribute("ref"), listUri);
    }
    else
      goEditCardDialog(resultsTree.getAttribute("ref"), card, top.gUpdateCardView);
  }
}

function AbNewMessage()
{
  var msgComposeType = Components.interfaces.nsIMsgCompType;
  var msgComposFormat = Components.interfaces.nsIMsgCompFormat;
  var msgComposeService = Components.classes["@mozilla.org/messengercompose;1"].getService(); 
  msgComposeService = msgComposeService.QueryInterface(Components.interfaces.nsIMsgComposeService); 

  msgComposeService.OpenComposeWindowWithValues(null, msgComposeType.New, msgComposFormat.Default,
                                                GetSelectedAddresses(), null, null,
                                                null, null, null, null, null); 
} 

function GetSelectedAddresses()
{
  var item, uri, rdf, cardResource, card;
  var selectedAddresses = "";
  
  rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService();
  rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);

  if (resultsTree.selectedItems && resultsTree.selectedItems.length) {
    for (item = 0; item < resultsTree.selectedItems.length; item++) {
      uri = resultsTree.selectedItems[item].getAttribute("id");
      cardResource = rdf.GetResource(uri);
      card = cardResource.QueryInterface(Components.interfaces.nsIAbCard);
      if (selectedAddresses)
        selectedAddresses += ",";
      selectedAddresses += "\"" + card.displayName + "\" <" + card.primaryEmail + ">";
    }
  }
  return selectedAddresses;  
}


function SelectFirstAddressBook()
{
  var body = document.getElementById("dirTreeBody");
  if (dirTree && body) {
    var treeitems = body.getElementsByTagName("treeitem");
    if (treeitems && treeitems.length > 0) {
      dirTree.selectItem(treeitems[0]);
      ChangeDirectoryByDOMNode(treeitems[0]);
    }
  }
}

function SelectFirstCard()
{
  var body = GetResultsTreeChildren();
  if (resultsTree && body) {
    var treeitems = body.getElementsByTagName("treeitem");
    if (treeitems && treeitems.length > 0) {
      resultsTree.selectItem(treeitems[0]);
      ResultsPaneSelectionChange();
    }
  }
}

function DirPaneSelectionChange()
{
  if (dirTree && dirTree.selectedItems && (dirTree.selectedItems.length == 1))
    ChangeDirectoryByDOMNode(dirTree.selectedItems[0]);
  else {
    if (resultsTree) {
      ClearResultsTreeSelection();
      resultsTree.setAttribute("ref", null);
    }
}

function ChangeDirectoryByDOMNode(dirNode)
{
  var uri = dirNode.getAttribute("id");
  
  if (resultsTree) {
    if (uri != resultsTree.getAttribute("ref")) {
      ClearResultsTreeSelection();
      resultsTree.setAttribute("ref", uri);
      WaitUntilDocumentIsLoaded();
      SortToPreviousSettings();
      SelectFirstCard();
    }
  }
}

function ResultsPaneSelectionChange()
{
  if ("gUpdateCardView" in top)
    top.gUpdateCardView();
  
  if ("gDialogResultsPaneSelectionChanged" in top)
    top.gDialogResultsPaneSelectionChanged();
}

function ClearResultsTreeSelection()
{
  if (resultsTree)
    resultsTree.clearItemSelection();
}

function RedrawResultsTree()
{
  if (resultsTree) {
    var ref = resultsTree.getAttribute("ref");
    resultsTree.setAttribute("ref", ref);
  }
}

function RememberResultsTreeSelection()
{
  var selectionArray = 0;
  
  if (resultsTree) {
    var selectedItems = resultsTree.selectedItems;
    var numSelected = selectedItems.length;

    selectionArray = new Array(numSelected);

    for (var i = 0; i < numSelected; i++) {
      selectionArray[i] = selectedItems[i].getAttribute("id");
      dump("selectionArray["+i+"] = " + selectionArray[i] + "\n");
    }
  }
  return selectionArray;
}

function RestoreResultsTreeSelection(selectionArray)
{
  if (resultsTree && selectionArray) {
    var numSelected = selectionArray.length;

    WaitUntilDocumentIsLoaded();

    var rowElement;
    for (var i = 0 ; i < numSelected; i++) {
      rowElement = document.getElementById(selectionArray[i]);
      resultsTree.addItemToSelection(rowElement);
      if (rowElement && (i==0))
        resultsTree.ensureElementIsVisible(rowElement);
    }
    ResultsPaneSelectionChange();
  }
}

function WaitUntilDocumentIsLoaded()
{
  // FIX ME - we should really have this function available in a global place that does not
  // require that we have access to mailnews code (msgNavigationService) from address book.
  var msgNavigationService = omponents.classes["@mozilla.org/messenger/msgviewnavigationservice;1"].getService();
  msgNavigationService= msgNavigationService.QueryInterface(Components.interfaces.nsIMsgViewNavigationService);
  msgNavigationService.EnsureDocumentIsLoaded(document);
}

function GetResultsTreeChildren()
{
  if (resultsTree && resultsTree.childNodes) {
    for (var index = resultsTree.childNodes.length - 1; index >= 0; index--) {
      if (resultsTree.childNodes[index].localName == "treechildren")
        return(resultsTree.childNodes[index]);
    }
  }
  return null;
}

function GetResultsTreeItem(row)
{
  var treechildren = GetResultsTreeChildren();
  
  if (treechildren && row > 0) {
    var treeitems = treechildren.getElementsByTagName("treeitem");
    if (treeitems && treeitems.length >= row)
      return treeitems[row-1];
  }
  return null;
}

function SortResultPane(column, sortKey)
{
  var node = document.getElementById(column);
  if (!node)
    return false;

  var sortDirection = "ascending";
  var currentDirection = node.getAttribute("sortDirection");

  if (currentDirection == "ascending")
    sortDirection = "descending";
  else
    sortDirection = "ascending";
  
  UpdateSortIndicator(column, sortDirection);
 
  DoSort(column, sortKey, sortDirection);

  SaveSortSetting(column, sortKey, sortDirection);

  return true;
}

function DoSort(column, key, direction)
{
  var isupports = Components.classes["@mozilla.org/rdf/xul-sort-service;1"].getService();
  if (!isupports)
    return false;
  
  var xulSortService = isupports.QueryInterface(Components.interfaces.nsIXULSortService);
  if (!xulSortService)
    return false;

  var node = document.getElementById(column);
  
  if (node) {
    var selectionArray = RememberResultsTreeSelection();
    if (selectionArray.length) {
      xulSortService.Sort(node, key, direction);
      ClearResultsTreeSelection();
      WaitUntilDocumentIsLoaded();
      RestoreResultsTreeSelection(selectionArray);
    }
    else {
      try {
        xulSortService.Sort(node, key, direction);
        WaitUntilDocumentIsLoaded();
      }
      catch(ex) {
        dump("failed to do sort\n");
      }
    } 
  }
  return true;
}
function SortToPreviousSettings()
{
  if (dirTree && resultsTree) {
    var ref = resultsTree.getAttribute("ref");
    var folder = document.getElementById(ref);
    if (folder) {
      var column = folder.getAttribute("sortColumn");  
      var key = folder.getAttribute("sortKey");
      var direction = folder.getAttribute("sortDirection");
      
      if (!column || !key) {
        column = "NameColumn";
        key = "http://home.netscape.com/NC-rdf#Name";
      }
      if (!direction)
        direction = "ascending";
      
      UpdateSortIndicator(column,direction);
      
      DoSort(column, key, direction);
    }  
  }
}

function SaveSortSetting(column, key, direction)
{
  if (dirTree && resultsTree) {
    var ref = resultsTree.getAttribute("ref");
    var folder = document.getElementById(ref);
    if (folder) {
      folder.setAttribute("sortColumn", column);  
      folder.setAttribute("sortKey", key);
      folder.setAttribute("sortDirection", direction);
    }  
  }
}

//------------------------------------------------------------
// Sets the column header sort icon based on the requested 
// column and direction.
// 
// Notes:
// (1) This function relies on the first part of the 
//     <treecell id> matching the <treecol id>.  The treecell
//     id must have a "Header" suffix.
// (2) By changing the "sortDirection" attribute, a different
//     CSS style will be used, thus changing the icon based on
//     the "sortDirection" parameter.
//------------------------------------------------------------
function UpdateSortIndicator(column,sortDirection)
{
  // Find the <treerow> element
  var treerow = document.getElementById("headRow");
  var id = column + "Header";
  
  if (treerow) {
    // Grab all of the <treecell> elements
    var treecell = treerow.getElementsByTagName("treecell");
    if (treecell) {
      // Loop through each treecell...
      var node_count = treecell.length;
      for (var i=0; i < node_count; i++) {
        // Is this the requested column ?
        if (id == treecell[i].getAttribute("id")) {
          // Set the sortDirection so the class (CSS) will add the
          // appropriate icon to the header cell
          treecell[i].setAttribute("sortDirection", sortDirection);
        }
        else {
          // This is not the sorted row
          treecell[i].removeAttribute("sortDirection");
        }
      }
    }
  }
}

function AbNewList()
{
  var selectedAB = 0;
  if (dirTree && dirTree.selectedItems && (dirTree.selectedItems.length == 1))
    selectedAB = dirTree.selectedItems[0].getAttribute("id");
    
  window.openDialog("chrome://messenger/content/addressbook/abMailListDialog.xul",
                    "",
                    "chrome,titlebar,resizeable=no",
                    {selectedAB:selectedAB});
}


function goEditListDialog(abURI, listURI)
{
  window.openDialog("chrome://messenger/content/addressbook/abEditListDialog.xul",
                    "",
                    "chrome,titlebar,resizeable=no",
                    {abURI:abURI, listURI:listURI});
}
