/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
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

#ifndef icodegenerator_h
#define icodegenerator_h

#include <vector>
#include <stack>

#include "utilities.h"
#include "parser.h"
#include "vmtypes.h"
#include "jsclasses.h"


namespace JavaScript {
namespace ICG {
    
    using namespace VM;
    using namespace JSTypes;
    using namespace JSClasses;
    

    struct VariableList {       // Maps from variable (parameter) name to a TypedRegister.
                                // But because we also want to map from a register number to
                                // it's type, we keep the TypedRegsters in a separate array and
                                // just store the index in the name map.

        typedef std::map<String, uint32, std::less<String> > VariableMap;
        typedef VariableMap::value_type MapValue;

        VariableMap variableMap;
        std::vector<TypedRegister> registerList;
    
        TypedRegister findVariable(const StringAtom& name)
        {
            VariableMap::iterator i = variableMap.find(name);
            return (i == variableMap.end()) 
                        ? TypedRegister(NotARegister, &None_Type) 
                        : registerList[i->second];
        }

        void add(const StringAtom& name, TypedRegister r)
        {
            variableMap.insert(MapValue(name, r.first));
            registerList.resize(r.first + 1);
            registerList[r.first] = r;
        }

        TypedRegister getRegister(uint32 i)
        {
            ASSERT(i < registerList.size());
            return registerList[i];
        }


    };

    struct ParameterList : public VariableList {
        typedef enum { 
            NoRestParameter, 
            HasRestParameterBeforeBar, 
            HasRestParameterAfterBar, 
            HasUnnamedRestParameter 
        } RestParameterStatus;

        
        
        std::vector<bool> mOptionalParameters;      // whether or not a parameter has an optional value
                                                    // ordered by lexical ordering === register number.
        RestParameterStatus mRestParameter;
        uint32 mPositionalCount;                    // number of positional parameters




        ParameterList() : mRestParameter(NoRestParameter), mPositionalCount(0) { }

        void add(const StringAtom& name, TypedRegister r, bool isOptional)
        {
            VariableList::add(name, r);
            mOptionalParameters.resize(r.first + 1);
            mOptionalParameters[r.first] = isOptional;
        }

        uint32 size()   { return registerList.size(); }     // the variableMap may be larger since it contains aliases

        bool isOptional(uint32 i)   { ASSERT(i < mOptionalParameters.size()); return mOptionalParameters[i]; }

        void setRestParameter(RestParameterStatus rs)   { mRestParameter = rs; }
        void setPositionalCount(uint32 x)               { mPositionalCount = x; }

    };


    typedef std::map<uint32, uint32, std::less<uint32> > InstructionMap;

    class ICodeModule {
    public:
        ICodeModule(InstructionStream *iCode, VariableList *variables,
                    ParameterList *parameters,
                    uint32 maxRegister,
                    InstructionMap *instructionMap, 
                    JSType *resultType, uint32 exceptionRegister) :
            its_iCode(iCode), itsVariables(variables), itsParameters(parameters),
            itsMaxRegister(maxRegister),
            mID(++sMaxID), mInstructionMap(instructionMap), 
            mParameterInit(NULL), 
            mEntryPoint(0),
            mResultType(resultType),
            mExceptionRegister(exceptionRegister)
        {
        }

        ~ICodeModule()
        {
            delete its_iCode;
            delete itsVariables;
            delete mInstructionMap;
            if (mParameterInit) delete mParameterInit;
        }

        Formatter& print(Formatter& f);
        void setFileName (String aFileName) { mFileName = aFileName; }
        String getFileName () { return mFileName; }
        
        InstructionStream *its_iCode;
        VariableList *itsVariables;
        ParameterList *itsParameters;
        uint32 itsMaxRegister;
        uint32 mID;
        InstructionMap *mInstructionMap;
        String mFileName;
        uint32 *mParameterInit;
        uint32 mEntryPoint;
        JSType *mResultType;
        uint32 mExceptionRegister;

        static uint32 sMaxID;
        
    };

    typedef std::vector<const StringAtom *> LabelSet;
    class LabelEntry {
    public:
        LabelEntry(LabelSet *labelSet, Label *breakLabel) 
            : labelSet(labelSet), breakLabel(breakLabel), continueLabel(NULL) { }
        LabelEntry(LabelSet *labelSet, Label *breakLabel, Label *continueLabel) 
            : labelSet(labelSet), breakLabel(breakLabel), continueLabel(continueLabel) { }

        bool containsLabel(const StringAtom *label);

        LabelSet *labelSet;
        Label *breakLabel;
        Label *continueLabel;
    };
    typedef std::vector<LabelEntry *> LabelStack;

    Formatter& operator<<(Formatter &f, ICodeModule &i);

    /****************************************************************/
    
    // An ICodeGenerator provides the interface between the parser and the
    // interpreter. The parser constructs one of these for each
    // function/script, adds statements and expressions to it and then
    // converts it into an ICodeModule, ready for execution.
    
    class ICodeGenerator {
    public:
        typedef enum { kNoFlags = 0, kIsTopLevel = 0x01, kIsStaticMethod = 0x02, kIsWithinWith = 0x04 } ICodeGeneratorFlags;
    private:
        InstructionStream *iCode;
        bool iCodeOwner;
        LabelList labels;
        
        Register mTopRegister;              // highest (currently) allocated register
        TypedRegister mExceptionRegister;   // reserved to carry the exception object.
        VariableList *variableList;         // name|register pair for each variable
        ParameterList *parameterList;       // name|register pair for each parameter
                                            // (with #0 reserved for 'this' regardless of scope)
        
        Context *mContext;                  // the world and global object
        LabelStack mLabelStack;             // stack of LabelEntry objects, one per nested looping construct
                                            // maps source position to instruction index
        InstructionMap *mInstructionMap;

        JSClass *mClass;                    // enclosing class when generating code for methods
        ICodeGeneratorFlags mFlags;         // assorted flags
        LabelList *pLabels;                 // label for each parameter initialization entry point

        const StringAtom &mInitName;
        ICodeGenerator *mContainingFunction;// outer function for nested functions

        std::vector<bool> mPermanentRegister;

        Register getTempRegister() 
        { 
            while (mTopRegister < mPermanentRegister.size())
                if (!mPermanentRegister[mTopRegister]) 
                    return mTopRegister++;
                else
                    ++mTopRegister;
            mPermanentRegister.resize(mTopRegister + 1);
            mPermanentRegister[mTopRegister] = false;
            return mTopRegister++;
        }

        void resetTopRegister() { mTopRegister = 0; }
        void resetStatement()   { resetTopRegister(); }

        TypedRegister allocateRegister(JSType *type);

        JSType *findType(const StringAtom& typeName);


        void addParameterLabel(Label *label)    { if (pLabels == NULL) pLabels = new LabelList(); pLabels->push_back(label); }


        Label *setLabel(Label *label);
        
        void jsr(Label *label)                  { iCode->push_back(new Jsr(label)); }
        void rts()                              { iCode->push_back(new Rts()); }
        void branch(Label *label);
        GenericBranch *branchTrue(Label *label, TypedRegister condition);
        GenericBranch *branchFalse(Label *label, TypedRegister condition);
        GenericBranch *branchInitialized(Label *label, TypedRegister condition);
        
        void beginTry(Label *catchLabel, Label *finallyLabel)
                                                { iCode->push_back(new Tryin(catchLabel, finallyLabel)); }
        void endTry()                           { iCode->push_back(new Tryout()); }

        void beginWith(TypedRegister obj)       { iCode->push_back(new Within(obj)); }
        void endWith()                          { iCode->push_back(new Without()); }
        

        void startStatement(uint32 pos)         { (*mInstructionMap)[iCode->size()] = pos; }

        ICodeOp mapExprNodeToICodeOp(ExprNode::Kind kind);
        ExprNode::Kind mapICodeOpToExprNode(ICodeOp op);

        bool isTopLevel()       { return (mFlags & kIsTopLevel) != 0; }
        bool isWithinWith()     { return (mFlags & kIsWithinWith) != 0; }
        bool isStaticMethod()   { return (mFlags & kIsStaticMethod) != 0; }

        void setFlag(uint32 flag, bool v) { mFlags = (ICodeGeneratorFlags)((v) ? mFlags | flag : mFlags & ~flag); }

        typedef enum { NoKind, Var, Property, Slot, Static, Constructor, Name, Method } LValueKind;

        LValueKind getVariableByName(const StringAtom &name, TypedRegister &v);
        LValueKind scanForVariable(const StringAtom &name, TypedRegister &v, uint32 &slotIndex, TypedRegister &base);
        LValueKind resolveIdentifier(const StringAtom &name, TypedRegister &v, uint32 &slotIndex, TypedRegister &base, bool lvalue);
        TypedRegister handleIdentifier(IdentifierExprNode *p, ExprNode::Kind use, ICodeOp xcrementOp, TypedRegister ret, ArgumentList *args, bool lvalue);
        TypedRegister handleDot(BinaryExprNode *b, ExprNode::Kind use, ICodeOp xcrementOp, TypedRegister ret, ArgumentList *args, bool lvalue);
        ICodeModule *genFunction(FunctionDefinition &function, bool isStatic, bool isConstructor, JSClass *superClass);
    
    public:

        ICodeGenerator(Context *cx, ICodeGenerator *containingFunction = NULL, JSClass *aClass = NULL, ICodeGeneratorFlags flags = kIsTopLevel);
        
        ~ICodeGenerator()
        {
            if (iCodeOwner) {
                delete iCode;
                delete mInstructionMap;
                if (pLabels) delete pLabels;
            }
        }
                
        ICodeModule *complete(JSType *resultType);
        void readICode(const char *fileName);

        JSType *extractType(ExprNode *t);
        JSType *getParameterType(FunctionDefinition &function, int index);
    
        TypedRegister genExpr(ExprNode *p, 
                                bool needBoolValueInBranch = false, 
                                Label *trueBranch = NULL, 
                                Label *falseBranch = NULL);
        TypedRegister genStmt(StmtNode *p, LabelSet *currentLabelSet = NULL);

        void returnStmt(TypedRegister r);
        void returnStmt();
        void throwStmt(TypedRegister r)         { iCode->push_back(new Throw(r)); }
        void debuggerStmt()                     { iCode->push_back(new Debugger()); }

        TypedRegister allocateVariable(const StringAtom& name)
        { 
            return allocateVariable(name, &Any_Type);
        }
        
        TypedRegister allocateVariable(const StringAtom& name, const StringAtom& typeName)
        {
            return allocateVariable(name, findType(typeName));
        }
        
        TypedRegister allocateVariable(const StringAtom& name, JSType *type)
        { 
            TypedRegister r = allocateRegister(type);
            variableList->add(name, r);
            return r;
        }
        
        TypedRegister allocateVariable(const StringAtom& name, TypedRegister r)
        { 
            variableList->add(name, r);
            return r;
        }
        
        TypedRegister allocateParameter(const StringAtom& name, bool isOptional)
        { 
            return allocateParameter(name, isOptional, &Any_Type);
        }
        
        TypedRegister allocateParameter(const StringAtom& name, bool isOptional, const StringAtom& typeName)
        {
            return allocateParameter(name, isOptional, findType(typeName));
        }
        
        TypedRegister allocateParameter(const StringAtom& name, bool isOptional, JSType *type)
        { 
            TypedRegister r = allocateRegister(type);
            parameterList->add(name, r, isOptional);
            return r;
        }

        
        Formatter& print(Formatter& f);
        
        TypedRegister op(ICodeOp op, TypedRegister source);
        TypedRegister op(ICodeOp op, TypedRegister source1, TypedRegister source2);
        TypedRegister binaryOp(ICodeOp op, TypedRegister source1, TypedRegister source2);
        TypedRegister call(TypedRegister target, ArgumentList *args);
        TypedRegister directCall(JSFunction *target, ArgumentList *args);
        TypedRegister bindThis(TypedRegister thisArg, TypedRegister target);
        TypedRegister getMethod(TypedRegister thisArg, uint32 slotIndex);
        TypedRegister getClosure(uint32 count);
        TypedRegister newClosure(ICodeModule *icm);

        void move(TypedRegister destination, TypedRegister source);
        TypedRegister logicalNot(TypedRegister source);
        TypedRegister test(TypedRegister source);
        
        TypedRegister loadBoolean(bool value);
        TypedRegister loadImmediate(double value);
        TypedRegister loadString(const String &value);
        TypedRegister loadString(const StringAtom &name);
                
        TypedRegister newObject(TypedRegister constructor);
        TypedRegister newArray();
        TypedRegister newFunction(ICodeModule *icm);
        TypedRegister newClass(JSClass *clazz);

        TypedRegister cast(TypedRegister arg, JSType *toType);
        
        TypedRegister super();
        TypedRegister loadName(const StringAtom &name, JSType *t = &Any_Type);
        void saveName(const StringAtom &name, TypedRegister value);
        TypedRegister nameXcr(const StringAtom &name, ICodeOp op);
       
        TypedRegister deleteProperty(TypedRegister base, const StringAtom &name);
        TypedRegister getProperty(TypedRegister base, const StringAtom &name);
        void setProperty(TypedRegister base, const StringAtom &name, TypedRegister value);
        TypedRegister propertyXcr(TypedRegister base, const StringAtom &name, ICodeOp op);
        
        TypedRegister getStatic(JSClass *base, const String &name);
        void setStatic(JSClass *base, const StringAtom &name, TypedRegister value);
        TypedRegister staticXcr(JSClass *base, const StringAtom &name, ICodeOp op);
        
        TypedRegister getElement(TypedRegister base, TypedRegister index);
        void setElement(TypedRegister base, TypedRegister index, TypedRegister value);
        TypedRegister elementXcr(TypedRegister base, TypedRegister index, ICodeOp op);

        TypedRegister getSlot(TypedRegister base, uint32 slot);
        void setSlot(TypedRegister base, uint32 slot, TypedRegister value);
        TypedRegister slotXcr(TypedRegister base, uint32 slot, ICodeOp op);

        TypedRegister varXcr(TypedRegister var, ICodeOp op);
        
        InstructionStream *getICode()   { return iCode; }
        
        Label *getLabel();

    };

    Formatter& operator<<(Formatter &f, ICodeGenerator &i);
    Formatter& operator<<(Formatter &f, ICodeModule &i);
    Formatter& operator<<(Formatter &f, std::string &s);
    /*
      std::ostream &operator<<(std::ostream &s, ICodeGenerator &i);
      std::ostream &operator<<(std::ostream &s, StringAtom &str);
    */



} /* namespace IGC */
} /* namespace JavaScript */

#endif /* icodegenerator_h */
