/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Daniel Glazman <glazman@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsEditProperty.h"
#include "nsString.h"

// singleton instance
static nsEditProperty *gInstance;

NS_IMPL_ADDREF(nsEditProperty)

NS_IMPL_RELEASE(nsEditProperty)


// XXX: remove when html atoms are exported from layout
// inline tags
nsIAtom * nsIEditProperty::b;          
nsIAtom * nsIEditProperty::big;        
nsIAtom * nsIEditProperty::i;          
nsIAtom * nsIEditProperty::small;      
nsIAtom * nsIEditProperty::strike;     
nsIAtom * nsIEditProperty::sub;        
nsIAtom * nsIEditProperty::sup;        
nsIAtom * nsIEditProperty::tt;        
nsIAtom * nsIEditProperty::u;          
nsIAtom * nsIEditProperty::em;         
nsIAtom * nsIEditProperty::strong;     
nsIAtom * nsIEditProperty::dfn;        
nsIAtom * nsIEditProperty::code;       
nsIAtom * nsIEditProperty::samp;       
nsIAtom * nsIEditProperty::kbd;        
nsIAtom * nsIEditProperty::var;        
nsIAtom * nsIEditProperty::cite;       
nsIAtom * nsIEditProperty::abbr;       
nsIAtom * nsIEditProperty::acronym;    
nsIAtom * nsIEditProperty::font;       
nsIAtom * nsIEditProperty::a;          
nsIAtom * nsIEditProperty::href;
nsIAtom * nsIEditProperty::name;
nsIAtom * nsIEditProperty::img;        
nsIAtom * nsIEditProperty::object;     
nsIAtom * nsIEditProperty::br;         
nsIAtom * nsIEditProperty::script;     
nsIAtom * nsIEditProperty::map;        
nsIAtom * nsIEditProperty::q;          
nsIAtom * nsIEditProperty::span;       
nsIAtom * nsIEditProperty::bdo;       
nsIAtom * nsIEditProperty::input;      
nsIAtom * nsIEditProperty::select;     
nsIAtom * nsIEditProperty::textarea;   
nsIAtom * nsIEditProperty::label;      
nsIAtom * nsIEditProperty::button;    
nsIAtom * nsIEditProperty::p;          
nsIAtom * nsIEditProperty::div;        
nsIAtom * nsIEditProperty::blockquote; 
nsIAtom * nsIEditProperty::h1;         
nsIAtom * nsIEditProperty::h2;         
nsIAtom * nsIEditProperty::h3;         
nsIAtom * nsIEditProperty::h4;         
nsIAtom * nsIEditProperty::h5;         
nsIAtom * nsIEditProperty::h6;         
nsIAtom * nsIEditProperty::ul;         
nsIAtom * nsIEditProperty::ol;         
nsIAtom * nsIEditProperty::dl;         
nsIAtom * nsIEditProperty::pre;        
nsIAtom * nsIEditProperty::noscript;   
nsIAtom * nsIEditProperty::form;      
nsIAtom * nsIEditProperty::hr;         
nsIAtom * nsIEditProperty::table;      
nsIAtom * nsIEditProperty::fieldset;   
nsIAtom * nsIEditProperty::address;    
nsIAtom * nsIEditProperty::body;       
nsIAtom * nsIEditProperty::head;       
nsIAtom * nsIEditProperty::tr;         
nsIAtom * nsIEditProperty::td;         
nsIAtom * nsIEditProperty::th;         
nsIAtom * nsIEditProperty::caption;    
nsIAtom * nsIEditProperty::col;        
nsIAtom * nsIEditProperty::colgroup;   
nsIAtom * nsIEditProperty::tbody;
nsIAtom * nsIEditProperty::thead;
nsIAtom * nsIEditProperty::tfoot;
nsIAtom * nsIEditProperty::li;         
nsIAtom * nsIEditProperty::dt;         
nsIAtom * nsIEditProperty::dd;         
nsIAtom * nsIEditProperty::legend;     
nsIAtom * nsIEditProperty::color;      
nsIAtom * nsIEditProperty::face;       
nsIAtom * nsIEditProperty::size;       

nsIAtom * nsIEditProperty::cssBackgroundColor;
nsIAtom * nsIEditProperty::cssBackgroundImage;
nsIAtom * nsIEditProperty::cssBorder;
nsIAtom * nsIEditProperty::cssCaptionSide;
nsIAtom * nsIEditProperty::cssColor;
nsIAtom * nsIEditProperty::cssFloat;
nsIAtom * nsIEditProperty::cssFontFamily;
nsIAtom * nsIEditProperty::cssFontSize;
nsIAtom * nsIEditProperty::cssFontStyle;
nsIAtom * nsIEditProperty::cssFontWeight;
nsIAtom * nsIEditProperty::cssHeight;
nsIAtom * nsIEditProperty::cssListStyleType;
nsIAtom * nsIEditProperty::cssMarginLeft;
nsIAtom * nsIEditProperty::cssMarginRight;
nsIAtom * nsIEditProperty::cssTextAlign;
nsIAtom * nsIEditProperty::cssTextDecoration;
nsIAtom * nsIEditProperty::cssVerticalAlign;
nsIAtom * nsIEditProperty::cssWhitespace;
nsIAtom * nsIEditProperty::cssWidth;


nsIAtom * nsIEditProperty::cssPxUnit;
nsIAtom * nsIEditProperty::cssEmUnit;
nsIAtom * nsIEditProperty::cssCmUnit;
nsIAtom * nsIEditProperty::cssPercentUnit;
nsIAtom * nsIEditProperty::cssInUnit;
nsIAtom * nsIEditProperty::cssMmUnit;
nsIAtom * nsIEditProperty::cssPtUnit;
nsIAtom * nsIEditProperty::cssPcUnit;
nsIAtom * nsIEditProperty::cssExUnit;

// special
nsString * nsIEditProperty::allProperties;

/* From the HTML 4.0 DTD, 

INLINE:
<!-- %inline; covers inline or "text-level" elements -->
 <!ENTITY % inline "#PCDATA | %fontstyle; | %phrase; | %special; | %formctrl;">
 <!ENTITY % fontstyle "TT | I | B | BIG | SMALL">
 <!ENTITY % phrase "EM | STRONG | DFN | CODE |
                    SAMP | KBD | VAR | CITE | ABBR | ACRONYM" >
 <!ENTITY % special
    "A | IMG | OBJECT | BR | SCRIPT | MAP | Q | SUB | SUP | SPAN | BDO">
 <!ENTITY % formctrl "INPUT | SELECT | TEXTAREA | LABEL | BUTTON">

BLOCK:
<!ENTITY % block
      "P | %heading (h1-h6); | %list (UL | OL); | %preformatted (PRE); | DL | DIV | NOSCRIPT |
       BLOCKQUOTE | FORM | HR | TABLE | FIELDSET | ADDRESS">

But what about BODY, TR, TD, TH, CAPTION, COL, COLGROUP, THEAD, TFOOT, LI, DT, DD, LEGEND, etc.?
 

*/
nsEditProperty::nsEditProperty()
{
  NS_INIT_ISUPPORTS();
  // inline tags
  nsIEditProperty::b          = NS_NewAtom("b");       
  nsIEditProperty::big        = NS_NewAtom("big");   
  nsIEditProperty::i          = NS_NewAtom("i");     
  nsIEditProperty::small      = NS_NewAtom("small"); 
  nsIEditProperty::strike     = NS_NewAtom("strike");
  nsIEditProperty::sub        = NS_NewAtom("sub");   
  nsIEditProperty::sup        = NS_NewAtom("sup");   
  nsIEditProperty::tt         = NS_NewAtom("tt");    
  nsIEditProperty::u          = NS_NewAtom("u");     
  nsIEditProperty::em         = NS_NewAtom("em");    
  nsIEditProperty::strong     = NS_NewAtom("strong");
  nsIEditProperty::dfn        = NS_NewAtom("dfn");   
  nsIEditProperty::code       = NS_NewAtom("code");  
  nsIEditProperty::samp       = NS_NewAtom("samp");  
  nsIEditProperty::kbd        = NS_NewAtom("kbd");   
  nsIEditProperty::var        = NS_NewAtom("var");   
  nsIEditProperty::cite       = NS_NewAtom("cite");  
  nsIEditProperty::abbr       = NS_NewAtom("abbr");  
  nsIEditProperty::acronym    = NS_NewAtom("acronym");
  nsIEditProperty::font       = NS_NewAtom("font");  
  nsIEditProperty::a          = NS_NewAtom("a");     
  nsIEditProperty::href       = NS_NewAtom("href");     // Use to differentiate between "a" for link, "a" for named anchor
  nsIEditProperty::name       = NS_NewAtom("name");   
  nsIEditProperty::img        = NS_NewAtom("img");   
  nsIEditProperty::object     = NS_NewAtom("object");
  nsIEditProperty::br         = NS_NewAtom("br");    
  nsIEditProperty::script     = NS_NewAtom("script");
  nsIEditProperty::map        = NS_NewAtom("map");   
  nsIEditProperty::q          = NS_NewAtom("q");     
  nsIEditProperty::span       = NS_NewAtom("span");  
  nsIEditProperty::bdo        = NS_NewAtom("bdo");   
  nsIEditProperty::input      = NS_NewAtom("input"); 
  nsIEditProperty::select     = NS_NewAtom("select");
  nsIEditProperty::textarea   = NS_NewAtom("textarea");
  nsIEditProperty::label      = NS_NewAtom("label");
  nsIEditProperty::button     = NS_NewAtom("button");
  // block tags
  nsIEditProperty::p          = NS_NewAtom("p");
  nsIEditProperty::div        = NS_NewAtom("div");
  nsIEditProperty::blockquote = NS_NewAtom("blockquote");
  nsIEditProperty::h1         = NS_NewAtom("h1");
  nsIEditProperty::h2         = NS_NewAtom("h2");
  nsIEditProperty::h3         = NS_NewAtom("h3");
  nsIEditProperty::h4         = NS_NewAtom("h4");
  nsIEditProperty::h5         = NS_NewAtom("h5");
  nsIEditProperty::h6         = NS_NewAtom("h6");
  nsIEditProperty::ul         = NS_NewAtom("ul");
  nsIEditProperty::ol         = NS_NewAtom("ol");
  nsIEditProperty::dl         = NS_NewAtom("dl");
  nsIEditProperty::pre        = NS_NewAtom("pre");
  nsIEditProperty::noscript   = NS_NewAtom("noscript");
  nsIEditProperty::form       = NS_NewAtom("form");
  nsIEditProperty::hr         = NS_NewAtom("hr");
  nsIEditProperty::table      = NS_NewAtom("table");
  nsIEditProperty::fieldset   = NS_NewAtom("fieldset");
  nsIEditProperty::address    = NS_NewAtom("address");
  // Unclear from 
  // DTD, block?
  nsIEditProperty::body       = NS_NewAtom("body");
  nsIEditProperty::head       = NS_NewAtom("head");
  nsIEditProperty::tr         = NS_NewAtom("tr");
  nsIEditProperty::td         = NS_NewAtom("td");
  nsIEditProperty::th         = NS_NewAtom("th");
  nsIEditProperty::caption    = NS_NewAtom("caption");
  nsIEditProperty::col        = NS_NewAtom("col");
  nsIEditProperty::colgroup   = NS_NewAtom("colgroup");
  nsIEditProperty::tbody      = NS_NewAtom("tbody");
  nsIEditProperty::thead      = NS_NewAtom("thead");
  nsIEditProperty::tfoot      = NS_NewAtom("tfoot");
  nsIEditProperty::li         = NS_NewAtom("li");
  nsIEditProperty::dt         = NS_NewAtom("dt");
  nsIEditProperty::dd         = NS_NewAtom("dd");
  nsIEditProperty::legend     = NS_NewAtom("legend");
  // inline 
  // properties
  nsIEditProperty::color      = NS_NewAtom("color");
  nsIEditProperty::face       = NS_NewAtom("face");
  nsIEditProperty::size       = NS_NewAtom("size");
  
  nsIEditProperty::cssBackgroundColor = NS_NewAtom("background-color");
  nsIEditProperty::cssBackgroundImage = NS_NewAtom("background-image");
  nsIEditProperty::cssBorder          = NS_NewAtom("border");
  nsIEditProperty::cssCaptionSide     = NS_NewAtom("caption-side");
  nsIEditProperty::cssColor           = NS_NewAtom("color");
  nsIEditProperty::cssFloat           = NS_NewAtom("float");
  nsIEditProperty::cssFontFamily      = NS_NewAtom("font-family");
  nsIEditProperty::cssFontSize        = NS_NewAtom("font-size");
  nsIEditProperty::cssFontStyle       = NS_NewAtom("font-style");
  nsIEditProperty::cssFontWeight      = NS_NewAtom("font-weight");
  nsIEditProperty::cssHeight          = NS_NewAtom("height");
  nsIEditProperty::cssListStyleType   = NS_NewAtom("list-style-type");
  nsIEditProperty::cssMarginRight     = NS_NewAtom("margin-right");
  nsIEditProperty::cssMarginLeft      = NS_NewAtom("margin-left");
  nsIEditProperty::cssTextAlign       = NS_NewAtom("text-align");
  nsIEditProperty::cssTextDecoration  = NS_NewAtom("text-decoration");
  nsIEditProperty::cssVerticalAlign   = NS_NewAtom("vertical-align");
  nsIEditProperty::cssWhitespace      = NS_NewAtom("white-space");
  nsIEditProperty::cssWidth           = NS_NewAtom("width");


  nsIEditProperty::cssPxUnit          = NS_NewAtom("px");
  nsIEditProperty::cssEmUnit          = NS_NewAtom("em");
  nsIEditProperty::cssCmUnit          = NS_NewAtom("cm");
  nsIEditProperty::cssPercentUnit     = NS_NewAtom("%");
  nsIEditProperty::cssInUnit          = NS_NewAtom("in");
  nsIEditProperty::cssMmUnit          = NS_NewAtom("mm");
  nsIEditProperty::cssPtUnit          = NS_NewAtom("pt");
  nsIEditProperty::cssPcUnit          = NS_NewAtom("pc");
  nsIEditProperty::cssExUnit          = NS_NewAtom("ex");

  // special
  if ( (nsIEditProperty::allProperties = new nsString) != nsnull )
    nsIEditProperty::allProperties->Assign(NS_LITERAL_STRING("moz_allproperties"));
}

nsEditProperty::~nsEditProperty()
{
  NS_IF_RELEASE(nsIEditProperty::b);          
  NS_IF_RELEASE(nsIEditProperty::big);        
  NS_IF_RELEASE(nsIEditProperty::i);          
  NS_IF_RELEASE(nsIEditProperty::small);      
  NS_IF_RELEASE(nsIEditProperty::strike);     
  NS_IF_RELEASE(nsIEditProperty::sub);        
  NS_IF_RELEASE(nsIEditProperty::sup);        
  NS_IF_RELEASE(nsIEditProperty::tt);         
  NS_IF_RELEASE(nsIEditProperty::u);          
  NS_IF_RELEASE(nsIEditProperty::em);         
  NS_IF_RELEASE(nsIEditProperty::strong);    
  NS_IF_RELEASE(nsIEditProperty::dfn);        
  NS_IF_RELEASE(nsIEditProperty::code);       
  NS_IF_RELEASE(nsIEditProperty::samp);       
  NS_IF_RELEASE(nsIEditProperty::kbd);        
  NS_IF_RELEASE(nsIEditProperty::var);        
  NS_IF_RELEASE(nsIEditProperty::cite);       
  NS_IF_RELEASE(nsIEditProperty::abbr);       
  NS_IF_RELEASE(nsIEditProperty::acronym);    
  NS_IF_RELEASE(nsIEditProperty::font);       
  NS_IF_RELEASE(nsIEditProperty::a);          
  NS_IF_RELEASE(nsIEditProperty::href);          
  NS_IF_RELEASE(nsIEditProperty::name);          
  NS_IF_RELEASE(nsIEditProperty::img);        
  NS_IF_RELEASE(nsIEditProperty::object);     
  NS_IF_RELEASE(nsIEditProperty::br);         
  NS_IF_RELEASE(nsIEditProperty::script);     
  NS_IF_RELEASE(nsIEditProperty::map);        
  NS_IF_RELEASE(nsIEditProperty::q);          
  NS_IF_RELEASE(nsIEditProperty::span);       
  NS_IF_RELEASE(nsIEditProperty::bdo);        
  NS_IF_RELEASE(nsIEditProperty::input);      
  NS_IF_RELEASE(nsIEditProperty::select);     
  NS_IF_RELEASE(nsIEditProperty::textarea);   
  NS_IF_RELEASE(nsIEditProperty::label);      
  NS_IF_RELEASE(nsIEditProperty::button);     
  NS_IF_RELEASE(nsIEditProperty::p);          
  NS_IF_RELEASE(nsIEditProperty::div);        
  NS_IF_RELEASE(nsIEditProperty::blockquote); 
  NS_IF_RELEASE(nsIEditProperty::h1);         
  NS_IF_RELEASE(nsIEditProperty::h2);         
  NS_IF_RELEASE(nsIEditProperty::h3);         
  NS_IF_RELEASE(nsIEditProperty::h4);         
  NS_IF_RELEASE(nsIEditProperty::h5);         
  NS_IF_RELEASE(nsIEditProperty::h6);         
  NS_IF_RELEASE(nsIEditProperty::ul);         
  NS_IF_RELEASE(nsIEditProperty::ol);         
  NS_IF_RELEASE(nsIEditProperty::dl);         
  NS_IF_RELEASE(nsIEditProperty::pre);        
  NS_IF_RELEASE(nsIEditProperty::noscript);   
  NS_IF_RELEASE(nsIEditProperty::form);       
  NS_IF_RELEASE(nsIEditProperty::hr);         
  NS_IF_RELEASE(nsIEditProperty::table);      
  NS_IF_RELEASE(nsIEditProperty::fieldset);   
  NS_IF_RELEASE(nsIEditProperty::address);    
  NS_IF_RELEASE(nsIEditProperty::body);       
  NS_IF_RELEASE(nsIEditProperty::head);       
  NS_IF_RELEASE(nsIEditProperty::tr);         
  NS_IF_RELEASE(nsIEditProperty::td);         
  NS_IF_RELEASE(nsIEditProperty::th);         
  NS_IF_RELEASE(nsIEditProperty::caption);    
  NS_IF_RELEASE(nsIEditProperty::col);        
  NS_IF_RELEASE(nsIEditProperty::colgroup);   
  NS_IF_RELEASE(nsIEditProperty::tbody);     
  NS_IF_RELEASE(nsIEditProperty::thead);     
  NS_IF_RELEASE(nsIEditProperty::tfoot);      
  NS_IF_RELEASE(nsIEditProperty::li);         
  NS_IF_RELEASE(nsIEditProperty::dt);         
  NS_IF_RELEASE(nsIEditProperty::dd);         
  NS_IF_RELEASE(nsIEditProperty::legend);     
  NS_IF_RELEASE(nsIEditProperty::color);      
  NS_IF_RELEASE(nsIEditProperty::face);       
  NS_IF_RELEASE(nsIEditProperty::size);      

  NS_IF_RELEASE(nsIEditProperty::cssBackgroundColor);
  NS_IF_RELEASE(nsIEditProperty::cssBackgroundImage);
  NS_IF_RELEASE(nsIEditProperty::cssBorder);
  NS_IF_RELEASE(nsIEditProperty::cssCaptionSide);
  NS_IF_RELEASE(nsIEditProperty::cssColor);
  NS_IF_RELEASE(nsIEditProperty::cssFloat);
  NS_IF_RELEASE(nsIEditProperty::cssFontFamily);
  NS_IF_RELEASE(nsIEditProperty::cssFontSize);
  NS_IF_RELEASE(nsIEditProperty::cssFontStyle);
  NS_IF_RELEASE(nsIEditProperty::cssFontWeight);
  NS_IF_RELEASE(nsIEditProperty::cssHeight);
  NS_IF_RELEASE(nsIEditProperty::cssListStyleType);
  NS_IF_RELEASE(nsIEditProperty::cssMarginRight);
  NS_IF_RELEASE(nsIEditProperty::cssMarginLeft);
  NS_IF_RELEASE(nsIEditProperty::cssTextAlign);
  NS_IF_RELEASE(nsIEditProperty::cssTextDecoration);
  NS_IF_RELEASE(nsIEditProperty::cssVerticalAlign);
  NS_IF_RELEASE(nsIEditProperty::cssWhitespace);
  NS_IF_RELEASE(nsIEditProperty::cssWidth);

  NS_IF_RELEASE(nsIEditProperty::cssPxUnit);
  NS_IF_RELEASE(nsIEditProperty::cssEmUnit);
  NS_IF_RELEASE(nsIEditProperty::cssCmUnit);
  NS_IF_RELEASE(nsIEditProperty::cssPercentUnit);
  NS_IF_RELEASE(nsIEditProperty::cssInUnit);
  NS_IF_RELEASE(nsIEditProperty::cssMmUnit);
  NS_IF_RELEASE(nsIEditProperty::cssPtUnit);
  NS_IF_RELEASE(nsIEditProperty::cssPcUnit);
  NS_IF_RELEASE(nsIEditProperty::cssExUnit);

  // special
  if (nsIEditProperty::allProperties) {
    delete (nsIEditProperty::allProperties);
    nsIEditProperty::allProperties = nsnull;
  }
  gInstance = nsnull;
}

NS_IMETHODIMP
nsEditProperty::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIEditProperty))) {
    *aInstancePtr = (void*)(nsIEditProperty*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

/* Factory for edit property object */
nsresult NS_NewEditProperty(nsIEditProperty **aResult)
{
  if (aResult)
  {
    if (!gInstance)
    {
      gInstance = new nsEditProperty();
      if (!gInstance) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    *aResult = gInstance;
    NS_ADDREF(*aResult);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}
