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
 */

top.MAX_RECIPIENTS = 1;
var inputElementType = "";

var mailList;
var parentURI;
var editList;
var hitReturnInList = false;

function handleKeyPress(element, event)
{
	if (event.which == 13) 
	{
		hitReturnInList = true;
		awReturnHit(element);
	}
}

function GetListValue(mailList, doAdd)
{
	mailList.listName = document.getElementById('ListName').value;

	if (mailList.listName.length == 0)
	{
		var strBundle = srGetStrBundle("chrome://messenger/locale/addressbook/addressBook.properties");
		var alertText = strBundle.GetStringFromName("emptyListName");
		alert(alertText);
		return false;
	}

	mailList.listNickName = document.getElementById('ListNickName').value;
	mailList.description = document.getElementById('ListDescription').value;
	
	var oldTotal = mailList.addressLists.Count();
	var i = 1;
	var pos = 0;
	while ((inputField = awGetInputElement(i)))
	{
	    fieldValue = inputField.value;
		if (doAdd || (doAdd == false && pos >= oldTotal))
			var cardproperty = Components.classes["component://netscape/addressbook/cardproperty"].createInstance();
		else
			var cardproperty = mailList.addressLists.GetElementAt(pos);
		if (cardproperty)
		{
			cardproperty = cardproperty.QueryInterface(Components.interfaces.nsIAbCard);
			if (cardproperty)
			{
				if (fieldValue != "")
				{
					var beginpos = fieldValue.search('<');
					var endpos = fieldValue.search('>');
					if (beginpos != -1)
					{
						beginpos++;
						var newValue = fieldValue.slice(beginpos, endpos);
						cardproperty.primaryEmail = newValue;
					}
					else
						cardproperty.primaryEmail = fieldValue;
					if (doAdd || (doAdd == false && pos >= oldTotal))
						mailList.addressLists.AppendElement(cardproperty);
					pos++;
				}
				else
				{
					if (doAdd == false)
					{
						cardproperty.primaryEmail = fieldValue;
						pos++;
					}
				}
			}
		}
	    i++;
	}
	return true;
}

function MailListOKButton()
{
	if (hitReturnInList)
	{
		hitReturnInList = false;
		return false;
	}
	var popup = document.getElementById('abPopup');
	if ( popup )
	{
		var uri = popup.getAttribute('data');
		
		// FIX ME - hack to avoid crashing if no ab selected because of blank option bug from template
		// should be able to just remove this if we are not seeing blank lines in the ab popup
		if ( !uri )
			return false;  // don't close window
		// -----
		
		//Add mailing list to database
		mailList = Components.classes["component://netscape/addressbook/directoryproperty"].createInstance();
		mailList = mailList.QueryInterface(Components.interfaces.nsIAbDirectory);

		if (GetListValue(mailList, true))
			mailList.addMailListToDatabase(uri);
		else
			return false;
	}		
	return true;	// close the window
}

function OnLoadMailList()
{
	doSetOKCancel(MailListOKButton, 0);
	
	if (window.arguments && window.arguments[0])
	{
		if ( window.arguments[0].selectedAB )
			selectedAB = window.arguments[0].selectedAB;
		else
			selectedAB = "abdirectory://abook.mab";
	}

	// set popup with address book names
	var abPopup = document.getElementById('abPopup');
	if ( abPopup )
	{
		var menupopup = document.getElementById('abPopup-menupopup');
		
		if ( selectedAB && menupopup && menupopup.childNodes )
		{
			for ( var index = menupopup.childNodes.length - 1; index >= 0; index-- )
			{
				if ( menupopup.childNodes[index].getAttribute('data') == selectedAB )
				{
					abPopup.value = menupopup.childNodes[index].getAttribute('value');
					abPopup.data = menupopup.childNodes[index].getAttribute('data');
					break;
				}
			}
		}
	}
	
	// focus on first name
	var listName = document.getElementById('ListName');
	if ( listName )
		listName.focus();
}

function EditListOKButton()
{
	if (hitReturnInList)
	{
		hitReturnInList = false;
		return false;
	}
	//Add mailing list to database
	if (GetListValue(editList, false))
	{
		editList.editMailListToDatabase(parentURI);
		return true;	// close the window
	}
	else
		return false;	
}

function OnLoadEditList()
{
	doSetOKCancel(EditListOKButton, 0);
	
	parentURI  = window.arguments[0].abURI;
	var listUri  = window.arguments[0].listURI;

	var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
	rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
	editList = rdf.GetResource(listUri);
	editList = editList.QueryInterface(Components.interfaces.nsIAbDirectory);

	document.getElementById('ListName').value = editList.listName;
	document.getElementById('ListNickName').value = editList.listNickName;
	document.getElementById('ListDescription').value = editList.description;

	var treeChildren = document.getElementById('addressList');
	var newTreeChildrenNode = treeChildren.cloneNode(false);
	var templateNode = treeChildren.firstChild;	
	top.MAX_RECIPIENTS = 0;

	if (editList.addressLists)
	{
		var total = editList.addressLists.Count();
dump("*** editList.Count = "+total+"\n");
		for ( var i = 0;  i < total; i++ )
		{
			var card = editList.addressLists.GetElementAt(i);
			card = card.QueryInterface(Components.interfaces.nsIAbCard);
			var address;
			if (card.name.length)
				address = card.name + " <" + card.primaryEmail + ">";
			else
				address = card.primaryEmail;
			SetInputValue(address, newTreeChildrenNode, templateNode);
		}
	}

    var parent = treeChildren.parentNode; 
    parent.replaceChild(newTreeChildrenNode, treeChildren); 

	// focus on first name
	var listName = document.getElementById('ListName');
	if ( listName )
		listName.focus();
}

function SetInputValue(inputValue, parentNode, templateNode)
{
    top.MAX_RECIPIENTS++;

    var newNode = templateNode.cloneNode(true);
    parentNode.appendChild(newNode); // we need to insert the new node before we set the value of the select element!

    var input = newNode.getElementsByTagName(awInputElementName());
    if ( input && input.length == 1 )
    {
		//We need to set the value using both setAttribute and .value else we will
		// loose the content when the field is not visible. See bug 37435
	    input[0].setAttribute("value", inputValue);
	    input[0].value = inputValue;
	    input[0].setAttribute("id", "address#" + top.MAX_RECIPIENTS);
	}
}

function awNotAnEmptyArea(event)
{
	//This is temporary until i figure out how to ensure to always having an empty space after the last row
	dump("awNotAnEmptyArea\n");

	var lastInput = awGetInputElement(top.MAX_RECIPIENTS);
	if ( lastInput && lastInput.value )
		awAppendNewRow(false);

	event.preventBubble();
}

function awClickEmptySpace(setFocus)
{
	dump("awClickEmptySpace\n");
	var lastInput = awGetInputElement(top.MAX_RECIPIENTS);

	if ( lastInput && lastInput.value )
		awAppendNewRow(setFocus);
	else
		if (setFocus)
			awSetFocus(top.MAX_RECIPIENTS, lastInput);
}

function awReturnHit(inputElement)
{
	dump("***** awReturnHit\n");
	var row = awGetRowByInputElement(inputElement);
	
	if ( inputElement.value )
	{
		var nextInput = awGetInputElement(row+1);
		if ( !nextInput )
			awAppendNewRow(true);
		else
			awSetFocus(row+1, nextInput);
	}
}

function awInputChanged(inputElement)
{
	dump("awInputChanged\n");
//	AutoCompleteAddress(inputElement);

	//Do we need to add a new row?
	var lastInput = awGetInputElement(top.MAX_RECIPIENTS);
	if ( lastInput && lastInput.value && !top.doNotCreateANewRow)
		awAppendNewRow(false);
	top.doNotCreateANewRow = false;
}

function awInputElementName()
{
    if (inputElementType == "")
        inputElementType = document.getElementById("address#1").localName;
    return inputElementType;
}

function awAppendNewRow(setFocus)
{
	var body = document.getElementById('addressList');
	var treeitem1 = awGetTreeItem(1);
	
	if ( body && treeitem1 )
	{
		newNode = awCopyNode(treeitem1, body, 0);
		top.MAX_RECIPIENTS++;

        var input = newNode.getElementsByTagName(awInputElementName());
        if ( input && input.length == 1 )
        {
    	    input[0].setAttribute("value", "");
    	    input[0].setAttribute("id", "address#" + top.MAX_RECIPIENTS);
    	}
		// focus on new input widget
		if (setFocus && input )
			awSetFocus(top.MAX_RECIPIENTS, input[0]);
	}
}


// functions for accessing the elements in the addressing widget

function awGetInputElement(row)
{
    return document.getElementById("address#" + row);
}

function awGetTreeRow(row)
{
	var body = document.getElementById('addressList');
	
	if ( body && row > 0)
	{
		var treerows = body.getElementsByTagName('treerow');
		if ( treerows && treerows.length >= row )
			return treerows[row-1];
	}
	return 0;
}

function awGetTreeItem(row)
{
	var body = document.getElementById('addressList');
	
	if ( body && row > 0)
	{
		var treeitems = body.getElementsByTagName('treeitem');
		if ( treeitems && treeitems.length >= row )
			return treeitems[row-1];
	}
	return 0;
}

function awGetRowByInputElement(inputElement)
{
	if ( inputElement )
	{
		var treerow;
		var inputElementTreerow = inputElement.parentNode.parentNode;
		
		if ( inputElementTreerow )
		{
			for ( var row = 1;  (treerow = awGetTreeRow(row)); row++ )
			{
				if ( treerow == inputElementTreerow )
				{
					return row;
				}
			}
		}
	}
	return 0;
}


// Copy Node - copy this node and insert ahead of the (before) node.  Append to end if before=0
function awCopyNode(node, parentNode, beforeNode)
{
	var newNode = node.cloneNode(true);
	
	if ( beforeNode )
		parentNode.insertBefore(newNode, beforeNode);
	else
		parentNode.appendChild(newNode);

    return newNode;
}

// remove row

function awRemoveRow(row)
{
	var body = document.getElementById('addressList');
	
	awRemoveNodeAndChildren(body, awGetTreeItem(row));

	top.MAX_RECIPIENTS--;
}

function awRemoveNodeAndChildren(parent, nodeToRemove)
{
	// children of nodes
	var childNode;
	
	while ( nodeToRemove.childNodes && nodeToRemove.childNodes.length )
	{
		childNode = nodeToRemove.childNodes[0];
	
		awRemoveNodeAndChildren(nodeToRemove, childNode);
	}
	
	parent.removeChild(nodeToRemove);

}

function awSetFocus(row, inputElement)
{
	top.awRow = row;
	top.awInputElement = inputElement;
	top.awFocusRetry = 0;
	setTimeout("_awSetFocus();", 0);
}

function _awSetFocus()
{
	var tree = document.getElementById('addressListTree');
	try
	{
		theNewRow = awGetTreeRow(top.awRow);
		//temporary patch for bug 26344
//		awFinishCopyNode(theNewRow);

		tree.ensureElementIsVisible(theNewRow);
		top.awInputElement.focus();
	}
	catch(ex)
	{
		top.awFocusRetry ++;
		if (top.awFocusRetry < 8)
		{
			dump("_awSetFocus failed, try it again...\n");
			setTimeout("_awSetFocus();", 0);
		}
		else
			dump("_awSetFocus failed, forget about it!\n");
	}
}


//temporary patch for bug 26344 & 26528
function awFinishCopyNode(node)
{
    msgCompose.ResetNodeEventHandlers(node);
    return;
}


function awFinishCopyNodes()
{
	var treeChildren = document.getElementById('addressList');
    awFinishCopyNode(treeChildren);
}


function awTabFromRecipient(element, event)
{
	//If we are le last element in the tree, we don't want to create a new row.
	if (element == awGetInputElement(top.MAX_RECIPIENTS))
		top.doNotCreateANewRow = true;
}

function awGetNumberOfRecipients()
{
    return top.MAX_RECIPIENTS;
}

function DragOverTree(event)
{
	var validFlavor = false;
	var dragSession = null;
	var retVal = true;

	var dragService = Components.classes["component://netscape/widget/dragservice"].getService();
	if (dragService) 
		dragService = dragService.QueryInterface(Components.interfaces.nsIDragService);
	if (!dragService)	return(false);

	dragSession = dragService.getCurrentSession();
	if (!dragSession)	return(false);

	if (dragSession.isDataFlavorSupported("text/nsabcard"))	validFlavor = true;
	//XXX other flavors here...

	// touch the attribute on the rowgroup to trigger the repaint with the drop feedback.
	if (validFlavor)
	{
		//XXX this is really slow and likes to refresh N times per second.
		var rowGroup = event.target.parentNode.parentNode;
		rowGroup.setAttribute ( "dd-triggerrepaint", 0 );
		dragSession.canDrop = true;
		// necessary??
		retVal = false; // do not propagate message
	}
	return(retVal);
}

function DropOnAddressListTree(event)
{
	var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
	if (rdf)   
		rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
	if (!rdf) return(false);

	var dragService = Components.classes["component://netscape/widget/dragservice"].getService();
	if (dragService) 
		dragService = dragService.QueryInterface(Components.interfaces.nsIDragService);
	if (!dragService)	return(false);
	
	var dragSession = dragService.getCurrentSession();
	if ( !dragSession )	return(false);

	var trans = Components.classes["component://netscape/widget/transferable"].createInstance(Components.interfaces.nsITransferable);
	if ( !trans ) return(false);
	trans.addDataFlavor("text/nsabcard");

	for ( var i = 0; i < dragSession.numDropItems; ++i )
	{
		dragSession.getData ( trans, i );
		dataObj = new Object();
		bestFlavor = new Object();
		len = new Object();
		trans.getAnyTransferData ( bestFlavor, dataObj, len );
		if ( dataObj )	dataObj = dataObj.value.QueryInterface(Components.interfaces.nsISupportsWString);
		if ( !dataObj )	continue;

		// pull the URL out of the data object
		var sourceID = dataObj.data.substring(0, len.value);
		if (!sourceID)	continue;

		var cardResource = rdf.GetResource(sourceID);
		var card = cardResource.QueryInterface(Components.interfaces.nsIAbCard);
		
		if (card.isMailList)
			DropListAddress(card.name); 
		else
		{
			var address;
			if (card.name.length)
				address = card.name + " <" + card.primaryEmail + ">";
			else
				address = card.primaryEmail;
			DropListAddress(address); 
		}
		
	}

	return(false);
}

function DropListAddress(address) 
{ 
    awClickEmptySpace(true);    //that will automatically set the focus on a new available row, and make sure is visible 
    if (top.MAX_RECIPIENTS == 0)
		top.MAX_RECIPIENTS = 1;
	var lastInput = awGetInputElement(top.MAX_RECIPIENTS); 
    lastInput.value = address; 
    awAppendNewRow(true); 
}
