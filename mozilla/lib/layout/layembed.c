/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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


#include "xp.h"
#include "net.h"
#include "pa_parse.h"
#include "layout.h"
#include "laylayer.h"
#include "np.h"
#include "np2.h"
#include "laystyle.h"
#include "layers.h"

#ifdef ANTHRAX /* 9.23.97 amusil */
#include "prefapi.h"
#endif /* ANTHRAX */

#ifdef XP_WIN16
#define SIZE_LIMIT              32000
#endif /* XP_WIN16 */


#define EMBED_DEF_DIM			50
#define EMBED_DEF_BORDER		0
#define EMBED_DEF_VERTICAL_SPACE	0
#define EMBED_DEF_HORIZONTAL_SPACE	0


void lo_FinishEmbed(MWContext *, lo_DocState *, LO_EmbedStruct *);
static void lo_FormatEmbedInternal(MWContext *, lo_DocState *, PA_Tag *,
								   LO_EmbedStruct *, Bool, Bool);
			   		   		

void
lo_AddEmbedData(MWContext *context, void *session_data, lo_FreeProc freeProc, int32 indx)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_SavedEmbedListData *embed_list;
	lo_EmbedDataElement* embed_data_list;
	int32 old_count;

	/*
	 * Get the state, so we can access the savedEmbedListData
	 * list for this document.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state != NULL) && (top_state->doc_state != NULL))
		state = top_state->doc_state;
	else
		return;

	embed_list = state->top_state->savedData.EmbedList;
	if (embed_list == NULL)
		return;

	/*
	 * We may have to grow the list to fit this new data.
	 */
	if (indx >= embed_list->embed_count)
	{
		PA_Block old_embed_data_list; /* really a (lo_EmbedDataElement **) */

#ifdef XP_WIN16
		/*
		 * On 16 bit windows arrays can just get too big.
		 */
		if (((indx + 1) * sizeof(lo_EmbedDataElement)) > SIZE_LIMIT)
			return;
#endif /* XP_WIN16 */

		old_count = embed_list->embed_count;
		embed_list->embed_count = indx + 1;
		old_embed_data_list = NULL;
		
		if (old_count == 0)		/* Allocate new array */
		{
			embed_list->embed_data_list = PA_ALLOC(
				embed_list->embed_count * sizeof(lo_EmbedDataElement));
		}
		else					/* Enlarge existing array */
		{
			old_embed_data_list = embed_list->embed_data_list;
			embed_list->embed_data_list = PA_REALLOC(
				embed_list->embed_data_list,
				(embed_list->embed_count * sizeof(lo_EmbedDataElement)));
		}
		
		/*
		 * If we ran out of memory to grow the array.
		 */
		if (embed_list->embed_data_list == NULL)
		{
			embed_list->embed_data_list = old_embed_data_list;
			embed_list->embed_count = old_count;
			state->top_state->out_of_memory = TRUE;
			return;
		}
		
		/*
		 * We enlarged the array, so make sure
		 * the new slots are zero-filled.
		 */
		PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);
		while (old_count < embed_list->embed_count)
		{
			embed_data_list[old_count].data = NULL;
			embed_data_list[old_count].freeProc = NULL;
			old_count++;
		}
		PA_UNLOCK(embed_list->embed_data_list);
	}

	/*
	 * Store the new data into the list.
	 */
	PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);
	embed_data_list[indx].data = session_data;
	embed_data_list[indx].freeProc = freeProc;
	PA_UNLOCK(embed_list->embed_data_list);
}


void
LO_ClearEmbedBlock(MWContext *context, LO_EmbedStruct *embed)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *main_doc_state;
	lo_DocState *state;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return;
	}

	if (top_state->layout_blocking_element == (LO_Element *)embed)
	{
		if (embed->objTag.width == 0)
		{
			embed->objTag.width = EMBED_DEF_DIM;
		}
		if (embed->objTag.height == 0)
		{
			embed->objTag.height = EMBED_DEF_DIM;
		}

		main_doc_state = top_state->doc_state;
		state = lo_CurrentSubState(main_doc_state);

		lo_FinishEmbed(context, state, embed);
		lo_FlushBlockage(context, state, main_doc_state);
	}
}


void
lo_FormatEmbed(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_EmbedStruct *embed;
	
#ifdef	ANTHRAX
	LO_ObjectStruct* object;
	PA_Block buff;
	NET_cinfo* fileInfo;
	char* str;
	char* appletName;
	uint32 src_len;
	
	/* check to see if the type maps to an applet */
	buff = lo_FetchParamValue(context, tag, PARAM_TYPE);
	if(buff == NULL)
		{
		/* get SRC */
		buff = lo_FetchParamValue(context, tag, PARAM_SRC);
		PA_LOCK(str, char *, buff);
		
		/* if we didn't find a type param, look for an association in the SRC */
		fileInfo = NET_cinfo_find_type(str);
		str = fileInfo->type;
		}
	else
		PA_LOCK(str, char *, buff);
	
	/* check to see if this mimetype has an applet handler */
	if((appletName = NPL_FindAppletEnabledForMimetype(str)) != NULL)
		{
		PA_UNLOCK(buff);
		if(buff)
			XP_FREE(buff);

		/* just pass it to our object handler */
		lo_FormatObject(context, state, tag);
		
		/* manually close the java app - this is normally done in lo_ProcessObjectTag */
		lo_CloseJavaApp(context, state, state->current_java);
		
		XP_FREE(appletName);
		}
	else
		{
		PA_UNLOCK(buff);
		if(buff)
			XP_FREE(buff);
#endif	/* ANTHRAX */

		embed = (LO_EmbedStruct *)lo_NewElement(context, state, LO_EMBED, NULL, 0);
		if (embed == NULL)
		{
			return;
		}

		embed->objTag.type = LO_EMBED;
		embed->objTag.ele_id = NEXT_ELEMENT;
		embed->objTag.x = state->x;
		embed->objTag.x_offset = 0;
		embed->objTag.y = state->y;
		embed->objTag.y_offset = 0;
		embed->objTag.width = 0;
		embed->objTag.height = 0;
		embed->objTag.next = NULL;
		embed->objTag.prev = NULL;
#ifdef OJI
        LO_NVList_Init( &embed->attributes );
        LO_NVList_Init( &embed->parameters );

		embed->attributes.n = PA_FetchAllNameValues(tag,
			&(embed->attributes.names), &(embed->attributes.values), CS_FE_ASCII);
#else
 		embed->attribute_cnt = 0;
 		embed->attribute_list = NULL;
 		embed->value_list = NULL;
  
 		embed->attribute_cnt = PA_FetchAllNameValues(tag,
 			&(embed->attribute_list), &(embed->value_list), CS_FE_ASCII);
#endif

		lo_FormatEmbedInternal(context, state, tag, embed, FALSE, FALSE);
#ifdef	ANTHRAX
		}
#endif /* ANTHRAX */
}



void
lo_FormatEmbedObject(MWContext* context, lo_DocState* state,
					 PA_Tag* tag , LO_EmbedStruct* embed, Bool streamStarted,
					 uint32 param_count, char** param_names, char** param_values)
{
	uint32 count;
	int32 typeIndex = -1;
	int32 classidIndex = -1;

#ifdef OJI
    LO_NVList_Init(&embed->attributes);

	embed->attributes.n = PA_FetchAllNameValues(tag,
		&(embed->attributes.names), &(embed->attributes.values), CS_FE_ASCII);
#else
 	embed->attribute_cnt = 0;
 	embed->attribute_list = NULL;
 	embed->value_list = NULL;
  
 	embed->attribute_cnt = PA_FetchAllNameValues(tag,
 		&(embed->attribute_list), &(embed->value_list), CS_FE_ASCII);
#endif

#ifdef OJI
 	for (count = 0; count < (uint32)embed->attributes.n; count++)
  	{
     if (  (XP_STRCASECMP(embed->attributes.names[count], PARAM_MAYSCRIPT) == 0) 
            &&(embed->attributes.values[count] == NULL) ) {
           StrAllocCopy(embed->attributes.values[count], "true");
     }
   }
#else
	/*
	 * Look through the parameters and replace "id"
	 * with "name" and "data" with "src", so that 
	 * other code that looks up parameters by name
	 * can believe this is a normal EMBED.
	 */
 	for (count = 0; count < (uint32)embed->attribute_cnt; count++)
  	{
 		if (XP_STRCASECMP(embed->attribute_list[count], PARAM_ID) == 0)
 			StrAllocCopy(embed->attribute_list[count], PARAM_NAME);
 		else if (XP_STRCASECMP(embed->attribute_list[count], PARAM_DATA) == 0)
 			StrAllocCopy(embed->attribute_list[count], "src");
 		else if (XP_STRCASECMP(embed->attribute_list[count], PARAM_TYPE) == 0)
  			typeIndex = count;
 		else if (XP_STRCASECMP(embed->attribute_list[count], PARAM_CLASSID) == 0)
  			classidIndex = count;
  	}
 	
	/*
	 * If we have a CLASSID attribute, add the appropriate
	 * TYPE attribute.
	 */
	if (classidIndex >= 0)
	{
		if (typeIndex >= 0)
		{
			/* Change current value of TYPE to application/oleobject */
 			if (XP_STRCASECMP(embed->value_list[typeIndex], APPLICATION_OLEOBJECT) != 0)
 				StrAllocCopy(embed->value_list[typeIndex], APPLICATION_OLEOBJECT);
		}
		else
		{
			/* Add TYPE=application/oleobject to the attribute list */
			char* names[1];
			char* values[1];
			names[0] = XP_STRDUP(PARAM_TYPE);
			values[0] = XP_STRDUP(APPLICATION_OLEOBJECT);
			
 			lo_AppendParamList((uint32*) &(embed->attribute_cnt),
 							   &(embed->attribute_list),
 							   &(embed->value_list),
							   1, names, values);
		}
		
		/* Lop off the "clsid:" prefix from the CLASSID attribute */
		if (XP_STRNCASECMP(embed->value_list[classidIndex], "clsid:", 6) == 0)
		{
			char* classID = &(embed->value_list[classidIndex][6]);
			XP_MEMMOVE(embed->value_list[classidIndex], classID,
					   (XP_STRLEN(classID) + 1) * sizeof(char));
		}
	}
#endif

	/*
	 * Merge any parameters passed in with the ones in this tag.
	 * Separate the merged <PARAM> tag attributes from the
	 * <OBJECT> tag attributes with an extra "PARAM" attribute.
	 */
#ifdef OJI
    LO_NVList_Init(&embed->parameters);
#endif

	if (param_count > 0)
	{
#ifdef OJI
        int i;

        /* Add all <PARAM> tag parameters to the parameters list */
        embed->parameters.names = (char**) PA_ALLOC(param_count*sizeof(char*));
        embed->parameters.values = (char**) PA_ALLOC(param_count*sizeof(char*));

        XP_MEMCPY( embed->parameters.names, 
                   param_names, 
                   param_count*sizeof(char*) );
        XP_MEMCPY( embed->parameters.values, 
                   param_values, 
                   param_count*sizeof(char*) );
        embed->parameters.n = param_count;
#else
		char* names[1];
 		char* values[1];
 		names[0] = XP_STRDUP(PT_PARAM);
 		values[0] = NULL;
 		
 		/* Add "PARAM" to the list */
 		lo_AppendParamList((uint32*) &(embed->attribute_cnt),
 						   &(embed->attribute_list),
 						   &(embed->value_list),
 						   1, names, values);
 
 		/* Add all <PARAM> tag paramters to the list */
 		lo_AppendParamList((uint32*) &(embed->attribute_cnt),
 						   &(embed->attribute_list),
 						   &(embed->value_list),
 						   param_count,
 						   param_names,
 						   param_values);
#endif /* OJI */

		if (param_names != NULL)
			XP_FREE(param_names);
		if (param_values != NULL)
			XP_FREE(param_values);
	}

	lo_FormatEmbedInternal(context, state, tag, embed, TRUE, streamStarted);
}


static void
lo_FormatEmbedInternal(MWContext *context, lo_DocState *state, PA_Tag *tag,
			   		   LO_EmbedStruct *embed, Bool isObject, Bool streamStarted)
{
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;
	XP_Bool widthSpecified = FALSE;
	XP_Bool heightSpecified = FALSE;
	lo_DocLists *doc_lists;

    embed->nextEmbed= NULL;
#ifdef MOCHA
    embed->objTag.mocha_object = NULL;
#endif

	embed->objTag.FE_Data = NULL;
	embed->objTag.session_data = NULL;
	embed->objTag.line_height = state->line_height;
	embed->embed_src = NULL;
	embed->objTag.alignment = LO_ALIGN_BASELINE;
	embed->objTag.border_width = EMBED_DEF_BORDER;
	embed->objTag.border_vert_space = EMBED_DEF_VERTICAL_SPACE;
	embed->objTag.border_horiz_space = EMBED_DEF_HORIZONTAL_SPACE;
	embed->objTag.tag = tag;
	embed->objTag.ele_attrmask = 0;
	embed->objTag.base_url = NULL;

	if (streamStarted)
		embed->objTag.ele_attrmask |= LO_ELE_STREAM_STARTED;

	/*
	 * Convert any javascript in the values
	 */
#ifdef OJI
	lo_ConvertAllValues(context, embed->attributes.values, embed->attributes.n,
						tag->newline_count);
#else
	lo_ConvertAllValues(context, embed->value_list, embed->attribute_cnt,
						tag->newline_count);
#endif

	/* don't double-count loading plugins due to table relayout */
	if (!state->in_relayout)
	{
		state->top_state->mocha_loading_embeds_count++;
	}

	/*
	 * Assign a unique index for this object 
	 * and increment the master index.
	 */
	embed->objTag.embed_index = state->top_state->embed_count++;

	/*
	 * Check for the hidden attribute value.
	 * HIDDEN embeds have no visible part.
	 */
	if (!isObject)		/* OBJECT tag can't have "hidden" attribute */
	{
		buff = lo_FetchParamValue(context, tag, PARAM_HIDDEN);
		if (buff != NULL)
		{
			Bool hidden;

			hidden = TRUE;
			PA_LOCK(str, char *, buff);
			if (pa_TagEqual("no", str))
			{
				hidden = FALSE;
			}
			else if (pa_TagEqual("false", str))
			{
				hidden = FALSE;
			}
			else if (pa_TagEqual("off", str))
			{
				hidden = FALSE;
			}
			PA_UNLOCK(buff);
			PA_FREE(buff);

            if (hidden != FALSE)
            {
                embed->objTag.ele_attrmask |= LO_ELE_HIDDEN;
				/* marcb says:  had to disable the printed hidden audio feature   */
				if (context->type==MWContextPrint){
					return;
				}

            }
        }
	}
	
	/*
	 * Save away the base of the document
	 */
	buff = PA_ALLOC(XP_STRLEN(state->top_state->base_url) + 1);
	if (buff != NULL)
	{
		char *cp;
		PA_LOCK(cp, char*, buff);
		XP_STRCPY(cp, state->top_state->base_url);
		PA_UNLOCK(buff);
		embed->objTag.base_url = buff;
	}
	else
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	/*
	 * Check for an align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		Bool floating;

		floating = FALSE;
		PA_LOCK(str, char *, buff);
		embed->objTag.alignment = lo_EvalAlignParam(str, &floating);
		if (floating != FALSE)
		{
			embed->objTag.ele_attrmask |= LO_ELE_FLOATING;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the optional src parameter.
	 */
	if (isObject)
		buff = lo_FetchParamValue(context, tag, PARAM_DATA);
	else
		buff = lo_FetchParamValue(context, tag, PARAM_SRC);
	if (buff != NULL)
	{
		PA_Block new_buff;
		char *new_str;
		int32 len;

		len = 1;
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		if (str != NULL)
		{
			/* bing: make sure this deals with "data:" URLs */
			new_str = NET_MakeAbsoluteURL(
				state->top_state->base_url, str);
		}
		else
		{
			new_str = NULL;
		}

		if ((new_str == NULL)||(len == 0))
		{
			new_buff = NULL;
		}
		else
		{
			char *enbed_src;

			new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
			if (new_buff != NULL)
			{
				PA_LOCK(enbed_src, char *, new_buff);
				XP_STRCPY(enbed_src, new_str);
				PA_UNLOCK(new_buff);
			}
			else
			{
				state->top_state->out_of_memory = TRUE;
			}
			XP_FREE(new_str);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		buff = new_buff;
	}
	embed->embed_src = buff;

	doc_width = state->right_margin - state->left_margin;

	/*
	 * Get the width parameter, in absolute or percentage.
	 * If percentage, make it absolute.  If the height is
	 * zero, make the embed hidden so the FE knows that the
	 * size really is zero (zero size normally means that
	 * we're blocking).
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_WIDTH);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			embed->objTag.percent_width = val;
		}
		else
		{
			embed->objTag.percent_width = 0;
			embed->objTag.width = val;
			val = FEUNITS_X(val, context);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		widthSpecified = TRUE;
	}
	val = LO_GetWidthFromStyleSheet(context, state);
	if(val)
	  {
		embed->objTag.width = val;
		embed->objTag.percent_width = 0;
		widthSpecified = TRUE;
	  }

	/*
	 * Get the height parameter, in absolute or percentage.
	 * If percentage, make it absolute.  If the height is
	 * zero, make the embed hidden so the FE knows that the
	 * size really is zero (zero size normally means that
	 * we're blocking).
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HEIGHT);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			embed->objTag.percent_height = val;
		}
		else
		{
			embed->objTag.percent_height = 0;
			val = FEUNITS_Y(val, context);
		}
		embed->objTag.height = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
		heightSpecified = TRUE;
	}

	val = LO_GetHeightFromStyleSheet(context, state);
	if(val)
	  {
		embed->objTag.height = val;
		embed->objTag.percent_height = 0;
		heightSpecified = TRUE;
	  }

#ifndef XP_WIN
	/*
	 * If they forgot to specify a width or height, make one up.
	 * Don't do this for Windows, because they want to block
	 * layout for unsized OLE objects.  (Eventually all FEs will
	 * want to support this behavior when there's a plug-in API
	 * to specify the desired size of the objTag.)
	 */
	if (!widthSpecified)
	{
		if (heightSpecified)
			embed->objTag.width = embed->objTag.height;
		else
			embed->objTag.width = EMBED_DEF_DIM;
	}
	
	if (!heightSpecified)
	{
		if (widthSpecified)
			embed->objTag.height = embed->objTag.width;
		else
			embed->objTag.height = EMBED_DEF_DIM;
	}
#endif /* ifndef XP_WIN */

	lo_FillInEmbedGeometry(state, embed, FALSE);

	/*
	 * Get the extra vertical space parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		embed->objTag.border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	embed->objTag.border_vert_space = FEUNITS_Y(embed->objTag.border_vert_space, context);

	/*
	 * Get the extra horizontal space parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		embed->objTag.border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	embed->objTag.border_horiz_space = FEUNITS_X(embed->objTag.border_horiz_space,
						context);

	/*
	 * See if we have some saved embed session data to restore.
	 */
	if (state->top_state->savedData.EmbedList != NULL)
	{
		lo_SavedEmbedListData *embed_list;
		embed_list = state->top_state->savedData.EmbedList;

		/*
		 * If there is still valid data to restore available.
		 */
		if (embed->objTag.embed_index < embed_list->embed_count)
		{
			lo_EmbedDataElement* embed_data_list;

			PA_LOCK(embed_data_list, lo_EmbedDataElement*,
				embed_list->embed_data_list);
			embed->objTag.session_data =
				embed_data_list[embed->objTag.embed_index].data;
			PA_UNLOCK(embed_list->embed_data_list);
		}
	}

    /*
     * Put embed onto the embed list and applets into applet list
     * for later possible reflection.
     */
    {
       int32 i, count, *pDoclistCount;
       LO_EmbedStruct *prev_embed, *cur_embed;
       LO_CommonPluginStruct **ppDoclistList;
       doc_lists = lo_GetCurrentDocLists(state);
       if( embed->objTag.sub_type == LO_JAVA )
       {
          cur_embed       = (LO_EmbedStruct *)doc_lists->applet_list;
          pDoclistCount   = &doc_lists->applet_list_count;
          ppDoclistList   = (LO_CommonPluginStruct **)&doc_lists->applet_list;
       }
       else
       {
          cur_embed       = doc_lists->embed_list;
          pDoclistCount   = &doc_lists->embed_list_count;
          ppDoclistList   = (LO_CommonPluginStruct **)&doc_lists->embed_list;
       }
       if (state->in_relayout) {
        
           /*
            * In the interest of changing as little as possible, I'm not
            * going to change the embed_list to be in the order of layout
            * (it is currently in reverse order). Instead, we do what 
            * everybody else does - iterate till the end of the list to get
            * find an element with the correct index.
            * If we're in table relayout, we need to replace the element
            * of the same index in this list with the new layout element
            * to prevent multiple reflection.
            */
           count = 0;
        
           /* reverse order... */
           prev_embed = NULL;
           while (cur_embed) {
               cur_embed = cur_embed->nextEmbed;
               count++;
           }
           for (i = count-1; i >= 0; i--) {
               if (i == *pDoclistCount) {
                   /* Copy over the mocha object (it might not exist) */
                   embed->objTag.mocha_object = cur_embed->objTag.mocha_object;

                   embed->nextEmbed = cur_embed->nextEmbed;
                
                   /* Replace the old embed with the new one */
                   if (prev_embed == NULL)
                       *ppDoclistList = (LO_CommonPluginStruct *)embed;
                   else
                       prev_embed->nextEmbed = embed;
                   (*pDoclistCount)++;
                   break;
               }
               prev_embed = cur_embed;
               cur_embed = cur_embed->nextEmbed;
           }
       }
       else {
           embed->nextEmbed = (LO_EmbedStruct *)*ppDoclistList;
           *ppDoclistList = (LO_CommonPluginStruct *)embed;
           (*pDoclistCount)++;
       }
    }

	/*
	 * Plug-ins won't be happy if we use a special colormap.
	 */
#ifndef M12N                    /* XXXM12N Fix me, this has gone away. Sets
                                   cx->colorSpace->install_colormap_forbidden*/
    IL_UseDefaultColormapThisPage(context);
#endif /* M12N */

	/* Create an initially hidden layer for this plugin to inhabit.  If the plugin is 
	   a windowless plugin, this will later become a "normal" layer.  If the plugin is
	   a windowed plugin, the layer will become a cutout layer. */
	embed->objTag.layer =
	  lo_CreateEmbeddedObjectLayer(context, state, (LO_Element*)embed);

	/*
	 * Have the front end fetch this embed data, and tell us
	 * the embed's dimensions if it knows them.
	 */
	FE_GetEmbedSize(context, embed, state->top_state->force_reload);

	/*
	 * Hidden embeds always have 0 width and height
	 */
	if (embed->objTag.ele_attrmask & LO_ELE_HIDDEN)
	{
		embed->objTag.width = 0;
		embed->objTag.height = 0;
	}

	/*
	 * We may have to block on this embed until later
	 * when the front end can give us the width and height.
	 * Never block on hidden embeds.
	 */
	if (((embed->objTag.width == 0)||(embed->objTag.height == 0))&&
	    ((embed->objTag.ele_attrmask & LO_ELE_HIDDEN) == 0))
	{
		state->top_state->layout_blocking_element = (LO_Element *)embed;
	}
	else
	{
		lo_FinishEmbed(context, state, embed);
	}
}


void
lo_FinishEmbed(MWContext *context, lo_DocState *state, LO_EmbedStruct *embed)
{
	int32 baseline_inc;
	int32 line_inc;
	int32 embed_height, embed_width;

	/*
	 * Eventually this will never happen since we always
	 * have dims here from either the embed tag itself or the
	 * front end.
	 * Hidden embeds are supposed to have 0 width and height.
	 */
	if ((embed->objTag.ele_attrmask & LO_ELE_HIDDEN) == 0)
	{
		if (embed->objTag.width == 0)
		{
			embed->objTag.width = EMBED_DEF_DIM;
		}
		if (embed->objTag.height == 0)
		{
			embed->objTag.height = EMBED_DEF_DIM;
		}
	}

	embed_width = embed->objTag.width + (2 * embed->objTag.border_width) +
		(2 * embed->objTag.border_horiz_space);
	embed_height = embed->objTag.height + (2 * embed->objTag.border_width) +
		(2 * embed->objTag.border_vert_space);

	/*
	 * SEVERE FLOW BREAK!  This may be a floating embed,
	 * which means at this point we go do something completely
	 * different.
	 */
	if (embed->objTag.ele_attrmask & LO_ELE_FLOATING)
	{

		embed->objTag.x_offset += (int16)embed->objTag.border_horiz_space;
		embed->objTag.y_offset += (int32)embed->objTag.border_vert_space;


		/*
		 * Insert this element into the float list.
		 */
		embed->objTag.next = state->float_list;
		state->float_list = (LO_Element *)embed;

		lo_AppendFloatInLineList(context, state, (LO_Element*)embed, NULL);

		lo_LayoutFloatEmbed(context, state, embed, TRUE);

	}
	else
	{

		/*
		 * Figure out how to align this embed.
		 * baseline_inc is how much to increase the baseline
		 * of previous element of this line.  line_inc is how
		 * much to increase the line height below the baseline.
		 */
		baseline_inc = 0;
		line_inc = 0;
		/*
		 * If we are at the beginning of a line, with no baseline,
		 * we first set baseline and line_height based on the current
		 * font, then place the embed.
		 */
		if (state->baseline == 0)
		{
			state->baseline = 0;
		}


		embed->objTag.x_offset += (int16)embed->objTag.border_horiz_space;
		embed->objTag.y_offset += (int32)embed->objTag.border_vert_space;

		lo_LayoutInflowEmbed(context, state, embed, FALSE, &line_inc, &baseline_inc);

		lo_AppendToLineList(context, state,
			(LO_Element *)embed, baseline_inc);

		lo_UpdateStateAfterEmbedLayout(state, embed, line_inc, baseline_inc);

	}
}

void
lo_RelayoutEmbed ( MWContext *context, lo_DocState *state, LO_EmbedStruct * embed, PA_Tag * tag )
{
	lo_DocLists *doc_lists;

	lo_PreLayoutTag ( context, state, tag );
	if (state->top_state->out_of_memory)
	{
		return;
	}

	LO_LockLayout();
	
	/*
	 * Fill in the embed structure with default data
	 */
	embed->objTag.type = LO_EMBED;
	embed->objTag.ele_id = NEXT_ELEMENT;
	embed->objTag.x = state->x;
	embed->objTag.x_offset = 0;
	embed->objTag.y = state->y;
	embed->objTag.y_offset = 0;
	embed->objTag.next = NULL;
	embed->objTag.prev = NULL;

	embed->objTag.line_height = state->line_height;

	/*
	 * Assign a unique index for this object 
	 * and increment the master index.
	 */
	embed->objTag.embed_index = state->top_state->embed_count++;

    /*
     * Since we saved the embed_list_count during table relayout,
     * we increment it as if we were reprocessing the EMBED tag.
     */
    doc_lists = lo_GetCurrentDocLists(state);
    doc_lists->embed_list_count++;

	/*
	 * Plug-ins won't be happy if we use a special colormap.
	 */
#ifndef M12N                    /* XXXM12N Fix me, this has gone away. Sets
                                   cx->colorSpace->install_colormap_forbidden*/
    IL_UseDefaultColormapThisPage(context);
#endif /* M12N */

	/*
	 * I don't think we need to worry about blocking at all since we're in
	 * relayout, but I'd rather be safe.
	 */
	if (((embed->objTag.width == 0)||(embed->objTag.height == 0))&&
	    ((embed->objTag.ele_attrmask & LO_ELE_HIDDEN) == 0))
	{
		state->top_state->layout_blocking_element = (LO_Element *)embed;
	}
	else
	{
		lo_FinishEmbed(context, state, embed);
	}

	lo_PostLayoutTag ( context, state, tag, FALSE );
	LO_UnlockLayout();
}

void
LO_CopySavedEmbedData(MWContext *context, SHIST_SavedData *saved_data)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_SavedEmbedListData *embed_list, *new_embed_list;
	lo_EmbedDataElement *embed_data_list, *new_embed_data_list;
	PA_Block old_embed_data_list = NULL;
	int32 old_count = 0;
	int32 index;

	/*
	 * Get the state, so we can access the savedEmbedListData
	 * list for this document.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state != NULL)&&(top_state->doc_state != NULL))
		state = top_state->doc_state;
	else
		return;

	embed_list = state->top_state->savedData.EmbedList;
	if (embed_list == NULL || embed_list->embed_data_list == NULL)
		return;
		
	
	PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);

	if (saved_data->EmbedList == NULL)
		saved_data->EmbedList = XP_NEW_ZAP(lo_SavedEmbedListData);
	new_embed_list = saved_data->EmbedList;
	
	if (new_embed_list->embed_data_list == NULL)
	{
		/* Allocate new array */
		new_embed_list->embed_data_list = PA_ALLOC(
			embed_list->embed_count * sizeof(lo_EmbedDataElement));
	}
	else if (new_embed_list->embed_count < embed_list->embed_count)
	{	
		/* Enlarge existing array */
		old_embed_data_list = new_embed_list->embed_data_list;
		old_count = new_embed_list->embed_count;
		new_embed_list->embed_data_list = PA_REALLOC(
			new_embed_list->embed_data_list,
			(embed_list->embed_count * sizeof(lo_EmbedDataElement)));
	}

	/*
	 * If we ran out of memory to grow the array.
	 */
	if (new_embed_list->embed_data_list == NULL)
	{
		new_embed_list->embed_data_list = old_embed_data_list;
		new_embed_list->embed_count = old_count;
		state->top_state->out_of_memory = TRUE;
		return;
	}
	
	/*
	 * Copy all the elements from the old array to the new one.
	 */
	PA_LOCK(new_embed_data_list, lo_EmbedDataElement*, new_embed_list->embed_data_list);
	for (index = 0; index < embed_list->embed_count; index++)
		new_embed_data_list[index] = embed_data_list[index];
	new_embed_list->embed_count = embed_list->embed_count;
	PA_UNLOCK(new_embed_list->embed_data_list);
	
	PA_UNLOCK(embed_list->embed_data_list);
}


void
LO_AddEmbedData(MWContext *context, LO_EmbedStruct* embed, void *session_data)
{
	lo_AddEmbedData(context, session_data, NPL_DeleteSessionData, embed->objTag.embed_index);
}


void
lo_FillInEmbedGeometry(lo_DocState *state,
					   LO_EmbedStruct *embed,
					   Bool relayout)
{
	int32 doc_width;

	if (relayout == TRUE)
	{
		embed->objTag.ele_id = NEXT_ELEMENT;
	}
	embed->objTag.x = state->x;
	embed->objTag.y = state->y;

	doc_width = state->right_margin - state->left_margin;

	/*
	 * Get the width parameter, in absolute or percentage.
	 * If percentage, make it absolute.
	 */

	if (embed->objTag.percent_width > 0) {
		int32 val = embed->objTag.percent_width;
		if (state->allow_percent_width == FALSE) {
			val = 0;
		}
		else {
			val = doc_width * val / 100;
		}
		embed->objTag.width = val;
	}

	/* Set embed->objTag.height if embed has a % height specified */
	if (embed->objTag.percent_height > 0) {
		int32 val = embed->objTag.percent_height;
		if (state->allow_percent_height == FALSE) {
			val = 0;
		}
		else {
			val = state->win_height * val / 100;
		}
		embed->objTag.height = val;
	}
}

void
lo_LayoutInflowEmbed(MWContext *context,
					 lo_DocState *state,
					 LO_EmbedStruct *embed,
					 Bool inRelayout,
					 int32 *line_inc,
					 int32 *baseline_inc)
{
  int32 embed_width, embed_height;
  Bool line_break;
  PA_Block buff;
  char *str;
  LO_TextStruct tmp_text;
  LO_TextInfo text_info;
  lo_TopState *top_state;
  
  top_state = state->top_state;
  
  /*
   * All this work is to get the text_info filled in for the current
   * font in the font stack. Yuck, there must be a better way.
   */
  memset (&tmp_text, 0, sizeof (tmp_text));
  buff = PA_ALLOC(1);
  if (buff == NULL)
	{
	  top_state->out_of_memory = TRUE;
	  return;
	}
  PA_LOCK(str, char *, buff);
  str[0] = ' ';
  PA_UNLOCK(buff);
  tmp_text.text = buff;
  tmp_text.text_len = 1;
  tmp_text.text_attr =
	state->font_stack->text_attr;
  FE_GetTextInfo(context, &tmp_text, &text_info);
  PA_FREE(buff);
  
  embed_width = embed->objTag.width + (2 * embed->objTag.border_width) +
	(2 * embed->objTag.border_horiz_space);
  embed_height = embed->objTag.height + (2 * embed->objTag.border_width) +
	(2 * embed->objTag.border_vert_space);
  
  /*
   * Will this embed make the line too wide.
   */
  if ((state->x + embed_width) > state->right_margin)
	{
	  line_break = TRUE;
	}
  else
	{
	  line_break = FALSE;
	}
  
  /*
   * if we are at the beginning of the line.  There is
   * no point in breaking, we are just too wide.
   * Also don't break in unwrapped preformatted text.
   * Also can't break inside a NOBR section.
   */
  if ((state->at_begin_line != FALSE)||
	  (state->preformatted == PRE_TEXT_YES)||
	  (state->breakable == FALSE))
	{
	  line_break = FALSE;
	}
  
  /*
   * break on the embed if we have
   * a break.
   */
  if (line_break != FALSE)
	{
	  /*
	   * We need to make the elements sequential, linefeed
	   * before embed.
	   */
		top_state->element_id = embed->objTag.ele_id;	  

		if (!inRelayout)
		{
			lo_SoftLineBreak(context, state, TRUE);
		}
		else 
		{
			lo_rl_AddSoftBreakAndFlushLine(context, state);
		}
		embed->objTag.x = state->x;
		embed->objTag.y = state->y;
		embed->objTag.ele_id = NEXT_ELEMENT;
	}

  lo_CalcAlignOffsets(state, &text_info, (intn)embed->objTag.alignment,
					  embed_width, embed_height,
					  &embed->objTag.x_offset, &embed->objTag.y_offset, line_inc, baseline_inc);
}

void
lo_LayoutFloatEmbed(MWContext *context,
					lo_DocState *state,
					LO_EmbedStruct *embed,
					Bool updateFE )
{
  int32 embed_width;

  embed_width = embed->objTag.width + (2 * embed->objTag.border_width) +
	(2 * embed->objTag.border_horiz_space);
  if (embed->objTag.alignment == LO_ALIGN_RIGHT)
	{
	  if (state->right_margin_stack == NULL)
		{
		  embed->objTag.x = state->right_margin - embed_width;
		}
	  else
		{
		  embed->objTag.x = state->right_margin_stack->margin -
			embed_width;
		}
	  if (embed->objTag.x < 0)
		{
		  embed->objTag.x = 0;
		}
	}
  else
	{
	  embed->objTag.x = state->left_margin;
	}
  
  embed->objTag.y = -1;

  lo_AddMarginStack(state, embed->objTag.x, embed->objTag.y,
					embed->objTag.width, embed->objTag.height,
					embed->objTag.border_width,
					embed->objTag.border_vert_space, embed->objTag.border_horiz_space,
					(intn)embed->objTag.alignment);
  
  if (state->at_begin_line != FALSE)
	{
	  lo_FindLineMargins(context, state, updateFE);
	  state->x = state->left_margin;
	}
}

void
lo_UpdateStateAfterEmbedLayout(lo_DocState *state,
							   LO_EmbedStruct *embed,
							   int32 line_inc,
							   int32 baseline_inc )
{
  int32 embed_width;
  int32 x, y;

  embed_width = embed->objTag.width + (2 * embed->objTag.border_width) +
	(2 * embed->objTag.border_horiz_space);

  state->baseline += (intn) baseline_inc;
  state->line_height += (intn) (baseline_inc + line_inc);
  
  /*
   * Clean up state
   */
  state->x = state->x + embed->objTag.x_offset +
	embed_width - embed->objTag.border_horiz_space;
  state->linefeed_state = 0;
  state->at_begin_line = FALSE;
  state->trailing_space = FALSE;
  state->cur_ele_type = LO_EMBED;

  /* Determine the new position of the layer. */
  x = embed->objTag.x + embed->objTag.x_offset + embed->objTag.border_width;
  y = embed->objTag.y + embed->objTag.y_offset + embed->objTag.border_width;
  
  /* Move layer to new position */
  CL_MoveLayer(embed->objTag.layer, x, y);
}

void
LO_SetEmbedSize( MWContext *context, LO_EmbedStruct *embed, int32 width, int32 height )
{
	/* For now, just setting the embed's dimensions.  Do we need to lock/unlock layout here? */
	embed->objTag.width = width;
	embed->objTag.height = height;
}
