
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

#ifndef js2metadata_h___
#define js2metadata_h___


namespace JavaScript {
namespace MetaData {


// forward declarations:
class JS2Object;
class JS2Metadata;
class JS2Class;
class LocalBinding;
class Environment;
class Context;
class CompoundAttribute;
class BytecodeContainer;
class Pond;
class SimpleInstance;


typedef void (Invokable)();
typedef js2val (Callor)(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
typedef js2val (Constructor)(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
typedef js2val (NativeCode)(JS2Metadata *meta, const js2val thisValue, js2val argv[], uint32 argc);

typedef bool (Read)(JS2Metadata *meta, js2val base, JS2Class *limit, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval);
typedef bool (Write)(JS2Metadata *meta, js2val base, JS2Class *limit, Multiname *multiname, LookupKind *lookupKind, js2val rval);

extern void initDateObject(JS2Metadata *meta);
extern void initStringObject(JS2Metadata *meta);
extern void initMathObject(JS2Metadata *meta);
extern void initArrayObject(JS2Metadata *meta);
extern void initRegExpObject(JS2Metadata *meta);
extern void initNumberObject(JS2Metadata *meta);
extern void initErrorObject(JS2Metadata *meta);
extern void initBooleanObject(JS2Metadata *meta);
extern void initFunctionObject(JS2Metadata *meta);

extern js2val Error_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val EvalError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val RangeError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val ReferenceError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val SyntaxError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val TypeError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val UriError_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val String_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val RegExp_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val RegExp_exec(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val Boolean_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);
extern js2val Number_Constructor(JS2Metadata *meta, const js2val thisValue, js2val *argv, uint32 argc);

typedef struct {
    char *name;
    uint16 length;
    NativeCode *code;
} FunctionData;


extern uint32 getLength(JS2Metadata *meta, JS2Object *obj);
extern js2val setLength(JS2Metadata *meta, JS2Object *obj, uint32 length);

// OBJECT is the semantic domain of all possible objects and is defined as:
// OBJECT = UNDEFINED | NULL | BOOLEAN | FLOAT64 | LONG | ULONG | CHARACTER | STRING | NAMESPACE |
// COMPOUNDATTRIBUTE | CLASS | METHODCLOSURE | INSTANCE | PACKAGE
//
//  In this implementation, the primitive types are distinguished by the tag value
// of a JS2Value (see js2value.h). Non-primitive types are distinguished by calling
// 'kind()' on the object to recover one of the values below:
//
enum ObjectKind { 
    AttributeObjectKind,
    SystemKind,                 
    PackageKind, 
    ParameterKind, 
    ClassKind, 
    BlockFrameKind, 
    SimpleInstanceKind,
    MultinameKind,
    MethodClosureKind,
    AlienInstanceKind,
    ForIteratorKind,
    WithFrameKind,

    EnvironmentKind,         // Not an available JS2 runtime kind
    MetaDataKind
};

enum Plurality { Singular, Plural };

enum Hint { NoHint, NumberHint, StringHint };

class PondScum {
public:    
    void resetMark()        { size &= 0x7FFFFFFF; }
    void mark()             { size |= 0x80000000; }
    bool isMarked()         { return ((size & 0x80000000) != 0); }
    uint32 getSize()        { return size & 0x7FFFFFFF; }
    void setSize(uint32 sz) { ASSERT((sz & 0x8000000) == 0); size = (sz & 0x7FFFFFFF); }

    Pond *owner;    // for a piece of scum in use, this points to it's own Pond
                    // otherwise it's a link to the next item on the free list
private:
    uint32 size;    // The high bit is used as the gc mark flag
};

// A pond is a place to get chunks of PondScum from and to return them to
#define POND_SIZE (64000)
#define POND_SANITY (0xFADE2BAD)
class Pond {
public:
    Pond(size_t sz, Pond *nextPond);
    
    void *allocFromPond(size_t sz);
    uint32 returnToPond(PondScum *p);

    void resetMarks();
    uint32 moveUnmarkedToFreeList();

    uint32 sanity;

    size_t pondSize;
    uint8 *pondBase;
    uint8 *pondBottom;
    uint8 *pondTop;

    PondScum *freeHeader;

    Pond *nextPond;
};

#define GCMARKOBJECT(n) if ((n) && !(n)->isMarked()) { (n)->markObject(); (n)->markChildren(); }
#define GCMARKVALUE(v) JS2Object::markJS2Value(v)

class JS2Object {
// Every object is either undefined, null, a Boolean,
// a number, a string, a namespace, a compound attribute, a class, a method closure, 
// a class instance, a package object, or the global object.
public:

    JS2Object(ObjectKind kind) : kind(kind) { }

    ObjectKind kind;

    static Pond pond;
    static std::list<PondScum **> rootList;
    typedef std::list<PondScum **>::iterator RootIterator;

    static uint32 gc();
    static RootIterator addRoot(void *t);   // pass the address of any JS2Object pointer
                                            // Note: Not the address of a JS2VAL!
    static void removeRoot(RootIterator ri);

    static void *alloc(size_t s);
    static void unalloc(void *p);

    void *operator new(size_t s)    { return alloc(s); }
    void operator delete(void *p)   { unalloc(p); }

    virtual void markChildren()     { }
    bool isMarked()                 { return ((PondScum *)this)[-1].isMarked(); }
    void markObject()               { ((PondScum *)this)[-1].mark(); }

    static void mark(const void *p)       { ((PondScum *)p)[-1].mark(); }
    static void markJS2Value(js2val v);

    virtual void writeProperty(JS2Metadata *meta, const String *name, js2val newValue, uint32 flags)  { ASSERT(false); }
};

class RootKeeper {
public:
    RootKeeper(void *t) : ri(JS2Object::addRoot(t)) { }
    ~RootKeeper() { JS2Object::removeRoot(ri); }

    JS2Object::RootIterator ri;
};

class Attribute : public JS2Object {
public:
    enum AttributeKind { TrueAttr, FalseAttr, NamespaceAttr, CompoundAttr };
    enum MemberModifier { NoModifier, Static, Constructor, Abstract, Virtual, Final};
    enum OverrideModifier { NoOverride, DoOverride, DontOverride, OverrideUndefined };


    Attribute(AttributeKind akind) : JS2Object(AttributeObjectKind), attrKind(akind) { }

    static Attribute *combineAttributes(Attribute *a, Attribute *b);
    static CompoundAttribute *toCompoundAttribute(Attribute *a);

    virtual CompoundAttribute *toCompoundAttribute()    { ASSERT(false); return NULL; }

    AttributeKind attrKind;
};

// A Namespace (is also an attribute)
class Namespace : public Attribute {
public:
    Namespace(const String *name) : Attribute(NamespaceAttr), name(name) { }

    virtual CompoundAttribute *toCompoundAttribute();

    const String *name;       // The namespace's name (used by toString)
};

// A QualifiedName is the combination of an identifier and a namespace
class QualifiedName {
public:
    QualifiedName() : nameSpace(NULL), id(NULL) { }
    QualifiedName(Namespace *nameSpace, const String *id) : nameSpace(nameSpace), id(id) { }

    bool operator ==(const QualifiedName &b) { return (nameSpace == b.nameSpace) && (*id == *b.id); }

    Namespace *nameSpace;    // The namespace qualifier
    const String *id;        // The name
};

// A MULTINAME is the semantic domain of sets of qualified names. Multinames are used internally in property lookup.
// We keep Multinames as a basename and a list of namespace qualifiers (XXX is that right - would the basename 
// ever be different for the same multiname?)

// XXX can nsList ever be null, or could allow null nsList to indicate public?
typedef std::vector<Namespace *> NamespaceList;
typedef NamespaceList::iterator NamespaceListIterator;
class Multiname : public JS2Object {
public:    
    Multiname(const String *name) : JS2Object(MultinameKind), name(name), nsList(new NamespaceList) { }
    Multiname(const String *name, Namespace *ns) : JS2Object(MultinameKind), name(name), nsList(new NamespaceList) { addNamespace(ns); }

    Multiname(const Multiname& m) : JS2Object(MultinameKind), name(m.name), nsList(m.nsList)    { }

    void addNamespace(Namespace *ns)                { nsList->push_back(ns); }
    void addNamespace(NamespaceList *ns);
    void addNamespace(Context &cxt);

    bool matches(QualifiedName &q)                  { return (*name == *q.id) && listContains(q.nameSpace); }
    bool listContains(Namespace *nameSpace);

    const String *name;
    NamespaceList *nsList;

    virtual void markChildren();
    virtual ~Multiname()            { }
};


class NamedParameter {
public:
    const String *name;      // This parameter's name
    JS2Class *type;        // This parameter's type
};

class Signature {
    JS2Class **requiredPositional;      // List of the types of the required positional parameters
    JS2Class **optionalPositional;      // List of the types of the optional positional parameters, which follow the
                                        // required positional parameters
    NamedParameter **optionalNamed;     // Set of the types and names of the optional named parameters
    JS2Class *rest;                     // The type of any extra arguments that may be passed or null if no extra
                                        // arguments are allowed
    bool restAllowsNames;               // true if the extra arguments may be named
    bool returnType;                    // The type of this function's result
};

// A base class for Instance and Local members for convenience.
class Member {
public:
    enum MemberKind { Forbidden, DynamicVariableKind, Variable, ConstructorMethod, Setter, Getter, InstanceVariableKind, InstanceMethodKind, InstanceAccessorKind };
    
    Member(MemberKind kind) : kind(kind) { }

    MemberKind kind;

    virtual void mark()                 { }
};

// A local member is either forbidden, a dynamic variable, a variable, a constructor method, a getter or a setter:
class LocalMember : public Member {
public:
    LocalMember(MemberKind kind) : Member(kind), forbidden(false) { }
    LocalMember(MemberKind kind, bool forbidden) : Member(kind), forbidden(forbidden) { }

    LocalMember *cloneContent;  // Used during cloning operation to prevent cloning of duplicates (i.e. once
                                // a clone exists for this member it's recorded here and used for any other
                                // bindings that refer to this member.)
                                // Also used thereafter by 'assignArguments' to initialize the singular
                                // variable instantations in a parameter frame.

    virtual LocalMember *clone()       { if (forbidden) return this; ASSERT(false); return NULL; }
    bool forbidden;
};

#define FUTURE_TYPE ((JS2Class *)(-1))

class Variable : public LocalMember {
public:
    Variable() : LocalMember(Member::Variable), type(NULL), value(JS2VAL_VOID), immutable(false), vb(NULL) { }
    Variable(JS2Class *type, js2val value, bool immutable) : LocalMember(LocalMember::Variable), type(type), value(value), immutable(immutable), vb(NULL) { }

    virtual LocalMember *clone()   { return new Variable(type, value, immutable); }
    
    JS2Class *type;                 // Type of values that may be stored in this variable, NULL if INACCESSIBLE, FUTURE_TYPE if pending
    js2val value;                   // This variable's current value; future if the variable has not been declared yet;
                                    // uninitialised if the variable must be written before it can be read
    bool immutable;                 // true if this variable's value may not be changed once set

    // XXX union this with the type field later?
    VariableBinding *vb;            // The variable definition node, to resolve future types

    virtual void mark()                 { GCMARKVALUE(value); }
};

class DynamicVariable : public LocalMember {
public:
    DynamicVariable() : LocalMember(Member::DynamicVariableKind), value(JS2VAL_UNDEFINED), sealed(false) { }

    js2val value;                   // This variable's current value
                                    // XXX may be an uninstantiated function at compile time
    bool sealed;                    // true if this variable cannot be deleted using the delete operator

    virtual LocalMember *clone()       { return new DynamicVariable(); }
    virtual void mark()                { GCMARKVALUE(value); }
};

class ConstructorMethod : public LocalMember {
public:
    ConstructorMethod() : LocalMember(Member::ConstructorMethod), value(JS2VAL_VOID) { }
    ConstructorMethod(js2val value) : LocalMember(Member::ConstructorMethod), value(value) { }

    js2val value;           // This constructor itself (a callable object)

    virtual void mark()                 { GCMARKVALUE(value); }
};

class Getter : public LocalMember {
public:
    Getter() : LocalMember(Member::Getter), type(NULL), code(NULL) { }

    JS2Class *type;         // The type of the value read from this getter
    Invokable *code;        // calling this object does the read

    virtual void mark();
};

class Setter : public LocalMember {
public:
    Setter() : LocalMember(Member::Setter), type(NULL), code(NULL) { }

    JS2Class *type;         // The type of the value written into the setter
    Invokable *code;        // calling this object does the write

    virtual void mark();
};



// A LOCALBINDING describes the member to which one qualified name is bound in a frame. Multiple 
// qualified names may be bound to the same member in a frame, but a qualified name may not be 
// bound to multiple members in a frame (except when one binding is for reading only and 
// the other binding is for writing only).
class LocalBinding {
public:
    LocalBinding(AccessSet accesses, LocalMember *content) : accesses(accesses), content(content), xplicit(false) { }

// The qualified name is to be inferred from the map where this binding is kept
//    QualifiedName qname;        // The qualified name bound by this binding

    AccessSet accesses;
    LocalMember *content;       // The member to which this qualified name was bound
    bool xplicit;               // true if this binding should not be imported into the global scope by an import statement
};

class InstanceMember : public Member {
public:
    InstanceMember(MemberKind kind, JS2Class *type, bool final) : Member(kind), type(type), final(final) { }

    JS2Class *type;             // Type of values that may be stored in this variable
    bool final;                 // true if this member may not be overridden in subclasses

    virtual void mark();
};

class InstanceVariable : public InstanceMember {
public:
    InstanceVariable(JS2Class *type, bool immutable, bool final, uint32 slotIndex) : InstanceMember(InstanceVariableKind, type, final), immutable(immutable), slotIndex(slotIndex) { }
    Invokable *evalInitialValue;    // A function that computes this variable's initial value
    bool immutable;                 // true if this variable's value may not be changed once set
    uint32 slotIndex;               // The index into an instance's slot array in which this variable is stored

    virtual void mark();
};

class InstanceMethod : public InstanceMember {
public:
    InstanceMethod(SimpleInstance *fInst) : InstanceMember(InstanceMethodKind, NULL, false), fInst(fInst) { }
    Signature type;         // This method's signature
//    Invokable *code;        // This method itself (a callable object); null if this method is abstract
    SimpleInstance *fInst;

    virtual void mark();
};

class InstanceAccessor : public InstanceMember {
public:
    InstanceAccessor(Invokable *code, JS2Class *type, bool final) : InstanceMember(InstanceAccessorKind, type, final), code(code) { }
    Invokable *code;        // A callable object which does the read or write; null if this method is abstract
};

class InstanceBinding {
public:
    InstanceBinding(AccessSet accesses, InstanceMember *content) : accesses(accesses), content(content) { }

// The qualified name is to be inferred from the map where this binding is kept
//    QualifiedName qname;         // The qualified name bound by this binding
    AccessSet accesses;
    InstanceMember *content;     // The member to which this qualified name was bound
};

// Override status is used to resolve overriden definitions for instance members
#define POTENTIAL_CONFLICT ((InstanceMember *)(-1))
class OverrideStatus {
public:
    OverrideStatus(InstanceMember *overriddenMember, const String *name)
        : overriddenMember(overriddenMember), multiname(name) { }
    
    InstanceMember *overriddenMember;   // NULL for none
    Multiname multiname;
};
typedef std::pair<OverrideStatus *, OverrideStatus *> OverrideStatusPair;


template<class Binding> class BindingEntry {
public:
    BindingEntry(const String s) : name(s) { }

    BindingEntry *clone();
    void clear();

    typedef std::pair<Namespace *, Binding *> NamespaceBinding;
    typedef std::vector<NamespaceBinding> NamespaceBindingList;
    typedef NamespaceBindingList::iterator NS_Iterator;

    NS_Iterator begin() { return bindingList.begin(); }
    NS_Iterator end() { return bindingList.end(); }


    const String name;
    NamespaceBindingList bindingList;

};

typedef BindingEntry<LocalBinding> LocalBindingEntry;

// A LocalBindingMap maps names to a list of LocalBindings. Each LocalBinding in the list
// will have the same QualifiedName.name, but (potentially) different QualifiedName.namespace values
typedef HashTable<LocalBindingEntry *, const String> LocalBindingMap;
typedef TableIterator<LocalBindingEntry *, const String> LocalBindingIterator;


typedef BindingEntry<InstanceBinding> InstanceBindingEntry;

typedef HashTable<InstanceBindingEntry *, const String> InstanceBindingMap;
typedef TableIterator<InstanceBindingEntry *, const String> InstanceBindingIterator;


// A frame contains bindings defined at a particular scope in a program. A frame is either the top-level system frame, 
// a global object, a package, a function frame, a class, or a block frame
class Frame : public JS2Object {
public:

    Frame(ObjectKind kind) : JS2Object(kind) { }

    virtual void instantiate(Environment * /*env*/)  { ASSERT(false); }

    virtual void markChildren()     { }
    virtual ~Frame()                { }

};

class WithFrame : public Frame {
public:
    WithFrame(JS2Object *b) : Frame(WithFrameKind), obj(b) { }
    virtual ~WithFrame()    { }

    virtual void markChildren()     { GCMARKOBJECT(obj); }

    JS2Object *obj;
};

class NonWithFrame : public Frame {
public:

    NonWithFrame(ObjectKind kind) : Frame(kind), temps(NULL), pluralFrame(NULL) { }
    NonWithFrame(ObjectKind kind, NonWithFrame *pluralFrame) : Frame(kind), temps(NULL), pluralFrame(pluralFrame) { }

    LocalBindingMap localBindings;        // Map of qualified names to members defined in this frame

    std::vector<js2val> *temps;               // temporaries allocted in this frame
    uint16 allocateTemp();

    virtual void instantiate(Environment * /*env*/)  { ASSERT(false); }

    NonWithFrame *pluralFrame;                // for a singular frame, this is the plural frame from which it will be instantiated

    virtual void markChildren();
    virtual ~NonWithFrame()                { }
};

// The top-level frame containing predefined constants, functions, and classes.
class SystemFrame : public NonWithFrame {
public:
    SystemFrame() : NonWithFrame(SystemKind) { }
    virtual ~SystemFrame()            { }
};


// Environments contain the bindings that are visible from a given point in the source code. An ENVIRONMENT is 
// a list of two or more frames. Each frame corresponds to a scope. More specific frames are listed first
// -each frame's scope is directly contained in the following frame's scope. The last frame is always the
// SYSTEMFRAME. The next-to-last frame is always a PACKAGE or GLOBAL frame.
typedef std::deque<Frame *> FrameList;
typedef FrameList::iterator FrameListIterator;

// Deriving from JS2Object for gc sake only
class Environment : public JS2Object {
public:
    Environment(SystemFrame *systemFrame, Frame *nextToLast) : JS2Object(EnvironmentKind) { frameList.push_back(nextToLast); frameList.push_back(systemFrame);  }
    virtual ~Environment()                  { }

    Environment(Environment *e) : JS2Object(EnvironmentKind), frameList(e->frameList) { }
    
    JS2Class *getEnclosingClass();
    FrameListIterator getRegionalFrame();
    FrameListIterator getRegionalEnvironment();
    Frame *getTopFrame()                    { return frameList.front(); }
    FrameListIterator getBegin()            { return frameList.begin(); }
    FrameListIterator getEnd()              { return frameList.end(); }
    Frame *getPackageFrame();
    SystemFrame *getSystemFrame()           { return checked_cast<SystemFrame *>(frameList.back()); }

    void setTopFrame(Frame *f)              { while (frameList.front() != f) frameList.pop_front(); }

    void addFrame(Frame *f)                 { frameList.push_front(f); }
    void removeTopFrame()                   { frameList.pop_front(); }

    js2val findThis(bool allowPrototypeThis);
    void lexicalRead(JS2Metadata *meta, Multiname *multiname, Phase phase, js2val *rval);
    void lexicalWrite(JS2Metadata *meta, Multiname *multiname, js2val newValue, bool createIfMissing, Phase phase);
    void lexicalInit(JS2Metadata *meta, Multiname *multiname, js2val newValue);
    bool lexicalDelete(JS2Metadata *meta, Multiname *multiname, Phase phase);

    void instantiateFrame(NonWithFrame *pluralFrame, NonWithFrame *singularFrame);

    void markChildren();

    uint32 getSize()            { return frameList.size(); }

private:
    FrameList frameList;
};


class JS2Class : public NonWithFrame {
public:
    JS2Class(JS2Class *super, JS2Object *proto, Namespace *privateNamespace, bool dynamic, bool allowNull, bool final, const String *name);

    const String *getName()                     { return typeofString; }
        
    JS2Class    *super;                         // This class's immediate superclass or null if none
    InstanceBindingMap instanceBindings;        // Map of qualified names to instance members defined in this class    
    InstanceVariable **instanceInitOrder;       // List of instance variables defined in this class in the order in which they are initialised
    bool complete;                              // true after all members of this class have been added to this CLASS record
    JS2Object   *prototype;                     // An object that serves as this class's prototype for compatibility with ECMAScript 3; may be null
    const String *typeofString;
    Namespace *privateNamespace;                // This class's private namespace
    bool dynamic;                               // true if this class or any of its ancestors was defined with the dynamic attribute
    bool final;                                 // true if this class cannot be subclassed
    js2val  defaultValue;                       // An instance of this class assigned when a variable is not explicitly initialized

    Callor *call;                               // A procedure to call when this class is used in a call expression
    Constructor *construct;                     // A procedure to call when this class is used in a new expression
    js2val implicitCoerce(JS2Metadata *meta, js2val newValue);
                                                // A procedure to call when a value is assigned whose type is this class

    void emitDefaultValue(BytecodeContainer *bCon, size_t pos);


    Read *read;    
    Write *write;    


    bool isAncestor(JS2Class *heir);


    uint32 slotCount;


    virtual void instantiate(Environment * /* env */)  { }      // nothing to do
    virtual void markChildren();
    virtual ~JS2Class()            { }

};

class Package : public NonWithFrame {
public:
    Package(Namespace *internal) : NonWithFrame(PackageKind), super(JS2VAL_VOID), internalNamespace(internal) { }
    js2val super;
    Namespace *internalNamespace;               // This Package's internal namespace
    virtual void markChildren();
    virtual ~Package()            { }
};


// A SLOT record describes the value of one fixed property of one instance.
class Slot {
public:
// We keep the slotIndex in the InstanceVariable rather than go looking for a specific id
//    InstanceVariable *id;        // The instance variable whose value this slot carries
    js2val value;                // This fixed property's current value; uninitialised if the fixed property is an uninitialised constant
};

class ParameterFrame;

class FunctionWrapper {
public:
    FunctionWrapper(bool unchecked, ParameterFrame *compileFrame, Environment *env) 
        : bCon(new BytecodeContainer()), code(NULL), unchecked(unchecked), compileFrame(compileFrame), env(new Environment(env)) { }
    FunctionWrapper(bool unchecked, ParameterFrame *compileFrame, NativeCode *code, Environment *env) 
        : bCon(NULL), code(code), unchecked(unchecked), compileFrame(compileFrame), env(new Environment(env)) { }

    BytecodeContainer   *bCon;
    NativeCode          *code;
    bool                unchecked;      // true if the function is untyped, non-method, normal
    ParameterFrame      *compileFrame;
    Environment         *env;
};


// Instances which do not respond to the function call or new operators are represented as SIMPLEINSTANCE records
class SimpleInstance : public JS2Object {
public:
    SimpleInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type);

    LocalBindingMap     localBindings;
    js2val              super;
    bool sealed;
    JS2Class            *type;              // This instance's type
    Slot                *slots;             // A set of slots that hold this instance's fixed property values

    FunctionWrapper *fWrap;

    virtual void markChildren();
    virtual ~SimpleInstance()            { }

    virtual void writeProperty(JS2Metadata *meta, const String *name, js2val newValue, uint32 flags);

};

// Date instances are simple instances created by the Date class, they have an extra field 
// that contains the millisecond count
class DateInstance : public SimpleInstance {
public:
    DateInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type) { }

    float64     ms;
};

// String instances are simple instances created by the String class, they have an extra field 
// that contains the string data
class StringInstance : public SimpleInstance {
public:
    StringInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type), mValue(NULL) { }

    String     *mValue;             // has been allocated by engine in the GC'able Pond

    virtual void markChildren()     { SimpleInstance::markChildren(); if (mValue) JS2Object::mark(mValue); }
    virtual ~StringInstance()            { }
};

// Number instances are simple instances created by the Number class, they have an extra field 
// that contains the float64 data
class NumberInstance : public SimpleInstance {
public:
    NumberInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type), mValue(0.0) { }

    float64     mValue;
    virtual ~NumberInstance()            { }
};

// Boolean instances are simple instances created by the Boolean class, they have an extra field 
// that contains the bool data
class BooleanInstance : public SimpleInstance {
public:
    BooleanInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type), mValue(false) { }

    bool     mValue;
    virtual ~BooleanInstance()           { }
};

// Function instances are SimpleInstances created by the Function class, they have an extra field 
// that contains a pointer to the function implementation
class FunctionInstance : public SimpleInstance {
public:
    FunctionInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type);

    FunctionWrapper *fWrap;

    virtual void markChildren();
    virtual ~FunctionInstance()          { }
};

// Array instances are SimpleInstances created by the Array class, they 
// maintain the value of the 'length' property when 'indexable' elements
// are added.
class ArrayInstance : public SimpleInstance {
public:
    ArrayInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type) { setLength(meta, this, 0); }

    virtual void writeProperty(JS2Metadata *meta, const String *name, js2val newValue, uint32 flags);
    virtual ~ArrayInstance()             { }
};

// RegExp instances are simple instances created by the RegExp class, they have an extra field 
// that contains the RegExp object
class RegExpInstance : public SimpleInstance {
public:
    RegExpInstance(JS2Metadata *meta, JS2Object *parent, JS2Class *type) : SimpleInstance(meta, parent, type) { }

    void setLastIndex(JS2Metadata *meta, js2val a);
    void setGlobal(JS2Metadata *meta, js2val a);
    void setMultiline(JS2Metadata *meta, js2val a);
    void setIgnoreCase(JS2Metadata *meta, js2val a);
    void setSource(JS2Metadata *meta, js2val a);

    js2val getLastIndex(JS2Metadata *meta);
    js2val getGlobal(JS2Metadata *meta);
    js2val getMultiline(JS2Metadata *meta);
    js2val getIgnoreCase(JS2Metadata *meta);
    js2val getSource(JS2Metadata *meta);

    REState  *mRegExp;
    virtual ~RegExpInstance()             { }
};

// A base class for objects from another world
// to which read & write property calls are dispatched
class AlienInstance : public JS2Object {
public:
    AlienInstance(void *d) : JS2Object(AlienInstanceKind), uData(d) { }

    void *uData;

    virtual ~AlienInstance()    { }

    virtual bool readProperty(Multiname *m, js2val *rval);      // return true/false to signal whether the property is available
    virtual void writeProperty(Multiname *m, js2val rval);
};

// A helper class for 'for..in' statements
class ForIteratorObject : public JS2Object {
public:
    ForIteratorObject(JS2Object *obj) : JS2Object(ForIteratorKind), obj(obj), nameList(NULL) { }

    bool first();
    bool next(JS2Engine *engine);

    js2val value(JS2Engine *engine);

    JS2Object *obj;
    JS2Object *originalObj;

    virtual void markChildren()     { GCMARKOBJECT(obj); GCMARKOBJECT(originalObj); }
    virtual ~ForIteratorObject()            { }

private:

    bool buildNameList();

    const String **nameList;
    uint32 it;
    uint32 length;
};

// A METHODCLOSURE tuple describes an instance method with a bound this value.
class MethodClosure : public JS2Object {
public:
    MethodClosure(js2val thisObject, InstanceMethod *method) : JS2Object(MethodClosureKind), thisObject(thisObject), method(method) { }
    js2val              thisObject;     // The bound this value
    InstanceMethod      *method;        // The bound method

    virtual void markChildren();
};


// Base class for all references (lvalues)
// References are generated during the eval stage (bytecode generation), but shouldn't live beyond that
class Reference {
public:
    virtual void emitReadBytecode(BytecodeContainer *, size_t)              { ASSERT(false); }
    virtual void emitWriteBytecode(BytecodeContainer *, size_t)             { ASSERT(false); }
    virtual void emitReadForInvokeBytecode(BytecodeContainer *, size_t)     { ASSERT(false); }
    virtual void emitReadForWriteBackBytecode(BytecodeContainer *, size_t)  { ASSERT(false); }
    virtual void emitWriteBackBytecode(BytecodeContainer *, size_t)         { ASSERT(false); }

    virtual void emitPostIncBytecode(BytecodeContainer *, size_t)           { ASSERT(false); }
    virtual void emitPostDecBytecode(BytecodeContainer *, size_t)           { ASSERT(false); }
    virtual void emitPreIncBytecode(BytecodeContainer *, size_t)            { ASSERT(false); }
    virtual void emitPreDecBytecode(BytecodeContainer *, size_t)            { ASSERT(false); }

    virtual void emitDeleteBytecode(BytecodeContainer *, size_t)            { ASSERT(false); }   
    
    // indicate whether building the reference generate any stack deposits
    virtual int hasStackEffect()                                            { ASSERT(false); return 0; }
};

class LexicalReference : public Reference {
// A LEXICALREFERENCE tuple has the fields below and represents an lvalue that refers to a variable with one
// of a given set of qualified names. LEXICALREFERENCE tuples arise from evaluating identifiers a and qualified identifiers
// q::a.
public:
    LexicalReference(Multiname *mname, bool strict) : variableMultiname(*mname), env(NULL), strict(strict) { }
    LexicalReference(const String *name, bool strict) : variableMultiname(name), env(NULL), strict(strict) { }
    LexicalReference(const String *name, Namespace *nameSpace, bool strict) : variableMultiname(name, nameSpace), env(NULL), strict(strict) { }
    virtual ~LexicalReference() { }
    
    Multiname variableMultiname;   // A nonempty set of qualified names to which this reference can refer
    Environment *env;               // The environment in which the reference was created.
    bool strict;                    // The strict setting from the context in effect at the point where the reference was created
    
    void emitInitBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eLexicalInit, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    
    virtual void emitReadBytecode(BytecodeContainer *bCon, size_t pos)      { bCon->emitOp(eLexicalRead, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitWriteBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eLexicalWrite, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitReadForInvokeBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eLexicalRef, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitReadForWriteBackBytecode(BytecodeContainer *bCon, size_t pos)  { emitReadBytecode(bCon, pos); }
    virtual void emitWriteBackBytecode(BytecodeContainer *bCon, size_t pos)         { emitWriteBytecode(bCon, pos); }

    virtual void emitPostIncBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eLexicalPostInc, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitPostDecBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eLexicalPostDec, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitPreIncBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eLexicalPreInc, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    virtual void emitPreDecBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eLexicalPreDec, pos); bCon->addMultiname(new Multiname(variableMultiname)); }
    
    virtual void emitDeleteBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eLexicalDelete, pos); bCon->addMultiname(new Multiname(variableMultiname)); }

    virtual int hasStackEffect()                                           { return 0; }
};

class DotReference : public Reference {
// A DOTREFERENCE tuple has the fields below and represents an lvalue that refers to a property of the base
// object with one of a given set of qualified names. DOTREFERENCE tuples arise from evaluating subexpressions such as a.b or
// a.q::b.
public:
    DotReference(const String *name) : propertyMultiname(name) { }
    DotReference(Multiname *mn) : propertyMultiname(*mn) { }
    virtual ~DotReference() { }

    // In this implementation, the base is established by the execution of the preceding expression and
    // is available on the execution stack, not in the reference object (which is a codegen-time only)
    // js2val base;                 // The object whose property was referenced (a in the examples above). The
                                    // object may be a LIMITEDINSTANCE if a is a super expression, in which case
                                    // the property lookup will be restricted to members defined in proper ancestors
                                    // of base.limit.
    Multiname propertyMultiname;    // A nonempty set of qualified names to which this reference can refer (b
                                    // qualified with the namespace q or all currently open namespaces in the
                                    // example above)
    virtual void emitReadBytecode(BytecodeContainer *bCon, size_t pos)      { bCon->emitOp(eDotRead, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitWriteBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eDotWrite, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitReadForInvokeBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eDotRef, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitReadForWriteBackBytecode(BytecodeContainer *bCon, size_t pos)  { emitReadForInvokeBytecode(bCon, pos); }
    virtual void emitWriteBackBytecode(BytecodeContainer *bCon, size_t pos)         { emitWriteBytecode(bCon, pos); }

    virtual void emitPostIncBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eDotPostInc, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitPostDecBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eDotPostDec, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitPreIncBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eDotPreInc, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }
    virtual void emitPreDecBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eDotPreDec, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }

    virtual void emitDeleteBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eDotDelete, pos); bCon->addMultiname(new Multiname(propertyMultiname)); }

    virtual int hasStackEffect()                                            { return 1; }
};

class SlotReference : public Reference {
// A special case of a DotReference with an Sl instead of a D
public:
    SlotReference(uint32 slotIndex) : slotIndex(slotIndex) { }
    virtual ~SlotReference()	{ }

    virtual void emitReadBytecode(BytecodeContainer *bCon, size_t pos)      { bCon->emitOp(eSlotRead, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitWriteBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eSlotWrite, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitReadForInvokeBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eSlotRef, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitReadForWriteBackBytecode(BytecodeContainer *bCon, size_t pos)  { emitReadForInvokeBytecode(bCon, pos); }
    virtual void emitWriteBackBytecode(BytecodeContainer *bCon, size_t pos)         { emitWriteBytecode(bCon, pos); }

    virtual void emitPostIncBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eSlotPostInc, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitPostDecBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eSlotPostDec, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitPreIncBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eSlotPreInc, pos); bCon->addShort((uint16)slotIndex); }
    virtual void emitPreDecBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eSlotPreDec, pos); bCon->addShort((uint16)slotIndex); }

    virtual void emitDeleteBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eFalse, pos); /* bCon->emitOp(eSlotDelete, pos); bCon->addShort((uint16)slotIndex); */ }

    uint32 slotIndex;
    virtual int hasStackEffect()                                            { return 1; }
};


class NamedArgument {
public:
    const String *name;               // This argument's name
    js2val value;                   // This argument's value
};

// An ARGUMENTLIST describes the arguments (other than this) passed to a function.
class ArgumentList {
public:
    JS2Object *positional;          // Ordered list of positional arguments
    NamedArgument *named;           // Set of named arguments
};


class BracketReference : public Reference {
// A BRACKETREFERENCE tuple has the fields below and represents an lvalue that refers to the result of
// applying the [] operator to the base object with the given arguments. BRACKETREFERENCE tuples arise from evaluating
// subexpressions such as a[x] or a[x,y].
public:
    virtual void emitReadBytecode(BytecodeContainer *bCon, size_t pos)      { bCon->emitOp(eBracketRead, pos); }
    virtual void emitWriteBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eBracketWrite, pos); }
    virtual void emitReadForInvokeBytecode(BytecodeContainer *bCon, size_t pos)     { bCon->emitOp(eBracketRef, pos); }
    virtual void emitReadForWriteBackBytecode(BytecodeContainer *bCon, size_t pos)  { bCon->emitOp(eBracketReadForRef, pos); }
    virtual void emitWriteBackBytecode(BytecodeContainer *bCon, size_t pos)         { bCon->emitOp(eBracketWriteRef, pos); }
/*
    js2val base;                    // The object whose property was referenced (a in the examples above). The object may be a
                                    // LIMITEDINSTANCE if a is a super expression, in which case the property lookup will be
                                    // restricted to definitions of the [] operator defined in proper ancestors of base.limit.
    ArgumentList args;              // The list of arguments between the brackets (x or x,y in the examples above)
*/

    virtual void emitPostIncBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eBracketPostInc, pos); }
    virtual void emitPostDecBytecode(BytecodeContainer *bCon, size_t pos)   { bCon->emitOp(eBracketPostDec, pos); }
    virtual void emitPreIncBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eBracketPreInc, pos); }
    virtual void emitPreDecBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eBracketPreDec, pos); }

    virtual void emitDeleteBytecode(BytecodeContainer *bCon, size_t pos)    { bCon->emitOp(eBracketDelete, pos); }
    virtual int hasStackEffect()                                            { return 2; }
};


// Frames holding bindings for invoked functions
class ParameterFrame : public NonWithFrame {
public:
    ParameterFrame(js2val thisObject, bool prototype) : NonWithFrame(ParameterKind), thisObject(thisObject), prototype(prototype), positional(NULL), positionalCount(0) { }    
    ParameterFrame(ParameterFrame *pluralFrame) : NonWithFrame(ParameterKind, pluralFrame), thisObject(JS2VAL_UNDEFINED), prototype(pluralFrame->prototype), positional(NULL), positionalCount(0) { }

//    Plurality plurality;
    js2val thisObject;              // The value of this; none if this function doesn't define this;
                                    // inaccessible if this function defines this but the value is not 
                                    // available because this function hasn't been called yet.

    bool prototype;                 // true if this function is not an instance method but defines this anyway

    Variable **positional;          // list of positional parameters, in order
    uint32 positionalCount;

    virtual void instantiate(Environment *env);
    void assignArguments(JS2Metadata *meta, JS2Object *fnObj, js2val *argBase, uint32 argCount);
    virtual void markChildren();
    virtual ~ParameterFrame()           { }
};

class BlockFrame : public NonWithFrame {
public:
    BlockFrame() : NonWithFrame(BlockFrameKind) { }
    BlockFrame(BlockFrame *pluralFrame) : NonWithFrame(BlockFrameKind, pluralFrame) { }

    Plurality plurality;

    virtual void instantiate(Environment *env);
    virtual ~BlockFrame()           { }
};


class LookupKind {
public:
    LookupKind(bool isLexical, js2val thisObject) : isLexical(isLexical), thisObject(thisObject) { }
    
    bool isPropertyLookup() { return !isLexical; }

    bool isLexical;         // if isLexical, use the 'this' below. Otherwise it's a propertyLookup
    js2val thisObject;
};


typedef std::vector<Namespace *> NamespaceList;
typedef NamespaceList::iterator NamespaceListIterator;

// A CONTEXT carries static information about a particular point in the source program.
class Context {
public:
    Context() : strict(false) { }
    Context(Context *cxt);
    bool strict;                    // true if strict mode is in effect
    NamespaceList openNamespaces;   // The set of namespaces that are open at this point. 
                                    // The public namespace is always a member of this set.
};

// The 'true' attribute
class TrueAttribute : public Attribute {
public:
    TrueAttribute() : Attribute(TrueAttr) { }
    virtual CompoundAttribute *toCompoundAttribute();
};

// The 'false' attribute
class FalseAttribute : public Attribute {
public:
    FalseAttribute() : Attribute(FalseAttr) { }
};

// Compound attribute objects are all values obtained from combining zero or more syntactic attributes 
// that are not Booleans or single namespaces. 
class CompoundAttribute : public Attribute {
public:
    CompoundAttribute();
    void addNamespace(Namespace *n);

    virtual CompoundAttribute *toCompoundAttribute()    { return this; }

    NamespaceList *namespaces;      // The set of namespaces contained in this attribute
    bool xplicit;                   // true if the explicit attribute has been given
    bool dynamic;                   // true if the dynamic attribute has been given
    MemberModifier memberMod;       // if one of these attributes has been given; none if not.
    OverrideModifier overrideMod;   // if the override attribute  with one of these arguments was given; 
                                    // true if the attribute override without arguments was given; none if the override attribute was not given.
    bool prototype;                 // true if the prototype attribute has been given
    bool unused;                    // true if the unused attribute has been given

    virtual void markChildren();
};


typedef std::vector<StmtNode *> TargetList;
typedef std::vector<StmtNode *>::iterator TargetListIterator;
typedef std::vector<StmtNode *>::reverse_iterator TargetListReverseIterator;

struct MemberDescriptor {
    LocalMember *localMember;
    Namespace *ns;
};

class CompilationData {
public:
    BytecodeContainer *compilation_bCon;
    BytecodeContainer *execution_bCon;
};

class JS2Metadata : public JS2Object {
public:
    
    JS2Metadata(World &world);
    virtual ~JS2Metadata()	{ }

    CompilationData *startCompilationUnit(BytecodeContainer *newBCon, const String &source, const String &sourceLocation);
    void restoreCompilationUnit(CompilationData *oldData);


    void ValidateStmtList(StmtNode *p);
    js2val EvalStmtList(Phase phase, StmtNode *p);

    js2val readEvalString(const String &str, const String& fileName);
    js2val readEvalFile(const String& fileName);
    js2val readEvalFile(const char *fileName);

// XXX - passing (Context *cxt, Environment *env) throughout - but do these really change?

    void ValidateStmtList(Context *cxt, Environment *env, Plurality pl, StmtNode *p);
    void ValidateTypeExpression(Context *cxt, Environment *env, ExprNode *e)    { ValidateExpression(cxt, env, e); } 
    void ValidateStmt(Context *cxt, Environment *env, Plurality pl, StmtNode *p);
    void ValidateExpression(Context *cxt, Environment *env, ExprNode *p);
    void ValidateAttributeExpression(Context *cxt, Environment *env, ExprNode *p);
    JS2Object *validateStaticFunction(FunctionDefinition *fnDef, js2val compileThis, bool prototype, bool unchecked, Context *cxt, Environment *env);

    js2val ExecuteStmtList(Phase phase, StmtNode *p);
    js2val EvalExpression(Environment *env, Phase phase, ExprNode *p);
    JS2Class *EvalTypeExpression(Environment *env, Phase phase, ExprNode *p);
    Reference *SetupExprNode(Environment *env, Phase phase, ExprNode *p, JS2Class **exprType);
    Attribute *EvalAttributeExpression(Environment *env, Phase phase, ExprNode *p);
    void SetupStmt(Environment *env, Phase phase, StmtNode *p);


    JS2Class *objectType(js2val obj);
    bool hasType(js2val objVal, JS2Class *c);
    bool relaxedHasType(js2val objVal, JS2Class *c);

    LocalMember *findFlatMember(NonWithFrame *container, Multiname *multiname, Access access, Phase phase);
    InstanceBinding *resolveInstanceMemberName(JS2Class *js2class, Multiname *multiname, Access access, Phase phase, QualifiedName *qname);

    DynamicVariable *defineHoistedVar(Environment *env, const String *id, StmtNode *p, bool isVar);
    Multiname *defineLocalMember(Environment *env, const String *id, NamespaceList *namespaces, Attribute::OverrideModifier overrideMod, bool xplicit, Access access, LocalMember *m, size_t pos);
    OverrideStatusPair *defineInstanceMember(JS2Class *c, Context *cxt, const String *id, NamespaceList *namespaces, Attribute::OverrideModifier overrideMod, bool xplicit, Access access, InstanceMember *m, size_t pos);
    OverrideStatus *resolveOverrides(JS2Class *c, Context *cxt, const String *id, NamespaceList *namespaces, Access access, bool expectMethod, size_t pos);
    OverrideStatus *searchForOverrides(JS2Class *c, const String *id, NamespaceList *namespaces, Access access, size_t pos);
    InstanceMember *findInstanceMember(JS2Class *c, QualifiedName *qname, Access access);
    Slot *findSlot(js2val thisObjVal, InstanceVariable *id);
    bool findLocalMember(JS2Class *c, Multiname *multiname, Access access, Phase phase, MemberDescriptor *result);
    JS2Class *getVariableType(Variable *v, Phase phase, size_t pos);

    js2val invokeFunction(const char *fname);
    bool invokeFunctionOnObject(js2val thisValue, const String *fnName, js2val &result);
    js2val invokeFunction(JS2Object *fnObj, js2val thisValue, js2val *argv, uint32 argc);

    bool readProperty(js2val *container, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval);
    bool readProperty(Frame *pf, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval);
    bool readDynamicProperty(JS2Object *container, const String *name, LookupKind *lookupKind, Phase phase, js2val *rval);
    bool readLocalMember(LocalMember *m, Phase phase, js2val *rval);
    bool readInstanceMember(js2val containerVal, JS2Class *c, QualifiedName *qname, Phase phase, js2val *rval);
    JS2Object *lookupDynamicProperty(JS2Object *obj, const String *name);
    bool JS2Metadata::hasOwnProperty(JS2Object *obj, const String *name);

    bool writeProperty(js2val container, Multiname *multiname, LookupKind *lookupKind, bool createIfMissing, js2val newValue, Phase phase);
    bool writeProperty(Frame *container, Multiname *multiname, LookupKind *lookupKind, bool createIfMissing, js2val newValue, Phase phase, bool initFlag);
    bool writeDynamicProperty(JS2Object *container, const String *name, bool createIfMissing, js2val newValue, Phase phase);
    bool writeLocalMember(LocalMember *m, js2val newValue, Phase phase, bool initFlag);
    bool writeInstanceMember(js2val containerVal, JS2Class *c, QualifiedName *qname, js2val newValue, Phase phase);

    bool deleteProperty(Frame *container, Multiname *multiname, LookupKind *lookupKind, Phase phase, bool *result);
    bool deleteProperty(js2val container, Multiname *multiname, LookupKind *lookupKind, Phase phase, bool *result);
    bool deleteDynamicProperty(JS2Object *container, const String *name, LookupKind *lookupKind, bool *result);
    bool deleteLocalMember(LocalMember *m, bool *result);
    bool deleteInstanceMember(JS2Class *c, QualifiedName *qname, bool *result);

    void addGlobalObjectFunction(char *name, NativeCode *code, uint32 length);
    void initBuiltinClass(JS2Class *builtinClass, FunctionData *protoFunctions, FunctionData *staticFunctions, NativeCode *construct, NativeCode *call);

    void reportError(Exception::Kind kind, const char *message, size_t pos, const char *arg = NULL);
    void reportError(Exception::Kind kind, const char *message, size_t pos, const String &name);
    void reportError(Exception::Kind kind, const char *message, size_t pos, const String *name);

    const String *convertValueToString(js2val x);
    js2val convertValueToPrimitive(js2val x, Hint hint);
    float64 convertValueToDouble(js2val x);
    float64 convertStringToDouble(const String *str);
    bool convertValueToBoolean(js2val x);
    int32 convertValueToInteger(js2val x);
    js2val convertValueToGeneralNumber(js2val x);
    js2val convertValueToObject(js2val x);

    const String *toString(js2val x)    { if (JS2VAL_IS_STRING(x)) return JS2VAL_TO_STRING(x); else return convertValueToString(x); }
    js2val toPrimitive(js2val x, Hint hint)        { if (JS2VAL_IS_PRIMITIVE(x)) return x; else return convertValueToPrimitive(x, hint); }
    float64 toFloat64(js2val x);
    js2val toGeneralNumber(js2val x)    { if (JS2VAL_IS_NUMBER(x)) return x; else return convertValueToGeneralNumber(x); }
    bool toBoolean(js2val x)            { if (JS2VAL_IS_BOOLEAN(x)) return JS2VAL_TO_BOOLEAN(x); else return convertValueToBoolean(x); }
    int32 toInteger(js2val x)           { if (JS2VAL_IS_INT(x)) return JS2VAL_TO_INT(x); else return convertValueToInteger(x); }
    js2val toObject(js2val x)           { if (JS2VAL_IS_OBJECT(x)) return x; else return convertValueToObject(x); }

    // Used for interning strings
    World &world;

    // The execution engine
    JS2Engine *engine;

    // Random number generator state
    bool      rngInitialized;
    int64     rngMultiplier;
    int64     rngAddend;
    int64     rngMask;
    int64     rngSeed;
    float64   rngDscale;

    
    // The one and only 'public' namespace
    Namespace *publicNamespace;

    Multiname *mn1, *mn2;           // useful, gc-rooted multiname temps.

    LocalMember *forbiddenMember;  // just need one of these hanging around

    // The base classes:
    JS2Class *objectClass;
    JS2Class *undefinedClass;
    JS2Class *nullClass;
    JS2Class *booleanClass;
    JS2Class *generalNumberClass;
    JS2Class *numberClass;
    JS2Class *characterClass;
    JS2Class *stringClass;
    JS2Class *namespaceClass;
    JS2Class *attributeClass;
    JS2Class *classClass;
    JS2Class *functionClass;
    JS2Class *packageClass;
    JS2Class *dateClass;
    JS2Class *regexpClass;
    JS2Class *mathClass;
    JS2Class *arrayClass;
    JS2Class *errorClass;
    JS2Class *evalErrorClass;
    JS2Class *rangeErrorClass;
    JS2Class *referenceErrorClass;
    JS2Class *syntaxErrorClass;
    JS2Class *typeErrorClass;
    JS2Class *uriErrorClass;

    BytecodeContainer *bCon;        // the current output container

    typedef std::vector<BytecodeContainer *> BConList;
    typedef std::vector<BytecodeContainer *>::iterator BConListIterator;
    BConList bConList;

    Package *glob;
    Environment *env;
    Context cxt;

    enum Flag { JS1, JS2 };
    Flag flags;

    TargetList targetList;          // stack of potential break/continue targets

    virtual void markChildren();

    bool showTrees;                 // debug only, causes parse tree dump 

};

    inline char narrow(char16 ch) { return char(ch); }

}; // namespace MetaData

inline bool operator==(MetaData::LocalBindingEntry *s1, const String &s2) { return s1->name == s2;}
inline bool operator!=(MetaData::LocalBindingEntry *s1, const String &s2) { return s1->name != s2;}

inline bool operator==(MetaData::InstanceBindingEntry *s1, const String &s2) { return s1->name == s2;}
inline bool operator!=(MetaData::InstanceBindingEntry *s1, const String &s2) { return s1->name != s2;}


}; // namespace Javascript

#endif
