#define XMLJS_INTERNAL
#include "xmljs.h"
#undef XMLJS_INTERNAL

static void
StartElementHandler(void *userdata, const char *name, const char **atts)
{
    XMLCallback *cb = (XMLCallback *)userdata;
    int i;
    JSString *namestr = NULL, *attstr = NULL;
    JSObject *attobj = NULL;
    JSContext *cx = cb->cx;
    jsval argv[2];
    jsval rval;
    JSFunction *fun;

    if (!JS_GetProperty(cx, cb->obj, "startElement", &rval) ||
	JSVAL_IS_VOID(rval) || JSVAL_IS_NULL(rval))
	return;

    fun = JS_ValueToFunction(cx, rval);
    if (!fun)
	return;

    /* oh, for local roots */
    JS_AddRoot(cx, &namestr);
    JS_AddRoot(cx, &attobj);
    JS_AddRoot(cx, &attstr);

    namestr = JS_NewStringCopyZ(cx, name);

    if (atts[0]) {
	if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), xmljs_newObj_str,
			       xmljs_newObj_size,
			       "XMLParser internal", 0, &rval)) {
	    JS_ReportError(cx, "failed to create attobj");
	    goto out;
	}
	attobj = JSVAL_TO_OBJECT(rval);
	
	for (i = 0; atts[i]; i+=2) {
	    attstr = JS_NewStringCopyZ(cx, atts[i+1]);
	    if (!attstr ||
		!JS_DefineProperty(cx, attobj, atts[i],
				   STRING_TO_JSVAL(attstr),
				   NULL, NULL, JSPROP_ENUMERATE)) {
		JS_ReportError(cx, "defining att prop %s failed", atts[i]);
		goto out;
	    }
	}
    }

    argv[0] = STRING_TO_JSVAL(namestr);
    argv[1] = OBJECT_TO_JSVAL(attobj);
    JS_CallFunction(cx, cb->obj, fun, 2, argv, &rval);

 out:
    JS_RemoveRoot(cx, &namestr);
    JS_RemoveRoot(cx, &attstr);
    JS_RemoveRoot(cx, &attobj);
}

static void
EndElementHandler(void *userdata, const char *name)
{
    XMLCallback *cb = (XMLCallback *)userdata;
    JSString *namestr = NULL;
    JSContext *cx = cb->cx;
    jsval argv[1];
    jsval rval;
    JSFunction *fun;
    
    if (!JS_GetProperty(cx, cb->obj, "endElement", &rval) ||
	JSVAL_IS_VOID(rval) || JSVAL_IS_NULL(rval))
	return;
    
    fun = JS_ValueToFunction(cx, rval);
    if (!fun)
	return;
    
    /* oh, for local roots */
    JS_AddRoot(cx, &namestr);

    namestr = JS_NewStringCopyZ(cx, name);
    if (!namestr)
	goto out;
    argv[0] = STRING_TO_JSVAL(namestr);
    JS_CallFunction(cx, cb->obj, fun, 1, argv, &rval);
    
 out:
    JS_RemoveRoot(cx, &namestr);
}

static void
CharacterDataHandler(void *userdata, const char *s, int len)
{
    XMLCallback *cb = (XMLCallback *)userdata;
    JSString *cdatastr = NULL;
    JSContext *cx = cb->cx;
    jsval argv[1];
    jsval rval;
    JSFunction *fun;
    
    if (!JS_GetProperty(cx, cb->obj, "characterData", &rval) ||
	JSVAL_IS_VOID(rval) || JSVAL_IS_NULL(rval))
	return;
    
    fun = JS_ValueToFunction(cx, rval);
    if (!fun)
	return;
    
    /* oh, for local roots */
    JS_AddRoot(cx, &cdatastr);

    cdatastr = JS_NewStringCopyZ(cx, s);
    if (!cdatastr)
	goto out;
    argv[0] = STRING_TO_JSVAL(cdatastr);
    JS_CallFunction(cx, cb->obj, fun, 1, argv, &rval);
    
 out:
    JS_RemoveRoot(cx, &cdatastr);
}

static void
ProcessingInstructionHandler(void *userdata, const char *target,
			    const char *data)
{
    XMLCallback *cb = (XMLCallback *)userdata;
    JSString *targetstr = NULL, *datastr = NULL;
    JSContext *cx = cb->cx;
    jsval argv[2];
    jsval rval;
    JSFunction *fun;

    if (!JS_GetProperty(cx, cb->obj, "processingInstruction", &rval) ||
	JSVAL_IS_VOID(rval) || JSVAL_IS_NULL(rval))
	return;

    fun = JS_ValueToFunction(cx, rval);
    if (!fun)
	return;
    JS_AddRoot(cx, &targetstr);
    JS_AddRoot(cx, &datastr);
    targetstr = JS_NewStringCopyZ(cx, target);
    if (!targetstr)
	goto out;
    datastr = JS_NewStringCopyZ(cx, data);
    if (!datastr)
	goto out;
    argv[0] = STRING_TO_JSVAL(targetstr);
    argv[1] = STRING_TO_JSVAL(datastr);
    JS_CallFunction(cx, cb->obj, fun, 2, argv, &rval);

 out:
    JS_RemoveRoot(cx, &targetstr);
    JS_RemoveRoot(cx, &datastr);
}

static JSBool
XMLParser(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    XMLCallback *cb = JS_malloc(cx, sizeof(XMLCallback));
    if (!cb)
	return JS_FALSE;
    cb->start = StartElementHandler;
    cb->end = EndElementHandler;
    cb->cdata = CharacterDataHandler;
    cb->processing = ProcessingInstructionHandler;
    cb->xml = NULL;
    cb->obj = obj;
    cb->preParse = cb->postParse = NULL;
    cb->cx = cx;		/* XXX obj can persist longer than cx *sigh* */
    return JS_SetPrivate(cx, obj, (void *)cb);
}

static void
xmlparser_finalize(JSContext *cx, JSObject *obj)
{
    XMLCallback *cb;

    cb = (XMLCallback *)JS_GetPrivate(cx, obj);
    if (!cb)
	return;
    JS_free(cx, cb);
}

JSClass XMLParser_class = {
    "XMLParser", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  xmlparser_finalize
};

JSFunctionSpec xmlparser_funcs[] = {
    {0}
};

JSPropertySpec xmlparser_props[] = {
    {0}
};

JSBool
XMLParser_Init(JSContext *cx, JSObject *obj, JSObject *parent_proto)
{
    JSObject *proto = JS_InitClass(cx, obj, parent_proto, &XMLParser_class,
				   XMLParser, 1,
				   xmlparser_props, xmlparser_funcs,
				   NULL, NULL);
    return proto &&
	JS_DefineProperty(cx, proto, "startElement", JSVAL_NULL, NULL, NULL,
			  JSPROP_ENUMERATE) &&
	JS_DefineProperty(cx, proto, "endElement", JSVAL_NULL, NULL, NULL,
			  JSPROP_ENUMERATE) &&
	JS_DefineProperty(cx, proto, "characterData", JSVAL_NULL, NULL, NULL,
			  JSPROP_ENUMERATE) &&
	JS_DefineProperty(cx, proto, "processingInstruction", JSVAL_NULL, 
			  NULL, NULL, JSPROP_ENUMERATE);
}
