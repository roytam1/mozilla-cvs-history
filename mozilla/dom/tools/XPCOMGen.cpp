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

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <fstream.h>
#include <ctype.h>
#include "XPComGen.h"
#include "Exceptions.h"
#include "plhash.h"
#include "IdlSpecification.h"
#include "IdlInterface.h"
#include "IdlVariable.h"
#include "IdlAttribute.h"
#include "IdlFunction.h"
#include "IdlParameter.h"

static const char *kFilePrefix = "nsIDOM";
static const char *kFileSuffix = "h";
static const char *kIfdefStr = "\n"
"#ifndef nsIDOM%s_h__\n"
"#define nsIDOM%s_h__\n\n";
static const char *kIncludeDefaultsStr = 
"#include \"nsISupports.h\"\n"
"#include \"nsString.h\"\n"
"#include \"nsIScriptContext.h\"\n";
static const char *kIncludeStr = "#include \"nsIDOM%s.h\"\n";
static const char *kIncludeJSStr = "#include \"jsapi.h\"\n";
static const char *kForwardClassStr = "class nsIDOM%s;\n";
static const char *kUuidStr = 
"#define %s \\\n"
"--- IID GOES HERE ---\n\n";
static const char *kClassDeclStr = "class nsIDOM%s : ";
static const char *kBaseClassStr = "public nsIDOM%s";
static const char *kNoBaseClassStr = "public nsISupports";
static const char *kClassPrologStr = " {\npublic:\n";
static const char *kEnumDeclBeginStr = "  enum {\n";
static const char *kEnumEntryStr = "    %s = %d%s\n";
static const char *kEnumDeclEndStr = "  };\n";
static const char *kGetterMethodDeclStr = "\n  NS_IMETHOD    Get%s(%s%s a%s)=0;\n";
static const char *kSetterMethodDeclStr = "  NS_IMETHOD    Set%s(%s a%s)=0;\n";
static const char *kMethodDeclStr = "\n  NS_IMETHOD    %s(%s)=0;\n";
static const char *kParamStr = "%s a%s";
static const char *kDelimiterStr = ", ";
static const char *kEllipsisParamStr = "JSContext *cx, jsval *argv, PRUint32 argc";
static const char *kReturnStr = "%s%s aReturn";
static const char *kClassEpilogStr = "};\n\n";
static const char *kGlobalInitClassStr = "extern nsresult NS_Init%sClass(nsIScriptContext *aContext, nsIScriptGlobalObject *aGlobal);\n\n";
static const char *kInitClassStr = "extern nsresult NS_Init%sClass(nsIScriptContext *aContext, void **aPrototype);\n\n";
static const char *kNewObjStr = "extern \"C\" NS_DOM NS_NewScript%s(nsIScriptContext *aContext, nsIDOM%s *aSupports, nsISupports *aParent, void **aReturn);\n\n";
static const char *kEndifStr = "#endif // nsIDOM%s_h__\n";


XPCOMGen::XPCOMGen()
{
}

XPCOMGen::~XPCOMGen()
{
}

void     
XPCOMGen::Generate(char *aFileName, 
                   char *aOutputDirName,
                   IdlSpecification &aSpec,
                   int aIsGlobal)
{
  if (!OpenFile(aFileName, aOutputDirName, kFilePrefix, kFileSuffix)) {
      throw new CantOpenFileException(aFileName);
  }
  
  mIsGlobal = aIsGlobal;

  GenerateNPL();
  GenerateIfdef(aSpec);
  GenerateIncludes(aSpec);
  GenerateForwardDecls(aSpec);
  
  int i, icount = aSpec.InterfaceCount();
  for (i = 0; i < icount; i++) {
    IdlInterface *iface = aSpec.GetInterfaceAt(i);
    
    if (iface) {
      GenerateGuid(*iface);
      GenerateClassDecl(*iface);
      GenerateEnums(*iface);
      GenerateMethods(*iface);
      GenerateEndClassDecl();
    }
  }

  GenerateEpilog(aSpec);
  CloseFile();
}


void     
XPCOMGen::GenerateIfdef(IdlSpecification &aSpec)
{
  char buf[512];
  IdlInterface *iface = aSpec.GetInterfaceAt(0);
  ofstream *file = GetFile();

  if (iface) {
    sprintf(buf, kIfdefStr, iface->GetName(), iface->GetName());
    *file << buf;
  } 
}

void     
XPCOMGen::GenerateIncludes(IdlSpecification &aSpec)
{
  char buf[512];
  ofstream *file = GetFile();

  *file << kIncludeDefaultsStr;
  int i, icount = aSpec.InterfaceCount();
  for (i = 0; i < icount; i++) {
    IdlInterface *iface = aSpec.GetInterfaceAt(i);

    if (iface) {
      int b, bcount = iface->BaseClassCount();
      for (b = 0; b < bcount; b++) {
        sprintf(buf, kIncludeStr, iface->GetBaseClassAt(b));
        *file << buf;
      }
    }

    int m, mcount = iface->FunctionCount();
    for (m = 0; m < mcount; m++) {
      IdlFunction *func = iface->GetFunctionAt(m);
      
      if (func->GetHasEllipsis()) {
        *file << kIncludeJSStr;
        break;
      }
    }
  }
  
  *file << "\n";
}

static PRIntn 
ForwardDeclEnumerator(PLHashEntry *he, PRIntn i, void *arg)
{
  char buf[512];

  ofstream *file = (ofstream *)arg;
  sprintf(buf, kForwardClassStr, (char *)he->key);
  *file << buf;
  
  return HT_ENUMERATE_NEXT;
}
 
void     
XPCOMGen::GenerateForwardDecls(IdlSpecification &aSpec)
{
  ofstream *file = GetFile();
  EnumerateAllObjects(aSpec, (PLHashEnumerator)ForwardDeclEnumerator, 
                      file, PR_FALSE);
  *file << "\n";
}
 
void     
XPCOMGen::GenerateGuid(IdlInterface &aInterface)
{
  char buf[512];
  char uuid_buf[256];
  ofstream *file = GetFile();

  // XXX Need to generate unique guids
  GetInterfaceIID(uuid_buf, aInterface);
  sprintf(buf, kUuidStr, uuid_buf);
  *file << buf;
}
 
void     
XPCOMGen::GenerateClassDecl(IdlInterface &aInterface)
{
  char buf[512];
  ofstream *file = GetFile();
  
  sprintf(buf, kClassDeclStr, aInterface.GetName());
  *file << buf;

  if (aInterface.BaseClassCount() > 0) {
    int b, bcount = aInterface.BaseClassCount();
    for (b = 0; b < bcount; b++) {
      if (b > 0) {
        *file << kDelimiterStr;
      }
      sprintf(buf, kBaseClassStr, aInterface.GetBaseClassAt(b));
      *file << buf;
    }
  }
  else {
    *file << kNoBaseClassStr;
  }
  
  *file << kClassPrologStr;
}

void     
XPCOMGen::GenerateEnums(IdlInterface &aInterface)
{
  char buf[512];
  ofstream *file = GetFile();

  if (aInterface.ConstCount() > 0) {
    int c, ccount = aInterface.ConstCount();

    *file << kEnumDeclBeginStr;
    
    for (c = 0; c < ccount; c++) {
      IdlVariable *var = aInterface.GetConstAt(c);
      
      if (NULL != var) {
        sprintf(buf, kEnumEntryStr, var->GetName(), var->GetLongValue(), 
                ((c < ccount-1) ? "," : ""));
        *file << buf;
      }
    }
    
    *file << kEnumDeclEndStr;
  }
}

void     
XPCOMGen::GenerateMethods(IdlInterface &aInterface)
{
  char buf[512];
  char name_buf[128];
  char type_buf[128];
  ofstream *file = GetFile();

  int a, acount = aInterface.AttributeCount();
  for (a = 0; a < acount; a++) {
    IdlAttribute *attr = aInterface.GetAttributeAt(a);

    GetVariableTypeForParameter(type_buf, *attr);
    GetCapitalizedName(name_buf, *attr);
    sprintf(buf, kGetterMethodDeclStr, name_buf, type_buf,
            attr->GetType() == TYPE_STRING ? "" : "*", name_buf);
    *file << buf;

    if (!attr->GetReadOnly()) {
      sprintf(buf, kSetterMethodDeclStr, name_buf, type_buf, 
              name_buf);
      *file << buf;
    }
  }
  
  int m, mcount = aInterface.FunctionCount();
  for (m = 0; m < mcount; m++) {
    char param_buf[256];
    char *cur_param = param_buf;
    IdlFunction *func = aInterface.GetFunctionAt(m);

    int p, pcount = func->ParameterCount();
    for (p = 0; p < pcount; p++) {
      IdlParameter *param = func->GetParameterAt(p);

      if (p > 0) {
        strcpy(cur_param, kDelimiterStr);
        cur_param += strlen(kDelimiterStr);
      }

      GetParameterType(type_buf, *param);
      GetCapitalizedName(name_buf, *param);
      sprintf(cur_param, kParamStr, type_buf, name_buf);
      cur_param += strlen(cur_param);
    }

    if (func->GetHasEllipsis()) {
      if (pcount > 0) {
        strcpy(cur_param, kDelimiterStr);
        cur_param += strlen(kDelimiterStr);
      }
      sprintf(cur_param, kEllipsisParamStr);
      cur_param += strlen(cur_param);
    }

    IdlVariable *rval = func->GetReturnValue();
    if (rval->GetType() != TYPE_VOID) {
      if ((pcount > 0) || func->GetHasEllipsis()) {
        strcpy(cur_param, kDelimiterStr);
        cur_param += strlen(kDelimiterStr);
      }
      GetVariableTypeForParameter(type_buf, *rval);
      sprintf(cur_param, kReturnStr, type_buf,
              rval->GetType() == TYPE_STRING ? "" : "*");
    }
    else {
      *cur_param++ = '\0';
    }
 
    GetCapitalizedName(name_buf, *func);
    sprintf(buf, kMethodDeclStr, name_buf, param_buf);
    *file << buf;
  }
}

void
XPCOMGen::GenerateEndClassDecl()
{
  ofstream *file = GetFile();
  
  *file << kClassEpilogStr;
}

void     
XPCOMGen::GenerateEpilog(IdlSpecification &aSpec)
{
  char buf[512];
  IdlInterface *iface = aSpec.GetInterfaceAt(0);
  ofstream *file = GetFile();
  char *iface_name = iface->GetName();

  if (mIsGlobal) {
    sprintf(buf, kGlobalInitClassStr, iface_name);
  }
  else {
    sprintf(buf, kInitClassStr, iface_name);
  }
  *file << buf;
  
  sprintf(buf, kNewObjStr, iface_name, iface_name);
  *file << buf;

  if (iface) {
    sprintf(buf, kEndifStr, iface_name);
    *file << buf;
  } 
}
