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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/******

  This file contains the list of all HTML tags 
  See nsHTMLTags.h for access to the enum values for tags

  It is designed to be used as inline input to nsHTMLTags.cpp *only*
  through the magic of C preprocessing.

  All entires must be enclosed in the macro HTML_TAG which will have cruel
  and unusual things done to it

  It is recommended (but not strictly necessary) to keep all entries
  in alphabetical order

  The first argument to HTML_TAG is both the enum identifier of the property
  and the string value

 ******/

HTML_TAG(a)
HTML_TAG(abbr)
HTML_TAG(acronym)
HTML_TAG(address)
HTML_TAG(applet)
HTML_TAG(area)
HTML_TAG(b)
HTML_TAG(base)
HTML_TAG(basefont)
HTML_TAG(bdo)
HTML_TAG(bgsound)
HTML_TAG(big)
HTML_TAG(blink)
HTML_TAG(blockquote)
HTML_TAG(body)
HTML_TAG(br)
HTML_TAG(button)
HTML_TAG(caption)
HTML_TAG(center)
HTML_TAG(cite)
HTML_TAG(code)
HTML_TAG(col)
HTML_TAG(colgroup)
HTML_TAG(counter)
HTML_TAG(dd)
HTML_TAG(del)
HTML_TAG(dfn)
HTML_TAG(dir)
HTML_TAG(div)
HTML_TAG(dl)
HTML_TAG(dt)
HTML_TAG(em)
HTML_TAG(embed)
HTML_TAG(endnote)
HTML_TAG(fieldset)
HTML_TAG(font)
HTML_TAG(form)
HTML_TAG(frame)
HTML_TAG(frameset)
HTML_TAG(h1)
HTML_TAG(h2)
HTML_TAG(h3)
HTML_TAG(h4)
HTML_TAG(h5)
HTML_TAG(h6)
HTML_TAG(head)
HTML_TAG(hr)
HTML_TAG(html)
HTML_TAG(i)
HTML_TAG(iframe)
HTML_TAG(ilayer)
HTML_TAG(image)
HTML_TAG(img)
HTML_TAG(input)
HTML_TAG(ins)
HTML_TAG(isindex)
HTML_TAG(kbd)
HTML_TAG(keygen)
HTML_TAG(label)
HTML_TAG(layer)
HTML_TAG(legend)
HTML_TAG(li)
HTML_TAG(link)
HTML_TAG(listing)
HTML_TAG(map)
HTML_TAG(menu)
HTML_TAG(meta)
HTML_TAG(multicol)
HTML_TAG(nobr)
HTML_TAG(noembed)
HTML_TAG(noframes)
HTML_TAG(nolayer)
HTML_TAG(noscript)
HTML_TAG(object)
HTML_TAG(ol)
HTML_TAG(optgroup)
HTML_TAG(option)
HTML_TAG(p)
HTML_TAG(param)
HTML_TAG(parsererror)
HTML_TAG(plaintext)
HTML_TAG(pre)
HTML_TAG(q)
HTML_TAG(s)
HTML_TAG(samp)
HTML_TAG(script)
HTML_TAG(select)
HTML_TAG(server)
HTML_TAG(small)
HTML_TAG(sound)
HTML_TAG(sourcetext)
HTML_TAG(spacer)
HTML_TAG(span)
HTML_TAG(strike)
HTML_TAG(strong)
HTML_TAG(style)
HTML_TAG(sub)
HTML_TAG(sup)
HTML_TAG(table)
HTML_TAG(tbody)
HTML_TAG(td)
HTML_TAG(textarea)
HTML_TAG(tfoot)
HTML_TAG(th)
HTML_TAG(thead)
HTML_TAG(title)
HTML_TAG(tr)
HTML_TAG(tt)
HTML_TAG(u)
HTML_TAG(ul)
HTML_TAG(var)
HTML_TAG(wbr)
HTML_TAG(xmp)
