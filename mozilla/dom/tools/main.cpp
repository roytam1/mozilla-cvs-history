/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

// initialize cout
#include <ostream.h>
#include <string.h>

#ifndef XP_MAC
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <ctype.h>
#include <stat.h>
#include <iostream.h>
#endif

#if !defined XP_UNIX && !defined XP_MAC
#include <direct.h>
#endif

#ifdef XP_MAC
#include <console.h>
#endif

#include "IdlParser.h"
#include "Exceptions.h"
#include "IdlSpecification.h"
#include "XPCOMGen.h"
#include "JSStubGen.h"

int main(int argc, char *argv[])
{
  int gen_xpcom = 0;
  int gen_js = 0;
  int gen_idl = 0;
  int is_global = 0;
  int op_dir = 0;
  int op_dir_arg;

#ifdef XP_MAC
	argc = ccommand(&argv);
#endif

  // extract filenames from argv
  if (argc >= 2) {
    int arg_cnt = 1;

    while ((arg_cnt < argc) && (*argv[arg_cnt] == '-')) {
      switch (*(argv[arg_cnt]+1)) {
        case 'd':
          op_dir = 1;
          op_dir_arg = ++arg_cnt;
          break;
        case 'x':
          gen_xpcom = 1;
          break;
        case 'j':
          gen_js = 1;
          break;
        case 'p':
          gen_idl = 1;
          break;
        case 'g':
          is_global = 1;
          break;
      }
      ++arg_cnt;
    }
    
    if (op_dir) {
      struct stat sb;
      if (stat(argv[op_dir_arg], &sb) == 0) {
#if defined XP_UNIX || defined XP_MAC
        if (!(sb.st_mode & S_IFDIR)) {
#else
        if (!(sb.st_mode & _S_IFDIR)) {
#endif
          cout << "Creating directory " << argv[op_dir_arg] << " ...\n";
#if defined XP_UNIX 
          if (mkdir(argv[op_dir_arg],S_IWGRP | S_IWOTH) < 0) {
#elif defined XP_MAC
          if (mkdir(argv[op_dir_arg], 0) < 0) {			// mode is ignored
#else
          if (mkdir(argv[op_dir_arg]) < 0) {
#endif
            cout << "WARNING: cannot create output directory [" << argv[op_dir_arg] << "]\n";
            cout << "++++++++ using current directory\n";
          }
        }
      }
    }

    for (int i = arg_cnt; i < argc; i++) {

      // create a specification object. On parser termination it will
      // contain all parsed interfaces
      IdlSpecification *specification = new IdlSpecification();

      // initialize and run the parser
      IdlParser *parser = new IdlParser();
       try {
        parser->Parse(argv[i], *specification);
      } catch(AbortParser &exc) {
        cout << exc;
        delete parser;
        return -1;
      } catch(FileNotFoundException &exc) {
        cout << exc;
        delete parser;
        return -1;
      } catch(...) {
        cout << "Unknown Exception. Parser Aborted.";
      }

      if (gen_idl) {
        cout << *specification;
      }

      if (gen_xpcom) {
        XPCOMGen *xpcomgen = new XPCOMGen();
        
        cout << "Generating XPCOM headers for " << argv[i] << ".\n";
       try {
          xpcomgen->Generate(argv[i], op_dir ? argv[op_dir_arg] :(char*)NULL,
                             *specification, is_global);
        }
        catch(CantOpenFileException &exc) {
          cout << exc;
          delete xpcomgen;
          delete parser;
          return -1;
        }
        delete xpcomgen;
      }

      if (gen_js) {
        JSStubGen *jsgen = new JSStubGen();
        
        cout << "Generating JavaScript stubs for " << argv[i] << ".\n";
        try {
          jsgen->Generate(argv[i], op_dir ? argv[op_dir_arg] : (char*)NULL,
                          *specification, is_global);
        }
        catch(CantOpenFileException &exc) {
          cout << exc;
          delete jsgen;
          delete parser;
          return -1;
        }
        delete jsgen;
      }

      delete parser;
    }

    return 0;
  }

  cout << "+++ERROR: no file specified\n";
  goto usage;
  return -1;

usage:
  cout << "Usage: [options] [-d outputdir] filename [filename...]\n";
  cout << "Options:\n";
  cout << "-x Spit out XPCOM interfaces\n";
  cout << "-j Spit out JavaScript stub files\n";
  cout << "-p Echo normalized idl to stdout\n";
  cout << "-g used for global objects\n";
  return -1;
}

