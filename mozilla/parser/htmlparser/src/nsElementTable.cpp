/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */


/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 * 
 */ 

#include "nsElementTable.h"


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool Contains(eHTMLTags aTag,TagList& aTagList){
  PRBool result=FindTagInSet(aTag,aTagList.mTags,aTagList.mCount);
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param  
 * @return
 */
PRInt32 GetTopmostIndexOf(nsEntryStack& aTagStack,TagList& aTagList){
  int max = aTagStack.GetCount();
  int index=0;
  for(index=max-1;index>=0;index--){
    if(FindTagInSet(aTagStack[index],aTagList.mTags,aTagList.mCount)) {
      return index;
    }
  }
  return kNotFound;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
eHTMLTags GetTagAt(PRUint32 anIndex,TagList& aTagList) {
  eHTMLTags result=eHTMLTag_unknown;
  if(anIndex<aTagList.mCount){
    result=aTagList.mTags[anIndex];
  }
  return result;
}


/***************************************************************************** 
  Now it's time to list all the html elements all with their capabilities...
******************************************************************************/


//First, define the set of taglists for tags with special parents...
TagList  gAParents={1,eHTMLTag_map};
TagList  gInAddress={1,eHTMLTag_address};
TagList  gInHead={1,eHTMLTag_head};
TagList  gInTable={1,eHTMLTag_table};
TagList  gInHTML={1,eHTMLTag_html};
TagList  gInBody={1,eHTMLTag_body};
TagList  gInForm={1,eHTMLTag_form};
TagList  gInFieldset={1,eHTMLTag_fieldset};
TagList  gInTR={1,eHTMLTag_tr};
TagList  gInDL={2,eHTMLTag_dl,eHTMLTag_body};
TagList  gInFrameset={1,eHTMLTag_frameset};
TagList  gInNoframes={1,eHTMLTag_noframes};
TagList  gInP={4,eHTMLTag_address,eHTMLTag_form,eHTMLTag_span,eHTMLTag_table};
TagList  gOptgroupParents={2,eHTMLTag_optgroup,eHTMLTag_select};
TagList  gBodyParents={2,eHTMLTag_html,eHTMLTag_noframes};
TagList  gColParents={2,eHTMLTag_table,eHTMLTag_colgroup};
TagList  gFramesetParents={2,eHTMLTag_html,eHTMLTag_frameset};
TagList  gLegendParents={1,eHTMLTag_fieldset};
TagList  gAreaParent={1,eHTMLTag_map};
TagList  gParamParents={2,eHTMLTag_applet,eHTMLTag_object};
TagList  gTRParents={4,eHTMLTag_tbody,eHTMLTag_tfoot,eHTMLTag_thead,eHTMLTag_table};
TagList  gTREndParents={6,eHTMLTag_tbody,eHTMLTag_tfoot,eHTMLTag_thead,eHTMLTag_table,eHTMLTag_td,eHTMLTag_th};


//*********************************************************************************************
//  Next, define the set of taglists for tags with special kids...
//*********************************************************************************************

TagList  gContainsText={4,eHTMLTag_text,eHTMLTag_newline,eHTMLTag_whitespace,eHTMLTag_entity};
TagList  gUnknownKids={2,eHTMLTag_html,eHTMLTag_frameset};
TagList  gContainsOpts={3,eHTMLTag_option,eHTMLTag_optgroup,eHTMLTag_script};
TagList  gContainsParam={1,eHTMLTag_param};
TagList  gColgroupKids={1,eHTMLTag_col}; 
TagList  gAddressKids={1,eHTMLTag_p};
TagList  gBodyKids={5,eHTMLTag_del,eHTMLTag_ins,eHTMLTag_noscript,eHTMLTag_script,eHTMLTag_li};
TagList  gButtonKids={2,eHTMLTag_caption,eHTMLTag_legend};
TagList  gDLKids={2,eHTMLTag_dd,eHTMLTag_dt};
TagList  gDTKids={1,eHTMLTag_dt};
TagList  gFieldsetKids={2,eHTMLTag_legend,eHTMLTag_text};
TagList  gFontKids={2,eHTMLTag_legend,eHTMLTag_text};
TagList  gFormKids={1,eHTMLTag_keygen};
TagList  gFramesetKids={3,eHTMLTag_frame,eHTMLTag_frameset,eHTMLTag_noframes};

TagList  gHtmlKids={8,eHTMLTag_body,eHTMLTag_frameset,eHTMLTag_head,eHTMLTag_map,eHTMLTag_noscript,eHTMLTag_script,eHTMLTag_newline,eHTMLTag_whitespace};
TagList  gHeadKids={9,eHTMLTag_base,eHTMLTag_bgsound,eHTMLTag_link,eHTMLTag_meta,eHTMLTag_script,eHTMLTag_style,eHTMLTag_title,eHTMLTag_noembed,eHTMLTag_noscript};

TagList  gLabelKids={1,eHTMLTag_span};
TagList  gLIKids={2,eHTMLTag_ol,eHTMLTag_ul};
TagList  gMapKids={1,eHTMLTag_area};
TagList  gNoframesKids={1,eHTMLTag_body};
TagList  gPreKids={2,eHTMLTag_hr,eHTMLTag_center};  //note that CENTER is here for backward compatibility; it's not 4.0 spec.

TagList  gTableKids={10,eHTMLTag_caption,eHTMLTag_col,eHTMLTag_colgroup,eHTMLTag_form,
                     eHTMLTag_thead,eHTMLTag_tbody,eHTMLTag_tfoot,
                     eHTMLTag_map,eHTMLTag_script,eHTMLTag_input};
  
TagList  gTableElemKids={7,eHTMLTag_form,eHTMLTag_map,eHTMLTag_noscript,eHTMLTag_script,eHTMLTag_td,eHTMLTag_th,eHTMLTag_tr};
TagList  gTRKids={6,eHTMLTag_td,eHTMLTag_th,eHTMLTag_map,eHTMLTag_form,eHTMLTag_script,eHTMLTag_input};
TagList  gTBodyKids={3,eHTMLTag_tr,eHTMLTag_form,eHTMLTag_input};
TagList  gULKids={2,eHTMLTag_li,eHTMLTag_p};


//*********************************************************************************************
// The following tag lists are used to define common set of root notes for the HTML elements...
//*********************************************************************************************

TagList  gRootTags={3,eHTMLTag_body,eHTMLTag_td,eHTMLTag_table};
TagList  gHTMLRootTags={1,eHTMLTag_unknown};

TagList  gLIRootTags={7,eHTMLTag_ul,eHTMLTag_ol,eHTMLTag_dir,eHTMLTag_menu,eHTMLTag_p,eHTMLTag_body,eHTMLTag_td};

TagList  gOLRootTags={3,eHTMLTag_body,eHTMLTag_li,eHTMLTag_td};
TagList  gTDRootTags={3,eHTMLTag_tr,eHTMLTag_tbody,eHTMLTag_table};
TagList  gNoframeRoot={2,eHTMLTag_body,eHTMLTag_frameset};

//*********************************************************************************************
// The following tag lists are used to define the autoclose properties of the html elements...
//*********************************************************************************************

TagList  gBodyAutoClose={1,eHTMLTag_head};
TagList  gTBodyAutoClose={3,eHTMLTag_thead,eHTMLTag_tfoot,eHTMLTag_tbody};
TagList  gCaptionAutoClose={1,eHTMLTag_tbody};
TagList  gLIAutoClose={2,eHTMLTag_p,eHTMLTag_li};
TagList  gPAutoClose={2,eHTMLTag_p,eHTMLTag_li};
TagList  gHRAutoClose={1,eHTMLTag_p};
TagList  gOLAutoClose={3,eHTMLTag_p,eHTMLTag_ol,eHTMLTag_ul};
TagList  gDivAutoClose={1,eHTMLTag_p};

TagList  gHeadingTags={6,eHTMLTag_h1,eHTMLTag_h2,eHTMLTag_h3,eHTMLTag_h4,eHTMLTag_h5,eHTMLTag_h6};

TagList  gTRCloseTags={3,eHTMLTag_tr,eHTMLTag_td,eHTMLTag_th};
TagList  gTDCloseTags={2,eHTMLTag_td,eHTMLTag_th};
TagList  gDTCloseTags={3,eHTMLTag_dt,eHTMLTag_dd,eHTMLTag_p};
TagList  gULCloseTags={1,eHTMLTag_li};

//*********************************************************************************************
// The following tag lists are used to define the non-autoclose properties of the html elements...
//*********************************************************************************************

TagList  gDontAutoClose={1,eHTMLTag_td};

//*********************************************************************************************
//Lastly, bind tags with their rules, their special parents and special kids.
//*********************************************************************************************


#define FSTYPE kInlineEntity
#define SPECIALTYPE kInlineEntity

const int kNoPropRange=0;
const int kDefaultPropRange=1;
const int kBodyPropRange=2;



//*********************************************************************************************
//
//        Now let's dynamically build the element table...
//
//*********************************************************************************************
nsHTMLElement* gHTMLElements=0;


void Initialize(eHTMLTags aTag,
                eHTMLTags aRequiredAncestor,
                eHTMLTags aExcludingAncestor, 
                TagList* aRootNodes, 
                TagList* aEndRootNodes,
                TagList* aAutocloseStart,    
                TagList* aAutocloseEnd,      
                TagList* aSynonymousTags,    
                TagList* aDontAutocloseEnd,  
                int       aParentBits,        
                int       aInclusionBits, 
                int       aExclusionBits,     
                int       aSpecialProperties,
                int       aPropagateRange,
                TagList* aSpecialParents,    
                TagList* aSpecialKids,    
                eHTMLTags aSkipTarget
                ) 
{
  gHTMLElements[aTag].mTagID=aTag;
  gHTMLElements[aTag].mRequiredAncestor=aRequiredAncestor;
  gHTMLElements[aTag].mExcludingAncestor=aExcludingAncestor; 
  gHTMLElements[aTag].mRootNodes=aRootNodes;         
  gHTMLElements[aTag].mEndRootNodes=aEndRootNodes;
  gHTMLElements[aTag].mAutocloseStart=aAutocloseStart;
  gHTMLElements[aTag].mAutocloseEnd=aAutocloseEnd;
  gHTMLElements[aTag].mSynonymousTags=aSynonymousTags;
  gHTMLElements[aTag].mDontAutocloseEnd=aDontAutocloseEnd;
  gHTMLElements[aTag].mParentBits=aParentBits;
  gHTMLElements[aTag].mInclusionBits=aInclusionBits;
  gHTMLElements[aTag].mExclusionBits=aExclusionBits;
  gHTMLElements[aTag].mSpecialProperties=aSpecialProperties;
  gHTMLElements[aTag].mPropagateRange=aPropagateRange;
  gHTMLElements[aTag].mSpecialParents=aSpecialParents;
  gHTMLElements[aTag].mSpecialKids=aSpecialKids;
  gHTMLElements[aTag].mSkipTarget=aSkipTarget;
}


void InitializeElementTable(void) {          
  if(!gHTMLElements) {
    gHTMLElements=new nsHTMLElement[eHTMLTag_userdefined+5];

    Initialize(
        /*tag*/                             eHTMLTag_unknown,
        /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	      /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,		
        /*autoclose starttags and endtags*/ 0,0,0,0,
        /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
        /*special props, prop-range*/       kOmitWS, 10,
        /*special parents,kids,skip*/       0,&gUnknownKids,eHTMLTag_unknown);       

    /*************************************************
      Note: I changed A to contain flow elements
            since it's such a popular (but illegal) 
            idiom.
     *************************************************/

    Initialize( 
      /*tag*/                             eHTMLTag_a,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, SPECIALTYPE|kBlockEntity, kNone,	  
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_abbr,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_acronym,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_address,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kInlineEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gAddressKids,eHTMLTag_unknown); 

    Initialize( 
      /*tag*/                             eHTMLTag_applet,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gContainsParam,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_area,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gAreaParent,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kInlineEntity, kSelf,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gAreaParent,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_b,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (FSTYPE|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_base,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHead,	&gRootTags,
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       &gInHead,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_basefont,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, SPECIALTYPE, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_bdo,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_bgsound,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, kNone, kNone,	
      /*special props, prop-range*/       0,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_big,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (FSTYPE|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_blink,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_blockquote,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_body,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_frameset,
	    /*rootnodes,endrootnodes*/          &gInHTML,	&gInHTML,
      /*autoclose starttags and endtags*/ &gBodyAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kHTMLContent,(kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       kOmitEndTag, kBodyPropRange,
      /*special parents,kids,skip*/       &gInNoframes,&gBodyKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_br,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, kNone, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_button,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFormControl, kFlowEntity, kFormControl,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gButtonKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_caption,
      /*req-parent excl-parent*/          eHTMLTag_table,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInTable,&gInTable,	
      /*autoclose starttags and endtags*/ &gCaptionAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kFlowEntity, kSelf,	
      /*special props, prop-range*/       kNoPropagate,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInTable,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_center,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, (kInlineEntity|kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_cite,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_code,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_col,
      /*req-parent excl-parent*/          eHTMLTag_table,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gColParents,&gColParents,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kNoPropagate|kOmitWS,kDefaultPropRange,
      /*special parents,kids,skip*/       &gColParents,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_colgroup,
      /*req-parent excl-parent*/          eHTMLTag_table,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInTable,&gInTable,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kOmitWS|kNoPropagate,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInTable,&gColgroupKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_dd,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,	&gRootTags,	
      /*autoclose starttags and endtags*/ &gDTCloseTags,0,0,0,
      /*parent,incl,exclgroups*/          kInlineEntity, kFlowEntity, kNone,	
      /*special props, prop-range*/       kNoPropagate|kMustCloseSelf,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInDL,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_del,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInBody,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_dfn,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_dir,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kList, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gULKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_div,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gDivAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_dl,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kSelf|kFlowEntity, kNone,	
      /*special props, prop-range*/       kOmitWS, kNoPropRange,
      /*special parents,kids,skip*/       0,&gDLKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_dt,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,	&gRootTags,	
      /*autoclose starttags and endtags*/ &gDTCloseTags,0,0,0,
      /*parent,incl,exclgroups*/          kInlineEntity, kFlowEntity, kNone,	
      /*special props, prop-range*/       (kNoPropagate|kMustCloseSelf),kDefaultPropRange,
      /*special parents,kids,skip*/       &gInDL,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_em,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_embed,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlockEntity, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gContainsParam,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_endnote,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_fieldset,
      /*requiredAncestor*/                eHTMLTag_form,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       kNoPropagate|kOmitWS,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gFieldsetKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_font,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gFontKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_form,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kFlowEntity, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       0,&gFormKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_frame, 
      /*req-parent excl-parent*/          eHTMLTag_frameset,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInFrameset,&gInFrameset,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kNoPropagate|kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       &gInFrameset,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_frameset,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_body,
	    /*rootnodes,endrootnodes*/          &gFramesetParents,&gInHTML,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHTMLContent, kSelf, kAllTags,	
      /*special props, prop-range*/       kOmitWS|kNoPropagate|kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       &gInHTML,&gFramesetKids,eHTMLTag_unknown);


    Initialize( 
      /*tag*/                             eHTMLTag_h1,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_h2,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_h3,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_h4,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_h5,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_h6,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
      /*parent,incl,exclgroups*/          kHeading, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_head,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHTML,	&gInHTML,
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHTMLContent, (kHeadContent|kHeadMisc), kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gInHTML,&gHeadKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_hr,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gHRAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_html,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_html,
	    /*rootnodes,endrootnodes*/          &gHTMLRootTags,	&gHTMLRootTags,
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kHTMLContent, kNone,	
      /*special props, prop-range*/       kSaveMisplaced|kOmitEndTag|kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gHtmlKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_i,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0, kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_iframe,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_ilayer,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kFlowEntity, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_image,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_img,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_input,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFormControl, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_ins,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_isindex,
      /*requiredAncestor*/                eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          (kBlock|kHeadContent), kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInBody,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_kbd,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_keygen,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_label,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFormControl, kInlineEntity, kSelf,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gLabelKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_layer,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kFlowEntity|kSelf, kSelf,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_legend,
      /*requiredAncestor*/                eHTMLTag_form,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInFieldset,&gInFieldset,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kInlineEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInFieldset,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_li,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gLIRootTags,&gLIRootTags,	
      /*autoclose starttags and endtags*/ &gLIAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kFlowEntity, kSelf,	
      /*special props, prop-range*/       kNoPropagate, kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gLIKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_link,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHead,&gInHead,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHeadMisc, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInHead,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_listing,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPreformatted, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_map,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, SPECIALTYPE|kBlockEntity, kNone,	
      /*special props, prop-range*/       kOmitWS, kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gMapKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_menu,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kList, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gULKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_meta,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHead,	&gInHead,
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHeadMisc, kNone, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gInHead,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_multicol,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_nobr,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, (kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_noembed, 
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kDiscardTag, kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_noembed);

    Initialize( 
      /*tag*/                             eHTMLTag_noframes,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gNoframeRoot,&gNoframeRoot,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kFlowEntity, kNone,	
      /*special props, prop-range*/       kDiscardTag, kNoPropRange,
      /*special parents,kids,skip*/       &gNoframeRoot,&gNoframesKids,eHTMLTag_noframes);

    Initialize( 
      /*tag*/                             eHTMLTag_nolayer,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kDiscardTag, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_nolayer);

    Initialize( 
      /*tag*/                             eHTMLTag_noscript,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kFlowEntity, kNone,	
      /*special props, prop-range*/       kDiscardTag, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_noscript);

    Initialize( 
      /*tag*/                             eHTMLTag_object,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          (kHeadMisc|kSpecial), (kFlowEntity|SPECIALTYPE|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gContainsParam,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_ol,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gOLRootTags,&gOLRootTags,	
      /*autoclose starttags and endtags*/ &gOLAutoClose, &gULCloseTags, 0,0,
      /*parent,incl,exclgroups*/          kList, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gULKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_optgroup,
      /*requiredAncestor*/                eHTMLTag_select,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gOptgroupParents,&gOptgroupParents,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gOptgroupParents,&gContainsOpts,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_option,
      /*requiredAncestor*/                eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gOptgroupParents,&gOptgroupParents,	 
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kPCDATA, kFlowEntity,	
      /*special props, prop-range*/       kNoPropagate|kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gOptgroupParents,&gContainsText,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_p,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInBody,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,&gDontAutoClose,
      /*parent,incl,exclgroups*/          kBlock, kInlineEntity, kNone,	  //this used to contain FLOW. But it's really an inline container.
      /*special props, prop-range*/       0,kDefaultPropRange,                      //otherwise it tries to contain things like H1..H6
      /*special parents,kids,skip*/       0,&gInP,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_param,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gParamParents,	&gParamParents,	
      /*autoclose starttags and endtags*/ &gPAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       &gParamParents,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_parsererror,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gDivAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kNone, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_plaintext,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, kFlowEntity, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_html);

    Initialize( 
      /*tag*/                             eHTMLTag_pre,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPreformatted, kFlowEntity, kNone,	//I'm allowing WAY too much in here. Spec says inline.
      /*special props, prop-range*/       kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gPreKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_q,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);
 
    Initialize( 
      /*tag*/                             eHTMLTag_s,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_samp,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_script,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          (kSpecial|kHeadMisc), kPCDATA, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn|kLegalOpen, kNoPropRange,
      /*special parents,kids,skip*/       0,&gContainsText,eHTMLTag_script);

    Initialize( 
      /*tag*/                             eHTMLTag_select,
      /*requiredAncestor*/                eHTMLTag_unknown, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInForm,&gInForm,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFormControl, kNone, kFlowEntity,	
      /*special props, prop-range*/       kNoPropagate|kOmitWS|kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gInForm,&gContainsOpts,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_server,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_small,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_sound,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_sourcetext,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ &gDivAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kNone, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_spacer,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_span,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlockEntity, (kInlineEntity|kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_strike,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_strong,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gContainsText,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_style,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHead,	&gInHead,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHeadMisc, kPCDATA, kNone,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       &gInHead,0,eHTMLTag_style);

    Initialize( 
      /*tag*/                             eHTMLTag_sub,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
    
      /*tag*/                             eHTMLTag_sup,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kSpecial, (kSelf|SPECIALTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_table,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kBlock, kNone, (kSelf|kInlineEntity),	
      /*special props, prop-range*/       (kOmitWS|kBadContentWatch|kNoStyleLeaksIn), 2,
      /*special parents,kids,skip*/       0,&gTableKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_tbody,
      /*requiredAncestor*/                eHTMLTag_table, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInTable,	&gInTable,	
      /*autoclose starttags and endtags*/ &gTBodyAutoClose,0,0,&gDontAutoClose,
      /*parent,incl,exclgroups*/          kNone, kNone, (kSelf|kInlineEntity),	
      /*special props, prop-range*/       (kNoPropagate|kOmitWS|kBadContentWatch|kNoStyleLeaksIn), kDefaultPropRange,
      /*special parents,kids,skip*/       &gInTable,&gTBodyKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_td,
      /*requiredAncestor*/                eHTMLTag_table, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gTDRootTags,&gTDRootTags,	
      /*autoclose starttags and endtags*/ &gTDCloseTags,&gTDCloseTags,0,0,
      /*parent,incl,exclgroups*/          kNone, kFlowEntity, kSelf,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gTDRootTags,&gBodyKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_textarea,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInForm,	&gInForm,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFormControl, kPCDATA, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       &gInForm,&gContainsText,eHTMLTag_textarea);

    Initialize( 
      /*tag*/                             eHTMLTag_tfoot,
      /*requiredAncestor*/                eHTMLTag_table, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInTable,	&gInTable,
      /*autoclose starttags and endtags*/ &gTBodyAutoClose,0,0,&gDontAutoClose,
      /*parent,incl,exclgroups*/          kNone, kNone, kSelf,	
      /*special props, prop-range*/       (kNoPropagate|kOmitWS|kBadContentWatch|kNoStyleLeaksIn), kNoPropRange,
      /*special parents,kids,skip*/       &gInTable,&gTableElemKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_th, 
      /*requiredAncestor*/                eHTMLTag_table, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gTDRootTags,&gTDRootTags,	
      /*autoclose starttags and endtags*/ &gTDCloseTags,&gTDCloseTags,0,0,
      /*parent,incl,exclgroups*/          kNone, kFlowEntity, kSelf,	
      /*special props, prop-range*/       kNoStyleLeaksIn, kDefaultPropRange,
      /*special parents,kids,skip*/       &gTDRootTags,&gBodyKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_thead,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInTable,&gInTable,		
      /*autoclose starttags and endtags*/ &gTBodyAutoClose,0,0,&gDontAutoClose,
      /*parent,incl,exclgroups*/          kNone, kNone, kSelf,	
      /*special props, prop-range*/       (kNoPropagate|kOmitWS|kBadContentWatch|kNoStyleLeaksIn), kNoPropRange,
      /*special parents,kids,skip*/       &gInTable,&gTableElemKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_title,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInHead,&gInHead,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kHeadMisc,kPCDATA, kNone,	
      /*special props, prop-range*/       kOmitWS|kNoStyleLeaksIn, kNoPropRange,
      /*special parents,kids,skip*/       &gInHead,&gContainsText,eHTMLTag_title);

    Initialize( 
      /*tag*/                             eHTMLTag_tr,
      /*requiredAncestor*/                eHTMLTag_table, eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gTRParents,&gTREndParents,	
      /*autoclose starttags and endtags*/ &gTRCloseTags,0,0,0,
      /*parent,incl,exclgroups*/          kNone, kNone, kInlineEntity,	
      /*special props, prop-range*/       (kOmitWS|kBadContentWatch|kNoStyleLeaksIn), kNoPropRange,
      /*special parents,kids,skip*/       &gTRParents,&gTRKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_tt,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_u,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFontStyle, (kSelf|FSTYPE), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_ul,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gOLRootTags,&gOLRootTags,	
      /*autoclose starttags and endtags*/ &gOLAutoClose,&gULCloseTags,0,0,
      /*parent,incl,exclgroups*/          kList, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,&gULKids,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_var,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPhrase, (kSelf|kFlowEntity), kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_wbr,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kExtensions, kNone, kNone,	
      /*special props, prop-range*/       0,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_xmp,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kPreformatted, kNone, kNone,	
      /*special props, prop-range*/       0,kDefaultPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_xmp);

    Initialize( 
      /*tag*/                             eHTMLTag_text,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInBody,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       0,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_whitespace, 
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInBody,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       0,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_newline,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInBody,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_comment,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       kOmitEndTag,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_entity,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gInBody,&gInBody,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       0, kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_markupDecl,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_unknown,
	    /*rootnodes,endrootnodes*/          &gRootTags,&gRootTags,	
      /*autoclose starttags and endtags*/ 0,0,0,0,
      /*parent,incl,exclgroups*/          kFlowEntity, kNone, kNone,	
      /*special props, prop-range*/       kOmitEndTag,kNoPropRange,
      /*special parents,kids,skip*/       0,0,eHTMLTag_unknown);

    Initialize( 
      /*tag*/                             eHTMLTag_userdefined,
      /*req-parent excl-parent*/          eHTMLTag_unknown,eHTMLTag_frameset,
	    /*rootnodes,endrootnodes*/          &gInHTML,&gInHTML,	
      /*autoclose starttags and endtags*/ &gBodyAutoClose,0,0,0,
      /*parent,incl,exclgroups*/          kHTMLContent, (kFlowEntity|kSelf), kNone,	
      /*special props, prop-range*/       kOmitEndTag|kLegalOpen, kBodyPropRange,
      /*special parents,kids,skip*/       &gInNoframes,&gBodyKids,eHTMLTag_unknown);
  }//if
};

int nsHTMLElement::GetSynonymousGroups(int aGroup) {
  int result=0;

  switch(aGroup) {

    case kPhrase:
    case kSpecial:
    case kFontStyle: 
      result=aGroup; 
      break;

    case kHTMLContent:
    case kHeadContent:
    case kHeadMisc:
    case kFormControl:
    case kPreformatted:
    case kHeading:
    case kBlockMisc:
    case kBlock:
    case kList:
    case kPCDATA:
    case kExtensions:
    case kTable:
    case kSelf:
    case kInlineEntity:
    case kBlockEntity:
    case kFlowEntity:
    case kAllTags:
    default:
      break;
  }

  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsContainer(eHTMLTags aChild) {
  PRBool result=(eHTMLTag_unknown==aChild);

  if(!result){
    static eHTMLTags gNonContainers[]={
      eHTMLTag_unknown,
      eHTMLTag_area,    eHTMLTag_base,      eHTMLTag_basefont,
      eHTMLTag_br,      eHTMLTag_col,       eHTMLTag_embed,
      eHTMLTag_frame,   eHTMLTag_hr,        eHTMLTag_img,     
      eHTMLTag_image,     eHTMLTag_input,   eHTMLTag_keygen,
      eHTMLTag_link,    eHTMLTag_isindex,   eHTMLTag_meta,    
      eHTMLTag_newline, eHTMLTag_param,     eHTMLTag_plaintext, 
      eHTMLTag_style,   eHTMLTag_spacer,    eHTMLTag_text,    
      eHTMLTag_unknown, eHTMLTag_wbr,       eHTMLTag_whitespace,
      eHTMLTag_xmp};


    result=!FindTagInSet(aChild,gNonContainers,sizeof(gNonContainers)/sizeof(eHTMLTag_unknown));
  }
  return result;
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
inline PRBool TestBits(int aBitset,int aTest) {
  PRInt32 result=aBitset & aTest;
  return (aTest) ? PRBool(result==aTest) : PR_FALSE;  //was aTest
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsBlockEntity(eHTMLTags aTag){
  PRBool result=PR_FALSE;

  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mParentBits,kBlockEntity);
  } 
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsBlockCloser(eHTMLTags aTag){
  PRBool result=PR_FALSE;
    
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){

//    result=IsFlowElement(aTag);
    result=gHTMLElements[aTag].IsMemberOf(kBlockEntity);  //was kFlowEntity...
    if(!result) {

      static eHTMLTags gClosers[]={ eHTMLTag_table,eHTMLTag_tbody,eHTMLTag_caption,eHTMLTag_dd,eHTMLTag_dt,
                                    eHTMLTag_td,eHTMLTag_tfoot,eHTMLTag_th,eHTMLTag_thead,eHTMLTag_tr,
                                    eHTMLTag_optgroup};
      result=FindTagInSet(aTag,gClosers,sizeof(gClosers)/sizeof(eHTMLTag_body));
    }
  }
  return result;
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsInlineEntity(eHTMLTags aTag){
  PRBool result=PR_FALSE;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mParentBits,kInlineEntity);
  } 
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsFlowEntity(eHTMLTags aTag){
  PRBool result=PR_FALSE;

  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mParentBits,kFlowEntity);
  } 
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsBlockParent(eHTMLTags aTag){
  PRBool result=PR_FALSE;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kBlockEntity);
  } 
  return result;
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsInlineParent(eHTMLTags aTag){
  PRBool result=PR_FALSE;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kInlineEntity);
  } 
  return result;
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsFlowParent(eHTMLTags aTag){
  PRBool result=PR_FALSE;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_userdefined)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kFlowEntity);
  } 
  return result;
}

/**
 * Tells us whether the given tag opens a section
 * @update	gess 01/04/99
 * @param   id of tag
 * @return  TRUE if opens section
 */
PRBool nsHTMLElement::IsSectionTag(eHTMLTags aTag){
  PRBool result=PR_FALSE;
  switch(aTag){
    case eHTMLTag_html:
    case eHTMLTag_frameset:
    case eHTMLTag_body:
    case eHTMLTag_head:
      result=PR_TRUE;
      break;
    default:
      result=PR_FALSE;
  }
  return result;
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanContain(eHTMLTags aParent,eHTMLTags aChild){
  PRBool result=PR_FALSE;
  if((aParent>=eHTMLTag_unknown) & (aParent<=eHTMLTag_userdefined)){
    result=gHTMLElements[aParent].CanContain(aChild);
  } 
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanExclude(eHTMLTags aChild) const{
  PRBool result=PR_FALSE;

    //Note that special kids takes precedence over exclusions...
  if(mSpecialKids) {
    if(Contains(aChild,*mSpecialKids)) {
      return PR_FALSE;
    }
  }

  if(eHTMLTag_unknown!=mExclusionBits){
    if(gHTMLElements[aChild].IsMemberOf(mExclusionBits)) {
      result=PR_TRUE;
    }
  }
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanOmitEndTag(void) const{
  PRBool result=!IsContainer(mTagID);
  if(!result)
    result=TestBits(mSpecialProperties,kOmitEndTag);
  return result;
}

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanOmitStartTag(eHTMLTags aChild) const{
  PRBool result=PR_FALSE;
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsChildOfHead(eHTMLTags aChild) {
  PRBool result=Contains(aChild,gHeadKids);
  return result;
}


/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::SectionContains(eHTMLTags aChild,PRBool allowDepthSearch) {
  PRBool result=PR_FALSE;
  TagList* theRootTags=gHTMLElements[aChild].GetRootTags();
  if(theRootTags){
    if(!Contains(mTagID,*theRootTags)){
      eHTMLTags theRootBase=GetTagAt(0,*theRootTags);
      if((eHTMLTag_unknown!=theRootBase) && (allowDepthSearch))
        result=SectionContains(theRootBase,allowDepthSearch);
    }
    else result=PR_TRUE;
  }
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsStyleTag(eHTMLTags aChild) {

  static eHTMLTags gStyleTags[]={
    eHTMLTag_a,       eHTMLTag_acronym,   eHTMLTag_b,
    eHTMLTag_bdo,     eHTMLTag_big,       eHTMLTag_blink,
    eHTMLTag_center,  eHTMLTag_cite,      eHTMLTag_code,
    eHTMLTag_del,     eHTMLTag_dfn,       eHTMLTag_em,
    eHTMLTag_font,    eHTMLTag_i,         eHTMLTag_ins,
    eHTMLTag_kbd,     eHTMLTag_q,
    eHTMLTag_s,       eHTMLTag_samp,      eHTMLTag_small,
    eHTMLTag_span,    eHTMLTag_strike,    eHTMLTag_strong,
    eHTMLTag_sub,     eHTMLTag_sup,       eHTMLTag_tt,
    eHTMLTag_u,       eHTMLTag_var
  };

  PRBool result=FindTagInSet(aChild,gStyleTags,sizeof(gStyleTags)/sizeof(eHTMLTag_body));
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsHeadingTag(eHTMLTags aChild) {
  return Contains(aChild,gHeadingTags);
}


/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanContainType(PRInt32 aType) const{
  PRInt32 answer=mInclusionBits & aType;
  PRBool  result=PRBool(0!=answer);
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsMemberOf(PRInt32 aSet) const{
  PRBool result=(aSet && TestBits(aSet,mParentBits));
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsWhitespaceTag(eHTMLTags aChild) {
  static eHTMLTags gWSTags[]={eHTMLTag_newline, eHTMLTag_whitespace};
  PRBool result=FindTagInSet(aChild,gWSTags,sizeof(gWSTags)/sizeof(eHTMLTag_body));
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::IsTextTag(eHTMLTags aChild) {
  static eHTMLTags gTextTags[]={eHTMLTag_text,eHTMLTag_entity,eHTMLTag_newline, eHTMLTag_whitespace};
  PRBool result=FindTagInSet(aChild,gTextTags,sizeof(gTextTags)/sizeof(eHTMLTag_body));
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanContainSelf(void) const {
  PRBool result=PRBool(TestBits(mInclusionBits,kSelf)!=0);
  return result;
}

/**
 * 
 * @update	harishd 09/20/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanAutoCloseTag(eHTMLTags aTag) const{
  PRBool result=PR_TRUE;
  if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_userdefined)) {
    TagList* theTagList=gHTMLElements[mTagID].GetNonAutoCloseEndTags();
    if(theTagList) {
      result=!Contains(aTag,*theTagList);
    }
  }
  return result;
}

/**
 * 
 * @update	gess12/13/98
 * @param 
 * @return
 */
eHTMLTags nsHTMLElement::GetCloseTargetForEndTag(nsEntryStack& aTagStack,PRInt32 anIndex) const{
  eHTMLTags result=eHTMLTag_unknown;

  int theCount=aTagStack.GetCount();
  int theIndex=theCount;
  if(IsMemberOf(kPhrase)){
    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aTagStack.TagAt(theIndex);
      if(theTag!=mTagID) {
        //phrasal elements can close other phrasals, along with fontstyle and special tags...
        if(!gHTMLElements[theTag].IsMemberOf(kSpecial|kFontStyle|kPhrase)) {
          break; //it's not something I can close
        }
      }
      else {
        result=theTag; //stop because you just found yourself on the stack
        break;
      }
    }
  }
  else if(IsMemberOf(kSpecial)){
    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aTagStack.TagAt(theIndex);
      if(theTag!=mTagID) {
        //phrasal elements can close other phrasals, along with fontstyle and special tags...
        if(gHTMLElements[theTag].IsMemberOf(kSpecial) ||
           gHTMLElements[theTag].IsMemberOf(kFontStyle)){
        }
        else {
          break; //it's not something I can close
        }
      }
      else {
        result=theTag; //stop because you just found yourself on the stack
        break;
      }
    }
  }
  else if(IsMemberOf(kFormControl|kExtensions)){
    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aTagStack.TagAt(theIndex);
      if(theTag!=mTagID) {
        if(!CanContain(theTag)) {
          break; //it's not something I can close
        }
      }
      else {
        result=theTag; //stop because you just found yourself on the stack
        break;
      }
    }
  }
  else if(IsMemberOf(kFontStyle)){
    eHTMLTags theTag=aTagStack.Last();
    if(gHTMLElements[theTag].IsMemberOf(kFontStyle)) {
      result=theTag;
    }
  }
  return result;
}


/**
 * See whether this tag can DIRECTLY contain the given child.
 * @update	gess12/13/98
 * @param 
 * @return
 */
PRBool nsHTMLElement::CanContain(eHTMLTags aChild) const{

  if(IsContainer(mTagID)){

    if(gHTMLElements[aChild].HasSpecialProperty(kLegalOpen)) {
      // Some tags could be opened anywhere, in the document, as they please.
      return PR_TRUE;
    }

    if(mTagID==aChild) {
      return CanContainSelf();  //not many tags can contain themselves...
    }

    TagList* theCloseTags=gHTMLElements[aChild].GetAutoCloseStartTags();
    if(theCloseTags){
      if(Contains(mTagID,*theCloseTags))
        return PR_FALSE;
    }


    if(nsHTMLElement::IsInlineEntity(aChild)){
      if(nsHTMLElement::IsInlineParent(mTagID)){
        return PR_TRUE;
      }
    }

    if(nsHTMLElement::IsFlowEntity(aChild)) {
      if(nsHTMLElement::IsFlowParent(mTagID)){
        return PR_TRUE;
      }
    }

    if(nsHTMLElement::IsTextTag(aChild)) {
      if(nsHTMLElement::IsInlineParent(mTagID)){
        return PR_TRUE;
      }
    }

    if(nsHTMLElement::IsBlockEntity(aChild)){
      if(nsHTMLElement::IsBlockParent(mTagID) || IsStyleTag(mTagID)){
        return PR_TRUE;
      }
    }

    if(CanContainType(gHTMLElements[aChild].mParentBits)) {
      return PR_TRUE;
    }
 
    if(mSpecialKids) {
      if(Contains(aChild,*mSpecialKids)) {
        return PR_TRUE;
      }
    }

  }
  
  return PR_FALSE;
}


/**
 * 
 * @update	gess1/21/99
 * @param 
 * @return
 */
PRBool nsHTMLElement::HasSpecialProperty(PRInt32 aProperty) const{
  PRBool result=TestBits(mSpecialProperties,aProperty);
  return result;
}

void nsHTMLElement::DebugDumpContainment(const char* aFilename,const char* aTitle){
#ifdef  RICKG_DEBUG

  PRBool t=CanContain(eHTMLTag_address,eHTMLTag_object);

  const char* prefix="     ";
  fstream out(aFilename,ios::out);
  out << "==================================================" << endl;
  out << aTitle << endl;
  out << "==================================================";
  int i,j=0;
  int written;
  int linenum=5;
  for(i=1;i<eHTMLTag_text;i++){

    const char* tag=nsHTMLTags::GetStringValue((eHTMLTags)i);
    out << endl << endl << "Tag: <" << tag << ">" << endl;
    out << prefix;
    linenum+=3;
    written=0;
    char startChar=0;
    if(IsContainer((eHTMLTags)i)) {
      for(j=1;j<eHTMLTag_text;j++){
        tag=nsHTMLTags::GetStringValue((eHTMLTags)j);
        if(tag) {
          if(!startChar)
            startChar=tag[0];
          if(12==written){
            out << endl << prefix;
            linenum++;
            written=0;
          }
          if(CanContain((eHTMLTags)i,(eHTMLTags)j)){
            if(tag[0]!=startChar){
              out << endl << prefix;
              linenum++;
              written=0;
              startChar=tag[0];
            }
            out<< tag << ", ";
            written++;
          }

        }//if
      }//for
    }
    else {
      out<<"(not container)" << endl;
      linenum++;
    }
  } //for
#endif
}

void nsHTMLElement::DebugDumpMembership(const char* aFilename){
#ifdef  RICKG_DEBUG

  const char* prefix="             ";
  const char* suffix="       ";
  const char* shortSuffix="     ";
  fstream out(aFilename,ios::out);
  int i,j=0;
  int written=0;

  out << "============================================================================================================" << endl;
  out << "Membership..." << endl;
  out << "============================================================================================================" << endl;
  out << prefix << "Block   Block   Inline  Flow    Font    Phrase  Special Head    List    Form    Pre" << endl;
  out << prefix << "Element Entity  Entity  Entity  Element Element Element Element Element Element Element" << endl;
  out << "============================================================================================================" << endl;

  char* answer[]={".","Y"};
  char startChar=0;
  for(i=1;i<eHTMLTag_text;i++){
    const char* tag=nsHTMLTags::GetStringValue((eHTMLTags)i);
    out << tag; 
    int len=strlen(tag);

    while(len++<15){
      out<<" ";
    }

    int b=kBlock;
    b=kBlockEntity;
   
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kBlock] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kBlockEntity)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kInlineEntity)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kFlowEntity)] << suffix ;

    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kFontStyle)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kPhrase)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kSpecial)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kHeading)] << suffix; 
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kList)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kFormControl)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].IsMemberOf(kPreformatted)] << suffix << endl;

/*
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kFontStyle] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kPhrase] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kSpecial] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kHeading] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kList] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kFormControl] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].mParentBits==kPreformatted] << suffix << endl;
*/
  } //for
  out<<endl<<endl;
#endif
}

void nsHTMLElement::DebugDumpContainType(const char* aFilename){
#ifdef RICKG_DEBUG

  const char* prefix="             ";
  const char* suffix="       ";
  const char* shortSuffix="     ";
  fstream out(aFilename,ios::out);
  int i,j=0;
  int written=0;

  out << "============================================================================================================" << endl;
  out << "ContainType..."<<endl;
  out << "============================================================================================================" << endl;
  out << prefix << "Block   Block   Inline  Flow    Font    Phrase  Special Head    List    Form    Pre" << endl;
  out << prefix << "Element Entity  Entity  Entity  Element Element Element Element Element Element Element" << endl;
  out << "============================================================================================================" << endl;

  char* answer[]={".","Y"};
  char startChar=0;
  for(i=1;i<eHTMLTag_text;i++){
    const char* tag=nsHTMLTags::GetStringValue((eHTMLTags)i);
    out << tag; 
    int len=strlen(tag);

    while(len++<15){
      out<<" ";
    }

    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kBlock)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kBlockEntity)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kInlineEntity)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kFlowEntity)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kFontStyle)] << suffix ;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kPhrase)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kSpecial)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kHeading)] << suffix; 
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kList)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kFormControl)] << suffix;
    out << answer[gHTMLElements[eHTMLTags(i)].CanContainType(kPreformatted)] << suffix << endl;

  } //for
  out<<endl<<endl;
#endif
}

