/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

var count = 0;

function trace(msg)
{
  dump("test " + msg + " (" + count + ")\n");
}

function findBody(node)
{
  var children = node.childNodes;
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children.item(count);
    if (child.tagName == "BODY") {
      return child;
    }
    var body = findBody(child);
    if (null != body) {
      return body;
    }
    count++;
  }
  return null;
}

function TestInsert(parent, child)
{
  var childTag = "(text)";
  if (child.nodeType != Node.TEXT_NODE) {
    childTag = child.tagName;
  }

  // Insert a piece of text before the span; this tests insertion at the
  // beginning
  trace("insert before [" + parent.tagName + "," + childTag + "]");
  count++;
  var beforeText = document.createTextNode("before ");
  parent.insertBefore(beforeText, child);

  // Insert a piece of text before the span; after the above, this tests
  // insertion in the middle.
  trace("insert middle [" + parent.tagName + "," + childTag + "]");
  count++;
  parent.insertBefore(document.createTextNode("middle "), child);
}

var body = findBody(document.documentElement);

// Create a block child with a span in it
var block = document.createElement("P");
var span = document.createElement("SPAN");
var spanText = document.createTextNode("Some text");
span.insertBefore(spanText, null);
block.insertBefore(span, null);

// Append the block to the body
body.insertBefore(block, null);

// Insert stuff into the block
TestInsert(block, span);

// Insert stuff into the span
TestInsert(span, spanText);
