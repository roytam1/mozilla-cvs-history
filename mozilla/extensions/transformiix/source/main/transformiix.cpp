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
 *
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Nathan Pride, npride@wavo.com
 *    -- fixed document base when stylesheet is specified,
 *       it was defaulting to the XML document.
 *
 * $Id$
 */


#include "XSLTProcessor.h"

  //--------------/
 //- Prototypes -/
//--------------/

/**
 * Prints the command line help screen to the console
**/
void printHelp();

/**
 * prints the command line usage information to the console
**/
void printUsage();

/**
 * The TransforMiiX command line interface
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
**/
int main(int argc, char** argv) {

    XSLTProcessor xsltProcessor;

    String copyright("(C) 1999 The MITRE Corporation, Keith Visco, and contributors");
    cout << xsltProcessor.getAppName() << " ";
    cout << xsltProcessor.getAppVersion() << endl;
    cout << copyright <<endl;

    //-- print banner line
    Int32 fillSize = 1;
    fillSize += copyright.length();
    String fill;
    fill.setLength(fillSize, '-');
    cout << fill <<endl<<endl;

    //-- add ErrorObserver
    SimpleErrorObserver seo;
    xsltProcessor.addErrorObserver(seo);

    //-- available flags
    StringList flags;
    flags.add(new String("i"));          // XML input
    flags.add(new String("s"));          // XSL input
    flags.add(new String("o"));          // Output filename

    NamedMap options;
    options.setObjectDeletion(MB_TRUE);
    CommandLineUtils::getOptions(options, argc, argv, flags);

    if ( options.get("h") ) {
        printHelp();
        return 0;
    }
    String* xmlFilename = (String*)options.get("i");
    String* xsltFilename = (String*)options.get("s");
    String* outFilename = (String*)options.get("o");

    if ( !xmlFilename ) {
        cout << "error: missing XML filename."<<endl <<endl;
        printUsage();
        return -1;
    }
    char* chars = 0;

    //-- open XML file
    chars = new char[xmlFilename->length()+1];
    ifstream xmlInput(xmlFilename->toCharArray(chars), ios::in);
    delete chars;

    String documentBase;

    //-- handle output stream
    ostream* resultOutput = &cout;
    ofstream resultFileStream;
    if ( outFilename ) {
        chars = new char[outFilename->length()+1];
        resultFileStream.open(outFilename->toCharArray(chars), ios::out);
        delete chars;
        if ( !resultFileStream ) {
            cout << "error opening output file: " << *xmlFilename << endl;
            return -1;
        }
        resultOutput = &resultFileStream;
    }
    //-- process
    if ( !xsltFilename ) {
        //-- use xml document to obtain a document base
        URIUtils::getDocumentBase(*xmlFilename, documentBase);
        xsltProcessor.process(xmlInput, *resultOutput, documentBase);
    }
    else {
        //-- open XSLT file
        chars = new char[xsltFilename->length()+1];
        ifstream xsltInput(xsltFilename->toCharArray(chars), ios::in);
        delete chars;
        // obtain document base from XSLT stylesheet
        URIUtils::getDocumentBase(*xmlFilename, documentBase);
        xsltProcessor.process(xmlInput, xsltInput, *resultOutput, documentBase);
    }
    resultFileStream.close();
    return 0;
} //-- main

void printHelp() {
    cout << "The following flags are available for use with Transformiix -";
    cout<<endl<<endl;
    cout << "-i  filename  : The XML file to process" << endl;
    cout << "-o  filename  : The Output file to create" << endl;
    cout << "-s  filename  : The XSLT file to use for processing  (Optional)" << endl;
    cout << "-h            : This help screen                     (Optional)" << endl;
    cout << endl;
}
void printUsage() {
    cout << endl;
    cout << "usage: ";
    cout << "transfrmx -i xml-file [-s xslt-file] [-o output-file]"<<endl;
    cout << endl;
    cout << "for more infomation use the -h flag"<<endl;
} //-- printUsage

