/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

/*
 * Generate JS API class stub functions from XPIDL.
 */

#include "xpidl.h"
#include <ctype.h>

struct stub_private {
    IDL_tree    iface;
    IDL_tree    *funcs;
    unsigned    nfuncs;
    IDL_tree    *const_dbls;
    unsigned    nconst_dbls;
    IDL_tree    *props;
    unsigned    nprops;
};

static void
free_stub_private(struct stub_private *priv)
{
    if (priv->funcs)
        free(priv->funcs);
    if (priv->const_dbls)
        free(priv->const_dbls);
    if (priv->props)
        free(priv->props);
    free(priv);
}

#define VECTOR_CHUNK    8U

/* XXXbe */ extern char OOM[];

static void
append_to_vector(IDL_tree **vecp, unsigned *lenp, IDL_tree elem)
{
    IDL_tree *vec = *vecp;
    unsigned len = *lenp;
    if (len % VECTOR_CHUNK == 0) {
        if (!vec) {
            vec = xpidl_malloc(VECTOR_CHUNK * sizeof *vec);
        } else {
            vec = realloc(vec, (len + VECTOR_CHUNK) * sizeof *vec);
            if (!vec) {
                fputs(OOM, stderr);
                exit(1);
            }
        }
    }
    vec[len++] = elem;
    *vecp = vec;
    *lenp = len;
}

static gboolean
stub_pass_1(TreeState *state)
{
    if (state->tree) {
        fprintf(state->file,
                "/*\n * DO NOT EDIT.  THIS FILE IS GENERATED FROM %s.idl\n */\n"
                "#include \"jsapi.h\"\n"
                "#include \"%s.h\"\n"
                "\n"
                "static char XXXnsresult2string_fmt[] = \"XPCOM error %%#x\";\n"
                "#define XXXnsresult2string(res) XXXnsresult2string_fmt, res\n",
                state->basename, g_basename(state->basename));
        state->priv = NULL;
    } else {
        struct stub_private *priv = state->priv;
        if (priv) {
            state->priv = NULL;
            free_stub_private(priv);
        }
    }
    return TRUE;
}

static gboolean
stub_list(TreeState *state)
{
    IDL_tree iter;
    for (iter = state->tree; iter; iter = IDL_LIST(iter).next) {
        state->tree = IDL_LIST(iter).data;
        if (!xpidl_process_node(state))
            return FALSE;
    }
    return TRUE;
}

static gboolean
stub_attr_dcl(TreeState *state)
{
    struct stub_private *priv = state->priv;
    append_to_vector(&priv->props, &priv->nprops, state->tree);
    return TRUE;
}

/* XXXbe */ extern gboolean xpcom_type(TreeState *state);
/* XXXbe */ extern gboolean xpcom_param(TreeState *state);

static char
js_convert_arguments_format(IDL_tree type)
{
    switch (IDL_NODE_TYPE(type)) {
      case IDLN_TYPE_INTEGER: {
        gboolean sign = IDL_TYPE_INTEGER(type).f_signed;
        switch (IDL_TYPE_INTEGER(type).f_type) {
          case IDL_INTEGER_TYPE_LONG:
            return sign ? 'i' : 'u';
          default:
            return '*'; /* XXXbe */
        }
        break;
      }
      case IDLN_TYPE_CHAR:
        return '*'; /* XXXbe */
      case IDLN_TYPE_WIDE_CHAR:
        return 'c';
      case IDLN_TYPE_STRING:
        return 's';
      case IDLN_TYPE_WIDE_STRING:
        return 'S';
      case IDLN_TYPE_BOOLEAN:
        return 'b';
      case IDLN_IDENT:
        return 'o';
      default:
        return '*'; /* XXXbe */
    }
}

static gboolean
emit_convert_result(TreeState *state, const char *from, const char *to,
                    const char *extra)
{
    switch (IDL_NODE_TYPE(state->tree)) {
      case IDLN_TYPE_INTEGER:
        fprintf(state->file,
                "%s  if (!JS_NewNumberValue(cx, (jsdouble) %s, %s))\n"
                "%s    return JS_FALSE;\n",
                extra, from, to,
                extra);
        break;
      case IDLN_TYPE_STRING:
        /* XXXbe must free 'from' using priv's nsIAllocator iff not weak! */
        fprintf(state->file,
                "%s  JSString *str = JS_NewStringCopyZ(cx, %s);\n"
                "%s  if (!str)\n"
                "%s    return JS_FALSE;\n"
                "%s  *%s = STRING_TO_JSVAL(str);\n",
                extra, from,
                extra,
                extra,
                extra, to);
        break;
      case IDLN_TYPE_BOOLEAN:
        fprintf(state->file,
                "%s  *%s = BOOLEAN_TO_JSVAL(%s);\n",
                extra, to, from);
        break;
      case IDLN_IDENT:
        if (IDL_NODE_UP(state->tree) &&
            IDL_NODE_TYPE(IDL_NODE_UP(state->tree)) == IDLN_NATIVE) {
            /* XXXbe issue warning, method should have been noscript? */
            fprintf(state->file,
                    "%s  *%s = JSVAL_NULL;\n",
                    extra, to);
            break;
        }
        fprintf(state->file,
                "%s  *%s = OBJECT_TO_JSVAL(%s::GetJSObject(cx, %s));\n"
                "%s  NS_RELEASE(%s);\n",
                extra, to, IDL_IDENT(state->tree).str, from,
                extra, from);
        break;
      default:
        assert(0); /* XXXbe */
        break;
    }
    return TRUE;
}

static gboolean
stub_op_dcl(TreeState *state)
{
    IDL_tree method = state->tree;
    struct _IDL_OP_DCL *op = &IDL_OP_DCL(method);
    struct stub_private *priv = state->priv;
    IDL_tree iface, iter, param;
    char *className;

    if (op->f_noscript)
        return TRUE;

    append_to_vector(&priv->funcs, &priv->nfuncs, method);
    xpidl_dump_comment(state, 0);

    assert(IDL_NODE_UP(IDL_NODE_UP(method)));
    iface = IDL_NODE_UP(IDL_NODE_UP(method));
    assert(IDL_NODE_TYPE(iface) == IDLN_INTERFACE);
    className = IDL_IDENT(IDL_INTERFACE(iface).ident).str;

    fprintf(state->file,
            "static JSBool\n"
            "%s_%s(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,"
                 " jsval *rval)\n"
            "{\n",
            className, IDL_IDENT(op->ident).str);

    fprintf(state->file,
            "  %s *priv = (%s *)JS_GetPrivate(cx, obj);\n"
            "  if (!priv)\n"
            "    return JS_TRUE;\n",
            className, className);

    for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
        state->tree = IDL_LIST(iter).data;
        fputs("  ", state->file);
        if (!xpcom_param(state))
            return FALSE;
        fputs(";\n", state->file);
    }

    fputs("  if (!JS_ConvertArguments(cx, argc, argv, \"", state->file);
    for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
        param = IDL_LIST(iter).data;
        assert(param);
        fputc(js_convert_arguments_format(IDL_PARAM_DCL(param).param_type_spec),
              state->file);
    }
    fputc('\"', state->file);
    for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
        param = IDL_LIST(iter).data;
        fprintf(state->file, ", &%s",
                IDL_IDENT(IDL_PARAM_DCL(param).simple_declarator).str);
    }
    fputs("))\n"
          "    return JS_FALSE;\n",
          state->file);

    if (op->op_type_spec) {
        state->tree = op->op_type_spec;
        fputs("  ", state->file);
        xpcom_type(state);
        fputs(" retval;\n", state->file);

        fprintf(state->file,
                "  nsresult result = priv->%s(",
                IDL_IDENT(op->ident).str);
        for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
            param = IDL_LIST(iter).data;
            fprintf(state->file,
                    "%s, ",
                    IDL_IDENT(IDL_PARAM_DCL(param).simple_declarator).str);
        }
        fputs("&retval);\n", state->file);

        fputs("  if (NS_FAILED(result)) {\n"
              "    JS_ReportError(cx, XXXnsresult2string(result));\n"
              "    return JS_FALSE;\n"
              "  }\n",
              state->file);

        if (!emit_convert_result(state, "retval", "rval", ""))
            return FALSE;
    }
    fputs("  return JS_TRUE;\n"
          "}\n",
          state->file);
    return TRUE;
}

static gboolean
stub_type_enum(TreeState *state)
{
    fputs("XXXbe type_enum\n", state->file);
    return TRUE;
}

static gboolean
emit_property_op(TreeState *state, const char *className, char which)
{
    struct stub_private *priv = state->priv;
    unsigned i;

    fprintf(state->file,
            "\nstatic JSBool\n"
            "%s_%cetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)\n"
            "{\n"
            "  if (!JSVAL_IS_INT(id))\n"
            "    return JS_TRUE;\n"
            "  nsresult result;\n"
            "  %s *priv = (%s *)JS_GetPrivate(cx, obj);\n"
            "  switch (JSVAL_TO_INT(id)) {\n",
            className, which, className, className);

    for (i = 0; i < priv->nprops; i++) {
        IDL_tree attr = priv->props[i];
        const char *id = IDL_IDENT(IDL_LIST(IDL_ATTR_DCL(attr)
                                            .simple_declarations)
                                   .data).str;

        if (which == 'S' && IDL_ATTR_DCL(attr).f_readonly)
            continue;
        fprintf(state->file,
                "   case %d:\n"
                "    ",
                -1 - (int)i);
        state->tree = IDL_ATTR_DCL(attr).param_type_spec;
        if (!xpcom_type(state))
            return FALSE;
        fprintf(state->file, " %s;\n", id);
        if (which == 'S') {
            fprintf(state->file,
                    "    if (!JS_ConvertArguments(cx, 1, vp, \"%c\", &%s))\n"
                    "      return JS_FALSE;\n",
                    js_convert_arguments_format(state->tree), id);
        }
        fprintf(state->file,
                "    result = priv->%cet%c%s(%s%s);\n",
                which, toupper(*id), id + 1, (which == 'G') ? "&" : "", id);

        fputs("    if (NS_FAILED(result))\n"
              "      goto bad;\n",
              state->file);

        if (which == 'G') {
            if (!emit_convert_result(state, id, "vp", "  "))
                return FALSE;
        }
        fputs(  "    break;\n",
                state->file);
    }

    fputs(  "  }\n"
            "  return JS_TRUE;\n"
            "bad:\n"
            "  JS_ReportError(cx, XXXnsresult2string(result));\n"
            "  return JS_FALSE;\n"
            "}\n",
            state->file);
    return TRUE;
}

static gboolean
stub_interface(TreeState *state)
{
    IDL_tree iface = state->tree;
    const char *className;
    struct stub_private *priv, *save_priv;
    gboolean ok;

    className = IDL_IDENT(IDL_INTERFACE(iface).ident).str;

    priv = xpidl_malloc(sizeof *priv);
    memset(priv, 0, sizeof *priv);
    priv->iface = iface;
    save_priv = state->priv;
    state->priv = priv;

    state->tree = IDL_INTERFACE(iface).body;
    ok = !state->tree || xpidl_process_node(state);
    if (ok) {
        gboolean allreadonly = TRUE;
        unsigned i;

        if (priv->funcs) {
            fprintf(state->file,
                    "\nstatic JSFunctionSpec %s_funcs[] = {\n",
                    className);
            for (i = 0; i < priv->nfuncs; i++) {
                IDL_tree method = priv->funcs[i];
                unsigned nargs = 0;
                IDL_tree iter;
                for (iter = IDL_OP_DCL(method).parameter_dcls;
                     iter;
                     iter = IDL_LIST(iter).next) {
                     nargs++;
                }
                fprintf(state->file, "  {\"%s\", %s_%s, %u},\n",
                        IDL_IDENT(IDL_OP_DCL(method).ident).str,
                        className,
                        IDL_IDENT(IDL_OP_DCL(method).ident).str,
                        nargs);
            }
            fputs("  {0}\n};\n", state->file);
        }

        if (priv->const_dbls) {
            fprintf(state->file,
                    "\nstatic JSConstDoubleSpec %s_const_dbls[] = {\n",
                    className);
            for (i = 0; i < priv->nconst_dbls; i++) {
                fprintf(state->file, "  {%g, \"%s\"},\n", 0., "d'oh!");
            }
            fputs("  {0}\n};\n", state->file);

        }

        if (priv->props) {
            /* XXXbe check for tinyid overflow */
            fprintf(state->file,
                    "\nstatic JSPropertySpec %s_props[] = {\n",
                    className);
            for (i = 0; i < priv->nprops; i++) {
                IDL_tree attr = priv->props[i];
                if (!IDL_ATTR_DCL(attr).f_readonly)
                    allreadonly = FALSE;
                fprintf(state->file, "  {\"%s\", %d",
                  IDL_IDENT(IDL_LIST(IDL_ATTR_DCL(attr).simple_declarations)
                            .data).str, -1 - (int)i);
                if (IDL_ATTR_DCL(attr).f_readonly)
                    fputs(", JSPROP_READONLY", state->file);
                fputs("},\n", state->file);
            }
            fputs("  {0}\n};\n", state->file);

            /* Emit GetProperty. */
            if (!emit_property_op(state, className, 'G'))
                return FALSE;

            /* Emit SetProperty. */
            if (!allreadonly && !emit_property_op(state, className, 'S'))
                return FALSE;
        }

        /* Emit a finalizer. */
        fprintf(state->file,
                "\nstatic void\n"
                "%s_Finalize(JSContext *cx, JSObject *obj)\n"
                "{\n"
                "  %s *priv = (%s *)JS_GetPrivate(cx, obj);\n"
                "  if (!priv)\n"
                "    return;\n"
                "  JSObject *globj = JS_GetGlobalObject(cx);\n"
                "  if (globj)\n"
                "    (void) JS_DeleteElement(cx, globj, (jsint)priv >> 1);\n"
                "  NS_RELEASE(priv);\n"
                "}\n",
                className, className, className);

        /* Emit a constructor. */
        fprintf(state->file,
                "\nstatic JSBool\n"
                "%s_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,"
                       " jsval *rval)\n"
                "{\n"
                "  return JS_TRUE;\n"
                "}\n",
                className);

        /* Emit an initialized JSClass struct. */
        fprintf(state->file,
                "\nstatic JSClass %s_class = {\n"
                "  \"%s\",\n"
                "  JSCLASS_HAS_PRIVATE,\n"
                "  JS_PropertyStub, JS_PropertyStub, ",
                className, className);
        if (priv->props) {
            fprintf(state->file, "%s_GetProperty, ", className);
            if (!allreadonly)
                fprintf(state->file, "%s_SetProperty", className);
            else
                fputs("JS_PropertyStub", state->file);
        } else {
            fputs("JS_PropertyStub, JS_PropertyStub",
                  state->file);
        }
        fputs(",\n  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ",
              state->file);
        fprintf(state->file, "%s_Finalize\n};\n", className);

        /* Emit a InitJSClass static method. */
        fprintf(state->file,
                "\nJSObject *\n"
                "%s::InitJSClass(JSContext *cx)\n"
                "{\n"
                "  JSObject *globj = JS_GetGlobalObject(cx);\n"
                "  if (!globj)\n"
                "    return 0;\n"
                "  JSObject *proto = JS_InitClass(cx, globj, 0, &%s_class, %s_ctor, 0,\n",
                className, className, className);
        fputs(  "                                 ",
                state->file);
        if (priv->props)
            fprintf(state->file, "%s_props", className);
        else
            fputc('0', state->file);
        fputs(", ", state->file);
        if (priv->funcs)
            fprintf(state->file, "%s_funcs", className);
        else
            fputc('0', state->file);
        fputs(  ", 0, 0);\n", state->file);
        if (priv->const_dbls) {
            fprintf(state->file,
                    "  if (!proto)\n"
                    "    return 0;\n"
                    "  JSObject *ctor = JS_GetConstructor(cx, proto);\n"
                    "  if (!ctor || !JS_DefineConstDoubles(cx, ctor, &%s_const_dbls))\n"
                    "    return 0;\n",
                    className);
        }
        fputs(  "  return proto;\n"
                "}\n",
                state->file);

        /* Emit a GetJSObject static method. */
        fprintf(state->file,
                "\nJSObject *\n"
                "%s::GetJSObject(JSContext *cx, %s *priv)\n"
                "{\n"
                "  JSObject *globj = JS_GetGlobalObject(cx);\n"
                "  if (!globj)\n"
                "    return 0;\n"
                "  if (!JS_LookupElement(cx, globj, (jsint)priv >> 1, &v))\n"
                "    return 0;\n"
                "  if (JSVAL_IS_VOID(v)) {\n"
                "    JSObject *obj = JS_NewObject(cx, &foo_class, 0, 0);\n"
                "    if (!obj || !JS_SetPrivate(cx, obj, priv))\n"
                "      return 0;\n"
                "    NS_ADDREF(priv);\n"
                "    v = PRIVATE_TO_JSVAL(obj);\n"
                "    if (!JS_DefineElement(cx, globj, (jsint)priv >> 1, v, 0, 0,\n"
                "                          JSPROP_READONLY | JSPROP_PERMANENT)) {\n"
                "      return 0;\n"
                "    }\n"
                "  }\n"
                "  return (JSObject *)JSVAL_TO_PRIVATE(v);\n"
                "}\n",
                className, className);
    }

    /* Clean up whether or not there were errors. */
    state->priv = save_priv;
    free_stub_private(priv);
    return ok;
}

nodeHandler *
xpidl_stub_dispatch(void)
{
    static nodeHandler table[IDLN_LAST];

    if (!table[IDLN_NONE]) {
        table[IDLN_NONE] = stub_pass_1;
        table[IDLN_LIST] = stub_list;
        table[IDLN_ATTR_DCL] = stub_attr_dcl;
        table[IDLN_OP_DCL] = stub_op_dcl;
        table[IDLN_TYPE_ENUM] = stub_type_enum;
        table[IDLN_INTERFACE] = stub_interface;
    }

    return table;
}
