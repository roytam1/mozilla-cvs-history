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

function FillRecipientTypeCombobox()
{
	top.MAX_RECIPIENTS = 1;
	
	var body;

	for ( var row = 2; row <= top.MAX_RECIPIENTS; row++ )
	{
		body = document.getElementById('addressWidgetBody');
		
		awCopyNode(awGetTreeItem(1), body, 0);
	}
}

function Recipients2CompFields(msgCompFields)
{
	if (msgCompFields)
	{
		var i = 1;
		var addrTo = "";
		var addrCc = "";
		var addrBcc = "";
		var addrReply = "";
		var addrNg = "";
		var addrFollow = "";
		var addrOther = "";
		var to_Sep = "";
		var cc_Sep = "";
		var bcc_Sep = "";
		var reply_Sep = "";
		var ng_Sep = "";
		var follow_Sep = "";

	    while ((inputField = awGetInputElement(i)))
	    {
	    	fieldValue = inputField.value;
	    	if (fieldValue != "")
	    	{
	    		switch (awGetPopupElement(i).value)
	    		{
	    			case "addr_to"			: addrTo += to_Sep + fieldValue; to_Sep = ",";					break;
	    			case "addr_cc"			: addrCc += cc_Sep + fieldValue; cc_Sep = ",";					break;
	    			case "addr_bcc"			: addrBcc += bcc_Sep + fieldValue; bcc_Sep = ",";				break;
	    			case "addr_reply"		: addrReply += reply_Sep + fieldValue; reply_Sep = ",";			break;
	    			case "addr_newsgroups"	: addrNg += ng_Sep + fieldValue; ng_Sep = ",";					break;
	    			case "addr_followup"	: addrFollow += follow_Sep + fieldValue; follow_Sep = ",";		break;
					case "addr_other"		: addrOther += other_header + ": " + fieldValue + "\n"; 		break;
	    		}
	    	}
	    	i ++;
	    }
    	msgCompFields.SetTo(addrTo);
    	msgCompFields.SetCc(addrCc);
    	msgCompFields.SetBcc(addrBcc);
    	msgCompFields.SetReplyTo(addrReply);
    	msgCompFields.SetNewsgroups(addrNg);
    	msgCompFields.SetFollowupTo(addrFollow);
        msgCompFields.SetOtherRandomHeaders(addrOther);
	}
	else
		dump("Message Compose Error: msgCompFields is null (ExtractRecipients)");
}

function CompFields2Recipients(msgCompFields)
{
	if (msgCompFields)
	{
		var row = 1;
		
		row = awSetInputAndPopupFromArray(row, msgCompFields.SplitRecipients(msgCompFields.GetTo()), "addr_to");
		row = awSetInputAndPopupFromArray(row, msgCompFields.SplitRecipients(msgCompFields.GetCc()), "addr_cc");
		row = awSetInputAndPopupFromArray(row, msgCompFields.SplitRecipients(msgCompFields.GetBcc()), "addr_bcc");
		row = awSetInputAndPopupFromArray(row, msgCompFields.SplitRecipients(msgCompFields.GetReplyTo()), "addr_reply");
		row = awSetInputAndPopup(row, msgCompFields.GetOtherRandomHeaders(), "addr_other");
		row = awSetInputAndPopup(row, msgCompFields.GetNewsgroups(), "addr_newsgroups");
		row = awSetInputAndPopup(row, msgCompFields.GetFollowupTo(), "addr_followup");
		
		if ( row > 1 )   row--;
			
		// remove extra rows
		while ( top.MAX_RECIPIENTS > row )
			awRemoveRow(top.MAX_RECIPIENTS);
	}
}

function awSetInputAndPopup(firstRow, inputValue, popupValue)
{
	var		row = firstRow;
	
	if ( inputValue && popupValue )
	{
		var addressArray = inputValue.split(",");
		
		for ( var index = 0; index < addressArray.length; index++ )
		{
			// remove leading spaces
			while ( addressArray[index][0] == " " )
				addressArray[index] = addressArray[index].substring(1, addressArray[index].length);
			
			// we can add one row if trying to add just beyond current size
			if ( row == (top.MAX_RECIPIENTS + 1))
			{
				var body = document.getElementById('addressWidgetBody');
				awCopyNode(awGetTreeItem(1), body, 0);
				top.MAX_RECIPIENTS++;
			}
			
			// if row is legal then set the values
			if ( row <= top.MAX_RECIPIENTS )
			{
				awGetInputElement(row).value = addressArray[index];
				awGetPopupElement(row).value = popupValue;
				row++;
			}
		}
		
		return(row);
	}
	return(firstRow);		
}

function awSetInputAndPopupFromArray(firstRow, inputArray, popupValue)
{
	var		row = firstRow;
	
	if ( inputArray && popupValue )
	{
		for ( var index = 0; index < inputArray.count; index++ )
		{
			// remove leading spaces
			inputValue = inputArray.StringAt(index);
			while ( inputValue[0] == " " )
				inputValue = inputValue.substring(1, inputValue.length);
			
			// we can add one row if trying to add just beyond current size
			if ( row == (top.MAX_RECIPIENTS + 1))
			{
				var body = document.getElementById('addressWidgetBody');
				awCopyNode(awGetTreeItem(1), body, 0);
				top.MAX_RECIPIENTS++;
			}
			
			// if row is legal then set the values
			if ( row <= top.MAX_RECIPIENTS )
			{
				awGetInputElement(row).value = inputValue;
				awGetPopupElement(row).value = popupValue;
				row++;
			}
		}
		
		return(row);
	}
	return(firstRow);		
}

function awClickRow()
{
	dump("awClickRow\n");
	if (! top.notAnEmptyArea)
		awClickEmptySpace(true);
	top.notAnEmptyArea=false;
	return;
}

function awNotAnEmptyArea()
{
	dump("awNotAnEmptyArea\n");
	top.notAnEmptyArea=true;
	return;
}

function awClickEmptySpace(setFocus)
{
	var lastInput = awGetInputElement(top.MAX_RECIPIENTS);

	if ( lastInput && lastInput.value )
		awAppendNewRow(setFocus);
	else
		if (setFocus)
			awSetFocus(top.MAX_RECIPIENTS, lastInput);
}

function awReturnHit(inputElement)
{
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
	AutoCompleteAddress(inputElement);
	//call awClickEmptySpace to add a new row if needed
	awClickEmptySpace(false);
}

function awAppendNewRow(setFocus)
{
	var body = document.getElementById('addressWidgetBody');
	var treeitem1 = awGetTreeItem(1);
	
	if ( body && treeitem1 )
	{
		awCopyNode(treeitem1, body, 0);
		top.MAX_RECIPIENTS++;

		// focus on new input widget
		if (setFocus)
		{
			var newInput = awGetInputElement(top.MAX_RECIPIENTS);
			if ( newInput )
				awSetFocus(top.MAX_RECIPIENTS, newInput);
		}
	}
}


// functions for accessing the elements in the addressing widget

function awGetPopupElement(row)
{
	var treerow = awGetTreeRow(row);
	
	if ( treerow )
	{
		var popup = treerow.getElementsByTagName('SELECT');
		if ( popup && popup.length == 1 )
			return popup[0];
	}
	return 0;
}

function awGetInputElement(row)
{
	var treerow = awGetTreeRow(row);
	
	if ( treerow )
	{
		var input = treerow.getElementsByTagName('INPUT');
		if ( input && input.length == 1 )
			return input[0];
	}
	return 0;
}

function awGetTreeRow(row)
{
	var body = document.getElementById('addressWidgetBody');
	
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
	var body = document.getElementById('addressWidgetBody');
	
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
					return row;
			}
		}
	}
	return 0;
}


// Copy Node - copy this node and insert ahead of the (before) node.  Append to end if before=0
function awCopyNode(node, parentNode, beforeNode)
{
	dump("awCopyNode\n");
	
	var newNode = awCopyNodeAndChildren(node);
	
	if ( beforeNode )
		parentNode.insertBefore(newNode, beforeNode);
	else
		parentNode.appendChild(newNode);
}

function awCopyNodeAndChildren(node)
{
	var newNode;
	if ( node.nodeName == "#text" )
	{
		// create new text node
		newNode = document.createTextNode(node.data);
	}
	else
	{
		// create new node
		if ( node.nodeName[0] >= 'A' && node.nodeName[0] <= 'Z' )
			newNode = createHTML(node.nodeName);
		else
			newNode = document.createElement(node.nodeName);
		
		var attributes = node.attributes;
		
		if ( attributes && attributes.length )
		{
			var attrNode, name, value;
			
			// copy attributes into new node
			for ( var index = 0; index < attributes.length; index++ )
			{
				attrNode = attributes.item(index);
				name = attrNode.nodeName;
				value = attrNode.nodeValue;
				if ( name != 'id' )
					newNode.setAttribute(name, value);
			}
		}
	
		if ( node.nodeName == "SELECT" )
		{
			// copy options inside of SELECT
			if ( newNode )
			{
				for ( var index = 0; index < node.options.length; index++ )
				{
					var option = new Option(node.options.item(index).text,
											node.options.item(index).value)
					newNode.add(option, null);
				}
			}
		}
		else
		{
			// children of nodes
			if ( node.childNodes )
			{
				var childNode;
				
				for ( var child = 0; child < node.childNodes.length; child++ )
				{
					childNode = awCopyNodeAndChildren(node.childNodes[child]);
				
					newNode.appendChild(childNode);
				}
			}
		}
	}
	
	return newNode;
}

function createHTML(tag)
{
    return document.createElementWithNameSpace(tag, "http://www.w3.org/TR/REC-html40");
}

// remove row

function awRemoveRow(row)
{
	var body = document.getElementById('addressWidgetBody');
	
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
	var tree = document.getElementById('addressingWidgetTree');
	try
	{
		//temporary patch for bug 26344
		//top.awInputElement.setAttribute("onclick", top.awInputElement.getAttribute("onclick"));
		//top.awInputElement.setAttribute("onkeyup", top.awInputElement.getAttribute("onkeyup"));
		//end of patch

		tree.ensureElementIsVisible(awGetTreeRow(top.awRow));
		top.awInputElement.focus();
	}
	catch(ex)
	{
		top.awFocusRetry ++;
		if (top.awFocusRetry < 3)
		{
			dump("_awSetFocus failed, try it again...\n");
			setTimeout("_awSetFocus();", 0);
		}
		else
			dump("_awSetFocus failed, forget about it!\n");
	}
}
