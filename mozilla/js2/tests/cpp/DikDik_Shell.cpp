// -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
//
// The contents of this file are subject to the Netscape Public
// License Version 1.1 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of
// the License at http://www.mozilla.org/NPL/
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
// The Original Code is the JavaScript 2 Prototype.
//
// The Initial Developer of the Original Code is Netscape
// Communications Corporation.  Portions created by Netscape are
// Copyright (C) 1998 Netscape Communications Corporation. All
// Rights Reserved.


//
// JS2 shell.
//

#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
 #pragma warning(disable: 4786)
#endif


#include <algorithm>
#include <assert.h>

#include "world.h"
#include "reader.h"
#include "parser.h"
#include "js2runtime.h"
#include "bytecodegen.h"

#if defined(XP_MAC) && !defined(XP_MAC_MPW)
#include <SIOUX.h>
#include <MacTypes.h>

static char *mac_argv[] = {"js2", 0};

static void initConsole(StringPtr consoleName,
                        const char* startupMessage,
                        int &argc, char **&argv)
{
    SIOUXSettings.autocloseonquit = false;
    SIOUXSettings.asktosaveonclose = false;
    SIOUXSetTitle(consoleName);

    // Set up a buffer for stderr (otherwise it's a pig).
    static char buffer[BUFSIZ];
    setvbuf(stderr, buffer, _IOLBF, BUFSIZ);

    JavaScript::stdOut << startupMessage;

    argc = 1;
    argv = mac_argv;
}

#endif

#ifndef _WIN32
using namespace JavaScript::JS2Runtime;
#endif

namespace JavaScript {
namespace Shell {

// Interactively read a line from the input stream in and put it into
// s. Return false if reached the end of input before reading anything.
static bool promptLine(LineReader &inReader, string &s, const char *prompt)
{
    if (prompt) {
        stdOut << prompt;
	  #ifdef XP_MAC_MPW
        // Print a CR after the prompt because MPW grabs the entire
        // line when entering an interactive command.
        stdOut << '\n';
	  #endif
    }
    return inReader.readLine(s) != 0;
}

World world;

/* "filename" of the console */
const String ConsoleName = widenCString("<console>");
const bool showTokens = false;

#define INTERPRET_INPUT 1
//#define SHOW_ICODE 1


JSValue load(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue result = kUndefinedValue;
    if ((argc >= 1) && (argv[0].isString())) {    
        const String& fileName = *argv[0].string;
        result = cx->readEvalFile(fileName);
    }    
    return result;
}
JSValue print(Context *cx, JSValue *argv, uint32 argc)
{
    for (uint32 i = 0; i < argc; i++) {
        stdOut << argv[i] << "\n";
    }
    return kUndefinedValue;
}
JSValue debug(Context *cx, JSValue *argv, uint32 argc)
{
    cx->mDebugFlag = !cx->mDebugFlag;
    return kUndefinedValue;
}


static void readEvalPrint(FILE *in, World &world)
{
            Arena a;
    String buffer;
    string line;
    LineReader inReader(in);

    JSObject globalObject;
    Context cx(&globalObject, world);

    while (promptLine(inReader, line, buffer.empty() ? "dd> " : "> ")) {
        appendChars(buffer, line.data(), line.size());
        try {
            Parser p(world, a, buffer, ConsoleName);
                
            if (showTokens) {
                Lexer &l = p.lexer;
                while (true) {
                    const Token &t = l.get(true);
                    if (t.hasKind(Token::end))
                        break;
                    stdOut << ' ';
                    t.print(stdOut, true);
                }
	            stdOut << '\n';
            } else {
                StmtNode *parsedStatements = p.parseProgram();
		ASSERT(p.lexer.peek(true).hasKind(Token::end));
#ifdef DUMP_PROGRAM                    
                {
                    PrettyPrinter f(stdOut, 30);
                    {
                	    PrettyPrinter::Block b(f, 2);
	                    f << "Program =";
	                    f.linearBreak(1);
	                    StmtNode::printStatements(f, parsedStatements);
                    }
                    f.end();

                }
        	stdOut << '\n';
#endif
#ifdef INTERPRET_INPUT
		// Generate code for parsedStatements, which is a linked 
                // list of zero or more statements
                globalObject.defineVariable(widenCString("load"), NULL, NULL, JSValue(new JSFunction(load, NULL)));
                globalObject.defineVariable(widenCString("print"), NULL, NULL, JSValue(new JSFunction(print, NULL)));
                globalObject.defineVariable(widenCString("debug"), NULL, NULL, JSValue(new JSFunction(debug, NULL)));

                cx.buildRuntime(parsedStatements);
//                stdOut << globalObject;
                JS2Runtime::ByteCodeModule* bcm = cx.genCode(parsedStatements, ConsoleName);
                if (bcm) {
#ifdef SHOW_ICODE
                    stdOut << *bcm;
#endif
                    JSValue result = cx.interpret(bcm, JSValueList());
//                    stdOut << "result = " << result << "\n";
                    if (!result.isUndefined())
                        stdOut << result << "\n";
                    delete bcm;
//                    stdOut << globalObject;
                }
#endif
            }
            clear(buffer);
        } catch (Exception &e) {
            /* If we got a syntax error on the end of input,
             * then wait for a continuation
             * of input rather than printing the error message. */
            if (!(e.hasKind(Exception::syntaxError) &&
                  e.lineNum && e.pos == buffer.size() &&
                  e.sourceFile == ConsoleName)) {
                stdOut << '\n' << e.fullMessage();
                clear(buffer);
            }
        }
    }
    stdOut << '\n';
}

} /* namespace Shell */
} /* namespace JavaScript */


#if defined(XP_MAC) && !defined(XP_MAC_MPW)
int main(int argc, char **argv)
{
    initConsole("\pJavaScript Shell", "Welcome to the js2 shell.\n", argc, argv);
#else
int main(int , char **)
{
#endif

    using namespace JavaScript;
    using namespace Shell;

    readEvalPrint(stdin, world);
    return 0;
}
