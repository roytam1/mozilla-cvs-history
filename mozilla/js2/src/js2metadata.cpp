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


#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#pragma warning(disable: 4711)
#pragma warning(disable: 4710)
#endif


#include <algorithm>
#include <assert.h>

#include "world.h"
#include "utilities.h"
#include "js2value.h"

#include <map>
#include <algorithm>

#include "reader.h"
#include "parser.h"
#include "js2engine.h"
#include "bytecodecontainer.h"
#include "js2metadata.h"


namespace JavaScript {
namespace MetaData {

/************************************************************************************
 *
 *  Statements and statement lists
 *
 ************************************************************************************/


    /*
     * Validate the linked list of statement nodes beginning at 'p'
     */
    void JS2Metadata::ValidateStmtList(StmtNode *p) {
        while (p) {
            ValidateStmt(&cxt, &env, p);
            p = p->next;
        }
    }

    void JS2Metadata::ValidateStmtList(Context *cxt, Environment *env, StmtNode *p) {
        while (p) {
            ValidateStmt(cxt, env, p);
            p = p->next;
        }
    }

        

    /*
     * Validate an individual statement 'p', including it's children
     */
    void JS2Metadata::ValidateStmt(Context *cxt, Environment *env, StmtNode *p) {
        switch (p->getKind()) {
        case StmtNode::block:
        case StmtNode::group:
            {
                BlockStmtNode *b = checked_cast<BlockStmtNode *>(p);
                b->compileFrame = new BlockFrame();
                env->addFrame(b->compileFrame);
                ValidateStmtList(cxt, env, b->statements);
                env->removeTopFrame();
            }
            break;
        case StmtNode::label:
            {
                LabelStmtNode *l = checked_cast<LabelStmtNode *>(p);
/*
    A labelled statement catches contained, named, 'breaks' but simply adds itself as a label for
    contained iteration statements. (i.e. you can 'break' out of a labelled statement, but not 'continue'
    one, however the statement label becomes a 'continuable' label for all contained iteration statements.
*/
                // Make sure there is no existing break target with the same name
                for (TargetListIterator si = targetList.begin(), end = targetList.end(); (si != end); si++) {
                    switch ((*si)->getKind()) {
                    case StmtNode::label:
                        if (checked_cast<LabelStmtNode *>(*si)->name == l->name)
                            reportError(Exception::syntaxError, "Duplicate statement label", p->pos);
                        break;
                    }
                }
                targetList.push_back(p);
                ValidateStmt(cxt, env, l->stmt);
                targetList.pop_back();
            }
            break;
        case StmtNode::If:
            {
                UnaryStmtNode *i = checked_cast<UnaryStmtNode *>(p);
                ValidateExpression(cxt, env, i->expr);
                ValidateStmt(cxt, env, i->stmt);
            }
            break;
        case StmtNode::IfElse:
            {
                BinaryStmtNode *i = checked_cast<BinaryStmtNode *>(p);
                ValidateExpression(cxt, env, i->expr);
                ValidateStmt(cxt, env, i->stmt);
                ValidateStmt(cxt, env, i->stmt2);
            }
            break;
        case StmtNode::For:
            {
                ForStmtNode *f = checked_cast<ForStmtNode *>(p);
                if (f->initializer)
                    ValidateStmt(cxt, env, f->initializer);
                if (f->expr2)
                    ValidateExpression(cxt, env, f->expr2);
                if (f->expr3)
                    ValidateExpression(cxt, env, f->expr3);
                targetList.push_back(p);
                ValidateStmt(cxt, env, f->stmt);
                targetList.pop_back();
            }
            break;
        case StmtNode::While:
        case StmtNode::DoWhile:
            {
                UnaryStmtNode *w = checked_cast<UnaryStmtNode *>(p);
                targetList.push_back(p);
                ValidateExpression(cxt, env, w->expr);
                ValidateStmt(cxt, env, w->stmt);
                targetList.pop_back();
            }
            break;
        case StmtNode::Break:
            {
                GoStmtNode *g = checked_cast<GoStmtNode *>(p);
                bool found = false;
                for (TargetListReverseIterator si = targetList.rbegin(), end = targetList.rend(); (si != end); si++) {
                    if (g->name) {
                        // Make sure the name is on the targetList as a viable break target...
                        // (only label statements can introduce names)
                        if ((*si)->getKind() == StmtNode::label) {
                            LabelStmtNode *l = checked_cast<LabelStmtNode *>(*si);
                            if (l->name == *g->name) {
                                g->tgtID = &l->labelID;
                                found = true;
                                break;
                            }
                        }
                    }
                    else {
                        // anything at all will do
                        switch ((*si)->getKind()) {
                        case StmtNode::label:
                            {
                                LabelStmtNode *l = checked_cast<LabelStmtNode *>(*si);
                                g->tgtID = &l->labelID;
                            }
                            break;
                        case StmtNode::While:
                        case StmtNode::DoWhile:
                            {
                                UnaryStmtNode *w = checked_cast<UnaryStmtNode *>(*si);
                                g->tgtID = &w->breakLabelID;
                            }
                            break;
                        case StmtNode::For:
                        case StmtNode::ForIn:
                            {
                                ForStmtNode *f = checked_cast<ForStmtNode *>(p);
                                g->tgtID = &f->breakLabelID;
                            }
                            break;
                       case StmtNode::Switch:
                            {
                                SwitchStmtNode *s = checked_cast<SwitchStmtNode *>(p);
                                g->tgtID = &s->breakLabelID;
                            }
                            break;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) 
                    reportError(Exception::syntaxError, "No such break target available", p->pos);
            }
            break;
        case StmtNode::Continue:
            {
                GoStmtNode *g = checked_cast<GoStmtNode *>(p);
                bool found = false;
                for (TargetListIterator si = targetList.begin(), end = targetList.end(); (si != end); si++) {
                    if (g->name) {
                        // Make sure the name is on the targetList as a viable continue target...
                        if ((*si)->getKind() == StmtNode::label) {
                            LabelStmtNode *l = checked_cast<LabelStmtNode *>(*si);
                            if (l->name == *g->name) {
                                g->tgtID = &l->labelID;
                                found = true;
                                break;
                            }
                        }
                    }
                    else {
                        // only some non-label statements will do
                        switch ((*si)->getKind()) {
                        case StmtNode::While:
                        case StmtNode::DoWhile:
                            {
                                UnaryStmtNode *w = checked_cast<UnaryStmtNode *>(*si);
                                g->tgtID = &w->breakLabelID;
                            }
                            break;
                        case StmtNode::For:
                        case StmtNode::ForIn:
                            {
                                ForStmtNode *f = checked_cast<ForStmtNode *>(p);
                                g->tgtID = &f->breakLabelID;
                            }
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) 
                    reportError(Exception::syntaxError, "No such break target available", p->pos);
            }
            break;
        case StmtNode::Return:
            {
                ExprStmtNode *e = checked_cast<ExprStmtNode *>(p);
                if (e->expr) {
                    ValidateExpression(cxt, env, e->expr);
                }
            }
            break;
        case StmtNode::Function:
            {
                Attribute *attr = NULL;
                FunctionStmtNode *f = checked_cast<FunctionStmtNode *>(p);
                if (f->attributes) {
                    ValidateAttributeExpression(cxt, env, f->attributes);
                    attr = EvalAttributeExpression(env, CompilePhase, f->attributes);
                }
                CompoundAttribute *a = Attribute::toCompoundAttribute(attr);
                if (a->dynamic)
                    reportError(Exception::definitionError, "Illegal attribute", p->pos);
                VariableBinding *vb = f->function.parameters;
                bool untyped = true;
                while (vb) {
                    if (vb->type) {
                        untyped = false;
                        break;
                    }
                    vb = vb->next;
                }
                bool unchecked = !cxt->strict && (env->getTopFrame()->kind != ClassKind)
                                    && (f->function.prefix == FunctionName::normal) && untyped;
                bool prototype = unchecked || a->prototype;
                Attribute::MemberModifier memberMod = a->memberMod;
                if (env->getTopFrame()->kind == ClassKind) {
                    if (memberMod == Attribute::NoModifier)
                        memberMod = Attribute::Virtual;
                }
                else {
                    if (memberMod != Attribute::NoModifier)
                        reportError(Exception::definitionError, "Illegal attribute", p->pos);
                }
                if (prototype && ((f->function.prefix != FunctionName::normal) || (memberMod == Attribute::Constructor))) {
                    reportError(Exception::definitionError, "Illegal attribute", p->pos);
                }
                js2val compileThis = JS2VAL_VOID;
                if (prototype || (memberMod == Attribute::Constructor) 
                              || (memberMod == Attribute::Virtual) 
                              || (memberMod == Attribute::Final))
                    compileThis = JS2VAL_INACCESSIBLE;
                ParameterFrame *compileFrame = new ParameterFrame(compileThis, prototype);
                Frame *topFrame = env->getTopFrame();
                env->addFrame(compileFrame);
                VariableBinding *pb = f->function.parameters;
                while (pb) {
                    // XXX define a static binding for each parameter
                    pb = pb->next;
                }
                ValidateStmt(cxt, env, f->function.body);
                env->removeTopFrame();
                if (unchecked 
                        && ((topFrame->kind == GlobalObjectKind)
                                        || (topFrame->kind == ParameterKind))
                        && (f->attributes == NULL)) {
                    defineHoistedVar(env, *f->function.name, p);
                }
                else {
                    switch (memberMod) {
                    case Attribute::NoModifier:
                    case Attribute::Static:
                        {
                            FixedInstance *fInst = new FixedInstance(functionClass);
                            fInst->fWrap = new FunctionWrapper(unchecked, compileFrame);
                            f->fWrap = fInst->fWrap;
                            Variable *v = new Variable(functionClass, OBJECT_TO_JS2VAL(fInst), true);
                            defineStaticMember(env, *f->function.name, a->namespaces, a->overrideMod, a->xplicit, ReadWriteAccess, v, p->pos);
                        }
                        break;
                    }
                }
            }
            break;
        case StmtNode::Var:
        case StmtNode::Const:
            {
                bool immutable = (p->getKind() == StmtNode::Const);
                Attribute *attr = NULL;
                VariableStmtNode *vs = checked_cast<VariableStmtNode *>(p);

                if (vs->attributes) {
                    ValidateAttributeExpression(cxt, env, vs->attributes);
                    attr = EvalAttributeExpression(env, CompilePhase, vs->attributes);
                }
                
                VariableBinding *vb = vs->bindings;
                Frame *regionalFrame = env->getRegionalFrame();
                while (vb)  {
                    const StringAtom *name = vb->name;
                    if (vb->type)
                        ValidateTypeExpression(cxt, env, vb->type);
                    vb->member = NULL;

                    if (cxt->strict && ((regionalFrame->kind == GlobalObjectKind)
                                        || (regionalFrame->kind == ParameterKind))
                                    && !immutable
                                    && (vs->attributes == NULL)
                                    && (vb->type == NULL)) {
                        defineHoistedVar(env, *name, p);
                    }
                    else {
                        CompoundAttribute *a = Attribute::toCompoundAttribute(attr);
                        if (a->dynamic || a->prototype)
                            reportError(Exception::definitionError, "Illegal attribute", p->pos);
                        Attribute::MemberModifier memberMod = a->memberMod;
                        if ((env->getTopFrame()->kind == ClassKind)
                                && (memberMod == Attribute::NoModifier))
                            memberMod = Attribute::Final;
                        switch (memberMod) {
                        case Attribute::NoModifier:
                        case Attribute::Static: 
                            {
                                // Set type to FUTURE_TYPE - it will be resolved during 'PreEval'. The value is either FUTURE_VALUE
                                // for 'const' - in which case the expression is compile time evaluated (or attempted) or set
                                // to INACCESSIBLE until run time initialization occurs.
                                Variable *v = new Variable(FUTURE_TYPE, immutable ? JS2VAL_FUTUREVALUE : JS2VAL_INACCESSIBLE, immutable);
                                vb->member = v;
                                v->vb = vb;
                                vb->mn = defineStaticMember(env, *name, a->namespaces, a->overrideMod, a->xplicit, ReadWriteAccess, v, p->pos);
                            }
                            break;
                        case Attribute::Virtual:
                        case Attribute::Final: 
                            {
                                JS2Class *c = checked_cast<JS2Class *>(env->getTopFrame());
                                InstanceMember *m = new InstanceVariable(FUTURE_TYPE, immutable, (memberMod == Attribute::Final), c->slotCount++);
                                vb->member = m;
                                vb->osp = defineInstanceMember(c, cxt, *name, a->namespaces, a->overrideMod, a->xplicit, ReadWriteAccess, m, p->pos);
                            }
                            break;
                        default:
                            reportError(Exception::definitionError, "Illegal attribute", p->pos);
                            break;
                        }
                    }
                    vb = vb->next;
                }
            }
            break;
        case StmtNode::expression:
            {
                ExprStmtNode *e = checked_cast<ExprStmtNode *>(p);
                ValidateExpression(cxt, env, e->expr);
            }
            break;
        case StmtNode::Namespace:
            {
                NamespaceStmtNode *ns = checked_cast<NamespaceStmtNode *>(p);
                Attribute *attr = NULL;
                if (ns->attributes) {
                    ValidateAttributeExpression(cxt, env, ns->attributes);
                    attr = EvalAttributeExpression(env, CompilePhase, ns->attributes);
                }
                CompoundAttribute *a = Attribute::toCompoundAttribute(attr);
                if (a->dynamic || a->prototype)
                    reportError(Exception::definitionError, "Illegal attribute", p->pos);
                if ( ! ((a->memberMod == Attribute::NoModifier) || ((a->memberMod == Attribute::Static) && (env->getTopFrame()->kind == ClassKind))) )
                    reportError(Exception::definitionError, "Illegal attribute", p->pos);
                Variable *v = new Variable(namespaceClass, OBJECT_TO_JS2VAL(new Namespace(ns->name)), true);
                defineStaticMember(env, ns->name, a->namespaces, a->overrideMod, a->xplicit, ReadWriteAccess, v, p->pos);
            }
            break;
        case StmtNode::Use:
            {
                UseStmtNode *u = checked_cast<UseStmtNode *>(p);
                ExprList *eList = u->namespaces;
                while (eList) {
                    js2val av = EvalExpression(env, CompilePhase, eList->expr);
                    if (JS2VAL_IS_NULL(av) || !JS2VAL_IS_OBJECT(av))
                        reportError(Exception::badValueError, "Namespace expected in use directive", p->pos);
                    JS2Object *obj = JS2VAL_TO_OBJECT(av);
                    if ((obj->kind != AttributeObjectKind) || (checked_cast<Attribute *>(obj)->attrKind != Attribute::NamespaceAttr))
                        reportError(Exception::badValueError, "Namespace expected in use directive", p->pos);
                    cxt->openNamespaces.push_back(checked_cast<Namespace *>(obj));                    
                    eList = eList->next;
                }
            }
            break;
        case StmtNode::Class:
            {
                ClassStmtNode *classStmt = checked_cast<ClassStmtNode *>(p);
                JS2Class *superClass = objectClass;
                if (classStmt->superclass) {
                    ValidateExpression(cxt, env, classStmt->superclass);                    
                    js2val av = EvalExpression(env, CompilePhase, classStmt->superclass);
                    if (JS2VAL_IS_NULL(av) || !JS2VAL_IS_OBJECT(av))
                        reportError(Exception::badValueError, "Class expected in inheritance", p->pos);
                    JS2Object *obj = JS2VAL_TO_OBJECT(av);
                    if (obj->kind != ClassKind)
                        reportError(Exception::badValueError, "Class expected in inheritance", p->pos);
                    superClass = checked_cast<JS2Class *>(obj);
                }
                Attribute *attr = NULL;
                if (classStmt->attributes) {
                    ValidateAttributeExpression(cxt, env, classStmt->attributes);
                    attr = EvalAttributeExpression(env, CompilePhase, classStmt->attributes);
                }
                CompoundAttribute *a = Attribute::toCompoundAttribute(attr);
                if (!superClass->complete || superClass->final)
                    reportError(Exception::definitionError, "Illegal inheritance", p->pos);
                JS2Object *proto = NULL;
                bool final;
                switch (a->memberMod) {
                case Attribute::NoModifier: 
                    final = false; 
                    break;
                case Attribute::Static: 
                    if (env->getTopFrame()->kind != ClassKind)
                        reportError(Exception::definitionError, "Illegal use of static modifier", p->pos);
                    final = false;
                    break;
                case Attribute::Final:
                    final = true;
                    break;
                default:
                    reportError(Exception::definitionError, "Illegal modifier for class definition", p->pos);
                    break;
                }
                JS2Class *c = new JS2Class(superClass, proto, new Namespace(engine->private_StringAtom), (a->dynamic || superClass->dynamic), true, final, classStmt->name);
                classStmt->c = c;
                Variable *v = new Variable(classClass, OBJECT_TO_JS2VAL(c), true);
                defineStaticMember(env, classStmt->name, a->namespaces, a->overrideMod, a->xplicit, ReadWriteAccess, v, p->pos);
                if (classStmt->body) {
                    env->addFrame(c);
                    ValidateStmtList(cxt, env, classStmt->body->statements);
                    ASSERT(env->getTopFrame() == c);
                    env->removeTopFrame();
                }
                c->complete = true;
            }
        }   // switch (p->getKind())
    }


    /*
     * Evaluate the linked list of statement nodes beginning at 'p' 
     * (generate bytecode and then execute that bytecode
     */
    js2val JS2Metadata::ExecuteStmtList(Phase phase, StmtNode *p)
    {
        BytecodeContainer *saveBacon = bCon;
        bCon = new BytecodeContainer();
        size_t lastPos = p->pos;
        while (p) {
            EvalStmt(&env, phase, p);
            lastPos = p->pos;
            p = p->next;
        }
        bCon->emitOp(eReturnVoid, lastPos);
        js2val retval = engine->interpret(this, phase, bCon);
        bCon = saveBacon;
        return retval;
    }

    JS2Class *JS2Metadata::getVariableType(Variable *v, Phase phase, size_t pos)
    {
        JS2Class *type = v->type;
        if (type == NULL) { // Inaccessible, Note that this can only happen when phase = compile 
                            // because the compilation phase ensures that all types are valid, 
                            // so invalid types will not occur during the run phase.          
            ASSERT(phase == CompilePhase);
            reportError(Exception::compileExpressionError, "No type assigned", pos);
        }
        else {
            if (v->type == FUTURE_TYPE) {
                // Note that phase = compile because all futures are resolved by the end of the compilation phase.
                ASSERT(phase == CompilePhase);
                if (v->vb->type) {
                    v->type = NULL;
                    v->type = EvalTypeExpression(&env, CompilePhase, v->vb->type);
                }
                else
                    v->type = objectClass;
            }
        }
        return v->type;
    }

    /*
     * Evaluate an individual statement 'p', including it's children
     *  - this generates bytecode for each statement, but doesn't actually
     * execute it.
     */
    void JS2Metadata::EvalStmt(Environment *env, Phase phase, StmtNode *p) 
    {
        switch (p->getKind()) {
        case StmtNode::block:
        case StmtNode::group:
            {
                BlockStmtNode *b = checked_cast<BlockStmtNode *>(p);
                BlockFrame *runtimeFrame = new BlockFrame(b->compileFrame);
                env->addFrame(runtimeFrame);
                bCon->emitOp(ePushFrame, p->pos);
                bCon->addFrame(runtimeFrame);
                StmtNode *bp = b->statements;
                while (bp) {
                    EvalStmt(env, phase, bp);
                    bp = bp->next;
                }
                bCon->emitOp(ePopFrame, p->pos);
                env->removeTopFrame();
            }
            break;
        case StmtNode::label:
            {
                LabelStmtNode *l = checked_cast<LabelStmtNode *>(p);
                EvalStmt(env, phase, l->stmt);
            }
            break;
        case StmtNode::If:
            {
                BytecodeContainer::LabelID skipOverStmt = bCon->getLabel();
                UnaryStmtNode *i = checked_cast<UnaryStmtNode *>(p);
                Reference *r = EvalExprNode(env, phase, i->expr);
                if (r) r->emitReadBytecode(bCon, p->pos);
                bCon->emitBranch(eBranchFalse, skipOverStmt, p->pos);
                EvalStmt(env, phase, i->stmt);
                bCon->setLabel(skipOverStmt);
            }
            break;
        case StmtNode::IfElse:
            {
                BytecodeContainer::LabelID falseStmt = bCon->getLabel();
                BytecodeContainer::LabelID skipOverFalseStmt = bCon->getLabel();
                BinaryStmtNode *i = checked_cast<BinaryStmtNode *>(p);
                Reference *r = EvalExprNode(env, phase, i->expr);
                if (r) r->emitReadBytecode(bCon, p->pos);
                bCon->emitBranch(eBranchFalse, falseStmt, p->pos);
                EvalStmt(env, phase, i->stmt);
                bCon->emitBranch(eBranch, skipOverFalseStmt, p->pos);
                bCon->setLabel(falseStmt);
                EvalStmt(env, phase, i->stmt2);
                bCon->setLabel(skipOverFalseStmt);
            }
            break;
        case StmtNode::Break:
        case StmtNode::Continue:
            {
                GoStmtNode *g = checked_cast<GoStmtNode *>(p);
                bCon->emitBranch(eBranch, *g->tgtID, p->pos);
            }
            break;
        case StmtNode::For:
            {
                ForStmtNode *f = checked_cast<ForStmtNode *>(p);
                f->breakLabelID = bCon->getLabel();
                f->continueLabelID = bCon->getLabel();
                BytecodeContainer::LabelID loopTop = bCon->getLabel();
                BytecodeContainer::LabelID testLocation = bCon->getLabel();

                if (f->initializer)
                    EvalStmt(env, phase, f->initializer);
                if (f->expr2)
                    bCon->emitBranch(eBranch, testLocation, p->pos);
                bCon->setLabel(loopTop);
                targetList.push_back(p);
                EvalStmt(env, phase, f->stmt);
                targetList.pop_back();
                bCon->setLabel(f->continueLabelID);
                if (f->expr3) {
                    Reference *r = EvalExprNode(env, phase, f->expr3);
                    if (r) r->emitReadBytecode(bCon, p->pos);
                }
                bCon->setLabel(testLocation);
                if (f->expr2) {
                    Reference *r = EvalExprNode(env, phase, f->expr2);
                    if (r) r->emitReadBytecode(bCon, p->pos);
                    bCon->emitBranch(eBranchTrue, loopTop, p->pos);
                }
                else
                    bCon->emitBranch(eBranch, loopTop, p->pos);
                bCon->setLabel(f->breakLabelID);
            }
            break;
        case StmtNode::While:
            {
                UnaryStmtNode *w = checked_cast<UnaryStmtNode *>(p);
                w->breakLabelID = bCon->getLabel();
                w->continueLabelID = bCon->getLabel();
                BytecodeContainer::LabelID loopTop = bCon->getLabel();
                bCon->emitBranch(eBranch, w->continueLabelID, p->pos);
                bCon->setLabel(loopTop);
                targetList.push_back(p);
                EvalStmt(env, phase, w->stmt);
                targetList.pop_back();
                bCon->setLabel(w->continueLabelID);
                Reference *r = EvalExprNode(env, phase, w->expr);
                if (r) r->emitReadBytecode(bCon, p->pos);
                bCon->emitBranch(eBranchTrue, loopTop, p->pos);
                bCon->setLabel(w->breakLabelID);
            }
            break;
        case StmtNode::DoWhile:
            {
                UnaryStmtNode *w = checked_cast<UnaryStmtNode *>(p);
                w->breakLabelID = bCon->getLabel();
                w->continueLabelID = bCon->getLabel();
                BytecodeContainer::LabelID loopTop = bCon->getLabel();
                bCon->setLabel(loopTop);
                targetList.push_back(p);
                EvalStmt(env, phase, w->stmt);
                targetList.pop_back();
                bCon->setLabel(w->continueLabelID);
                Reference *r = EvalExprNode(env, phase, w->expr);
                if (r) r->emitReadBytecode(bCon, p->pos);
                bCon->emitBranch(eBranchTrue, loopTop, p->pos);
                bCon->setLabel(w->breakLabelID);
            }
            break;
        case StmtNode::Return:
            {
                ExprStmtNode *e = checked_cast<ExprStmtNode *>(p);
                if (e->expr) {
                    EvalExprNode(env, phase, e->expr);
                    bCon->emitOp(eReturn, p->pos);
                }
            }
            break;
        case StmtNode::Function:
            {
                FunctionStmtNode *f = checked_cast<FunctionStmtNode *>(p);
                BytecodeContainer *saveBacon = bCon;
                bCon = f->fWrap->bCon;
                env->addFrame(f->fWrap->compileFrame);
                EvalStmt(env, phase, f->function.body);
                env->removeTopFrame();
                bCon = saveBacon;
            }
            break;
        case StmtNode::Var:
        case StmtNode::Const:
            {
                // Note that the code here is the PreEval code plus the emit of the Eval bytecode
                VariableStmtNode *vs = checked_cast<VariableStmtNode *>(p);                
                VariableBinding *vb = vs->bindings;
                while (vb)  {
                    if (vb->member) {
                        if (vb->member->kind == Member::Variable) {
                            Variable *v = checked_cast<Variable *>(vb->member);
                            JS2Class *type = getVariableType(v, CompilePhase, p->pos);
                            if (JS2VAL_IS_FUTURE(v->value)) {
                                v->value = JS2VAL_INACCESSIBLE;
                                if (vb->initializer) {
                                    try {
                                        js2val newValue = EvalExpression(env, CompilePhase, vb->initializer);
                                        v->value = engine->assignmentConversion(newValue, type);
                                    }
                                    catch (Exception x) {
                                        // If a compileExpressionError occurred, then the initialiser is not a compile-time 
                                        // constant expression. In this case, ignore the error and leave the value of the 
                                        // variable inaccessible until it is defined at run time.
                                        if (x.kind != Exception::compileExpressionError)
                                            throw x;
                                    }
                                    // XXX more here - 
                                    //
                                    // eGET_TOP_FRAME    <-- establish base
                                    // eDotRead <v->mn>
                                    // eIS_INACCESSIBLE      ??
                                    // eBRANCH_FALSE <lbl>   ?? eBRANCH_ACC <lbl>
                                    //      eGET_TOP_FRAME
                                    //      <vb->initializer code>
                                    //      <convert to 'type'>
                                    //      eDotWrite <v->mn>
                                    // <lbl>:
                                }
                                else
                                    // Would only have come here if the variable was immutable - i.e. a 'const' definition
                                    reportError(Exception::compileExpressionError, "Missing compile time expression", p->pos);
                            }
                        }
                        else {
                            ASSERT(vb->member->kind == Member::InstanceVariableKind);
                            InstanceVariable *v = checked_cast<InstanceVariable *>(vb->member);
                            JS2Class *t;
                            if (vb->type)
                                t = EvalTypeExpression(env, CompilePhase, vb->type);
                            else {
                                if (vb->osp->first->overriddenMember && (vb->osp->first->overriddenMember != POTENTIAL_CONFLICT))
                                    t = vb->osp->first->overriddenMember->type;
                                else
                                    if (vb->osp->second->overriddenMember && (vb->osp->second->overriddenMember != POTENTIAL_CONFLICT))
                                        t = vb->osp->second->overriddenMember->type;
                                else
                                    t = objectClass;
                            }
                            v->type = t;
                        }
                    }
                    else { // HoistedVariable
                        if (vb->initializer) {
                            Reference *r = EvalExprNode(env, phase, vb->initializer);
                            if (r) r->emitReadBytecode(bCon, p->pos);
                            LexicalReference *lVal = new LexicalReference(*vb->name, cxt.strict);
                            lVal->variableMultiname->addNamespace(publicNamespace);
                            lVal->emitWriteBytecode(bCon, p->pos);                                                        
                        }
                    }
                    vb = vb->next;
                }
            }
            break;
        case StmtNode::expression:
            {
                ExprStmtNode *e = checked_cast<ExprStmtNode *>(p);
                Reference *r = EvalExprNode(env, phase, e->expr);
                if (r) r->emitReadBytecode(bCon, p->pos);
                bCon->emitOp(ePopv, p->pos);
            }
            break;
        case StmtNode::Namespace:
            {
            }
            break;
        case StmtNode::Use:
            {
            }
            break;
        case StmtNode::Class:
            {
                ClassStmtNode *classStmt = checked_cast<ClassStmtNode *>(p);
                JS2Class *c = classStmt->c;
                if (classStmt->body) {
                    env->addFrame(c);
                    bCon->emitOp(ePushFrame, p->pos);
                    bCon->addFrame(c);
                    StmtNode *bp = classStmt->body->statements;
                    while (bp) {
                        EvalStmt(env, phase, bp);
                        bp = bp->next;
                    }
                    ASSERT(env->getTopFrame() == c);
                    env->removeTopFrame();
                    bCon->emitOp(ePopFrame, p->pos);
                }
            }
            break;
        default:
            NOT_REACHED("Not Yet Implemented");
        }   // switch (p->getKind())
    }


/************************************************************************************
 *
 *  Attributes
 *
 ************************************************************************************/

    //
    // Validate the Attribute expression at p
    // An attribute expression can only be a list of 'juxtaposed' attribute elements
    //
    // Note : "AttributeExpression" here is a different beast than in the spec. - here it
    // describes the entire attribute part of a directive, not just the qualified identifier
    // and other references encountered in an attribute.
    //
    void JS2Metadata::ValidateAttributeExpression(Context *cxt, Environment *env, ExprNode *p)
    {
        switch (p->getKind()) {
        case ExprNode::boolean:
            break;
        case ExprNode::juxtapose:
            {
                BinaryExprNode *j = checked_cast<BinaryExprNode *>(p);
                ValidateAttributeExpression(cxt, env, j->op1);
                ValidateAttributeExpression(cxt, env, j->op2);
            }
            break;
        case ExprNode::identifier:
            {
                const StringAtom &name = checked_cast<IdentifierExprNode *>(p)->name;
                CompoundAttribute *ca = NULL;
                switch (name.tokenKind) {
                case Token::Public:
                    return;
                case Token::Abstract:
                    return;
                case Token::Final:
                    return;
                case Token::Private:
                    {
                        JS2Class *c = env->getEnclosingClass();
                        if (!c)
                            reportError(Exception::syntaxError, "Private can only be used inside a class definition", p->pos);
                    }
                    return;
                case Token::Static:
                    return;
                }
                // fall thru to handle as generic expression element...
            }            
        default:
            {
                ValidateExpression(cxt, env, p);
            }
            break;

        } // switch (p->getKind())
    }

    // Evaluate the Attribute expression rooted at p.
    // An attribute expression can only be a list of 'juxtaposed' attribute elements
    Attribute *JS2Metadata::EvalAttributeExpression(Environment *env, Phase phase, ExprNode *p)
    {
        switch (p->getKind()) {
        case ExprNode::boolean:
            if (checked_cast<BooleanExprNode *>(p)->value)
                return new TrueAttribute();
            else
                return new FalseAttribute();
        case ExprNode::juxtapose:
            {
                BinaryExprNode *j = checked_cast<BinaryExprNode *>(p);
                Attribute *a = EvalAttributeExpression(env, phase, j->op1);
                if (a && (a->attrKind == Attribute::FalseAttr))
                    return a;
                Attribute *b = EvalAttributeExpression(env, phase, j->op2);
                try {
                    return Attribute::combineAttributes(a, b);
                }
                catch (char *err) {
                    reportError(Exception::badValueError, err, p->pos);
                }
            }
            break;

        case ExprNode::identifier:
            {
                const StringAtom &name = checked_cast<IdentifierExprNode *>(p)->name;
                CompoundAttribute *ca = NULL;
                switch (name.tokenKind) {
                case Token::Public:
                    return publicNamespace;
                case Token::Abstract:
                    ca = new CompoundAttribute();
                    ca->memberMod = Attribute::Abstract;
                    return ca;
                case Token::Final:
                    ca = new CompoundAttribute();
                    ca->memberMod = Attribute::Final;
                    return ca;
                case Token::Private:
                    {
                        JS2Class *c = env->getEnclosingClass();
                        return c->privateNamespace;
                    }
                case Token::Static:
                    ca = new CompoundAttribute();
                    ca->memberMod = Attribute::Static;
                    return ca;
                }
            }            
            // fall thru to execute a readReference on the identifier...
        default:
            {
                // anything else (just references of one kind or another) must
                // be compile-time constant values that resolve to namespaces
                js2val av = EvalExpression(env, CompilePhase, p);
                if (JS2VAL_IS_NULL(av) || !JS2VAL_IS_OBJECT(av))
                    reportError(Exception::badValueError, "Namespace expected in attribute", p->pos);
                JS2Object *obj = JS2VAL_TO_OBJECT(av);
                if ((obj->kind != AttributeObjectKind) || (checked_cast<Attribute *>(obj)->attrKind != Attribute::NamespaceAttr))
                    reportError(Exception::badValueError, "Namespace expected in attribute", p->pos);
                return checked_cast<Attribute *>(obj);
            }
            break;

        } // switch (p->getKind())
        return NULL;
    }

    // Combine attributes a & b, reporting errors for incompatibilities
    // a is not false
    Attribute *Attribute::combineAttributes(Attribute *a, Attribute *b)
    {
        if (b && (b->attrKind == FalseAttr)) {
            if (a) delete a;
            return b;
        }
        if (!a || (a->attrKind == TrueAttr)) {
            if (a) delete a;
            return b;
        }
        if (!b || (b->attrKind == TrueAttr)) {
            if (b) delete b;
            return a;
        }
        if (a->attrKind == NamespaceAttr) {
            if (a == b) {
                delete b;
                return a;
            }
            Namespace *na = checked_cast<Namespace *>(a);
            if (b->kind == NamespaceAttr) {
                Namespace *nb = checked_cast<Namespace *>(b);
                CompoundAttribute *c = new CompoundAttribute();
                c->addNamespace(na);
                c->addNamespace(nb);
                delete a;
                delete b;
                return (Attribute *)c;
            }
            else {
                ASSERT(b->attrKind == CompoundAttr);
                CompoundAttribute *cb = checked_cast<CompoundAttribute *>(b);
                cb->addNamespace(na);
                delete a;
                return b;
            }
        }
        else {
            // Both a and b are compound attributes. Ensure that they have no conflicting contents.
            ASSERT((a->attrKind == CompoundAttr) && (b->attrKind == CompoundAttr));
            CompoundAttribute *ca = checked_cast<CompoundAttribute *>(a);
            CompoundAttribute *cb = checked_cast<CompoundAttribute *>(b);
            if ((ca->memberMod != NoModifier) && (cb->memberMod != NoModifier) && (ca->memberMod != cb->memberMod))
                throw("Illegal combination of member modifier attributes");
            if ((ca->overrideMod != NoOverride) && (cb->overrideMod != NoOverride) && (ca->overrideMod != cb->overrideMod))
                throw("Illegal combination of override attributes");
            for (NamespaceListIterator i = cb->namespaces->begin(), end = cb->namespaces->end(); (i != end); i++)
                ca->addNamespace(*i);
            ca->xplicit |= cb->xplicit;
            ca->dynamic |= cb->dynamic;
            if (ca->memberMod == NoModifier)
                ca->memberMod = cb->memberMod;
            if (ca->overrideMod == NoModifier)
                ca->overrideMod = cb->overrideMod;
            ca->prototype |= cb->prototype;
            ca->unused |= cb->unused;
            delete b;
            return a;
        }
    }

    // add the namespace to our list, but only if it's not there already
    void CompoundAttribute::addNamespace(Namespace *n)
    {
        if (namespaces) {
            for (NamespaceListIterator i = namespaces->begin(), end = namespaces->end(); (i != end); i++)
                if (*i == n)
                    return;
        }
        else
            namespaces = new NamespaceList();
        namespaces->push_back(n);
    }

    CompoundAttribute::CompoundAttribute() : Attribute(CompoundAttr),
            namespaces(NULL), xplicit(false), dynamic(false), memberMod(NoModifier), 
            overrideMod(NoOverride), prototype(false), unused(false) 
    { 
    }

    CompoundAttribute *Attribute::toCompoundAttribute(Attribute *a)
    { 
        if (a) 
            return a->toCompoundAttribute(); 
        else
            return new CompoundAttribute();
    }

    CompoundAttribute *Namespace::toCompoundAttribute()    
    { 
        CompoundAttribute *t = new CompoundAttribute(); 
        t->addNamespace(this); 
        return t; 
    }

    CompoundAttribute *TrueAttribute::toCompoundAttribute()    
    { 
        return new CompoundAttribute(); 
    }



/************************************************************************************
 *
 *  Expressions
 *
 ************************************************************************************/

    // Validate the entire expression rooted at p
    void JS2Metadata::ValidateExpression(Context *cxt, Environment *env, ExprNode *p)
    {
        switch (p->getKind()) {
        case ExprNode::Null:
        case ExprNode::number:
        case ExprNode::boolean:
            break;
        case ExprNode::This:
            {
                if (env->findThis(true) == JS2VAL_VOID)
                    reportError(Exception::syntaxError, "No 'this' available", p->pos);
            }
            break;
        case ExprNode::objectLiteral:
            break;
        case ExprNode::index:
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                ValidateExpression(cxt, env, i->op);
                ExprPairList *ep = i->pairs;
                uint16 positionalCount = 0;
                while (ep) {
                    if (ep->field)
                        reportError(Exception::argumentMismatchError, "Indexing doesn't support named arguments", p->pos);
                    else {
                        if (positionalCount)
                            reportError(Exception::argumentMismatchError, "Indexing doesn't support more than 1 argument", p->pos);
                        positionalCount++;
                        ValidateExpression(cxt, env, ep->value);
                    }
                    ep = ep->next;
                }
            }
            break;
        case ExprNode::dot:
            {
                BinaryExprNode *b = checked_cast<BinaryExprNode *>(p);
                ValidateExpression(cxt, env, b->op1);
                ValidateExpression(cxt, env, b->op2);
            }
            break;

        case ExprNode::equal:
        case ExprNode::notEqual:
        case ExprNode::assignment:
        case ExprNode::add:
        case ExprNode::subtract:
            {
                BinaryExprNode *b = checked_cast<BinaryExprNode *>(p);
                ValidateExpression(cxt, env, b->op1);
                ValidateExpression(cxt, env, b->op2);
            }
            break;

        case ExprNode::postIncrement:
        case ExprNode::postDecrement:
        case ExprNode::preIncrement:
        case ExprNode::preDecrement:
            {
                UnaryExprNode *u = checked_cast<UnaryExprNode *>(p);
                ValidateExpression(cxt, env, u->op);
            }
            break;

        case ExprNode::qualify:
        case ExprNode::identifier:
            {
//                IdentifierExprNode *i = checked_cast<IdentifierExprNode *>(p);
            }
            break;
        case ExprNode::call:
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                ValidateExpression(cxt, env, i->op);
                ExprPairList *args = i->pairs;
                while (args) {
                    ValidateExpression(cxt, env, args->value);
                    args = args->next;
                }
            }
            break;
        case ExprNode::New: 
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                ValidateExpression(cxt, env, i->op);
                ExprPairList *args = i->pairs;
                while (args) {
                    ValidateExpression(cxt, env, args->value);
                    args = args->next;
                }
            }
            break;
        default:
            NOT_REACHED("Not Yet Implemented");
        } // switch (p->getKind())
    }



    /*
     * Evaluate an expression 'p' and execute the associated bytecode
     */
    js2val JS2Metadata::EvalExpression(Environment *env, Phase phase, ExprNode *p)
    {
        BytecodeContainer *saveBacon = bCon;
        bCon = new BytecodeContainer();
        Reference *r = EvalExprNode(env, phase, p);
        if (r) r->emitReadBytecode(bCon, p->pos);
        bCon->emitOp(eReturn, p->pos);
        js2val retval = engine->interpret(this, phase, bCon);
        bCon = saveBacon;
        return retval;
    }

    /*
     * Evaluate the expression rooted at p.
     */
    Reference *JS2Metadata::EvalExprNode(Environment *env, Phase phase, ExprNode *p)
    {
        Reference *returnRef = NULL;
        JS2Op binaryOp;

        switch (p->getKind()) {

        case ExprNode::assignment:
            {
                if (phase == CompilePhase) reportError(Exception::compileExpressionError, "Inappropriate compile time expression", p->pos);
                BinaryExprNode *b = checked_cast<BinaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, b->op1);
                if (lVal) {
                    Reference *rVal = EvalExprNode(env, phase, b->op2);
                    if (rVal) rVal->emitReadBytecode(bCon, p->pos);
                    lVal->emitWriteBytecode(bCon, p->pos);
                }
                else
                    reportError(Exception::semanticError, "Assignment needs an lValue", p->pos);
            }
            break;
        case ExprNode::equal:
            binaryOp = eEqual;
            goto doBinary;
        case ExprNode::notEqual:
            binaryOp = eNotEqual;
            goto doBinary;

        case ExprNode::add:
            binaryOp = eAdd;
            goto doBinary;
        case ExprNode::subtract:
            binaryOp = eSubtract;
            goto doBinary;
doBinary:
            {
                BinaryExprNode *b = checked_cast<BinaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, b->op1);
                if (lVal) lVal->emitReadBytecode(bCon, p->pos);
                Reference *rVal = EvalExprNode(env, phase, b->op2);
                if (rVal) rVal->emitReadBytecode(bCon, p->pos);
                bCon->emitOp(binaryOp, p->pos);
            }
            break;
        case ExprNode::This:
            {

            }
            break;
        case ExprNode::Null:
            {
                bCon->emitOp(eNull, p->pos);
            }
            break;
        case ExprNode::number:
            {
                bCon->emitOp(eNumber, p->pos);
                bCon->addFloat64(checked_cast<NumberExprNode *>(p)->value);
            }
            break;
        case ExprNode::qualify:
            {
                QualifyExprNode *qe = checked_cast<QualifyExprNode *>(p);
                const StringAtom &name = qe->name;

                js2val av = EvalExpression(env, CompilePhase, qe->qualifier);
                if (JS2VAL_IS_NULL(av) || !JS2VAL_IS_OBJECT(av))
                    reportError(Exception::badValueError, "Namespace expected in qualifier", p->pos);
                JS2Object *obj = JS2VAL_TO_OBJECT(av);
                if ((obj->kind != AttributeObjectKind) || (checked_cast<Attribute *>(obj)->attrKind != Attribute::NamespaceAttr))
                    reportError(Exception::badValueError, "Namespace expected in qualifier", p->pos);
                Namespace *ns = checked_cast<Namespace *>(obj);
                
                returnRef = new LexicalReference(name, ns, cxt.strict);
            }
            break;
        case ExprNode::identifier:
            {
                IdentifierExprNode *i = checked_cast<IdentifierExprNode *>(p);
                returnRef = new LexicalReference(i->name, cxt.strict);
                ((LexicalReference *)returnRef)->variableMultiname->addNamespace(cxt);
            }
            break;
        case ExprNode::postIncrement:
            {
                UnaryExprNode *u = checked_cast<UnaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, u->op);
                if (lVal)
                    lVal->emitPostIncBytecode(bCon, p->pos);
                else
                    reportError(Exception::semanticError, "PostIncrement needs an lValue", p->pos);
            }
            break;
        case ExprNode::postDecrement:
            {
                UnaryExprNode *u = checked_cast<UnaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, u->op);
                if (lVal)
                    lVal->emitPostDecBytecode(bCon, p->pos);
                else
                    reportError(Exception::semanticError, "PostDecrement needs an lValue", p->pos);
            }
            break;
        case ExprNode::preIncrement:
            {
                UnaryExprNode *u = checked_cast<UnaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, u->op);
                if (lVal)
                    lVal->emitPreIncBytecode(bCon, p->pos);
                else
                    reportError(Exception::semanticError, "PreIncrement needs an lValue", p->pos);
            }
            break;
        case ExprNode::preDecrement:
            {
                UnaryExprNode *u = checked_cast<UnaryExprNode *>(p);
                Reference *lVal = EvalExprNode(env, phase, u->op);
                if (lVal)
                    lVal->emitPreDecBytecode(bCon, p->pos);
                else
                    reportError(Exception::semanticError, "PreDecrement needs an lValue", p->pos);
            }
            break;
        case ExprNode::index:
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                Reference *baseVal = EvalExprNode(env, phase, i->op);
                if (baseVal) baseVal->emitReadBytecode(bCon, p->pos);
                ExprPairList *ep = i->pairs;
                while (ep) {
                    Reference *argVal = EvalExprNode(env, phase, ep->value);
                    if (argVal) argVal->emitReadBytecode(bCon, p->pos);
                    ep = ep->next;
                }
                returnRef = new BracketReference();
            }
            break;
        case ExprNode::dot:
            {
                BinaryExprNode *b = checked_cast<BinaryExprNode *>(p);
                Reference *baseVal = EvalExprNode(env, phase, b->op1);
                if (baseVal) baseVal->emitReadBytecode(bCon, p->pos);

                if (b->op2->getKind() == ExprNode::identifier) {
                    IdentifierExprNode *i = checked_cast<IdentifierExprNode *>(b->op2);
                    returnRef = new DotReference(i->name);
                } 
                else {
                    if (b->op2->getKind() == ExprNode::qualify) {
                        Reference *rVal = EvalExprNode(env, phase, b->op2);                        
                        ASSERT(rVal && checked_cast<LexicalReference *>(rVal));
                        returnRef = new DotReference(((LexicalReference *)rVal)->variableMultiname);
                    }
                    // else bracketRef...
                }
            }
            break;
        case ExprNode::boolean:
            if (checked_cast<BooleanExprNode *>(p)->value) 
                bCon->emitOp(eTrue, p->pos);
            else 
                bCon->emitOp(eFalse, p->pos);
            break;
        case ExprNode::objectLiteral:
            {
                uint32 argCount = 0;
                PairListExprNode *plen = checked_cast<PairListExprNode *>(p);
                ExprPairList *e = plen->pairs;
                while (e) {
                    ASSERT(e->field && e->value);
                    Reference *rVal = EvalExprNode(env, phase, e->value);
                    if (rVal) rVal->emitReadBytecode(bCon, p->pos);
                    switch (e->field->getKind()) {
                    case ExprNode::identifier:
                        bCon->addString(checked_cast<IdentifierExprNode *>(e->field)->name, p->pos);
                        break;
                    case ExprNode::string:
                        bCon->addString(checked_cast<StringExprNode *>(e->field)->str, p->pos);
                        break;
                    case ExprNode::number:
                        bCon->addString(numberToString(&(checked_cast<NumberExprNode *>(e->field))->value), p->pos);
                        break;
                    default:
                        NOT_REACHED("bad field name");
                    }
                    argCount++;
                    e = e->next;
                }
                bCon->emitOp(eNewObject, p->pos, -argCount + 1);    // pop argCount args and push a new object
                bCon->addShort(argCount);
            }
            break;
        case ExprNode::call:
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                Reference *rVal = EvalExprNode(env, phase, i->op);
                if (rVal) rVal->emitReadForInvokeBytecode(bCon, p->pos);
                ExprPairList *args = i->pairs;
                uint16 argCount = 0;
                while (args) {
                    EvalExprNode(env, phase, args->value);
                    argCount++;
                    args = args->next;
                }
                bCon->emitOp(eCall, p->pos, -argCount + 1);    // pop argCount args and push a result
                bCon->addShort(argCount);
            }
            break;
        case ExprNode::New: 
            {
                InvokeExprNode *i = checked_cast<InvokeExprNode *>(p);
                Reference *rVal = EvalExprNode(env, phase, i->op);
                if (rVal) rVal->emitReadBytecode(bCon, p->pos);
                ExprPairList *args = i->pairs;
                uint16 argCount = 0;
                while (args) {
                    EvalExprNode(env, phase, args->value);
                    argCount++;
                    args = args->next;
                }
                bCon->emitOp(eNew, p->pos, -argCount + 1);    // pop argCount args and push a new object
                bCon->addShort(argCount);
            }
            break;
        default:
            NOT_REACHED("Not Yet Implemented");
        }
        return returnRef;
    }

    JS2Class *JS2Metadata::EvalTypeExpression(Environment *env, Phase phase, ExprNode *p)
    {
        js2val retval = EvalExpression(env, phase, p);
        if (JS2VAL_IS_PRIMITIVE(retval))
            reportError(Exception::badValueError, "Type expected", p->pos);
        JS2Object *obj = JS2VAL_TO_OBJECT(retval);
        if (obj->kind != ClassKind)
            reportError(Exception::badValueError, "Type expected", p->pos);
        return checked_cast<JS2Class *>(obj);        
    }

/************************************************************************************
 *
 *  Environment
 *
 ************************************************************************************/

    // If env is from within a class's body, getEnclosingClass(env) returns the 
    // innermost such class; otherwise, it returns none.
    JS2Class *Environment::getEnclosingClass()
    {
        Frame *pf = firstFrame;
        while (pf && (pf->kind != ClassKind))
            pf = pf->nextFrame;
        return checked_cast<JS2Class *>(pf);
    }

    // returns the most specific regional frame.
    Frame *Environment::getRegionalFrame()
    {
        Frame *pf = firstFrame;
        Frame *prev = NULL;
        while (pf->kind == BlockKind) { 
            prev = pf;
            pf = pf->nextFrame;
        }
        if ((pf != firstFrame) && (pf->kind == ClassKind))
            pf = prev;
        return pf;
    }

    // XXX makes the argument for vector instead of linked list...
    // Returns the penultimate frame, either Package or Global
    Frame *Environment::getPackageOrGlobalFrame()
    {
        Frame *pf = firstFrame;
        while (pf && (pf->nextFrame) && (pf->nextFrame->nextFrame))
            pf = pf->nextFrame;
        return pf;
    }

    // findThis returns the value of this. If allowPrototypeThis is true, allow this to be defined 
    // by either an instance member of a class or a prototype function. If allowPrototypeThis is 
    // false, allow this to be defined only by an instance member of a class.
    js2val Environment::findThis(bool allowPrototypeThis)
    {
        Frame *pf = firstFrame;
        while (pf) {
            if ((pf->kind == ParameterKind)
                    && !JS2VAL_IS_NULL(checked_cast<ParameterFrame *>(pf)->thisObject))
                if (allowPrototypeThis || !checked_cast<ParameterFrame *>(pf)->prototype)
                    return checked_cast<ParameterFrame *>(pf)->thisObject;
            pf = pf->nextFrame;
        }
        return JS2VAL_VOID;
    }

    // Read the value of a lexical reference - it's an error if that reference
    // doesn't have a binding somewhere
    js2val Environment::lexicalRead(JS2Metadata *meta, Multiname *multiname, Phase phase)
    {
        LookupKind lookup(true, findThis(false));
        Frame *pf = firstFrame;
        while (pf) {
            js2val rval;
            if (meta->readProperty(pf, multiname, &lookup, phase, &rval))
                return rval;

            pf = pf->nextFrame;
        }
        meta->reportError(Exception::referenceError, "{0} is undefined", meta->engine->errorPos(), multiname->name);
        return JS2VAL_VOID;
    }

    void Environment::lexicalWrite(JS2Metadata *meta, Multiname *multiname, js2val newValue, bool createIfMissing, Phase phase)
    {
        LookupKind lookup(true, findThis(false));
        Frame *pf = firstFrame;
        while (pf) {
            if (meta->writeProperty(pf, multiname, &lookup, false, newValue, phase))
                return;
            pf = pf->nextFrame;
        }
        if (createIfMissing) {
            pf = getPackageOrGlobalFrame();
            if (pf->kind == GlobalObjectKind) {
                if (meta->writeProperty(pf, multiname, &lookup, true, newValue, phase))
                    return;
            }
        }
        meta->reportError(Exception::referenceError, "{0} is undefined", meta->engine->errorPos(), multiname->name);
    }

    // Clone the pluralFrame bindings into the singularFrame, instantiating new members for each binding
    void Environment::instantiateFrame(Frame *pluralFrame, Frame *singularFrame)
    {
        StaticBindingIterator sbi, sbend;
        for (sbi = pluralFrame->staticReadBindings.begin(), sbend = pluralFrame->staticReadBindings.end(); (sbi != sbend); sbi++) {
            sbi->second->content->cloneContent = NULL;
        }
        for (sbi = pluralFrame->staticWriteBindings.begin(), sbend = pluralFrame->staticWriteBindings.end(); (sbi != sbend); sbi++) {
            sbi->second->content->cloneContent = NULL;
        }
        for (sbi = pluralFrame->staticReadBindings.begin(), sbend = pluralFrame->staticReadBindings.end(); (sbi != sbend); sbi++) {
            StaticBinding *sb;
            StaticBinding *m = sbi->second;
            if (m->content->cloneContent == NULL) {
                m->content->cloneContent = m->content->clone();
            }
            sb = new StaticBinding(m->qname, m->content->cloneContent);
            sb->xplicit = m->xplicit;
            const StaticBindingMap::value_type e(sbi->first, sb);
            singularFrame->staticReadBindings.insert(e);
        }
        for (sbi = pluralFrame->staticWriteBindings.begin(), sbend = pluralFrame->staticWriteBindings.end(); (sbi != sbend); sbi++) {
            StaticBinding *sb;
            StaticBinding *m = sbi->second;
            if (m->content->cloneContent == NULL) {
                m->content->cloneContent = m->content->clone();
            }
            sb = new StaticBinding(m->qname, m->content->cloneContent);
            sb->xplicit = m->xplicit;
            const StaticBindingMap::value_type e(sbi->first, sb);
            singularFrame->staticWriteBindings.insert(e);
        }

    }




/************************************************************************************
 *
 *  Context
 *
 ************************************************************************************/

    // clone a context
    Context::Context(Context *cxt) : strict(cxt->strict), openNamespaces(cxt->openNamespaces)
    {
        ASSERT(false);  // ?? used ??
    }


/************************************************************************************
 *
 *  Multiname
 *
 ************************************************************************************/

    // return true if the given namespace is on the namespace list
    bool Multiname::onList(Namespace *nameSpace)
    { 
        if (nsList.empty())
            return true;
        for (NamespaceListIterator n = nsList.begin(), end = nsList.end(); (n != end); n++) {
            if (*n == nameSpace)
                return true;
        }
        return false;
    }

    void Multiname::addNamespace(Context &cxt)    
    { 
        addNamespace(&cxt.openNamespaces); 
    }


    // add every namespace from the list to this Multiname
    void Multiname::addNamespace(NamespaceList *ns)
    {
        for (NamespaceListIterator nli = ns->begin(), end = ns->end();
                (nli != end); nli++)
            nsList.push_back(*nli);
    }


/************************************************************************************
 *
 *  JS2Metadata
 *
 ************************************************************************************/

    // - Define namespaces::id (for all namespaces or at least 'public') in the top frame 
    //     unless it's there already. 
    // - If the binding exists (not forbidden) in lower frames in the regional environment, it's an error.
    // - Define a forbidden binding in all the lower frames.
    // 
    Multiname *JS2Metadata::defineStaticMember(Environment *env, const StringAtom &id, NamespaceList *namespaces, Attribute::OverrideModifier overrideMod, bool xplicit, Access access, StaticMember *m, size_t pos)
    {
        NamespaceList publicNamespaceList;

        Frame *localFrame = env->getTopFrame();
        if ((overrideMod != Attribute::NoOverride) || (xplicit && localFrame->kind != PackageKind))
            reportError(Exception::definitionError, "Illegal definition", pos);
        if ((namespaces == NULL) || namespaces->empty()) {
            publicNamespaceList.push_back(publicNamespace);
            namespaces = &publicNamespaceList;
        }
        Multiname *mn = new Multiname(id);
        mn->addNamespace(namespaces);

        for (StaticBindingIterator b = localFrame->staticReadBindings.lower_bound(id),
                end = localFrame->staticReadBindings.upper_bound(id); (b != end); b++) {
            if (mn->matches(b->second->qname))
                reportError(Exception::definitionError, "Duplicate definition {0}", pos, id);
        }
        

        // Check all frames below the current - up to the RegionalFrame
        Frame *regionalFrame = env->getRegionalFrame();
        if (localFrame != regionalFrame) {
            Frame *fr = localFrame->nextFrame;
            while (fr != regionalFrame) {
                for (b = fr->staticReadBindings.lower_bound(id),
                        end = fr->staticReadBindings.upper_bound(id); (b != end); b++) {
                    if (mn->matches(b->second->qname) && (b->second->content->kind == StaticMember::Forbidden))
                        reportError(Exception::definitionError, "Duplicate definition {0}", pos, id);
                }
                fr = fr->nextFrame;
            }
        }
        if (regionalFrame->kind == GlobalObjectKind) {
            GlobalObject *gObj = checked_cast<GlobalObject *>(regionalFrame);
            DynamicPropertyIterator dp = gObj->dynamicProperties.find(id);
            if (dp != gObj->dynamicProperties.end())
                reportError(Exception::definitionError, "Duplicate definition {0}", pos, id);
        }
        
        for (NamespaceListIterator nli = mn->nsList.begin(), nlend = mn->nsList.end(); (nli != nlend); nli++) {
            QualifiedName qName(*nli, id);
            StaticBinding *sb = new StaticBinding(qName, m);
            const StaticBindingMap::value_type e(id, sb);
            if (access & ReadAccess)
                regionalFrame->staticReadBindings.insert(e);
            if (access & WriteAccess)
                regionalFrame->staticWriteBindings.insert(e);
        }
        
        if (localFrame != regionalFrame) {
            Frame *fr = localFrame->nextFrame;
            while (fr != regionalFrame) {
                for (NamespaceListIterator nli = mn->nsList.begin(), nlend = mn->nsList.end(); (nli != nlend); nli++) {
                    QualifiedName qName(*nli, id);
                    StaticBinding *sb = new StaticBinding(qName, forbiddenMember);
                    const StaticBindingMap::value_type e(id, sb);
                    if (access & ReadAccess)
                        fr->staticReadBindings.insert(e);
                    if (access & WriteAccess)
                        fr->staticWriteBindings.insert(e);
                }
                fr = fr->nextFrame;
            }
        }
        return mn;
    }

    // Look through 'c' and all it's super classes for a identifier 
    // matching the qualified name and access.
    InstanceMember *JS2Metadata::findInstanceMember(JS2Class *c, QualifiedName *qname, Access access)
    {
        if (qname == NULL)
            return NULL;
        JS2Class *s = c;
        while (s) {
            if (access & ReadAccess) {
                for (InstanceBindingIterator b = s->instanceReadBindings.lower_bound(qname->id),
                        end = s->instanceReadBindings.upper_bound(qname->id); (b != end); b++) {
                    if (*qname == b->second->qname)
                        return b->second->content;
                }
            }        
            if (access & WriteAccess) {
                for (InstanceBindingIterator b = s->instanceWriteBindings.lower_bound(qname->id),
                        end = s->instanceWriteBindings.upper_bound(qname->id); (b != end); b++) {
                    if (*qname == b->second->qname)
                        return b->second->content;
                }
            }
            s = s->super;
        }
        return NULL;
    }

    // Examine class 'c' and find all instance members that would be overridden
    // by 'id' in any of the given namespaces.
    OverrideStatus *JS2Metadata::searchForOverrides(JS2Class *c, const StringAtom &id, NamespaceList *namespaces, Access access, size_t pos)
    {
        OverrideStatus *os = new OverrideStatus(NULL, id);
        for (NamespaceListIterator ns = namespaces->begin(), end = namespaces->end(); (ns != end); ns++) {
            QualifiedName qname(*ns, id);
            InstanceMember *m = findInstanceMember(c, &qname, access);
            if (m) {
                os->multiname.addNamespace(*ns);
                if (os->overriddenMember == NULL)
                    os->overriddenMember = m;
                else
                    if (os->overriddenMember != m)  // different instance members by same id
                        reportError(Exception::definitionError, "Illegal override", pos);
            }
        }
        return os;
    }

    // Find the possible override conflicts that arise from the given id and namespaces
    // Fall back on the currently open namespace list if no others are specified.
    OverrideStatus *JS2Metadata::resolveOverrides(JS2Class *c, Context *cxt, const StringAtom &id, NamespaceList *namespaces, Access access, bool expectMethod, size_t pos)
    {
        OverrideStatus *os = NULL;
        if ((namespaces == NULL) || namespaces->empty()) {
            os = searchForOverrides(c, id, &cxt->openNamespaces, access, pos);
            if (os->overriddenMember == NULL) {
                ASSERT(os->multiname.nsList.empty());
                os->multiname.addNamespace(publicNamespace);
            }
        }
        else {
            OverrideStatus *os2 = searchForOverrides(c, id, namespaces, access, pos);
            if (os2->overriddenMember == NULL) {
                OverrideStatus *os3 = searchForOverrides(c, id, &cxt->openNamespaces, access, pos);
                if (os3->overriddenMember == NULL) {
                    os->multiname.addNamespace(namespaces);
                }
                else {
                    os->overriddenMember = POTENTIAL_CONFLICT;  // Didn't find the member with a specified namespace, but did with
                                                                // the use'd ones. That'll be an error unless the override is 
                                                                // disallowed (in defineInstanceMember below)
                    os->multiname.addNamespace(namespaces);
                }
                delete os3;
                delete os2;
            }
            else {
                os = os2;
                os->multiname.addNamespace(namespaces);
            }
        }
        // For all the discovered possible overrides, make sure the member doesn't already exist in the class
        for (NamespaceListIterator nli = os->multiname.nsList.begin(), nlend = os->multiname.nsList.end(); (nli != nlend); nli++) {
            QualifiedName qname(*nli, id);
            if (access & ReadAccess) {
                for (InstanceBindingIterator b = c->instanceReadBindings.lower_bound(id),
                        end = c->instanceReadBindings.upper_bound(id); (b != end); b++) {
                    if (qname == b->second->qname)
                        reportError(Exception::definitionError, "Illegal override", pos);
                }
            }        
            if (access & WriteAccess) {
                for (InstanceBindingIterator b = c->instanceWriteBindings.lower_bound(id),
                        end = c->instanceWriteBindings.upper_bound(id); (b != end); b++) {
                    if (qname == b->second->qname)
                        reportError(Exception::definitionError, "Illegal override", pos);
                }
            }
        }
        // Make sure we're getting what we expected
        if (expectMethod) {
            if (os->overriddenMember && (os->overriddenMember != POTENTIAL_CONFLICT) && (os->overriddenMember->kind != InstanceMember::InstanceMethodKind))
                reportError(Exception::definitionError, "Illegal override, expected method", pos);
        }
        else {
            if (os->overriddenMember && (os->overriddenMember != POTENTIAL_CONFLICT) && (os->overriddenMember->kind == InstanceMember::InstanceMethodKind))
                reportError(Exception::definitionError, "Illegal override, didn't expect method", pos);
        }

        return os;
    }

    // Define an instance member in the class. Verify that, if any overriding is happening, it's legal. The result pair indicates
    // the members being overridden.
    OverrideStatusPair *JS2Metadata::defineInstanceMember(JS2Class *c, Context *cxt, const StringAtom &id, NamespaceList *namespaces, Attribute::OverrideModifier overrideMod, bool xplicit, Access access, InstanceMember *m, size_t pos)
    {
        OverrideStatus *readStatus;
        OverrideStatus *writeStatus;
        if (xplicit)
            reportError(Exception::definitionError, "Illegal use of explicit", pos);

        if (access & ReadAccess)
            readStatus = resolveOverrides(c, cxt, id, namespaces, ReadAccess, (m->kind == InstanceMember::InstanceMethodKind), pos);
        else
            readStatus = new OverrideStatus(NULL, id);

        if (access & WriteAccess)
            writeStatus = resolveOverrides(c, cxt, id, namespaces, WriteAccess, (m->kind == InstanceMember::InstanceMethodKind), pos);
        else
            writeStatus = new OverrideStatus(NULL, id);

        if ((readStatus->overriddenMember && (readStatus->overriddenMember != POTENTIAL_CONFLICT))
                || (writeStatus->overriddenMember && (writeStatus->overriddenMember != POTENTIAL_CONFLICT))) {
            if ((overrideMod != Attribute::DoOverride) && (overrideMod != Attribute::OverrideUndefined))
                reportError(Exception::definitionError, "Illegal override", pos);
        }
        else {
            if ((readStatus->overriddenMember == POTENTIAL_CONFLICT) || (writeStatus->overriddenMember == POTENTIAL_CONFLICT)) {
                if ((overrideMod != Attribute::DontOverride) && (overrideMod != Attribute::OverrideUndefined))
                    reportError(Exception::definitionError, "Illegal override", pos);
            }
        }

        NamespaceListIterator nli, nlend;
        for (nli = readStatus->multiname.nsList.begin(), nlend = readStatus->multiname.nsList.end(); (nli != nlend); nli++) {
            QualifiedName qName(*nli, id);
            InstanceBinding *ib = new InstanceBinding(qName, m);
            const InstanceBindingMap::value_type e(id, ib);
            c->instanceReadBindings.insert(e);
        }
        
        for (nli = writeStatus->multiname.nsList.begin(), nlend = writeStatus->multiname.nsList.end(); (nli != nlend); nli++) {
            QualifiedName qName(*nli, id);
            InstanceBinding *ib = new InstanceBinding(qName, m);
            const InstanceBindingMap::value_type e(id, ib);
            c->instanceWriteBindings.insert(e);
        }
        
        return new OverrideStatusPair(readStatus, writeStatus);;
    }

    // Define a hoisted var in the current frame (either Global or a Function)
    void JS2Metadata::defineHoistedVar(Environment *env, const StringAtom &id, StmtNode *p)
    {
        QualifiedName qName(publicNamespace, id);
        Frame *regionalFrame = env->getRegionalFrame();
        ASSERT((env->getTopFrame()->kind == GlobalObjectKind) || (env->getTopFrame()->kind == ParameterKind));
    
        // run through all the existing bindings, both read and write, to see if this
        // variable already exists.
        StaticBindingIterator b, end;
        bool existing = false;
        for (b = regionalFrame->staticReadBindings.lower_bound(id),
                end = regionalFrame->staticReadBindings.upper_bound(id); (b != end); b++) {
            if (b->second->qname == qName) {
                if (b->second->content->kind != StaticMember::HoistedVariable)
                    reportError(Exception::definitionError, "Duplicate definition {0}", p->pos, id);
                else {
                    existing = true;
                    break;
                }
            }
        }
        for (b = regionalFrame->staticWriteBindings.lower_bound(id),
                end = regionalFrame->staticWriteBindings.upper_bound(id); (b != end); b++) {
            if (b->second->qname == qName) {
                if (b->second->content->kind != StaticMember::HoistedVariable)
                    reportError(Exception::definitionError, "Duplicate definition {0}", p->pos, id);
                else {
                    existing = true;
                    break;
                }
            }
        }
        if (!existing) {
            if (regionalFrame->kind == GlobalObjectKind) {
                GlobalObject *gObj = checked_cast<GlobalObject *>(regionalFrame);
                DynamicPropertyIterator dp = gObj->dynamicProperties.find(id);
                if (dp != gObj->dynamicProperties.end())
                    reportError(Exception::definitionError, "Duplicate definition {0}", p->pos, id);
            }
            // XXX ok to use same binding in read & write maps?
            StaticBinding *sb = new StaticBinding(qName, new HoistedVar());
            const StaticBindingMap::value_type e(id, sb);

            // XXX ok to use same value_type in different multimaps?
            regionalFrame->staticReadBindings.insert(e);
            regionalFrame->staticWriteBindings.insert(e);
        }
        //else A hoisted binding of the same var already exists, so there is no need to create another one
    }

    JS2Metadata::JS2Metadata(World &world) :
        world(world),
        engine(new JS2Engine(world)),
        publicNamespace(new Namespace(engine->public_StringAtom)),
        bCon(new BytecodeContainer()),
        glob(world),
        env(new MetaData::SystemFrame(), &glob)
    {
        cxt.openNamespaces.clear();
        cxt.openNamespaces.push_back(publicNamespace);

        objectClass = new JS2Class(NULL, new JS2Object(PrototypeInstanceKind), new Namespace(engine->private_StringAtom), false, true, false, engine->object_StringAtom);
        objectClass->complete = true;

        functionClass = new JS2Class(objectClass, new JS2Object(PrototypeInstanceKind), new Namespace(engine->private_StringAtom), false, true, true, engine->function_StringAtom);
        functionClass->complete = true;
    }

    // objectType(o) returns an OBJECT o's most specific type.
    JS2Class *JS2Metadata::objectType(js2val objVal)
    {
        if (JS2VAL_IS_VOID(objVal))
            return undefinedClass;
        if (JS2VAL_IS_NULL(objVal))
            return nullClass;
        if (JS2VAL_IS_BOOLEAN(objVal))
            return booleanClass;
        if (JS2VAL_IS_NUMBER(objVal))
            return numberClass;
        if (JS2VAL_IS_STRING(objVal)) {
            if (JS2VAL_TO_STRING(objVal)->length() == 1)
                return characterClass;
            else 
                return stringClass;
        }
        ASSERT(JS2VAL_IS_OBJECT(objVal));
        JS2Object *obj = JS2VAL_TO_OBJECT(objVal);
        switch (obj->kind) {
        case AttributeObjectKind:
            return attributeClass;
        case MultinameKind:
            return namespaceClass;
        case ClassKind:
            return classClass;
        case PrototypeInstanceKind: 
            return prototypeClass;

        case FixedInstanceKind: 
            return checked_cast<FixedInstance *>(obj)->type;
        case DynamicInstanceKind:
            return checked_cast<DynamicInstance *>(obj)->type;

        case GlobalObjectKind: 
        case PackageKind:
            return packageClass;

        case SystemKind:
        case ParameterKind: 
        case BlockKind: 
        default:
            ASSERT(false);
            return NULL;
        }
    }

    // Read the property from the container given by the public id in multiname - if that exists
    // 
    bool JS2Metadata::readDynamicProperty(JS2Object *container, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval)
    {
        ASSERT(container && ((container->kind == DynamicInstanceKind) 
                                || (container->kind == GlobalObjectKind)
                                || (container->kind == PrototypeInstanceKind)));
        if (!multiname->onList(publicNamespace))
            return false;
        const StringAtom &name = multiname->name;
        if (phase == CompilePhase) 
            reportError(Exception::compileExpressionError, "Inappropriate compile time expression", engine->errorPos());
        DynamicPropertyMap *dMap = NULL;
        bool isPrototypeInstance = false;
        if (container->kind == DynamicInstanceKind)
            dMap = &(checked_cast<DynamicInstance *>(container))->dynamicProperties;
        else
        if (container->kind == GlobalObjectKind)
            dMap = &(checked_cast<GlobalObject *>(container))->dynamicProperties;
        else {
            isPrototypeInstance = true;
            dMap = &(checked_cast<PrototypeInstance *>(container))->dynamicProperties;
        }
        for (DynamicPropertyIterator i = dMap->begin(), end = dMap->end(); (i != end); i++) {
            if (i->first == name) {
                *rval = i->second;
                return true;
            }
        }
        if (isPrototypeInstance) {
            PrototypeInstance *pInst = (checked_cast<PrototypeInstance *>(container))->parent;
            while (pInst) {
                for (DynamicPropertyIterator i = pInst->dynamicProperties.begin(), end = pInst->dynamicProperties.end(); (i != end); i++) {
                    if (i->first == name) {
                        *rval = i->second;
                        return true;
                    }
                }
                pInst = pInst->parent;
            }
        }
        if (lookupKind->isPropertyLookup()) {
            *rval = JS2VAL_UNDEFINED;
            return true;
        }
        return false;   // 'None'
    }

    // Write a value to a dynamic container - inserting into the map if not already there (if createIfMissing)
    bool JS2Metadata::writeDynamicProperty(JS2Object *container, Multiname *multiname, bool createIfMissing, js2val newValue, Phase phase)
    {
        ASSERT(container && ((container->kind == DynamicInstanceKind) 
                                || (container->kind == GlobalObjectKind)
                                || (container->kind == PrototypeInstanceKind)));
        if (!multiname->onList(publicNamespace))
            return false;
        const StringAtom &name = multiname->name;
        DynamicPropertyMap *dMap = NULL;
        if (container->kind == DynamicInstanceKind)
            dMap = &(checked_cast<DynamicInstance *>(container))->dynamicProperties;
        else
        if (container->kind == GlobalObjectKind)
            dMap = &(checked_cast<GlobalObject *>(container))->dynamicProperties;
        else 
            dMap = &(checked_cast<PrototypeInstance *>(container))->dynamicProperties;
        for (DynamicPropertyIterator i = dMap->begin(), end = dMap->end(); (i != end); i++) {
            if (i->first == name) {
                i->second = newValue;
                return true;
            }
        }
        if (!createIfMissing)
            return false;
        if (container->kind == DynamicInstanceKind) {
            DynamicInstance *dynInst = checked_cast<DynamicInstance *>(container);
            InstanceBinding *ib = resolveInstanceMemberName(dynInst->type, multiname, ReadAccess, phase);
            if (ib == NULL) {
                const DynamicPropertyMap::value_type e(name, newValue);
                dynInst->dynamicProperties.insert(e);
                return true;
            }
        }
        else {
            if (container->kind == GlobalObjectKind) {
                GlobalObject *glob = checked_cast<GlobalObject *>(container);
                StaticMember *m = findFlatMember(glob, multiname, ReadAccess, phase);
                if (m == NULL) {
                    const DynamicPropertyMap::value_type e(name, newValue);
                    glob->dynamicProperties.insert(e);
                    return true;
                }
            }
        }
        return false;   // 'None'
    }

    // Read the static member
    bool JS2Metadata::readStaticMember(StaticMember *m, Phase phase, js2val *rval)
    {
        if (m == NULL)
            return false;   // 'None'
        switch (m->kind) {
        case StaticMember::Forbidden:
            reportError(Exception::propertyAccessError, "Forbidden access", engine->errorPos());
            break;
        case StaticMember::Variable:
            *rval = (checked_cast<Variable *>(m))->value;
            return true;
        case StaticMember::HoistedVariable:
            *rval = (checked_cast<HoistedVar *>(m))->value;
            return true;
        case StaticMember::ConstructorMethod:
            break;
        case StaticMember::Accessor:
            break;
        }
        NOT_REACHED("Bad member kind");
        return false;
    }

    // Write the static member
    bool JS2Metadata::writeStaticMember(StaticMember *m, js2val newValue, Phase phase)
    {
        if (m == NULL)
            return false;   // 'None'
        switch (m->kind) {
        case StaticMember::Forbidden:
        case StaticMember::ConstructorMethod:
            reportError(Exception::propertyAccessError, "Forbidden access", engine->errorPos());
            break;
        case StaticMember::Variable:
            (checked_cast<Variable *>(m))->value = newValue;
            return true;
        case StaticMember::HoistedVariable:
            (checked_cast<HoistedVar *>(m))->value = newValue;
            return true;
        case StaticMember::Accessor:
            break;
        }
        NOT_REACHED("Bad member kind");
        return false;
    }

    // Read the value of a property in the container. Return true/false if that container has
    // the property or not. If it does, return it's value
    bool JS2Metadata::readProperty(js2val containerVal, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval)
    {
        bool isDynamicInstance = false;
        if (JS2VAL_IS_PRIMITIVE(containerVal)) {
readClassProperty:
            JS2Class *c = objectType(containerVal);
            InstanceBinding *ib = resolveInstanceMemberName(c, multiname, ReadAccess, phase);
            if ((ib == NULL) && isDynamicInstance) 
                return readDynamicProperty(JS2VAL_TO_OBJECT(containerVal), multiname, lookupKind, phase, rval);
            else 
                // XXX passing a primitive here ???
                return readInstanceMember(containerVal, c, (ib) ? &ib->qname : NULL, phase, rval);
        }
        JS2Object *container = JS2VAL_TO_OBJECT(containerVal);
        switch (container->kind) {
        case AttributeObjectKind:
        case MultinameKind:
        case FixedInstanceKind: 
            goto readClassProperty;
        case DynamicInstanceKind:
            isDynamicInstance = true;
            goto readClassProperty;

        case SystemKind:
        case GlobalObjectKind: 
        case PackageKind:
        case ParameterKind: 
        case BlockKind: 
            return readProperty(checked_cast<Frame *>(container), multiname, lookupKind, phase, rval);

        case ClassKind:
            {
                // this:
                // JS2VAL_UNINITIALIZED --> generic
                // JS2VAL_NULL --> none
                // JS2VAL_VOID --> inaccessible
                // 
                js2val thisObject;
                if (lookupKind->isPropertyLookup()) 
                    thisObject = JS2VAL_UNINITIALIZED;
                else
                    thisObject = lookupKind->thisObject;
                MemberDescriptor m2;
                if (findStaticMember(checked_cast<JS2Class *>(container), multiname, ReadAccess, phase, &m2) && m2.staticMember)
                    return readStaticMember(m2.staticMember, phase, rval);
                else {
                    if (JS2VAL_IS_NULL(thisObject))
                        reportError(Exception::propertyAccessError, "Null 'this' object", engine->errorPos());
                    if (JS2VAL_IS_VOID(thisObject))
                        reportError(Exception::compileExpressionError, "Undefined 'this' object", engine->errorPos());
                    if (JS2VAL_IS_UNINITIALIZED(thisObject)) {
                        // 'this' is {generic}
                        // XXX is ??? in spec.
                    }
                    return readInstanceMember(thisObject, objectType(thisObject), m2.qname, phase, rval);
                }
            }

        case PrototypeInstanceKind: 
            return readDynamicProperty(container, multiname, lookupKind, phase, rval);

        default:
            ASSERT(false);
            return false;
        }
    }

    // Use the slotIndex from the instanceVariable to access the slot
    Slot *JS2Metadata::findSlot(js2val thisObjVal, InstanceVariable *id)
    {
        ASSERT(JS2VAL_IS_OBJECT(thisObjVal) 
                    && ((JS2VAL_TO_OBJECT(thisObjVal)->kind == DynamicInstanceKind)
                        || (JS2VAL_TO_OBJECT(thisObjVal)->kind == FixedInstanceKind)));
        JS2Object *thisObj = JS2VAL_TO_OBJECT(thisObjVal);
        if (thisObj->kind == DynamicInstanceKind)
            return &checked_cast<DynamicInstance *>(thisObj)->slots[id->slotIndex];
        else
            return &checked_cast<FixedInstance *>(thisObj)->slots[id->slotIndex];
    }

    // Read the value of an instanceMember, if valid
    bool JS2Metadata::readInstanceMember(js2val containerVal, JS2Class *c, QualifiedName *qname, Phase phase, js2val *rval)
    {
        InstanceMember *m = findInstanceMember(c, qname, ReadAccess);
        if (m == NULL) return false;
        switch (m->kind) {
        case InstanceMember::InstanceVariableKind:
            {
                InstanceVariable *mv = checked_cast<InstanceVariable *>(m);
                if ((phase == CompilePhase) && !mv->immutable)
                    reportError(Exception::compileExpressionError, "Inappropriate compile time expression", engine->errorPos());
                Slot *s = findSlot(containerVal, mv);
                if (JS2VAL_IS_UNINITIALIZED(s->value))
                    reportError(Exception::uninitializedError, "Reference to uninitialized instance variable", engine->errorPos());
                *rval = s->value;
                return true;
            }
            break;

        case InstanceMember::InstanceMethodKind:
        case InstanceMember::InstanceAccessorKind:
            break;
        }
        ASSERT(false);
        return false;
    }

    // Read the value of a property in the frame. Return true/false if that frame has
    // the property or not. If it does, return it's value
    bool JS2Metadata::readProperty(Frame *container, Multiname *multiname, LookupKind *lookupKind, Phase phase, js2val *rval)
    {
        StaticMember *m = findFlatMember(container, multiname, ReadAccess, phase);
        if (!m && (container->kind == GlobalObjectKind))
            return readDynamicProperty(container, multiname, lookupKind, phase, rval);
        else
            return readStaticMember(m, phase, rval);
    }

    // Write the value of a property in the container. Return true/false if that container has
    // the property or not.
    bool JS2Metadata::writeProperty(js2val containerVal, Multiname *multiname, LookupKind *lookupKind, bool createIfMissing, js2val newValue, Phase phase)
    {
        JS2Class *c = NULL;
        if (JS2VAL_IS_PRIMITIVE(containerVal))
            return false;
        JS2Object *container = JS2VAL_TO_OBJECT(containerVal);
        switch (container->kind) {
        case AttributeObjectKind:
        case MultinameKind:
            return false;

        case FixedInstanceKind:
            c = checked_cast<FixedInstance *>(container)->type;
            goto instanceWrite;
        case DynamicInstanceKind:
            c = checked_cast<DynamicInstance *>(container)->type;
            goto instanceWrite;
    instanceWrite:
            {
                InstanceBinding *ib = resolveInstanceMemberName(c, multiname, WriteAccess, phase);
                if ((ib == NULL) && (container->kind == DynamicInstanceKind))
                    return writeDynamicProperty(container, multiname, createIfMissing, newValue, phase);
                else
                    return writeInstanceMember(containerVal, c, (ib) ? &ib->qname : NULL, newValue, phase); 
            }

        case SystemKind:
        case GlobalObjectKind: 
        case PackageKind:
        case ParameterKind: 
        case BlockKind: 
            return writeProperty(checked_cast<Frame *>(container), multiname, lookupKind, createIfMissing, newValue, phase);

        case ClassKind:
            {
                // this:
                // JS2VAL_UNINITIALIZED --> generic
                // JS2VAL_NULL --> none
                // JS2VAL_VOID --> inaccessible
                // 
                js2val thisObject;
                if (lookupKind->isPropertyLookup()) 
                    thisObject = JS2VAL_UNINITIALIZED;
                else
                    thisObject = lookupKind->thisObject;
                MemberDescriptor m2;
                if (findStaticMember(checked_cast<JS2Class *>(container), multiname, WriteAccess, phase, &m2) && m2.staticMember)
                    return writeStaticMember(m2.staticMember, newValue, phase);
                else {
                    if (JS2VAL_IS_NULL(thisObject))
                        reportError(Exception::propertyAccessError, "Null 'this' object", engine->errorPos());
                    if (JS2VAL_IS_VOID(thisObject))
                        reportError(Exception::compileExpressionError, "Undefined 'this' object", engine->errorPos());
                    if (JS2VAL_IS_UNINITIALIZED(thisObject)) {
                        // 'this' is {generic}
                        // XXX is ??? in spec.
                    }
                    return writeInstanceMember(thisObject, objectType(thisObject), m2.qname, newValue, phase);
                }
            }
            break;

        case PrototypeInstanceKind: 
            return writeDynamicProperty(container, multiname, createIfMissing, newValue, phase);

        default:
            ASSERT(false);
            return false;
        }

    }

    bool JS2Metadata::writeInstanceMember(js2val containerVal, JS2Class *c, QualifiedName *qname, js2val newValue, Phase phase)
    {
        if (phase == CompilePhase)
            reportError(Exception::compileExpressionError, "Inappropriate compile time expression", engine->errorPos());
        InstanceMember *m = findInstanceMember(c, qname, WriteAccess);
        if (m == NULL) return false;
        switch (m->kind) {
        case InstanceMember::InstanceVariableKind:
            {
                InstanceVariable *mv = checked_cast<InstanceVariable *>(m);
                Slot *s = findSlot(containerVal, mv);
                if (mv->immutable && JS2VAL_IS_INITIALIZED(s->value))
                    reportError(Exception::propertyAccessError, "Reinitialization of constant", engine->errorPos());
                s->value = engine->assignmentConversion(newValue, mv->type);
                return true;
            }
        
        case InstanceMember::InstanceMethodKind:
        case InstanceMember::InstanceAccessorKind:
            reportError(Exception::propertyAccessError, "Attempt to write to a method", engine->errorPos());
            break;
        }
        ASSERT(false);
        return false;

    }

    // Write the value of a property in the frame. Return true/false if that frame has
    // the property or not.
    bool JS2Metadata::writeProperty(Frame *container, Multiname *multiname, LookupKind *lookupKind, bool createIfMissing, js2val newValue, Phase phase)
    {
        StaticMember *m = findFlatMember(container, multiname, WriteAccess, phase);
        if (!m && (container->kind == GlobalObjectKind))
            return writeDynamicProperty(container, multiname, createIfMissing, newValue, phase);
        else
            return writeStaticMember(m, newValue, phase);
    }

    // Find a binding in the frame that matches the multiname and access
    // It's an error if more than one such binding exists.
    StaticMember *JS2Metadata::findFlatMember(Frame *container, Multiname *multiname, Access access, Phase phase)
    {
        StaticMember *found = NULL;
        StaticBindingIterator b, end;
        if (access & ReadAccess) {
            b = container->staticReadBindings.lower_bound(multiname->name);
            end = container->staticReadBindings.upper_bound(multiname->name);
        }
        else {
            b = container->staticWriteBindings.lower_bound(multiname->name);
            end = container->staticWriteBindings.upper_bound(multiname->name);
        }
        while (true) {
            if (b == end) {
                if (access == ReadWriteAccess) {
                    access = WriteAccess;
                    b = container->staticWriteBindings.lower_bound(multiname->name);
                    end = container->staticWriteBindings.upper_bound(multiname->name);
                    continue;
                }
                else
                    break;
            }
            if (multiname->matches(b->second->qname)) {
                if (found && (b->second->content != found))
                    reportError(Exception::propertyAccessError, "Ambiguous reference to {0}", engine->errorPos(), multiname->name);
                else
                    found = b->second->content;
            }
            b++;
        }
        return found;
    }


    bool JS2Metadata::findStaticMember(JS2Class *c, Multiname *multiname, Access access, Phase phase, MemberDescriptor *result)
    {
        JS2Class *s = c;
        while (s) {
            StaticBindingIterator b, end;
            if (access & ReadAccess) {
                b = s->staticReadBindings.lower_bound(multiname->name);
                end = s->staticReadBindings.upper_bound(multiname->name);
            }
            else {
                b = s->staticWriteBindings.lower_bound(multiname->name);
                end = s->staticWriteBindings.upper_bound(multiname->name);
            }
            StaticMember *found = NULL;
            while (b != end) {
                if (multiname->matches(b->second->qname)) {
                    if (found && (b->second->content != found))
                        reportError(Exception::propertyAccessError, "Ambiguous reference to {0}", engine->errorPos(), multiname->name);
                    else
                        found = b->second->content;
                }
                b++;
            }
            if (found) {
                result->staticMember = found;
                result->qname = NULL;
                return true;
            }

            InstanceBinding *iFound = NULL;
            InstanceBindingIterator ib, iend;
            if (access & ReadAccess) {
                ib = s->instanceReadBindings.lower_bound(multiname->name);
                iend = s->instanceReadBindings.upper_bound(multiname->name);
            }
            else {
                ib = s->instanceWriteBindings.lower_bound(multiname->name);
                iend = s->instanceWriteBindings.upper_bound(multiname->name);
            }
            while (ib != iend) {
                if (multiname->matches(ib->second->qname)) {
                    if (iFound && (ib->second->content != iFound->content))
                        reportError(Exception::propertyAccessError, "Ambiguous reference to {0}", engine->errorPos(), multiname->name);
                    else
                        iFound = ib->second;
                }
                b++;
            }
            if (iFound) {
                result->staticMember = NULL;
                result->qname = &iFound->qname;
                return true;
            }
            s = s->super;
        }
        return false;
    }

    /*
    * Start from the root class (Object) and proceed through more specific classes that are ancestors of c.
    * Find the binding that matches the given access and multiname, it's an error if more than one such exists.
    *
    */
    InstanceBinding *JS2Metadata::resolveInstanceMemberName(JS2Class *c, Multiname *multiname, Access access, Phase phase)
    {
        InstanceBinding *result = NULL;
        if (c->super) {
            result = resolveInstanceMemberName(c->super, multiname, access, phase);
            if (result) return result;
        }
        InstanceBindingIterator b, end;
        if (access & ReadAccess) {
            b = c->instanceReadBindings.lower_bound(multiname->name);
            end = c->instanceReadBindings.upper_bound(multiname->name);
        }
        else {
            b = c->instanceWriteBindings.lower_bound(multiname->name);
            end = c->instanceWriteBindings.upper_bound(multiname->name);
        }
        while (true) {
            if (b == end) {
                if (access == ReadWriteAccess) {
                    access = WriteAccess;
                    b = c->instanceWriteBindings.lower_bound(multiname->name);
                    end = c->instanceWriteBindings.upper_bound(multiname->name);
                    continue;
                }
                else
                    break;
            }
            if (multiname->matches(b->second->qname)) {
                if (result && (b->second->content != result->content))
                    reportError(Exception::propertyAccessError, "Ambiguous reference to {0}", engine->errorPos(), multiname->name);
                else
                    result = b->second;
            }
            b++;
        }
        return result;
    }

    /*
     * Throw an exception of the specified kind, indicating the position 'pos' and
     * attaching the given message. If 'arg' is specified, replace {0} in the message
     * with the argument value. [This is intended to be widened into a more complete
     * argument handling scheme].
     */
    void JS2Metadata::reportError(Exception::Kind kind, const char *message, size_t pos, const char *arg)
    {
        const char16 *lineBegin;
        const char16 *lineEnd;
        String x = widenCString(message);
        if (arg) {
            // XXX handle multiple occurences and extend to {1} etc.
            uint32 a = x.find(widenCString("{0}"));
            x.replace(a, 3, widenCString(arg));
        }
        uint32 lineNum = mParser->lexer.reader.posToLineNum(pos);
        size_t linePos = mParser->lexer.reader.getLine(lineNum, lineBegin, lineEnd);
        ASSERT(lineBegin && lineEnd && linePos <= pos);
        throw Exception(kind, x, mParser->lexer.reader.sourceLocation, 
                            lineNum, pos - linePos, pos, lineBegin, lineEnd);
    }

    inline char narrow(char16 ch) { return char(ch); }
    // Accepts a String as the error argument and converts to char *
    void JS2Metadata::reportError(Exception::Kind kind, const char *message, size_t pos, const String& name)
    {
        std::string str(name.length(), char());
        std::transform(name.begin(), name.end(), str.begin(), narrow);
        reportError(kind, message, pos, str.c_str());
    }

/************************************************************************************
 *
 *  Pond
 *
 ************************************************************************************/

    Pond::Pond(size_t sz, Pond *next) : sanity(POND_SANITY), pondSize(sz + POND_SIZE), pondBase(new uint8[pondSize]), pondTop(pondBase), freeHeader(NULL), nextPond(next) 
    { 
    }
    
    // Allocate from this or the next Pond (make a new one if necessary)
    void *Pond::allocFromPond(int32 sz)
    {
        // Try scannning the free list...
        PondScum *p = freeHeader;
        PondScum *pre = NULL;
        while (p) {
            ASSERT(p->getSize() > 0);
            if (p->getSize() >= sz) {
                if (pre)
                    pre->owner = p->owner;
                else
                    freeHeader = (PondScum *)(p->owner);
                p->owner = this;
                return (p + 1);
            }
            pre = p;
            p = (PondScum *)(p->owner);
        }

        // See if there's room left...
        if (sz > (pondSize - (pondTop - pondBase))) {
            if (nextPond == NULL)
                nextPond = new Pond(sz, nextPond);
            return nextPond->allocFromPond(sz);
        }
        p = (PondScum *)pondTop;
        p->owner = this;
        p->setSize(sz);
        pondTop += sz;
#ifdef DEBUG
        memset((p + 1), 0xB7, sz - sizeof(PondScum));
#endif
        return (p + 1);
    }

    // Stick the chunk at the start of the free list
    void Pond::returnToPond(PondScum *p)
    {
        p->owner = (Pond *)freeHeader;
        p->resetMark();      // might have lingering mark from previous gc
        uint8 *t = (uint8 *)(p + 1);
#ifdef DEBUG
        memset(t, 0xB7, p->getSize() - sizeof(PondScum));
#endif
        freeHeader = p;
    }

    // Clear the mark bit from all PondScums
    void Pond::resetMarks()
    {
        uint8 *t = pondBase;
        while (t != pondTop) {
            PondScum *p = (PondScum *)t;
            p->resetMark();
            t += p->getSize();
        }
        if (nextPond)
            nextPond->resetMarks();
    }

    // Anything left unmarked is now moved to the free list
    void Pond::moveUnmarkedToFreeList()
    {
        uint8 *t = pondBase;
        while (t != pondTop) {
            PondScum *p = (PondScum *)t;
            if (!p->isMarked())
                returnToPond(p);
            t += p->getSize();
        }
        if (nextPond)
            nextPond->moveUnmarkedToFreeList();
    }


 /************************************************************************************
 *
 *  JS2Class
 *
 ************************************************************************************/


    JS2Class::JS2Class(JS2Class *super, JS2Object *proto, Namespace *privateNamespace, bool dynamic, bool allowNull, bool final, const StringAtom &name) 
        : Frame(ClassKind), 
            instanceInitOrder(NULL), 
            complete(false), 
            super(super), 
            prototype(proto), 
            privateNamespace(privateNamespace), 
            dynamic(dynamic),
            allowNull(allowNull),
            final(final),
            call(NULL),
            construct(JS2Engine::defaultConstructor),
            slotCount(0),
            name(name)
    {

    }

 
 /************************************************************************************
 *
 *  DynamicInstance
 *
 ************************************************************************************/


    DynamicInstance::DynamicInstance(JS2Class *type) 
        : JS2Object(DynamicInstanceKind), 
            type(type), 
            call(NULL), 
            construct(NULL), 
            env(NULL), 
            typeofString(type->getName()),
            slots(new Slot[type->slotCount])
    {
        for (uint32 i = 0; i < type->slotCount; i++) {
            slots[i].value = JS2VAL_UNINITIALIZED;
        }
    }

 
 /************************************************************************************
 *
 *  FixedInstance
 *
 ************************************************************************************/


    FixedInstance::FixedInstance(JS2Class *type) 
        : JS2Object(FixedInstanceKind), 
            type(type), 
            call(NULL), 
            construct(NULL), 
            env(NULL), 
            typeofString(type->getName()),
            slots(new Slot[type->slotCount])
    {
        for (uint32 i = 0; i < type->slotCount; i++) {
            slots[i].value = JS2VAL_UNINITIALIZED;
        }
    }


 /************************************************************************************
 *
 *  ParameterFrame
 *
 ************************************************************************************/

    void ParameterFrame::instantiate(Environment *env)
    {
        env->instantiateFrame(pluralFrame, this);
    }


 /************************************************************************************
 *
 *  BlockFrame
 *
 ************************************************************************************/

    void BlockFrame::instantiate(Environment *env)
    {
        if (pluralFrame)
            env->instantiateFrame(pluralFrame, this);
    }


/************************************************************************************
 *
 *  JS2Object
 *
 ************************************************************************************/

    Pond JS2Object::pond(POND_SIZE, NULL);
    std::vector<PondScum **> JS2Object::rootList;

    void JS2Object::addRoot(void *t)
    {
        PondScum **p = (PondScum **)t;
        ASSERT(p);
        rootList.push_back(p);
    }

    void JS2Object::gc()
    {
        pond.resetMarks();
        for (std::vector<PondScum **>::iterator i = rootList.begin(), end = rootList.end(); (i != end); i++) {
            PondScum *p = **i;
            if (p) {
                ASSERT(p->owner && (p->getSize() >= sizeof(PondScum)) && (p->owner->sanity == POND_SANITY));
                p->mark();
                // now examine the object contained in the PondScum and mark
                // all gc objects referenced from it
            }
        }
        pond.moveUnmarkedToFreeList();
    }

    void *JS2Object::alloc(size_t s)
    {
        s += sizeof(PondScum);
        // make sure that the thing is 8-byte aligned
        if (s & 0x7) s += 8 - (s & 0x7);
        ASSERT(s <= 0x7FFFFFFF);
        return pond.allocFromPond((int32)s);
    }

    void JS2Object::unalloc(void *t)
    {
        PondScum *p = (PondScum *)t - 1;
        ASSERT(p->owner && (p->getSize() >= sizeof(PondScum)) && (p->owner->sanity == POND_SANITY));
        p->owner->returnToPond(p);
    }

}; // namespace MetaData
}; // namespace Javascript