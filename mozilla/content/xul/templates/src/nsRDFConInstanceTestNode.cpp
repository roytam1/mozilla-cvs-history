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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chris Waterson <waterson@netscape.com>
 */

#include "nsConflictSet.h"
#include "nsIComponentManager.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsRDFConInstanceTestNode.h"
#include "nsResourceSet.h"
#include "nsString.h"

#include "prlog.h"
#ifdef PR_LOGGING
#include "nsXULContentUtils.h"
extern PRLogModuleInfo* gXULTemplateLog;

static const char*
TestToString(nsRDFConInstanceTestNode::Test aTest) {
    switch (aTest) {
    case nsRDFConInstanceTestNode::eFalse:    return "false";
    case nsRDFConInstanceTestNode::eTrue:     return "true";
    case nsRDFConInstanceTestNode::eDontCare: return "dontcare";
    }
    return "?";
}
#endif

nsRDFConInstanceTestNode::nsRDFConInstanceTestNode(InnerNode* aParent,
                                                   nsConflictSet& aConflictSet,
                                                   nsIRDFDataSource* aDataSource,
                                                   const nsResourceSet& aMembershipProperties,
                                                   PRInt32 aContainerVariable,
                                                   Test aContainer,
                                                   Test aEmpty)
    : nsRDFTestNode(aParent),
      mConflictSet(aConflictSet),
      mDataSource(aDataSource),
      mMembershipProperties(aMembershipProperties),
      mContainerVariable(aContainerVariable),
      mContainer(aContainer),
      mEmpty(aEmpty)
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsCAutoString props;

        nsResourceSet::ConstIterator last = aMembershipProperties.Last();
        nsResourceSet::ConstIterator first = aMembershipProperties.First();
        nsResourceSet::ConstIterator iter;

        for (iter = first; iter != last; ++iter) {
            if (iter != first)
                props += " ";

            const char* str;
            iter->GetValueConst(&str);

            props += str;
        }

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFConInstanceTestNode[%p]: parent=%p member-props=(%s) container-var=%d container=%s empty=%s",
                this,
                aParent,
                props.get(),
                mContainerVariable,
                TestToString(aContainer),
                TestToString(aEmpty)));
    }
#endif
}

nsresult
nsRDFConInstanceTestNode::FilterInstantiations(InstantiationSet& aInstantiations, void* aClosure) const
{
    nsresult rv;

    nsCOMPtr<nsIRDFContainerUtils> rdfc
        = do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {
        Value value;
        if (! inst->mAssignments.GetAssignmentFor(mContainerVariable, &value)) {
            NS_ERROR("can't do unbounded container testing");
            return NS_ERROR_UNEXPECTED;
        }

#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            const char* container;
            VALUE_TO_IRDFRESOURCE(value)->GetValueConst(&container);

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("nsRDFConInstanceTestNode[%p]::FilterInstantiations() container=[%s]",
                    this, container));
        }
#endif

        nsCOMPtr<nsIRDFContainer> rdfcontainer;

        PRBool isRDFContainer;
        rv = rdfc->IsContainer(mDataSource, VALUE_TO_IRDFRESOURCE(value), &isRDFContainer);
        if (NS_FAILED(rv)) return rv;

        // If they've asked us to test for emptiness, do that first
        // because it doesn't require us to create any enumerators.
        if (mEmpty != eDontCare) {
            Test empty;

            if (isRDFContainer) {
                // It's an RDF container. Use the container utilities
                // to deduce what's in it.
                //
                // XXX should cache the factory
                rdfcontainer = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
                if (NS_FAILED(rv)) return rv;

                rv = rdfcontainer->Init(mDataSource, VALUE_TO_IRDFRESOURCE(value));
                if (NS_FAILED(rv)) return rv;

                PRInt32 count;
                rv = rdfcontainer->GetCount(&count);
                if (NS_FAILED(rv)) return rv;

                empty = (count == 0) ? eTrue : eFalse;
            }
            else {
                empty = eTrue;

                for (nsResourceSet::ConstIterator property = mMembershipProperties.First();
                     property != mMembershipProperties.Last();
                     ++property) {
                    nsCOMPtr<nsIRDFNode> target;
                    rv = mDataSource->GetTarget(VALUE_TO_IRDFRESOURCE(value), *property, PR_TRUE, getter_AddRefs(target));
                    if (NS_FAILED(rv)) return rv;

                    if (target != nsnull) {
                        // bingo. we found one.
                        empty = eFalse;
                        break;
                    }
                }
            }

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("    empty => %s",
                    (empty == mEmpty) ? "consistent" : "inconsistent"));

            if (empty == mEmpty) {
                Element* element =
                    nsRDFConInstanceTestNode::Element::Create(mConflictSet.GetPool(),
                                                              VALUE_TO_IRDFRESOURCE(value),
                                                              mContainer, mEmpty);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                inst->AddSupportingElement(element);
            }
            else {
                aInstantiations.Erase(inst--);
            }
        }
        else if (mContainer != eDontCare) {
            // We didn't care about emptiness, only containerhood.
            Test container;

            if (isRDFContainer) {
                // Wow, this is easy.
                container = eTrue;
            }
            else {
                // Okay, suckage. We need to look at all of the arcs
                // leading out of the thing, and see if any of them
                // are properties that are deemed as denoting
                // containerhood.
                container = eFalse;

                nsCOMPtr<nsISimpleEnumerator> arcsout;
                rv = mDataSource->ArcLabelsOut(VALUE_TO_IRDFRESOURCE(value), getter_AddRefs(arcsout));
                if (NS_FAILED(rv)) return rv;

                while (1) {
                    PRBool hasmore;
                    rv = arcsout->HasMoreElements(&hasmore);
                    if (NS_FAILED(rv)) return rv;

                    if (! hasmore)
                        break;

                    nsCOMPtr<nsISupports> isupports;
                    rv = arcsout->GetNext(getter_AddRefs(isupports));
                    if (NS_FAILED(rv)) return rv;

                    nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
                    NS_ASSERTION(property != nsnull, "not a property");
                    if (! property)
                        return NS_ERROR_UNEXPECTED;

                    if (mMembershipProperties.Contains(property)) {
                        container = eTrue;
                        break;
                    }
                }
            }

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("    container => %s",
                    (container == mContainer) ? "consistent" : "inconsistent"));

            if (container == mContainer) {
                Element* element =
                    nsRDFConInstanceTestNode::Element::Create(mConflictSet.GetPool(),
                                                              VALUE_TO_IRDFRESOURCE(value),
                                                              mContainer, mEmpty);


                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                inst->AddSupportingElement(element);
            }
            else {
                aInstantiations.Erase(inst--);
            }
        }
    }

    return NS_OK;
}

nsresult
nsRDFConInstanceTestNode::GetAncestorVariables(VariableSet& aVariables) const
{
    nsresult rv;

    rv = aVariables.Add(mContainerVariable);
    if (NS_FAILED(rv)) return rv;

    return TestNode::GetAncestorVariables(aVariables);
}

PRBool
nsRDFConInstanceTestNode::CanPropogate(nsIRDFResource* aSource,
                                       nsIRDFResource* aProperty,
                                       nsIRDFNode* aTarget,
                                       Instantiation& aInitialBindings) const
{
    nsresult rv;

    PRBool canpropogate = PR_FALSE;

    nsCOMPtr<nsIRDFContainerUtils> rdfc
        = do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    // We can certainly propogate ordinal properties
    rv = rdfc->IsOrdinalProperty(aProperty, &canpropogate);
    if (NS_FAILED(rv)) return PR_FALSE;

    if (! canpropogate) {
        canpropogate = mMembershipProperties.Contains(aProperty);
    }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* source;
        aSource->GetValueConst(&source);

        const char* property;
        aProperty->GetValueConst(&property);

        nsAutoString target;
        nsXULContentUtils::GetTextForNode(aTarget, target);

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFConInstanceTestNode[%p]: CanPropogate([%s]==[%s]=>[%s]) => %s",
                this, source, property, NS_ConvertUCS2toUTF8(target).get(),
                canpropogate ? "true" : "false"));
    }
#endif

    if (canpropogate) {
        aInitialBindings.AddAssignment(mContainerVariable, Value(aSource));
        return PR_TRUE;
    }

    return PR_FALSE;
}

void
nsRDFConInstanceTestNode::Retract(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget,
                                  nsTemplateMatchSet& aFirings,
                                  nsTemplateMatchSet& aRetractions) const
{
    // XXXwaterson oof. complicated. figure this out.
    if (0) {
        mConflictSet.Remove(Element(aSource, mContainer, mEmpty), aFirings, aRetractions);
    }
}

