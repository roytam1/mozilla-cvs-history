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

#ifndef js2engine_h___
#define js2engine_h___


#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#pragma warning(disable: 4711)
#pragma warning(disable: 4710)
#endif

namespace JavaScript {
namespace MetaData {




enum JS2Op {
    eAdd,
    eSubtract,
    eMultiply,
    eDivide,
    eModulo,
    eEqual,
    eNotEqual,
    eLess,
    eGreater,
    eLessEqual,
    eGreaterEqual,
    eLogicalXor,
    eMinus,
    ePlus,
    eComplement,
    eLeftShift,
    eRightShift,
    eLogicalRightShift,
    eBitwiseAnd,
    eBitwiseXor,
    eBitwiseOr,
    eTrue,
    eFalse,
    eNull,
    eNumber,
    eString,            // <string pointer:u32>
    eThis,
    eNewObject,         // <argCount:u16>

    eLexicalRead,       // <multiname index:u16>
    eLexicalWrite,      // <multiname index:u16>
    eLexicalRef,        // <multiname index:u16>
    eLexicalDelete,     // <multiname index:u16>
    eDotRead,           // <multiname index:u16>
    eDotWrite,          // <multiname index:u16>
    eDotRef,            // <multiname index:u16>
    eDotDelete,         // <multiname index:u16>
    eBracketRead,
    eBracketWrite,
    eBracketRef,
    eBracketReadForRef,
    eBracketWriteRef,
    eBracketDelete,

    eReturn,
    eReturnVoid,
    ePushFrame,         // <frame index:u16>
    ePopFrame,
    eBranchFalse,       // <branch displacement:s32> XXX save space with short and long versions instead ?
    eBranchTrue,        // <branch displacement:s32>
    eBranch,            // <branch displacement:s32>
    eNew,               // <argCount:u16>
    eCall,              // <argCount:u16>
    eTypeof,

    ePopv,
    ePop,
    eDup,

    eLexicalPostInc,    // <multiname index:u16>
    eLexicalPostDec,    // <multiname index:u16>
    eLexicalPreInc,     // <multiname index:u16>
    eLexicalPreDec,     // <multiname index:u16>
    eLexicalAssignOp,   // <op:u8> <multiname index:u16>
    eDotPostInc,        // <multiname index:u16>
    eDotPostDec,        // <multiname index:u16>
    eDotPreInc,         // <multiname index:u16>
    eDotPreDec,         // <multiname index:u16>
    eDotAssignOp,       // <op:u8> <multiname index:u16>
    eBracketPostInc,
    eBracketPostDec,
    eBracketPreInc,
    eBracketPreDec,
    eBracketAssignOp,   // <op:u8>

};

int getStackEffect(JS2Op op);


class Frame;
class JS2Object;
class JS2Metadata;
class BytecodeContainer;

class JS2Engine {
public:

    JS2Engine(World &world);

    js2val interpret(Phase execPhase, BytecodeContainer *targetbCon);
    js2val interpreterLoop();

    // Use the pc map in the current bytecode container to get a source offset
    size_t errorPos();

    int32 toInt32(float64 f);
    uint32 toUInt32(float64 f);
    uint16 toUInt16(float64 f);

    String *convertValueToString(js2val x);
    js2val convertValueToPrimitive(js2val x);
    float64 convertValueToDouble(js2val x);
    bool convertValueToBoolean(js2val x);
    int32 convertValueToInteger(js2val x);

    String *toString(js2val x)      { if (JS2VAL_IS_STRING(x)) return JS2VAL_TO_STRING(x); else return convertValueToString(x); }
    js2val toPrimitive(js2val x)    { if (JS2VAL_IS_PRIMITIVE(x)) return x; else return convertValueToPrimitive(x); }
    float64 toNumber(js2val x)      { if (JS2VAL_IS_INT(x)) return JS2VAL_TO_INT(x); else if (JS2VAL_IS_DOUBLE(x)) return *JS2VAL_TO_DOUBLE(x); else return convertValueToDouble(x); }
    bool toBoolean(js2val x)        { if (JS2VAL_IS_BOOLEAN(x)) return JS2VAL_TO_BOOLEAN(x); else return convertValueToBoolean(x); }
    int32 toInteger(js2val x)       { if (JS2VAL_IS_INT(x)) return JS2VAL_TO_INT(x); else return convertValueToInteger(x); }

    js2val assignmentConversion(js2val val, JS2Class *type)     { return val; } // XXX s'more code, please


    JS2Metadata *meta;

    // Current engine execution state
    uint8 *pc;
    BytecodeContainer *bCon;
    Phase phase;

    // Handy f.p. values
    js2val nanValue;
    js2val posInfValue;
    js2val negInfValue;

    // A cache of f.p. values (XXX experimentally trying to reduce # of double pointers XXX)
    float64 *float64Table[256];
    js2val allocNumber(float64 x); 
    js2val pushNumber(float64 x)        { js2val retval = allocNumber(x); push(retval); return retval; }
private:
    float64 *newDoubleValue(float64 x);
    js2val retval;

    js2val a,b;
    js2val baseVal,indexVal;


public:


    // Cached StringAtoms for handy access
    StringAtom &true_StringAtom;
    StringAtom &false_StringAtom;
    StringAtom &null_StringAtom;
    StringAtom &undefined_StringAtom;
    StringAtom &public_StringAtom;
    StringAtom &private_StringAtom;
    StringAtom &function_StringAtom;
    StringAtom &object_StringAtom;
    StringAtom &Empty_StringAtom;
    StringAtom &Dollar_StringAtom;

    // The activation stack, when it's empty and a return is executed, the
    // interpreter quits
#define MAX_ACTIVATION_STACK (20)
    struct ActivationFrame {
        uint8 *pc;
        BytecodeContainer *bCon;
        Frame *topFrame;
        Phase phase;
    };
    void jsr(Phase execPhase, BytecodeContainer *bCon);
    bool activationStackEmpty() { return (activationStackTop == activationStack); }
    void rts();
    ActivationFrame *activationStack;
    ActivationFrame *activationStackTop;
    
    
    // The execution stack for expression evaluation, should be empty
    // between statements.
#define MAX_EXEC_STACK (20)

    js2val *execStack;
    js2val *sp;
    
    void push(js2val x)         { ASSERT(sp < (execStack + MAX_EXEC_STACK)); *sp++ = x; }
    js2val pop()                { ASSERT(sp > execStack); return *--sp; }
    js2val top()                { return *(sp - 1); }
    js2val top(int argCount)    { return *(sp - (1 + argCount)); }
    js2val *base(int argCount)  { return (sp - (1 + argCount)); }

    void mark();


    static js2val defaultConstructor(JS2Metadata *meta, const js2val thisValue, js2val argv[], uint32 argc);


};


String *numberToString(float64 *number);
String *numberToString(int32 i);



}
}



#endif

