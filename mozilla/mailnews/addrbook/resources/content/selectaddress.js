var composeWindow = 0;
var msgCompFields = 0;

// localization strings
var prefixTo = "To: ";
var prefixCc = "Cc: ";
var prefixBcc = "Bcc: ";

function OnLoadSelectAddress()
{
	var toAddress="", ccAddress="", bccAddress="";

	doSetOKCancel(SelectAddressOKButton, 0);

	// look in arguments[0] for parameters
	if (window.arguments && window.arguments[0])
	{
		// keep parameters in global for later
		if ( window.arguments[0].composeWindow )
			top.composeWindow = window.arguments[0].composeWindow;
		if ( window.arguments[0].msgCompFields )
			top.msgCompFields = window.arguments[0].msgCompFields;
		if ( window.arguments[0].toAddress )
			toAddress = window.arguments[0].toAddress;
		if ( window.arguments[0].ccAddress )
			ccAddress = window.arguments[0].ccAddress;
		if ( window.arguments[0].bccAddress )
			bccAddress = window.arguments[0].bccAddress;
			
		dump("onload top.composeWindow: " + top.composeWindow + "\n");
		dump("onload toAddress: " + toAddress + "\n");

		// put the addresses into the bucket
		AddAddressFromComposeWindow(toAddress, prefixTo);
		AddAddressFromComposeWindow(ccAddress, prefixCc);
		AddAddressFromComposeWindow(bccAddress, prefixBcc);
	}
}

function AddAddressFromComposeWindow(addresses, prefix)
{
	if ( addresses )
	{
		var bucketDoc = frames["addressbucket"].document;
		var addressArray = addresses.split(",");
		
		for ( var index = 0; index < addressArray.length; index++ )
		{
			// remove leading spaces
			while ( addressArray[index][0] == " " )
				addressArray[index] = addressArray[index].substring(1, addressArray[index].length);
			
			AddAddressIntoBucket(bucketDoc, prefix + addressArray[index]);
		}
	}
}


function SelectAddressOKButton()
{
	var bucketDoc = frames["addressbucket"].document;
	var body = bucketDoc.getElementById('bucketBody');
	var item, row, cell, text, colon;
	var toAddress="", ccAddress="", bccAddress="";
	
	for ( var index = 0; index < body.childNodes.length; index++ )
	{
		item = body.childNodes[index];
		if ( item.childNodes && item.childNodes.length )
		{
			row = item.childNodes[0];
			if (  row.childNodes &&  row.childNodes.length )
			{
				cell = row.childNodes[0];
				if ( cell.childNodes &&  cell.childNodes.length )
				{
					text = cell.childNodes[0];
					if ( text && text.data && text.data.length )
					{
						switch ( text.data[0] )
						{
							case prefixTo[0]:
								if ( toAddress )
									toAddress += ", ";
								toAddress += text.data.substring(prefixTo.length, text.data.length);
								break;
							case prefixCc[0]:
								if ( ccAddress )
									ccAddress += ", ";
								ccAddress += text.data.substring(prefixCc.length, text.data.length);
								break;
							case prefixBcc[0]:
								if ( bccAddress )
									bccAddress += ", ";
								bccAddress += text.data.substring(prefixBcc.length, text.data.length);
								break;
						}
					}
				}
			}
		}
	}
	
	// reset the UI in compose window
	msgCompFields.SetTo(toAddress);
	msgCompFields.SetCc(ccAddress);
	msgCompFields.SetBcc(bccAddress);
	top.composeWindow.CompFields2Recipients(top.msgCompFields);

	return true;
}

function saChangeDirectoryByDOMNode(dirNode)
{
	var uri = dirNode.getAttribute('id');
	dump(uri + "\n");
	saChangeDirectoryByURI(uri);
}

function saChangeDirectoryByURI(uri)
{
	var tree = frames["resultsFrame"].document.getElementById('resultTree');
	//dump("tree = " + tree + "\n");
	tree.setAttribute('ref', uri);// body no longer valid after setting id.
}


function SelectAddressToButton()
{
	AddSelectedAddressesIntoBucket(prefixTo);
}

function SelectAddressCcButton()
{
	AddSelectedAddressesIntoBucket(prefixCc);
}

function SelectAddressBccButton()
{
	AddSelectedAddressesIntoBucket(prefixBcc);
}

function SelectAddressNewButton()
{
	AbNewCardDialog();
}

function SelectAddressEditButton()
{
	var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
	rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);

	var resultsDoc = frames["resultsFrame"].document;
	var selArray = resultsDoc.getElementsByAttribute('selected', 'true');

	if ( selArray && selArray.length == 1 )
	{
		var uri = selArray[0].getAttribute('id');
		var card = rdf.GetResource(uri);
		card = card.QueryInterface(Components.interfaces.nsIAbCard);
		AbEditCardDialog(card, 0);
	}
}

function AddSelectedAddressesIntoBucket(prefix)
{
	var item, uri, rdf, cardResource, card, address;
	var resultsDoc = frames["resultsFrame"].document;
	var bucketDoc = frames["addressbucket"].document;
	
	rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
	rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);

	var selArray = resultsDoc.getElementsByAttribute('selected', 'true');
	if ( selArray && selArray.length )
	{
		for ( item = 0; item < selArray.length; item++ )
		{
			uri = selArray[item].getAttribute('id');
			cardResource = rdf.GetResource(uri);
			card = cardResource.QueryInterface(Components.interfaces.nsIAbCard);
			address = prefix + "\"" + card.DisplayName + "\" <" + card.PrimaryEmail + ">";
			AddAddressIntoBucket(bucketDoc, address);
		}
	}	
}

function AddAddressIntoBucket(doc, address)
{
	var body = doc.getElementById("bucketBody");
	
	var item = doc.createElement('treeitem');
	var row = doc.createElement('treerow');
	var cell = doc.createElement('treecell');
	var text = doc.createTextNode(address);
	
	cell.appendChild(text);
	row.appendChild(cell);
	item.appendChild(row);
	body.appendChild(item);
}

function RemoveSelectedFromBucket()
{
	var bucketDoc = frames["addressbucket"].document;
	var body = bucketDoc.getElementById("bucketBody");
	
	var selArray = body.getElementsByAttribute('selected', 'true');
	if ( selArray && selArray.length )
	{
		for ( var item = selArray.length - 1; item >= 0; item-- )
			body.removeChild(selArray[item]);
	}	
}
