
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
* Communications Corporation.   Portions created by Netscape are
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

    case eMinus:
        {
	    a = pop();
            a = toGeneralNumber(a);
            if (JS2VAL_IS_LONG(a)) {
                int64 v = *JS2VAL_TO_LONG(a);
                if (JSLL_EQ(v, JSLL_MININT))
                    meta->reportError(Exception::rangeError, "Arithmetic overflow", errorPos());
                JSLL_NEG(v, v);
                pushLong(v);
            }
            else
            if (JS2VAL_IS_ULONG(a)) {
                uint64 v = *JS2VAL_TO_ULONG(a);
                if (JSLL_UCMP(v, >, JSLL_MAXINT))
                    meta->reportError(Exception::rangeError, "Arithmetic overflow", errorPos());
                JSLL_NEG(v, v);
                pushLong(v);
            }
            else
                pushNumber(-toNumber(a));
        }
        break;

    case ePlus:
        {
	    a = pop();
            pushNumber(toNumber(a));
        }
        break;

    case eComplement:
        {
	    a = pop();
            pushNumber(~toInteger(a));
        }
        break;    
    case eLeftShift:
        {
	    b = pop();
	    a = pop();
            a = toGeneralNumber(a);
            int32 count = toInteger(b);
            if (JS2VAL_IS_LONG(a)) {
                int64 r;
                JSLL_SHL(r, *JS2VAL_TO_LONG(a), count & 0x3F);
                pushLong(r);
            }
            else
            if (JS2VAL_IS_ULONG(a)) {
                uint64 r;
                JSLL_SHL(r, *JS2VAL_TO_ULONG(a), count & 0x3F);
                pushULong(r);
            }
            else
            pushNumber(toInteger(a) << (count & 0x1F));
        }
        break;
    case eRightShift:
        {
	    b = pop();
	    a = pop();
            a = toGeneralNumber(a);
            int32 count = toInteger(b);
            if (JS2VAL_IS_LONG(a)) {
                int64 r;
                JSLL_SHR(r, *JS2VAL_TO_LONG(a), count & 0x3F);
                pushLong(r);
            }
            else
            if (JS2VAL_IS_ULONG(a)) {
                uint64 r;
                JSLL_USHR(r, *JS2VAL_TO_ULONG(a), count & 0x3F);
                pushULong(r);
            }
            else
            pushNumber(toInteger(a) >> (count & 0x1F));
        }
        break;
    case eLogicalRightShift:
        {
	    b = pop();
	    a = pop();
            a = toGeneralNumber(a);
            int32 count = toInteger(b);
            if (JS2VAL_IS_LONG(a)) {
                int64 r;
                JSLL_SHR(r, *JS2VAL_TO_LONG(a), count & 0x3F);
                pushLong(r);
            }
            else
            if (JS2VAL_IS_ULONG(a)) {
                uint64 r;
                JSLL_USHR(r, *JS2VAL_TO_ULONG(a), count & 0x3F);
                pushULong(r);
            }
            else
            pushNumber(toUInt32(toInteger(a)) >> (count & 0x1F));
        }
        break;
    case eBitwiseAnd:
        {
	    b = pop();
	    a = pop();
            b = toGeneralNumber(b);
            a = toGeneralNumber(a);
            pushNumber(toInteger(a) & toInteger(b));
        }
        break;
    case eBitwiseXor:
        {
	    b = pop();
	    a = pop();
            pushNumber(toInteger(a) ^ toInteger(b));
        }
        break;
    case eBitwiseOr:
        {
	    b = pop();
	    a = pop();
            pushNumber(toInteger(a) | toInteger(b));
        }
        break;

    case eAdd: 
        {
	    b = pop();
	    a = pop();
	    a = toPrimitive(a);
	    b = toPrimitive(b);
	    if (JS2VAL_IS_STRING(a) || JS2VAL_IS_STRING(b)) {
	        String *astr = toString(a);
	        String *bstr = toString(b);
                String *c = new String(*astr);
                *c += *bstr;
	        push(STRING_TO_JS2VAL(c));
	    }
	    else {
                float64 anum = toNumber(a);
                float64 bnum = toNumber(b);
                pushNumber(anum + bnum);
	    } 
        }
        break;

    case eSubtract: 
        {
	    b = pop();
	    a = pop();
            float64 anum = toNumber(a);
            float64 bnum = toNumber(b);
            pushNumber(anum - bnum);
        }
        break;

    case eMultiply:
        {
	    b = pop();
	    a = pop();
            float64 anum = toNumber(a);
            float64 bnum = toNumber(b);
            pushNumber(anum * bnum);
        }
        break;

    case eDivide:
        {
	    b = pop();
	    a = pop();
            float64 anum = toNumber(a);
            float64 bnum = toNumber(b);
            pushNumber(anum / bnum);
        }
        break;

    case eModulo:
        {
	    b = pop();
	    a = pop();
            float64 anum = toNumber(a);
            float64 bnum = toNumber(b);
#ifdef XP_PC
    /* Workaround MS fmod bug where 42 % (1/0) => NaN, not 42. */
            if (JSDOUBLE_IS_FINITE(anum) && JSDOUBLE_IS_INFINITE(bnum))
                pushNumber(anum);
            else
#endif
            pushNumber(fd::fmod(anum, bnum));
        }
        break;

    case eLogicalXor: 
        {
	    b = pop();
	    a = pop();
            push(BOOLEAN_TO_JS2VAL(toBoolean(a) ^ toBoolean(b)));
        }
        break;

    case eLess:
        {
	    b = pop();
	    a = pop();
            a = toPrimitive(a);
            b = toPrimitive(b);
            bool rval;
            if (JS2VAL_IS_STRING(a) && JS2VAL_IS_STRING(b))
                rval = (*JS2VAL_TO_STRING(a) < *JS2VAL_TO_STRING(b));
            else
                rval = toNumber(a) < toNumber(b);
            push(BOOLEAN_TO_JS2VAL(rval));
        }
        break;

    case eLessEqual:
        {
	    b = pop();
	    a = pop();
            a = toPrimitive(a);
            b = toPrimitive(b);
            bool rval;
            if (JS2VAL_IS_STRING(a) && JS2VAL_IS_STRING(b))
                rval = (*JS2VAL_TO_STRING(a) <= *JS2VAL_TO_STRING(b));
            else
                rval = toNumber(a) <= toNumber(b);
            push(BOOLEAN_TO_JS2VAL(rval));
        }
        break;

    case eGreater:
        {
	    b = pop();
	    a = pop();
            a = toPrimitive(a);
            b = toPrimitive(b);
            bool rval;
            if (JS2VAL_IS_STRING(a) && JS2VAL_IS_STRING(b))
                rval = (*JS2VAL_TO_STRING(a) > *JS2VAL_TO_STRING(b));
            else
                rval = toNumber(a) > toNumber(b);
            push(BOOLEAN_TO_JS2VAL(rval));
        }
        break;
    
    case eGreaterEqual:
        {
	    b = pop();
	    a = pop();
            a = toPrimitive(a);
            b = toPrimitive(b);
            bool rval;
            if (JS2VAL_IS_STRING(a) && JS2VAL_IS_STRING(b))
                rval = (*JS2VAL_TO_STRING(a) >= *JS2VAL_TO_STRING(b));
            else
                rval = toNumber(a) >= toNumber(b);
            push(BOOLEAN_TO_JS2VAL(rval));
        }
        break;
    
    case eNotEqual:
    case eEqual:
        {
            bool rval;
	    b = pop();
	    a = pop();
            if (JS2VAL_IS_NULL(a) || JS2VAL_IS_UNDEFINED(a))
                rval = (JS2VAL_IS_NULL(b) || JS2VAL_IS_UNDEFINED(b));
            else
            if (JS2VAL_IS_BOOLEAN(a)) {
                if (JS2VAL_IS_BOOLEAN(b))
                    rval = (JS2VAL_TO_BOOLEAN(a) == JS2VAL_TO_BOOLEAN(b));
                else {
                    b = toPrimitive(b);
                    if (JS2VAL_IS_NULL(b) || JS2VAL_IS_UNDEFINED(b))
                        rval = false;
                    else
                        rval = (toNumber(a) == toNumber(b));
                }
            }
            else
            if (JS2VAL_IS_NUMBER(a)) {
                b = toPrimitive(b);
                if (JS2VAL_IS_NULL(b) || JS2VAL_IS_UNDEFINED(b))
                    rval = false;
                else
                    rval = (toNumber(a) == toNumber(b));
            }
            else 
            if (JS2VAL_IS_STRING(a)) {
                b = toPrimitive(b);
                if (JS2VAL_IS_NULL(b) || JS2VAL_IS_UNDEFINED(b))
                    rval = false;
                else
                if (JS2VAL_IS_BOOLEAN(b) || JS2VAL_IS_NUMBER(b))
                    rval = (toNumber(a) == toNumber(b));
                else
                    rval = (*JS2VAL_TO_STRING(a) == *JS2VAL_TO_STRING(b));
            }
            else     // a is not a primitive at this point, see if b is...
            if (JS2VAL_IS_NULL(b) || JS2VAL_IS_UNDEFINED(b))
                rval = false;
            else
            if (JS2VAL_IS_BOOLEAN(b)) {
                a = toPrimitive(a);
                if (JS2VAL_IS_NULL(a) || JS2VAL_IS_UNDEFINED(a))
                    rval = false;
                else
                if (JS2VAL_IS_BOOLEAN(a))
                    rval = (JS2VAL_TO_BOOLEAN(a) == JS2VAL_TO_BOOLEAN(b));
                else
                    rval = (toNumber(a) == toNumber(b));
            }
            else
            if (JS2VAL_IS_NUMBER(b)) {
                a = toPrimitive(a);
                if (JS2VAL_IS_NULL(a) || JS2VAL_IS_UNDEFINED(a))
                    rval = false;
                else
                    rval = (toNumber(a) == toNumber(b));
            }
            else
            if (JS2VAL_IS_STRING(b)) {
                a = toPrimitive(a);
                if (JS2VAL_IS_NULL(a) || JS2VAL_IS_UNDEFINED(a))
                    rval = false;
                else
                if (JS2VAL_IS_BOOLEAN(a) || JS2VAL_IS_NUMBER(a))
                    rval = (toNumber(a) == toNumber(b));
                else
                    rval = (*JS2VAL_TO_STRING(a) == *JS2VAL_TO_STRING(b));
            }
            else
                rval = (JS2VAL_TO_OBJECT(a) == JS2VAL_TO_OBJECT(b));
               
            if (op == eEqual)
                push(BOOLEAN_TO_JS2VAL(rval));
            else
                push(BOOLEAN_TO_JS2VAL(!rval));
        }
        break;

    case eLexicalAssignOp:
        {
            op = (JS2Op)*pc++;
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            a = meta->env.lexicalRead(meta, mn, phase);
	    b = pop();
            switch (op) {
            case eAdd:
                {
	            a = toPrimitive(a);
	            b = toPrimitive(b);
	            if (JS2VAL_IS_STRING(a) || JS2VAL_IS_STRING(b)) {
	                String *astr = toString(a);
	                String *bstr = toString(b);
                        String *c = new String(*astr);
                        *c += *bstr;
	                a = STRING_TO_JS2VAL(c);
	            }
	            else {
                        float64 anum = toNumber(a);
                        float64 bnum = toNumber(b);
                        a = allocNumber(anum + bnum);
	            }
                }
                break;
            case eSubtract:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum - bnum);
                }
                break;
            case eMultiply:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum * bnum);
                }
                break;

            case eDivide:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum / bnum);
                }
                break;

            case eModulo:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
#ifdef XP_PC
            /* Workaround MS fmod bug where 42 % (1/0) => NaN, not 42. */
                    if (JSDOUBLE_IS_FINITE(anum) && JSDOUBLE_IS_INFINITE(bnum))
                        a = allocNumber(anum);
                    else
#endif
                    a = allocNumber(fd::fmod(anum, bnum));
                }
                break;

            case eLeftShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) << count);
                }
                break;
            case eRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) >> count);
                }
                break;
            case eLogicalRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toUInt32(toInteger(a)) >> count);
                }
                break;
            case eBitwiseAnd:
                {
                    a = allocNumber(toInteger(a) & toInteger(b));
                }
                break;
            case eBitwiseXor:
                {
                    a = allocNumber(toInteger(a) ^ toInteger(b));
                }
                break;
            case eBitwiseOr:
                {
                    a = allocNumber(toInteger(a) | toInteger(b));
                }
                break;
            case eLogicalXor:
                {
                    a = allocNumber(toBoolean(a) ^ toBoolean(b));
                }
                break;
            }
            meta->env.lexicalWrite(meta, mn, a, true, phase);
            push(a);
        }
        break;

    case eLexicalPostInc:
        {
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            a = meta->env.lexicalRead(meta, mn, phase);
            float64 num = toNumber(a);
            meta->env.lexicalWrite(meta, mn, allocNumber(num + 1.0), true, phase);
            pushNumber(num);
        }
        break;
    case eLexicalPostDec:
        {
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            a = meta->env.lexicalRead(meta, mn, phase);
            float64 num = toNumber(a);
            meta->env.lexicalWrite(meta, mn, allocNumber(num - 1.0), true, phase);
            pushNumber(num);
        }
        break;
    case eLexicalPreInc:
        {
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            a = meta->env.lexicalRead(meta, mn, phase);
            float64 num = toNumber(a);
            a = pushNumber(num + 1.0);
            meta->env.lexicalWrite(meta, mn, a, true, phase);
        }
        break;
    case eLexicalPreDec:
        {
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            a = meta->env.lexicalRead(meta, mn, phase);
            float64 num = toNumber(a);
            a = pushNumber(num - 1.0);
            meta->env.lexicalWrite(meta, mn, a, true, phase);
        }
        break;

    case eDotAssignOp:
        {
            op = (JS2Op)*pc++;
            LookupKind lookup(false, NULL);
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
	    b = pop();
            baseVal = pop();
            if (!meta->readProperty(baseVal, mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn->name);
            switch (op) {
            case eAdd:
                {
	            a = toPrimitive(a);
	            b = toPrimitive(b);
	            if (JS2VAL_IS_STRING(a) || JS2VAL_IS_STRING(b)) {
	                String *astr = toString(a);
	                String *bstr = toString(b);
                        String *c = new String(*astr);
                        *c += *bstr;
	                a = STRING_TO_JS2VAL(c);
	            }
	            else {
                        float64 anum = toNumber(a);
                        float64 bnum = toNumber(b);
                        a = allocNumber(anum + bnum);
	            }
                }
                break;
            case eSubtract:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum - bnum);
                }
                break;
            case eMultiply:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum * bnum);
                }
                break;

            case eDivide:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum / bnum);
                }
                break;

            case eModulo:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
#ifdef XP_PC
            /* Workaround MS fmod bug where 42 % (1/0) => NaN, not 42. */
                    if (JSDOUBLE_IS_FINITE(anum) && JSDOUBLE_IS_INFINITE(bnum))
                        a = allocNumber(anum);
                    else
#endif
                    a = allocNumber(fd::fmod(anum, bnum));
                }
                break;

            case eLeftShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) << count);
                }
                break;
            case eRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) >> count);
                }
                break;
            case eLogicalRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toUInt32(toInteger(a)) >> count);
                }
                break;
            case eBitwiseAnd:
                {
                    a = allocNumber(toInteger(a) & toInteger(b));
                }
                break;
            case eBitwiseXor:
                {
                    a = allocNumber(toInteger(a) ^ toInteger(b));
                }
                break;
            case eBitwiseOr:
                {
                    a = allocNumber(toInteger(a) | toInteger(b));
                }
                break;
            case eLogicalXor:
                {
                    a = allocNumber(toBoolean(a) ^ toBoolean(b));
                }
                break;
            }
            meta->writeProperty(baseVal, mn, &lookup, true, a, RunPhase);
            push(a);
            baseVal = JS2VAL_VOID;
        }
        break;

    case eDotPostInc:
        {
            LookupKind lookup(false, NULL);
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            baseVal = pop();
            if (!meta->readProperty(baseVal, mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn->name);
            float64 num = toNumber(a);
            meta->writeProperty(baseVal, mn, &lookup, true, allocNumber(num + 1.0), RunPhase);
            pushNumber(num);
            baseVal = JS2VAL_VOID;
        }
        break;
    case eDotPostDec:
        {
            LookupKind lookup(false, NULL);
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            baseVal = pop();
            if (!meta->readProperty(baseVal, mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn->name);
            float64 num = toNumber(a);
            meta->writeProperty(baseVal, mn, &lookup, true, allocNumber(num - 1.0), RunPhase);
            pushNumber(num);
            baseVal = JS2VAL_VOID;
        }
        break;
    case eDotPreInc:
        {
            LookupKind lookup(false, NULL);
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            baseVal = pop();
            if (!meta->readProperty(baseVal, mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn->name);
            float64 num = toNumber(a);
            a = pushNumber(num + 1.0);
            meta->writeProperty(baseVal, mn, &lookup, true, a, RunPhase);
            baseVal = JS2VAL_VOID;
        }
        break;
    case eDotPreDec:
        {
            LookupKind lookup(false, NULL);
            Multiname *mn = bCon->mMultinameList[BytecodeContainer::getShort(pc)];
            pc += sizeof(short);
            baseVal = pop();
            if (!meta->readProperty(baseVal, mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn->name);
            float64 num = toNumber(a);
            a = pushNumber(num - 1.0);
            meta->writeProperty(baseVal, mn, &lookup, true, a, RunPhase);
            baseVal = JS2VAL_VOID;
        }
        break;

    case eBracketAssignOp:
        {
            op = (JS2Op)*pc++;
            LookupKind lookup(false, NULL);
	    b = pop();
            indexVal = pop();
            baseVal = pop();
            String *indexStr = toString(indexVal);
            Multiname mn(meta->world.identifiers[*indexStr], meta->publicNamespace);
            if (!meta->readProperty(baseVal, &mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn.name);
            switch (op) {
            case eAdd:
                {
	            a = toPrimitive(a);
	            b = toPrimitive(b);
	            if (JS2VAL_IS_STRING(a) || JS2VAL_IS_STRING(b)) {
	                String *astr = toString(a);
	                String *bstr = toString(b);
                        String *c = new String(*astr);
                        *c += *bstr;
	                a = STRING_TO_JS2VAL(c);
	            }
	            else {
                        float64 anum = toNumber(a);
                        float64 bnum = toNumber(b);
                        a = allocNumber(anum + bnum);
	            }
                }
                break;
            case eSubtract:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum - bnum);
                }
                break;
            case eMultiply:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum * bnum);
                }
                break;

            case eDivide:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
                    a = allocNumber(anum / bnum);
                }
                break;

            case eModulo:
                {
                    float64 anum = toNumber(a);
                    float64 bnum = toNumber(b);
#ifdef XP_PC
            /* Workaround MS fmod bug where 42 % (1/0) => NaN, not 42. */
                    if (JSDOUBLE_IS_FINITE(anum) && JSDOUBLE_IS_INFINITE(bnum))
                        a = allocNumber(anum);
                    else
#endif
                    a = allocNumber(fd::fmod(anum, bnum));
                }
                break;

            case eLeftShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) << count);
                }
                break;
            case eRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toInteger(a) >> count);
                }
                break;
            case eLogicalRightShift:
                {
                    int32 count = toInteger(b) & 0x1F;
                    a = allocNumber(toUInt32(toInteger(a)) >> count);
                }
                break;
            case eBitwiseAnd:
                {
                    a = allocNumber(toInteger(a) & toInteger(b));
                }
                break;
            case eBitwiseXor:
                {
                    a = allocNumber(toInteger(a) ^ toInteger(b));
                }
                break;
            case eBitwiseOr:
                {
                    a = allocNumber(toInteger(a) | toInteger(b));
                }
                break;
            case eLogicalXor:
                {
                    a = allocNumber(toBoolean(a) ^ toBoolean(b));
                }
                break;
            }
            meta->writeProperty(baseVal, &mn, &lookup, true, a, RunPhase);
            push(a);
            baseVal = JS2VAL_VOID;
            indexVal = JS2VAL_VOID;
        }
        break;

case eBracketPostInc:
        {
            LookupKind lookup(false, NULL);
            indexVal = pop();
            baseVal = pop();
            String *indexStr = toString(indexVal);
            Multiname mn(meta->world.identifiers[*indexStr], meta->publicNamespace);
            if (!meta->readProperty(baseVal, &mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn.name);
            float64 num = toNumber(a);
            meta->writeProperty(baseVal, &mn, &lookup, true, allocNumber(num + 1.0), RunPhase);
            pushNumber(num);
            baseVal = JS2VAL_VOID;
            indexVal = JS2VAL_VOID;
        }
        break;
    case eBracketPostDec:
        {
            LookupKind lookup(false, NULL);
            indexVal = pop();
            baseVal = pop();
            String *indexStr = toString(indexVal);
            Multiname mn(meta->world.identifiers[*indexStr], meta->publicNamespace);
            if (!meta->readProperty(baseVal, &mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn.name);
            float64 num = toNumber(a);
            meta->writeProperty(baseVal, &mn, &lookup, true, allocNumber(num - 1.0), RunPhase);
            pushNumber(num);
            baseVal = JS2VAL_VOID;
            indexVal = JS2VAL_VOID;
        }
        break;
    case eBracketPreInc:
        {
            LookupKind lookup(false, NULL);
            indexVal = pop();
            baseVal = pop();
            String *indexStr = toString(indexVal);
            Multiname mn(meta->world.identifiers[*indexStr], meta->publicNamespace);
            if (!meta->readProperty(baseVal, &mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn.name);
            float64 num = toNumber(a);
            a = pushNumber(num + 1.0);
            meta->writeProperty(baseVal, &mn, &lookup, true, a, RunPhase);
            baseVal = JS2VAL_VOID;
            indexVal = JS2VAL_VOID;
        }
        break;
    case eBracketPreDec:
        {
            LookupKind lookup(false, NULL);
            indexVal = pop();
            baseVal = pop();
            String *indexStr = toString(indexVal);
            Multiname mn(meta->world.identifiers[*indexStr], meta->publicNamespace);
            if (!meta->readProperty(baseVal, &mn, &lookup, RunPhase, &a))
                meta->reportError(Exception::propertyAccessError, "No property named {0}", errorPos(), mn.name);
            float64 num = toNumber(a);
            a = pushNumber(num - 1.0);
            meta->writeProperty(baseVal, &mn, &lookup, true, a, RunPhase);
            baseVal = JS2VAL_VOID;
            indexVal = JS2VAL_VOID;
        }
        break;
