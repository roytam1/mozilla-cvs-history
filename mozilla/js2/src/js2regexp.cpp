/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is the JavaScript 2 Prototype.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

#ifdef _WIN32
#include "msvc_pragma.h"
#endif

#include <algorithm>
#include <list>
#include <map>
#include <stack>

#include <assert.h>
#include <ctype.h>

#include "world.h"
#include "utilities.h"
#include "js2value.h"
#include "numerics.h"
#include "reader.h"
#include "parser.h"
#include "regexp.h"
#include "js2engine.h"
#include "bytecodecontainer.h"
#include "js2metadata.h"
#include "jslong.h"
#include "fdlibm_ns.h"
#include "prmjtime.h"

namespace JavaScript {    
namespace MetaData {

    void RegExpInstance::setLastIndex(JS2Metadata *meta, js2val a)
    {
        meta->regexpClass->writePublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["lastIndex"], true, a);
    }
    void RegExpInstance::setGlobal(JS2Metadata *meta, js2val a)
    {
        meta->regexpClass->writePublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["global"], true, a);
    }
    void RegExpInstance::setMultiline(JS2Metadata *meta, js2val a)
    {
        meta->regexpClass->writePublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["multiline"], true, a);
    }
    void RegExpInstance::setIgnoreCase(JS2Metadata *meta, js2val a)
    {
        meta->regexpClass->writePublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["ignoreCase"], true, a);
    }
    void RegExpInstance::setSource(JS2Metadata *meta, js2val a)
    {
        meta->regexpClass->writePublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["source"], true, a);
    }

    js2val RegExpInstance::getLastIndex(JS2Metadata *meta)
    {
        js2val r;
        if (meta->regexpClass->readPublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["lastIndex"], RunPhase, &r))
            ASSERT(false);
        return r;
    }
    js2val RegExpInstance::getGlobal(JS2Metadata *meta)
    {
        js2val r;
        if (meta->regexpClass->readPublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["global"], RunPhase, &r))
            ASSERT(false);
        return r;
    }
    js2val RegExpInstance::getMultiline(JS2Metadata *meta)
    {
        js2val r;
        if (meta->regexpClass->readPublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["multiline"], RunPhase, &r))
            ASSERT(false);
        return r;
    }
    js2val RegExpInstance::getIgnoreCase(JS2Metadata *meta)
    {
        js2val r;
        if (meta->regexpClass->readPublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["ignoreCase"], RunPhase, &r))
            ASSERT(false);
        return r;
    }
    js2val RegExpInstance::getSource(JS2Metadata *meta)
    {
        js2val r;
        if (meta->regexpClass->readPublic(meta, OBJECT_TO_JS2VAL(this), meta->regexpClass, &meta->world.identifiers["source"], RunPhase, &r))
            ASSERT(false);
        return r;
    }

    js2val RegExp_exec(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc)
    {
        if (meta->objectType(thisValue) != meta->regexpClass)
            meta->reportError(Exception::typeError, "RegExp.exec can only be applied to RegExp objects", meta->engine->errorPos());
        RegExpInstance *thisInst = checked_cast<RegExpInstance *>(JS2VAL_TO_OBJECT(thisValue));
   
        js2val result = JS2VAL_NULL;
        if (argc > 0) {
            int32 index = 0;

            const String *str = meta->toString(argv[0]);
            js2val globalMultiline = thisInst->getMultiline(meta);

            if (thisInst->getGlobal(meta)) {
                js2val lastIndex = thisInst->getLastIndex(meta);
                index = meta->toInteger(lastIndex);            
            }

            REMatchState *match = REExecute(thisInst->mRegExp, str->begin(), index, toInt32(str->length()), meta->toBoolean(globalMultiline));
            if (match) {
                ArrayInstance *A = new ArrayInstance(meta, meta->arrayClass->prototype, meta->arrayClass);
                result = OBJECT_TO_JS2VAL(A);
                js2val matchStr = meta->engine->allocString(str->substr((uint32)match->startIndex, (uint32)match->endIndex - match->startIndex));
                meta->createDynamicProperty(A, &meta->world.identifiers[*meta->toString((long)0)], matchStr, ReadWriteAccess, false, true);
                for (int32 i = 0; i < match->parenCount; i++) {
                    if (match->parens[i].index != -1) {
                        js2val parenStr = meta->engine->allocString(str->substr((uint32)(match->parens[i].index), (uint32)(match->parens[i].length)));
                        meta->createDynamicProperty(A, &meta->world.identifiers[*meta->toString(i + 1)], parenStr, ReadWriteAccess, false, true);
                    }
		            else
                        meta->createDynamicProperty(A, &meta->world.identifiers[*meta->toString(i + 1)], JS2VAL_UNDEFINED, ReadWriteAccess, false, true);
                }
/*
                // XXX SpiderMonkey also adds 'index' and 'input' properties to the result
                JSValue::instance(result)->setProperty(cx, cx->Index_StringAtom, CURRENT_ATTR, JSValue::newNumber((float64)(match->startIndex)));
                JSValue::instance(result)->setProperty(cx, cx->Input_StringAtom, CURRENT_ATTR, JSValue::newString(str));

                // XXX Set up the SpiderMonkey 'RegExp statics'
                RegExp_Type->setProperty(cx, cx->LastMatch_StringAtom, CURRENT_ATTR, JSValue::newString(matchStr));
                RegExp_Type->setProperty(cx, cx->LastParen_StringAtom, CURRENT_ATTR, JSValue::newString(parenStr));            
                String *contextStr = new String(str->substr(0, (uint32)match->startIndex));
                RegExp_Type->setProperty(cx, cx->LeftContext_StringAtom, CURRENT_ATTR, JSValue::newString(contextStr));
                contextStr = new String(str->substr((uint32)match->endIndex, (uint32)str->length() - match->endIndex));
                RegExp_Type->setProperty(cx, cx->RightContext_StringAtom, CURRENT_ATTR, JSValue::newString(contextStr));
*/
                if (thisInst->getGlobal(meta)) {
                    index = match->endIndex;
                    thisInst->setLastIndex(meta, meta->engine->allocNumber((float64)index));
                }

            }

        }
        return result;
    }

    js2val RegExp_Constructor(JS2Metadata *meta, const js2val /* thisValue */, js2val *argv, uint32 argc)
    {
        // XXX Change constructors to take js2val pointer for the result (which would be an already
        // rooted pointer).
        RegExpInstance *thisInst = new RegExpInstance(meta, meta->regexpClass->prototype, meta->regexpClass);
        RootKeeper rk(&thisInst);
        js2val thatValue = OBJECT_TO_JS2VAL(thisInst);
        REuint32 flags = 0;

        const String *regexpStr = meta->engine->Empty_StringAtom;
        const String *flagStr = meta->engine->Empty_StringAtom;
        if (argc > 0) {
            if (meta->objectType(argv[0]) == meta->regexpClass) {
                if ((argc == 1) || JS2VAL_IS_UNDEFINED(argv[1])) {
                    RegExpInstance *otherInst = checked_cast<RegExpInstance *>(JS2VAL_TO_OBJECT(argv[0]));
                    js2val src  = otherInst->getSource(meta);
                    ASSERT(JS2VAL_IS_STRING(src));
                    regexpStr = JS2VAL_TO_STRING(src);
                    flags = otherInst->mRegExp->flags;
                }
                else
                    meta->reportError(Exception::typeError, "Illegal RegExp constructor args", meta->engine->errorPos());
            }
            else
                regexpStr = meta->toString(argv[0]);
            if ((argc > 1) && !JS2VAL_IS_UNDEFINED(argv[1])) {
                flagStr = meta->toString(argv[1]);
                if (parseFlags(flagStr->begin(), (int32)flagStr->length(), &flags) != RE_NO_ERROR)
                    meta->reportError(Exception::syntaxError, "Failed to parse RegExp : '{0}'", meta->engine->errorPos(), *regexpStr + "/" + *flagStr);  // XXX error message?
            }
        }
        REState *pState = REParse(regexpStr->begin(), (int32)regexpStr->length(), flags, RE_VERSION_1);
        if (pState) {
            thisInst->mRegExp = pState;
            // XXX ECMA spec says these are DONTENUM
            thisInst->setSource(meta, STRING_TO_JS2VAL(regexpStr));
            thisInst->setGlobal(meta, BOOLEAN_TO_JS2VAL((pState->flags & RE_GLOBAL) == RE_GLOBAL));
            thisInst->setIgnoreCase(meta, BOOLEAN_TO_JS2VAL((pState->flags & RE_IGNORECASE) == RE_IGNORECASE));
            thisInst->setLastIndex(meta, INT_TO_JS2VAL(0));
            thisInst->setMultiline(meta, BOOLEAN_TO_JS2VAL((pState->flags & RE_MULTILINE) == RE_MULTILINE));
        }
        else
            meta->reportError(Exception::syntaxError, "Failed to parse RegExp : '{0}'", meta->engine->errorPos(), "/" + *regexpStr + "/" + *flagStr);  // XXX what about the RE parser error message?
        return thatValue;
    }

    void initRegExpObject(JS2Metadata *meta)
    {
        meta->regexpClass->construct = RegExp_Constructor;

        NamespaceList publicNamespaceList;
        publicNamespaceList.push_back(meta->publicNamespace);

#define INSTANCE_VAR_COUNT (5)

        struct {
            char *name;
            JS2Class *type;
        } RegExpInstanceVars[INSTANCE_VAR_COUNT] = {
            { "source", meta->stringClass }, 
            { "global", meta->booleanClass }, 
            { "multiline", meta->booleanClass }, 
            { "ignoreCase", meta->booleanClass }, 
            { "lastIndex", meta->numberClass },         
        };


        for (uint32 i = 0; i < INSTANCE_VAR_COUNT; i++)
        {
            Multiname *mn = new Multiname(&meta->world.identifiers[RegExpInstanceVars[i].name], &publicNamespaceList);
            InstanceMember *m = new InstanceVariable(mn, RegExpInstanceVars[i].type, true, true, true, meta->regexpClass->slotCount++);
            meta->defineInstanceMember(meta->regexpClass, &meta->cxt, mn->name, mn->nsList, Attribute::NoOverride, false, m, 0);
        }

    }

}
}
