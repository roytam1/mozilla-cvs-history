/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifdef DOM

#ifndef LM_DOM_H
#define LM_DOM_H

#include "libmocha.h"
#include "dom.h"
#include "lo_ele.h"
#include "pa_parse.h"

typedef struct DOM_HTMLElementPrivate {
    TagType             tagtype;
    LO_Element *        ele_start;
    LO_Element *        ele_end;
    int32               doc_id;
    uint32		flags;
} DOM_HTMLElementPrivate;


#define STYLE_NODE_NEED_TO_POP_TABLE         0x01
#define STYLE_NODE_NEED_TO_POP_LIST          0x02
#define STYLE_NODE_NEED_TO_POP_MARGINS       0x04
#define STYLE_NODE_NEED_TO_POP_FONT          0x08
#define STYLE_NODE_NEED_TO_POP_PRE           0x10
#define STYLE_NODE_NEED_TO_POP_ALIGNMENT     0x20
#define STYLE_NODE_NEED_TO_POP_LINE_HEIGHT   0x40
#define STYLE_NODE_NEED_TO_POP_LAYER         0x80

#define LM_NODE_FLAGS_ALL		0xff

#define ELEMENT_PRIV(e) ((DOM_HTMLElementPrivate *)(((DOM_Node *)(e))->data))
#define CURRENT_NODE(d) ((DOM_Node *)(d->top_state->current_node))
#define TOP_NODE(d) ((DOM_Node *)(d->top_state->top_node))

DOM_Element *
DOM_HTMLPopElementByType(TagType type, DOM_Element *node);

JSBool
DOM_HTMLPushElement(DOM_Element *element, DOM_Node *parent);

JSBool
lm_DOMInitNode(MochaDecoder *decoder);

JSBool
lm_DOMInitElement(MochaDecoder *decoder);

JSBool
lm_DOMInitAttribute(MochaDecoder *decoder);

void
lm_DestroyDocumentNodes(MWContext *context);

JSBool
lm_CheckDocId(MWContext *context, DOM_HTMLElementPrivate *priv);

JSBool
LM_SetNodeFlags(DOM_Node *node, uint32 flags);

JSBool
LM_ClearNodeFlags(DOM_Node *node, uint32 flags);

#endif /* DOM */

#endif /* LM_DOM_H */
