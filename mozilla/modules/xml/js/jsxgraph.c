#define XMLJS_INTERNAL
#include "xmljs.h"
#undef XMLJS_INTERNAL

#if DEBUG_shaver
#include <stdio.h>
#endif

/**
 * An XMLGraph JS object produces an object graph in response
 * to XML parser events.
 */

typedef struct {
    XMLCallback cb;
    JSObject *current;
    char *nameBy;
} XMLGraphCallback;

static char
xmlg_up_str[] = "__up__";

static void
StartElementHandler(void *userdata, const char *name, const char **atts)
{
    XMLGraphCallback * cb = (XMLGraphCallback *)userdata;
    int i;
    JSObject *element, *attobj, *array;
    JSContext *cx = cb->cb.cx;
    jsval rval;

    if (!cb->current) {
	if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), xmljs_newObj_str,
			       xmljs_newObj_size, 
			       "XMLGraph internal", 0, &rval)) {
	    JS_ReportError(cx, "failed to create new Object");
	    return;
	}
	element = JSVAL_TO_OBJECT(rval);
	cb->current = element;
	if (!JS_DefineProperty(cx, cb->cb.obj, "graph", rval,
			       NULL, NULL, JSPROP_ENUMERATE)) {
	    JS_ReportError(cx, "failed to define graph prop");
	    return;
	}
    }
    if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), xmljs_newObj_str,
			   xmljs_newObj_size,
			   "XMLGraph internal", 0, &rval)) {
	JS_ReportError(cx, "failed to create new Object");
	return;
    }
    element = JSVAL_TO_OBJECT(rval);

    /* populate and set __attributes__ */
    if (atts[0]) {
	JSString *attstr;
	if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), xmljs_newObj_str,
			       xmljs_newObj_size,
			       "XMLGraph internal", 0, &rval))
	    return;
	attobj = JSVAL_TO_OBJECT(rval);
	
	for (i = 0; atts[i]; i+=2) {
	    /*
	     * if we have an attribute that matches nameBy, use
	     * the value of that attribute to name the node
	     */
	    if (cb->nameBy && !strcmp(cb->nameBy, atts[i]))
		name = atts[i+1];

	    attstr = JS_NewStringCopyZ(cx, atts[i+1]);
	    if (!attstr ||
		!JS_DefineProperty(cx, attobj, atts[i],
				   STRING_TO_JSVAL(attstr),
				   NULL, NULL, JSPROP_ENUMERATE))
		return;
	}
	if (!JS_DefineProperty(cx, element, "__attributes__",
			       OBJECT_TO_JSVAL(attobj), NULL, NULL,
			       0))
	    return;
    }

    /* Check for existing property with this name */
    if (!JS_GetProperty(cx, cb->current, name, &rval))
	return;

    if (JSVAL_IS_OBJECT(rval)) {
	JSObject *existing = JSVAL_TO_OBJECT(rval);
	if (JS_IsArrayObject(cx, existing)) {
	    /* already an array, so append */
	    jsuint length;
	    if (!JS_GetArrayLength(cx, existing, &length) ||
		!JS_DefineElement(cx, existing, (jsint)length, 
				  OBJECT_TO_JSVAL(element), NULL, NULL,
				  JSPROP_ENUMERATE))
		return;
	} else {
	    /* replace with an array [current, new] */
	    jsval vector[2];
	    vector[0] = OBJECT_TO_JSVAL(existing);
	    vector[1] = OBJECT_TO_JSVAL(element);
	    array = JS_NewArrayObject(cx, 2, vector);
	    if (!array ||
		!JS_DefineProperty(cx, cb->current, name,
				   OBJECT_TO_JSVAL(array), NULL, NULL,
				   JSPROP_ENUMERATE))
		return;
	}
    } else if (!JS_DefineProperty(cx, cb->current, name,
				  OBJECT_TO_JSVAL(element), NULL, NULL,
				  JSPROP_ENUMERATE))
	return;

    /* define backpointer __up__ */
    if (!JS_DefineProperty(cx, element, xmlg_up_str,
			   OBJECT_TO_JSVAL(cb->current),
			   NULL, NULL, 0))
	return;
    /*
     * XXX DefineProperty(element, "__element__", elementstr)
     */

    cb->current = element;
}

static void
EndElementHandler(void *userdata, const char *name)
{
    /* XXX check to be sure that it's the right one ending! */
    XMLGraphCallback *cb = (XMLGraphCallback *)userdata;
    JSContext *cx = cb->cb.cx;
    jsval rval;

    if (!JS_GetProperty(cx, cb->current, xmlg_up_str, &rval))
	return;
    cb->current = JSVAL_TO_OBJECT(rval);
}

static void
CharacterDataHandler(void *userdata, const char *s, int len)
{
    XMLGraphCallback *cb = (XMLGraphCallback *)userdata;
    JSString *str;
    JSContext *cx = cb->cb.cx;

    str = JS_NewStringCopyN(cx, s, len);
    if (!str)
	return;
    JS_DefineProperty(cx, cb->current, "__cdata__", STRING_TO_JSVAL(str),
		      NULL, NULL, 0);
}

static void
ProcessingInstructionHandler(void *userdata, const char *target,
			     const char *data)
{
    /*
     * (cb->obj).processingInstruction(element, target, data)
     */
}

static JSBool
PreParse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    XMLGraphCallback *cb;
    if (!JS_DeleteProperty(cx, obj, "graph"))
	return JS_FALSE;
    cb = (XMLGraphCallback *)JS_GetPrivate(cx, obj);
    if (!cb)
	return JS_FALSE;
    cb->current = NULL;
    return JS_TRUE;
}

static JSBool
XMLGraph(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    XMLGraphCallback *cb = JS_malloc(cx, sizeof(XMLGraphCallback));
    if (!cb)
	return JS_FALSE;
    cb->cb.xml = NULL;
    cb->cb.obj = obj;
    cb->cb.cx = cx;
    cb->current = NULL;
    cb->nameBy = NULL;
    cb->cb.start = StartElementHandler;
    cb->cb.end = EndElementHandler;
    cb->cb.cdata = CharacterDataHandler;
    cb->cb.processing = ProcessingInstructionHandler;
    cb->cb.preParse = PreParse;
    cb->cb.postParse = NULL;
    return JS_SetPrivate(cx, obj, (void *)cb);
}

static void
xmlgraph_finalize(JSContext *cx, JSObject *obj)
{
    XMLGraphCallback *cb;
    cb = (XMLGraphCallback *)JS_GetPrivate(cx, obj);
    if (!cb)
	return;
    if (cb->nameBy)
	JS_free(cx, cb->nameBy);
    JS_free(cx, cb);
}

static JSBool
xmlgraph_nameBySetter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSString *namestr;
    XMLGraphCallback *cb = JS_GetPrivate(cx, obj);
    if (!cb)
	return JS_TRUE;
    if (*vp == JSVAL_NULL) {
	if (cb->nameBy)
	    JS_free(cx, cb->nameBy);
	cb->nameBy = NULL;
	return JS_TRUE;
    }
    namestr = JS_ValueToString(cx, *vp);
    if (!namestr)
	return JS_FALSE;
    if (cb->nameBy)
	JS_free(cx, cb->nameBy);
    cb->nameBy = JS_strdup(cx, JS_GetStringBytes(namestr));
    if (!cb->nameBy)
	return JS_FALSE;
    return JS_TRUE;
}

JSClass XMLGraph_class = {
    "XMLGraph", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  xmlgraph_finalize
};    

JSFunctionSpec xmlgraph_funcs[] = {
    {0}
};

JSPropertySpec xmlgraph_props[] = {
    {"nameBy",	-1,	JSPROP_ENUMERATE,	NULL,  	xmlgraph_nameBySetter},
    {0}
};

JSBool
XMLGraph_Init(JSContext *cx, JSObject *obj, JSObject *parent_proto)
{
    jsval v = JSVAL_NULL;
    JSObject *proto = JS_InitClass(cx, obj, parent_proto, &XMLGraph_class,
				   XMLGraph, 1,
				   xmlgraph_props, xmlgraph_funcs,
				   NULL, NULL);

    if (!proto)
	return JS_FALSE;
    if (!JS_SetProperty(cx, proto, "nameBy", &v)) {
	JS_ReportError(cx, "failed to set XMLGraph.prototype.nameBy = null");
	return JS_FALSE;
    }
    return JS_TRUE;
}
