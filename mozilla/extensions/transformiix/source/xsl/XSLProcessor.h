/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * $Id$
 */


#ifndef MITREXSL_XSLPROCESSOR_H
#define MITREXSL_XSLPROCESSOR_H

#ifndef __BORLANDC__
#include <iostream.h>
#include <fstream.h>
#endif
#include "CommandLineUtils.h"
#include "dom.h"
#include "ExprParser.h"
#include "MITREObject.h"
#include "NamedMap.h"
#include "Names.h"
#include "NodeSet.h"
#include "printers.h"
#include "ProcessorState.h"
#include "String.h"
#include "Tokenizer.h"
#include "URIUtils.h"
#include "XMLDOMUtils.h"
#include "XMLParser.h"
#include "ErrorObserver.h"
#include "List.h"
#include "VariableBinding.h"
#include "Numbering.h"

/**
 * A class for Processing XSL Stylesheets
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
 * @version $Revision$ $Date$
**/
class XSLProcessor {

public:

    /**
     * A warning message used by all templates that do not allow non character
     * data to be generated
    **/
    static const String NON_TEXT_TEMPLATE_WARNING;

    /**
     * Creates a new XSLProcessor
    **/
    XSLProcessor();

    /**
     * Default destructor for XSLProcessor
    **/
    ~XSLProcessor();

    /**
     * Registers the given ErrorObserver with this XSLProcessor
    **/
    void addErrorObserver(ErrorObserver& errorObserver);

    /**
     * Returns the name of this XSL processor
    **/
    String& getAppName();

    /**
     * Returns the version of this XSL processor
    **/
    String& getAppVersion();

      //--------------------------------------------/
     //-- Methods that return the Result Document -/
    //--------------------------------------------/

    /**
     * Parses all XML Stylesheet PIs associated with the
     * given XML document. If any stylesheet PIs are found with
     * type="text/xsl" the href psuedo attribute value will be
     * added to the given href argument. If multiple text/xsl stylesheet PIs
     * are found, the one closest to the end of the document is used.
    **/
    void getHrefFromStylesheetPI(Document& xmlDocument, String& href);

    /**
     * Processes the given XML Document, the XSL stylesheet
     * will be retrieved from the XML Stylesheet Processing instruction,
     * otherwise an empty document will be returned.
     * @param xmlDocument the XML document to process
     * @param documentBase the document base of the XML document, for
     * resolving relative URIs
     * @return the result tree.
    **/
    Document* process(Document& xmlDocument, String& documentBase);

    /**
     * Processes the given XML Document using the given XSL document
     * @return the result tree.
     * @param documentBase the document base for resolving relative URIs.
    **/
    Document* process
         (Document& xmlDocument, Document& xslDocument, String& documentBase);

    /**
     * Reads an XML Document from the given XML input stream, and
     * processes the document using the XSL document derived from
     * the given XSL input stream.
     * @param documentBase the document base for resolving relative URIs.
     * @return the result tree.
    **/
    Document* process(istream& xmlInput, istream& xslInput, 
           String& documentBase);

    /**
     * Reads an XML document from the given XML input stream. The
     * XML document is processed using the associated XSL document
     * retrieved from the XML document's Stylesheet Processing Instruction,
     * otherwise an empty document will be returned.
     * @param xmlDocument the XML document to process
     * @param documentBase the document base of the XML document, for
     * resolving relative URIs
     * @return the result tree.
    **/
    Document* process(istream& xmlInput, String& documentBase);

       //----------------------------------------------/
      //-- Methods that print the result to a stream -/
     //----------------------------------------------/

    /**
     * Reads an XML Document from the given XML input stream.
     * The XSL Stylesheet is obtained from the XML Documents stylesheet PI.
     * If no Stylesheet is found, an empty document will be the result;
     * otherwise the XML Document is processed using the stylesheet.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
    **/
    void process(istream& xmlInput, ostream& out, String& documentBase);

    /**
     * Processes the given XML Document using the given XSL document.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     * @param documentBase the document base for resolving relative URIs.
    **/
    void process
      (Document& xmlDocument, Document& xslDocument, 
         ostream& out, String& documentBase);

    /**
     * Reads an XML Document from the given XML input stream, and
     * processes the document using the XSL document derived from
     * the given XSL input stream.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     * @param documentBase the document base for resolving relative URIs.
    **/
    void process(istream& xmlInput, istream& xslInput, 
           ostream& out, String& documentBase);


private:


    /**
     * Application Name and version
    **/
    String appName;
    String appVersion;

    /**
     * The list of ErrorObservers
    **/
    List   errorObservers;

    /**
     * Fatal ErrorObserver
    **/
    SimpleErrorObserver fatalObserver;

    /**
     * An expression parser for creating AttributeValueTemplates
    **/
    ExprParser exprParser;

    /**
     * The version of XSL which this Processes
    **/
    String xslVersion;

    /**
     * Named Map for quick reference to XSL Types
    **/
    NamedMap xslTypes;

    /**
     * Binds the given Variable
    **/
    void bindVariable(String& name,
                      ExprResult* value,
                      MBool allowShadowing,
                      ProcessorState* ps);

    XMLPrinter* createPrinter(Document& xslDocument, ostream& out);


    /**
     * Processes the xsl:with-param elements of the given xsl action
    **/
    void processParameters(Element* xslAction, Node* context, ProcessorState* ps);

    /**
     * Looks up the given XSLType with the given name
     * The ProcessorState is used to get the current XSLT namespace
    **/
    short getElementType(String& name, ProcessorState* ps);


    /**
     * Gets the Text value of the given DocumentFragment. The value is placed
     * into the given destination String. If a non text node element is
     * encountered and warningForNonTextNodes is turned on, the MB_FALSE
     * will be returned, otherwise true is always returned.
     * @param dfrag the document fragment to get the text from
     * @param dest the destination string to place the text into.
     * @param deep indicates to process non text nodes and recusively append
     * their value. If this value is true, the allowOnlyTextNodes flag is ignored.
    **/
    MBool getText
        (DocumentFragment* dfrag, String& dest, MBool deep, MBool allowOnlyTextNodes);

    /**
     * Notifies all registered ErrorObservers of the given error
    **/
    void notifyError(const char* errorMessage);

    /**
     * Notifies all registered ErrorObservers of the given error
    **/
    void notifyError(String& errorMessage);

    /**
     * Notifies all registered ErrorObservers of the given error
    **/
    void notifyError(String& errorMessage, ErrorObserver::ErrorLevel level);

    /**
     * Parses the contents of data, and returns the type and href psuedo attributes
    **/
    void parseStylesheetPI(String& data, String& type, String& href);

    void process(Node* node, Node* context, ProcessorState* ps);
    void process(Node* node, Node* context, String* mode, ProcessorState* ps);

    void processAction(Node* node, Node* xslAction, ProcessorState* ps);

    /**
     * Processes the attribute sets specified in the names argument
    **/
    void processAttributeSets(const String& names, Node* node, ProcessorState* ps);

    /**
     * Processes the given attribute value as an AttributeValueTemplate
     * @param attValue the attribute value to process
     * @param result, the String in which to store the result
     * @param context the current context node
     * @param ps the current ProcessorState
    **/
    void processAttrValueTemplate
        (const String& attValue, String& result, Node* context, ProcessorState* ps);

    void processTemplate(Node* node, Node* xslTemplate, ProcessorState* ps);
    void processTemplateParams(Node* xslTemplate, Node* context, ProcessorState* ps);

    void processTopLevel(Document* xslDocument, ProcessorState* ps);

    ExprResult* processVariable(Node* node, Element* xslVariable, ProcessorState* ps);

    /**
     * Performs the xsl:copy action as specified in the XSL Working Draft
    **/
    void xslCopy(Node* node, Element* action, ProcessorState* ps);

    /**
     * Performs the xsl:copy-of action as specified in the XSL Working Draft
    **/
    void xslCopyOf(ExprResult* exprResult, ProcessorState* ps);

}; //-- XSLProcessor

class XSLType : public MITREObject {

public:
    enum _XSLType {
        APPLY_IMPORTS =  1,
        APPLY_TEMPLATES,
        ATTRIBUTE,
        ATTRIBUTE_SET,
        CALL_TEMPLATE,
        CHOOSE,
        COMMENT,
        COPY,
        COPY_OF,
        ELEMENT,
        IF,
        INCLUDE,
        FOR_EACH,
        LITERAL,
        NUMBER,
        OTHERWISE,
        PARAM,
        PI,
        PRESERVE_SPACE,
        STRIP_SPACE,
        TEMPLATE,
        TEXT,
        VALUE_OF,
        VARIABLE,
        WHEN,
        WITH_PARAM,
        MESSAGE,
        EXPR_DEBUG,  // temporary, used for debugging
    };

    XSLType(const XSLType& xslType);
    XSLType();
    XSLType(short type);
    short type;
};

#endif
