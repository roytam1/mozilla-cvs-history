/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

/* Implementation of XDR routines for typelib structures. */

#include "xpt_xdr.h"
#include "xpt_struct.h"
#include <string.h>
#include <stdio.h>

#ifdef XP_MAC
static char *strdup(const char *c)
{
	char	*newStr = XPT_MALLOC(strlen(c) + 1);
	if (newStr)
	{
		strcpy(newStr, c);
	}
	return newStr;
}
#endif

static PRBool
DoInterfaceDirectoryEntry(XPTCursor *cursor,
                          XPTInterfaceDirectoryEntry *ide, PRUint16 entry_index);

#if 0
/* currently unused */
static PRBool
DoInterfaceDirectoryEntryIndex(XPTCursor *cursor,
                               XPTInterfaceDirectoryEntry **idep);
#endif

static PRBool
DoConstDescriptor(XPTCursor *cursor, XPTConstDescriptor *cd,
                  XPTInterfaceDescriptor *id);

static PRBool
DoMethodDescriptor(XPTCursor *cursor, XPTMethodDescriptor *md, 
                   XPTInterfaceDescriptor *id);

static PRBool
DoAnnotation(XPTCursor *cursor, XPTAnnotation **annp);

static PRBool
DoInterfaceDescriptor(XPTCursor *outer, XPTInterfaceDescriptor **idp);

static PRBool
DoTypeDescriptorPrefix(XPTCursor *cursor, XPTTypeDescriptorPrefix *tdp);

static PRBool
DoTypeDescriptor(XPTCursor *cursor, XPTTypeDescriptor *td,
                 XPTInterfaceDescriptor *id);

static PRBool
DoParamDescriptor(XPTCursor *cursor, XPTParamDescriptor *pd,
                  XPTInterfaceDescriptor *id);

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfHeader(XPTHeader *header)
{
    XPTAnnotation *ann, *last;
    PRUint32 size = 16 /* magic */ +
        1 /* major */ + 1 /* minor */ +
        2 /* num_interfaces */ + 4 /* file_length */ +
        4 /* interface_directory */ + 4 /* data_pool */;

    ann = header->annotations;
    do {
        size += 1; /* Annotation prefix */
        if (XPT_ANN_IS_PRIVATE(ann->flags))
            size += 2 + ann->creator->length + 2 + ann->private_data->length;
        last = ann;
        ann = ann->next;
    } while (!XPT_ANN_IS_LAST(last->flags));
        
    return size;
}

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfHeaderBlock(XPTHeader *header)
{
    PRUint32 size = XPT_SizeOfHeader(header);

    size += header->num_interfaces * sizeof (XPTInterfaceDirectoryEntry);

    return size;
}

XPT_PUBLIC_API(XPTHeader *)
XPT_NewHeader(PRUint16 num_interfaces)
{
    XPTHeader *header = XPT_NEWZAP(XPTHeader);
    if (!header)
        return NULL;
    memcpy(header->magic, XPT_MAGIC, 16);
    header->major_version = XPT_MAJOR_VERSION;
    header->minor_version = XPT_MINOR_VERSION;
    header->num_interfaces = num_interfaces;
    if (num_interfaces) {
        header->interface_directory = XPT_CALLOC(num_interfaces *
                                                 sizeof(XPTInterfaceDirectoryEntry));
        if (!header->interface_directory) {
            XPT_DELETE(header);
            return NULL;
        }
    }
    header->data_pool = 0;      /* XXX do we even need this struct any more? */
    
    return header;
}

XPT_PUBLIC_API(void)
XPT_FreeHeader(XPTHeader* aHeader)
{
    if (aHeader) {
        XPTAnnotation* ann;
        XPTInterfaceDirectoryEntry* entry = aHeader->interface_directory;
        XPTInterfaceDirectoryEntry* end = entry + aHeader->num_interfaces;
        for (; entry < end; entry++) {
            XPT_DestroyInterfaceDirectoryEntry(entry);
        }

        ann = aHeader->annotations;
        while (ann) {
            XPTAnnotation* next = ann->next;
            if (XPT_ANN_IS_PRIVATE(ann->flags)) {
                XPT_FREEIF(ann->creator);
                XPT_FREEIF(ann->private_data);
            }
            XPT_DELETE(ann);
            ann = next;
        }

        XPT_FREEIF(aHeader->interface_directory);
        XPT_DELETE(aHeader);
    }
}

XPT_PUBLIC_API(PRBool)
XPT_DoHeader(XPTCursor *cursor, XPTHeader **headerp)
{
    XPTMode mode = cursor->state->mode;
    XPTHeader *header;
    PRUint32 ide_offset;
    int i;
    XPTAnnotation *ann, *next, **annp;

    if (mode == XPT_DECODE) {
        header = XPT_NEWZAP(XPTHeader);
        if (!header)
            return PR_FALSE;
        *headerp = header;
    } else {
        header = *headerp;
    }

    if (mode == XPT_ENCODE) {
        /* IDEs appear after header, including annotations */
        ide_offset = XPT_SizeOfHeader(*headerp) + 1; /* one-based offset */
        header->data_pool = XPT_SizeOfHeaderBlock(*headerp);
        XPT_SetDataOffset(cursor->state, header->data_pool);
    }

    for (i = 0; i < 16; i++) {
        if (!XPT_Do8(cursor, &header->magic[i]))
            goto error;
    }

    if (mode == XPT_DECODE && 
        strncmp((const char*)header->magic, XPT_MAGIC, 16) != 0)
    {
        /* Require that the header contain the proper magic */
        fprintf(stderr,
                "libxpt: bad magic header in input file; "
                "found '%s', expected '%s'\n",
                header->magic, XPT_MAGIC_STRING);
        goto error;
    }
    
    if(!XPT_Do8(cursor, &header->major_version) ||
       !XPT_Do8(cursor, &header->minor_version) ||
       /* XXX check major for compat! */
       !XPT_Do16(cursor, &header->num_interfaces) ||
       !XPT_Do32(cursor, &header->file_length) ||
       !XPT_Do32(cursor, &ide_offset)) {
        goto error;
    }
    
    if (mode == XPT_ENCODE)
        XPT_DataOffset(cursor->state, &header->data_pool);
    if (!XPT_Do32(cursor, &header->data_pool))
        goto error;
    if (mode == XPT_DECODE)
        XPT_DataOffset(cursor->state, &header->data_pool);

    if (mode == XPT_DECODE && header->num_interfaces) {
        header->interface_directory = 
            XPT_CALLOC(header->num_interfaces * 
                       sizeof(XPTInterfaceDirectoryEntry));
        if (!header->interface_directory)
            goto error;
    }

    /*
     * Iterate through the annotations rather than recurring, to avoid blowing
     * the stack on large xpt files.
     */
    ann = next = header->annotations;
    annp = &header->annotations;
    do {
        ann = next;
        if (!DoAnnotation(cursor, &ann))
            goto error;
        if (mode == XPT_DECODE) {
            /*
             * Make sure that we store the address of the newly allocated
             * annotation in the previous annotation's ``next'' slot, or
             * header->annotations for the first one.
             */
            *annp = ann;
            annp = &ann->next;
        }
        next = ann->next;
    } while (!XPT_ANN_IS_LAST(ann->flags));

    /* shouldn't be necessary now, but maybe later */
    XPT_SeekTo(cursor, ide_offset); 

    for (i = 0; i < header->num_interfaces; i++) {
        if (!DoInterfaceDirectoryEntry(cursor, 
                                       &header->interface_directory[i],
                                       (PRUint16)(i + 1)))
            goto error;
    }
    
    return PR_TRUE;

    /* XXX need to free child data sometimes! */
    XPT_ERROR_HANDLE(header);    
}   

XPT_PUBLIC_API(PRBool)
XPT_FillInterfaceDirectoryEntry(XPTInterfaceDirectoryEntry *ide,
                                nsID *iid, char *name, char *name_space,
                                XPTInterfaceDescriptor *descriptor)
{
    XPT_COPY_IID(ide->iid, *iid);
    ide->name = name ? strdup(name) : NULL; /* what good is it w/o a name? */
    ide->name_space = name_space ? strdup(name_space) : NULL;
    ide->interface_descriptor = descriptor;
    return PR_TRUE;
}

XPT_PUBLIC_API(void)
XPT_DestroyInterfaceDirectoryEntry(XPTInterfaceDirectoryEntry* ide)
{
    if (ide) {
        if (ide->name) free(ide->name);
        if (ide->name_space) free(ide->name_space);
        XPT_FreeInterfaceDescriptor(ide->interface_descriptor);
    }
}

/* InterfaceDirectoryEntry records go in the header */
PRBool
DoInterfaceDirectoryEntry(XPTCursor *cursor,
                          XPTInterfaceDirectoryEntry *ide, PRUint16 entry_index)
{
    XPTMode mode = cursor->state->mode;
    
    /* write the IID in our cursor space */
    if (!XPT_DoIID(cursor, &(ide->iid)) ||
        
        /* write the name string in the data pool, and the offset in our
           cursor space */
        !XPT_DoCString(cursor, &(ide->name)) ||
        
        /* write the name_space string in the data pool, and the offset in our
           cursor space */
        !XPT_DoCString(cursor, &(ide->name_space)) ||
        
        /* do InterfaceDescriptors -- later, only on encode (see below) */
        !DoInterfaceDescriptor(cursor, &ide->interface_descriptor)) {
        goto error;
    }
    
    if (mode == XPT_DECODE)
        XPT_SetOffsetForAddr(cursor, ide, entry_index);

#if 0 /* not yet -- we eagerly load for now */
    /* write the InterfaceDescriptor in the data pool, and the offset
       in our cursor space, but only if we're encoding. */
    if (mode == XPT_ENCODE) {
        if (!DoInterfaceDescriptor(cursor, 
                                       &ide->interface_descriptor)) {
            goto error;
        }
    }
#endif

    return PR_TRUE;

    XPT_ERROR_HANDLE(ide);    
}

#if 0
/*
 * Decode: Get the interface directory entry for the on-disk index.
 * Encode: Write the index.
 */
PRBool
DoInterfaceDirectoryEntryIndex(XPTCursor *cursor,
                                   XPTInterfaceDirectoryEntry **idep)
{
    XPTMode mode = cursor->state->mode;
    PRUint16 index;
    
    if (mode == XPT_ENCODE) {
        /* XXX index zero is legal, so how do I detect an error? */
        if (*idep) {
            index = (PRUint16) XPT_GetOffsetForAddr(cursor, *idep);
            if (!index)
                return PR_FALSE;
        } else {
            index = 0;          /* no interface */
        } 
    }
 
    if (!XPT_Do16(cursor, &index))
        return PR_FALSE;

    if (mode == XPT_DECODE) {
        if (index) {
            *idep = XPT_GetAddrForOffset(cursor, index);
            if (!*idep)
                return PR_FALSE;
        } else {
            *idep = NULL;
        }
    }

    return PR_TRUE;
}
#endif

XPT_PUBLIC_API(XPTInterfaceDescriptor *)
XPT_NewInterfaceDescriptor(PRUint16 parent_interface, PRUint16 num_methods,
                           PRUint16 num_constants, PRUint8 flags)
{

    XPTInterfaceDescriptor *id = XPT_NEWZAP(XPTInterfaceDescriptor);
    if (!id)
        return NULL;

    if (num_methods) {
        id->method_descriptors = XPT_CALLOC(num_methods *
                                            sizeof(XPTMethodDescriptor));
        if (!id->method_descriptors)
            goto free_id;
        id->num_methods = num_methods;
    }

    if (num_constants) {
        id->const_descriptors = XPT_CALLOC(num_constants *
                                           sizeof(XPTConstDescriptor));
        if (!id->const_descriptors)
            goto free_meth;
        id->num_constants = num_constants;
    }

    if (parent_interface) {
        id->parent_interface = parent_interface;
    } else {
        id->parent_interface = 0;
    }

    id->flags = flags;

    return id;

 free_meth:
    XPT_FREEIF(id->method_descriptors);
 free_id:
    XPT_DELETE(id);
    return NULL;
}

XPT_PUBLIC_API(void)
XPT_FreeInterfaceDescriptor(XPTInterfaceDescriptor* id)
{
    if (id) {
        XPTMethodDescriptor *md, *mdend;
        XPTConstDescriptor *cd, *cdend;

        /* Free up method descriptors */
        md = id->method_descriptors;
        mdend = md + id->num_methods;
        for (; md < mdend; md++) {
            XPT_FREEIF(md->name);
            XPT_FREEIF(md->params);
            XPT_FREEIF(md->result);
        }
        XPT_FREEIF(id->method_descriptors);

        /* Free up const descriptors */
        cd = id->const_descriptors;
        cdend = cd + id->num_constants;
        for (; cd < cdend; cd++) {
            XPT_FREEIF(cd->name);
        }
        XPT_FREEIF(id->const_descriptors);

        /* Free up type descriptors */
        XPT_FREEIF(id->additional_types);

        XPT_DELETE(id);
    }
}

XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddTypes(XPTInterfaceDescriptor *id, PRUint16 num)
{
    XPTTypeDescriptor *old = id->additional_types, *new;

    /* XXX should grow in chunks to minimize realloc overhead */
    new = XPT_REALLOC(old,
                (id->num_additional_types + num) * sizeof(XPTTypeDescriptor));
    if (!new)
        return PR_FALSE;

    memset(new + id->num_additional_types, 0, sizeof(XPTTypeDescriptor) * num);
    id->additional_types = new;
    id->num_additional_types += num;
    return PR_TRUE;
}

XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddMethods(XPTInterfaceDescriptor *id, PRUint16 num)
{
    XPTMethodDescriptor *old = id->method_descriptors, *new;

    /* XXX should grow in chunks to minimize realloc overhead */
    new = XPT_REALLOC(old,
                      (id->num_methods + num) * sizeof(XPTMethodDescriptor));
    if (!new)
        return PR_FALSE;

    memset(new + id->num_methods, 0, sizeof(XPTMethodDescriptor) * num);
    id->method_descriptors = new;
    id->num_methods += num;
    return PR_TRUE;
}

XPT_PUBLIC_API(PRBool)
XPT_InterfaceDescriptorAddConsts(XPTInterfaceDescriptor *id, PRUint16 num)
{
    XPTConstDescriptor *old = id->const_descriptors, *new;

    /* XXX should grow in chunks to minimize realloc overhead */
    new = XPT_REALLOC(old,
                      (id->num_constants + num) * sizeof(XPTConstDescriptor));
    if (!new)
        return PR_FALSE;

    memset(new + id->num_constants, 0, sizeof(XPTConstDescriptor) * num);
    id->const_descriptors = new;
    id->num_constants += num;
    return PR_TRUE;
}

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfTypeDescriptor(XPTTypeDescriptor *td, XPTInterfaceDescriptor *id)
{
    PRUint32 size = 1; /* prefix */

    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_INTERFACE_TYPE:
        size += 2; /* interface_index */
        break;
      case TD_INTERFACE_IS_TYPE:
        size += 1; /* argnum */
        break;
      case TD_ARRAY:
        size += 2 + XPT_SizeOfTypeDescriptor(
                        &id->additional_types[td->type.additional_type], id);
        break;
      case TD_PSTRING_SIZE_IS:
        size += 2; /* argnum + argnum2 */
        break;
      case TD_PWSTRING_SIZE_IS:
        size += 2; /* argnum + argnum2 */
        break;
      default:
        /* nothing special */
        break;
    }
    return size;
}

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfMethodDescriptor(XPTMethodDescriptor *md, XPTInterfaceDescriptor *id)
{
    PRUint32 i, size =  1 /* flags */ + 4 /* name */ + 1 /* num_args */;

    for (i = 0; i < md->num_args; i++) 
        size += 1 + XPT_SizeOfTypeDescriptor(&md->params[i].type, id);

    size += 1 + XPT_SizeOfTypeDescriptor(&md->result->type, id);
    return size;
}

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfConstDescriptor(XPTConstDescriptor *cd, XPTInterfaceDescriptor *id)
{
    PRUint32 size = 4 /* name */ + XPT_SizeOfTypeDescriptor(&cd->type, id);

    switch (XPT_TDP_TAG(cd->type.prefix)) {
      case TD_INT8:
      case TD_UINT8:
      case TD_CHAR:
        size ++;
        break;
      case TD_INT16:
      case TD_UINT16:
      case TD_WCHAR:
        size += 2;
        break;
      case TD_INT32:
      case TD_UINT32:
      case TD_PBSTR:            /* XXX check for pointer! */
      case TD_PSTRING:
        size += 4;
        break;
      case TD_INT64:
      case TD_UINT64:
        size += 8;
        break;
      default:
        fprintf(stderr, "libxpt: illegal type in ConstDescriptor: 0x%02x\n",
                XPT_TDP_TAG(cd->type.prefix));
        return 0;
    }

    return size;
}

XPT_PUBLIC_API(PRUint32)
XPT_SizeOfInterfaceDescriptor(XPTInterfaceDescriptor *id)
{
    PRUint32 size = 2 /* parent interface */ + 2 /* num_methods */
        + 2 /* num_constants */ + 1 /* flags */, i;
    for (i = 0; i < id->num_methods; i++)
        size += XPT_SizeOfMethodDescriptor(&id->method_descriptors[i], id);
    for (i = 0; i < id->num_constants; i++)
        size += XPT_SizeOfConstDescriptor(&id->const_descriptors[i], id);
    return size;
}

PRBool
DoInterfaceDescriptor(XPTCursor *outer, XPTInterfaceDescriptor **idp)
{
    XPTMode mode = outer->state->mode;
    XPTInterfaceDescriptor *id;
    XPTCursor curs, *cursor = &curs;
    PRUint32 i, id_sz = 0;

    if (mode == XPT_DECODE) {
        id = XPT_NEWZAP(XPTInterfaceDescriptor);
        if (!id)
            return PR_FALSE;
        *idp = id;
    } else {
        id = *idp;
        if (!id) {
            id_sz = 0;
            return XPT_Do32(outer, &id_sz);
        }
        id_sz = XPT_SizeOfInterfaceDescriptor(id);
    }

    if (!XPT_MakeCursor(outer->state, XPT_DATA, id_sz, cursor))
        goto error;

    if (!XPT_Do32(outer, &cursor->offset))
        goto error;
    if (mode == XPT_DECODE && !cursor->offset) {
        XPT_DELETE(*idp);
        return PR_TRUE;
    }
    if(!XPT_Do16(cursor, &id->parent_interface) ||
       !XPT_Do16(cursor, &id->num_methods)) {
        goto error;
    }

    if (mode == XPT_DECODE && id->num_methods) {
        id->method_descriptors = XPT_CALLOC(id->num_methods *
                                            sizeof(XPTMethodDescriptor));
        if (!id->method_descriptors)
            goto error;
    }
    
    for (i = 0; i < id->num_methods; i++) {
        if (!DoMethodDescriptor(cursor, &id->method_descriptors[i], id))
            goto error;   
    }
    
    if (!XPT_Do16(cursor, &id->num_constants)) {
        goto error;
    }
    
    if (mode == XPT_DECODE && id->num_constants) {
        id->const_descriptors = XPT_CALLOC(id->num_constants * 
                                           sizeof(XPTConstDescriptor));
        if (!id->const_descriptors)
            goto error;
    }
    
    for (i = 0; i < id->num_constants; i++) {
        if (!DoConstDescriptor(cursor, &id->const_descriptors[i], id)) {
            goto error;
        }
    }

    if (!XPT_Do8(cursor, &id->flags)) {
        goto error;
    }
    
    return PR_TRUE;

    XPT_ERROR_HANDLE(id);    
}

XPT_PUBLIC_API(PRBool)
XPT_FillConstDescriptor(XPTConstDescriptor *cd, char *name,
                        XPTTypeDescriptor type, union XPTConstValue value)
{
    cd->name = strdup(name);
    if (!cd->name)
        return PR_FALSE;
    XPT_COPY_TYPE(cd->type, type);
    /* XXX copy value */
    return PR_TRUE;
}

PRBool
DoConstDescriptor(XPTCursor *cursor, XPTConstDescriptor *cd,
                  XPTInterfaceDescriptor *id)
{
    PRBool ok = PR_FALSE;

    if (!XPT_DoCString(cursor, &cd->name) ||
        !DoTypeDescriptor(cursor, &cd->type, id)) {

        return PR_FALSE;
    }

    switch(XPT_TDP_TAG(cd->type.prefix)) {
      case TD_INT8:
        ok = XPT_Do8(cursor, (PRUint8*) &cd->value.i8);
        break;
      case TD_INT16:
        ok = XPT_Do16(cursor, (PRUint16*) &cd->value.i16);
        break;
      case TD_INT32:
        ok = XPT_Do32(cursor, (PRUint32*) &cd->value.i32);
        break;
      case TD_INT64:
        ok = XPT_Do64(cursor, &cd->value.i64);
        break;
      case TD_UINT8:
        ok = XPT_Do8(cursor, &cd->value.ui8);
        break;
      case TD_UINT16:
        ok = XPT_Do16(cursor, &cd->value.ui16);
        break;
      case TD_UINT32:
        ok = XPT_Do32(cursor, &cd->value.ui32);
        break;
      case TD_UINT64:
        ok = XPT_Do64(cursor, (PRInt64 *)&cd->value.ui64);
        break;
      case TD_CHAR:
        ok = XPT_Do8(cursor, (PRUint8*) &cd->value.ch);
        break;
      case TD_WCHAR:
        ok = XPT_Do16(cursor, &cd->value.wch);
        break;
      case TD_PBSTR:
        if (cd->type.prefix.flags & XPT_TDP_POINTER) {
            ok = XPT_DoString(cursor, &cd->value.string);
            break;
        }
        /* fall-through */
      default:
        fprintf(stderr, "illegal type!\n");
        break;
    }

    return ok;

}

XPT_PUBLIC_API(PRBool)
XPT_FillMethodDescriptor(XPTMethodDescriptor *meth, PRUint8 flags, char *name,
                         PRUint8 num_args)
{
    meth->flags = flags & XPT_MD_FLAGMASK;
    meth->name = strdup(name);
    if (!name)
        return PR_FALSE;
    meth->num_args = num_args;
    if (num_args) {
        meth->params = XPT_CALLOC(num_args * sizeof(XPTParamDescriptor));
        if (!meth->params)
            goto free_name;
    } else {
        meth->params = NULL;
    }
    meth->result = XPT_NEWZAP(XPTParamDescriptor);
    if (!meth->result)
        goto free_params;
    return PR_TRUE;

 free_params:
    XPT_DELETE(meth->params);
 free_name:
    XPT_DELETE(meth->name);
    return PR_FALSE;
}

PRBool
DoMethodDescriptor(XPTCursor *cursor, XPTMethodDescriptor *md,
                   XPTInterfaceDescriptor *id)
{
    XPTMode mode = cursor->state->mode;
    int i;

    if (!XPT_Do8(cursor, &md->flags) ||
        !XPT_DoCString(cursor, &md->name) ||
        !XPT_Do8(cursor, &md->num_args))
        return PR_FALSE;

    if (mode == XPT_DECODE && md->num_args) {
        md->params = XPT_CALLOC(md->num_args * sizeof(XPTParamDescriptor));
        if (!md->params)
            return PR_FALSE;
    }

    for(i = 0; i < md->num_args; i++) {
        if (!DoParamDescriptor(cursor, &md->params[i], id))
            goto error;
    }
    
    if (mode == XPT_DECODE) {
        md->result = XPT_NEWZAP(XPTParamDescriptor);
        if (!md->result)
            return PR_FALSE;
    }

    if (!md->result ||
        !DoParamDescriptor(cursor, md->result, id))
        goto error;
    
    return PR_TRUE;
    
    XPT_ERROR_HANDLE(md->params);    
}

XPT_PUBLIC_API(PRBool)
XPT_FillParamDescriptor(XPTParamDescriptor *pd, PRUint8 flags,
                        XPTTypeDescriptor *type)
{
    pd->flags = flags & XPT_PD_FLAGMASK;
    XPT_COPY_TYPE(pd->type, *type);
    return PR_TRUE;
}

PRBool
DoParamDescriptor(XPTCursor *cursor, XPTParamDescriptor *pd, 
                  XPTInterfaceDescriptor *id)
{
    if (!XPT_Do8(cursor, &pd->flags) ||
        !DoTypeDescriptor(cursor, &pd->type, id))
        return PR_FALSE;
        
    return PR_TRUE;
}

/* XXX when we lose the useless TDP wrapper struct, #define this to Do8 */
PRBool
DoTypeDescriptorPrefix(XPTCursor *cursor, XPTTypeDescriptorPrefix *tdp)
{
    return XPT_Do8(cursor, &tdp->flags);
}

PRBool
DoTypeDescriptor(XPTCursor *cursor, XPTTypeDescriptor *td,
                 XPTInterfaceDescriptor *id)
{
    if (!DoTypeDescriptorPrefix(cursor, &td->prefix)) {
        goto error;
    }
    
    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_INTERFACE_TYPE:
        if (!XPT_Do16(cursor, &td->type.interface))
            goto error;
        break;
      case TD_INTERFACE_IS_TYPE:
        if (!XPT_Do8(cursor, &td->argnum))
            goto error;
        break;
      case TD_ARRAY:
        if (!XPT_Do8(cursor, &td->argnum) ||
            !XPT_Do8(cursor, &td->argnum2))
            goto error;

        if (cursor->state->mode == XPT_DECODE) {
            if(!XPT_InterfaceDescriptorAddTypes(id, 1))
                goto error;
            td->type.additional_type = id->num_additional_types - 1;
        }

        if (!DoTypeDescriptor(cursor, 
                              &id->additional_types[td->type.additional_type], 
                              id))
            goto error;
        break;
      case TD_PSTRING_SIZE_IS:
      case TD_PWSTRING_SIZE_IS:
        if (!XPT_Do8(cursor, &td->argnum) ||
            !XPT_Do8(cursor, &td->argnum2))
            goto error;
        break;

      default:
        /* nothing special */
        break;
    }
    return PR_TRUE;
    
    XPT_ERROR_HANDLE(td);    
}

XPT_PUBLIC_API(XPTAnnotation *)
XPT_NewAnnotation(PRUint8 flags, XPTString *creator, XPTString *private_data)
{
    XPTAnnotation *ann = XPT_NEWZAP(XPTAnnotation);
    if (!ann)
        return NULL;
    ann->flags = flags;
    if (XPT_ANN_IS_PRIVATE(flags)) {
        ann->creator = creator;
        ann->private_data = private_data;
    }
    return ann;
}

PRBool
DoAnnotation(XPTCursor *cursor, XPTAnnotation **annp)
{
    XPTMode mode = cursor->state->mode;
    XPTAnnotation *ann;
    
    if (mode == XPT_DECODE) {
        ann = XPT_NEWZAP(XPTAnnotation);
        if (!ann)
            return PR_FALSE;
        *annp = ann;
    } else {
        ann = *annp;
    }
    
    if (!XPT_Do8(cursor, &ann->flags))
        goto error;

    if (XPT_ANN_IS_PRIVATE(ann->flags)) {
        if (!XPT_DoStringInline(cursor, &ann->creator) ||
            !XPT_DoStringInline(cursor, &ann->private_data))
            goto error_2;
    }

    return PR_TRUE;
    
 error_2:
    if (ann && XPT_ANN_IS_PRIVATE(ann->flags)) {
        XPT_FREEIF(ann->creator);
        XPT_FREEIF(ann->private_data);
    }
    XPT_ERROR_HANDLE(ann);
}

PRBool
XPT_GetInterfaceIndexByName(XPTInterfaceDirectoryEntry *ide_block,
                            PRUint16 num_interfaces, char *name, 
                            PRUint16 *indexp) 
{
    int i;
    
    for (i=1; i<=num_interfaces; i++) {
        fprintf(stderr, "%s == %s ?\n", ide_block[i].name, name);
        if (strcmp(ide_block[i].name, name) == 0) {
            *indexp = i;
            return PR_TRUE;
        }
    }
    indexp = 0;
    return PR_FALSE;
}

#if 0 /* need hashtables and stuff */
XPT_PUBLIC_API(XPTInterfaceDescriptor *)
XPT_GetDescriptorByIndex(XPTCursor *cursor, XPTHeader *header, PRUint16 index)
{
    XPTInterfaceDescriptor *id = header->interface_directory + index;
    if (id)
        return id;              /* XXX refcnt? */
    /* XXX lazily load and allocate later, for now we always read them all
       in */
    return id;
}
#endif
