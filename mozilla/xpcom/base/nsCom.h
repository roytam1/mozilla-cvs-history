/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsCom_h__
#define nsCom_h__

/* Core XPCOM declarations. */

/*
 * People who create their own Win32 MSDev projects to compile against mozilla
 * code *often* neglect to define XP_WIN and XP_WIN32. Rather than force
 * those definitions here - and risk having some code get compiled incorrectly
 * before this code is reached - we #error here to let the programmers know
 * that they must modify their build projects.
 * We would *like* to reduce the usage of these roughly synonymous defines.
 * But it is a big modular project with a lot of brainprint issues...
 * See bug: http://bugzilla.mozilla.org/show_bug.cgi?id=65727
 */
#if defined(_WIN32) && (!defined(XP_WIN) || !defined(XP_WIN32))
#error Add defines for XP_WIN and XP_WIN32 to your Win32 build project.
#endif

/*
 * API Import/Export macros
 */

#ifdef _IMPL_NS_COM
#if defined(XP_WIN)
#define NS_COM _declspec(dllexport)
#elif defined(XP_MAC)
#define NS_COM __declspec(export)
#else  /* !XP_WIN */
#define NS_COM
#endif /* !XP_WIN */
#else  /* !_IMPL_NS_COM */
#if defined(XP_WIN)
#define NS_COM _declspec(dllimport)
#else  /* !XP_WIN */
#define NS_COM
#endif /* !XP_WIN */
#endif /* !_IMPL_NS_COM */

/*
 * DLL Export macro
 */

#ifdef XP_WIN

#define NS_EXPORT _declspec(dllexport)
#define NS_EXPORT_(type) type _declspec(dllexport) __stdcall

#define NS_IMETHOD_(type) virtual type __stdcall
#define NS_IMETHOD virtual nsresult __stdcall
#define NS_IMETHODIMP_(type) type __stdcall
#define NS_IMETHODIMP nsresult __stdcall

#define NS_METHOD_(type) type __stdcall
#define NS_METHOD nsresult __stdcall

#define NS_CALLBACK_(_type, _name) _type (__stdcall * _name)
#define NS_CALLBACK(_name) nsresult (__stdcall * _name)

#elif defined(XP_MAC)

#define NS_EXPORT __declspec(export)
#define NS_EXPORT_(type) __declspec(export) type

#define NS_IMETHOD_(type) virtual type
#define NS_IMETHOD virtual nsresult
#define NS_IMETHODIMP_(type) type
#define NS_IMETHODIMP nsresult

#define NS_METHOD_(type) type
#define NS_METHOD nsresult

#define NS_CALLBACK_(_type, _name) _type (* _name)
#define NS_CALLBACK(_name) nsresult (* _name)

#elif defined(XP_OS2)

#define NS_EXPORT
#define NS_EXPORT_(type) type

#define NS_IMETHOD_(type) virtual type
#define NS_IMETHOD virtual nsresult
#define NS_IMETHODIMP_(type) type
#define NS_IMETHODIMP nsresult

#define NS_METHOD_(type) type
#define NS_METHOD nsresult

#define NS_CALLBACK_(_type, _name) _type ( _System * _name)
#define NS_CALLBACK(_name) nsresult (* _name)

#else  /* !XP_WIN && !XP_MAC && !XP_OS2 */

#define NS_EXPORT
#define NS_EXPORT_(type) type

#define NS_IMETHOD_(type) virtual type
#define NS_IMETHOD virtual nsresult
#define NS_IMETHODIMP_(type) type
#define NS_IMETHODIMP nsresult

#define NS_METHOD_(type) type
#define NS_METHOD nsresult

#define NS_CALLBACK_(_type, _name) _type (* _name)
#define NS_CALLBACK(_name) nsresult (* _name)

#endif /* !XP_WIN */

/* use these functions to associate get/set methods with a
   C++ member variable
*/

#define NS_METHOD_GETTER(_method, _type, _member) \
_method(_type* aResult) \
{\
    if (!aResult) return NS_ERROR_NULL_POINTER; \
    *aResult = _member; \
    return NS_OK; \
}
    
#define NS_METHOD_SETTER(_method, _type, _member) \
_method(_type aResult) \
{ \
    _member = aResult; \
    return NS_OK; \
}

/*
 * special for strings to get/set char* strings
 * using PL_strdup and PR_FREEIF
 */
#define NS_METHOD_GETTER_STR(_method,_member) \
_method(char* *aString)\
{\
    if (!aString) return NS_ERROR_NULL_POINTER; \
    *aString = PL_strdup(_member); \
    return NS_OK; \
}

#define NS_METHOD_SETTER_STR(_method, _member) \
_method(const char *aString)\
{\
    PR_FREEIF(_member);\
    if (aString) _member = PL_strdup(aString); \
    else _member = nsnull;\
    return NS_OK; \
}

/* Getter/Setter macros.
   Usage:
   NS_IMPL_[CLASS_]GETTER[_<type>](method, [type,] member);
   NS_IMPL_[CLASS_]SETTER[_<type>](method, [type,] member);
   NS_IMPL_[CLASS_]GETSET[_<type>]([class, ]postfix, [type,] member);
   
   where:
   CLASS_  - implementation is inside a class definition
             (otherwise the class name is needed)
             Do NOT use in publicly exported header files, because
             the implementation may be included many times over.
             Instead, use the non-CLASS_ version.
   _<type> - For more complex (STR, IFACE) data types
             (otherwise the simple data type is needed)
   method  - name of the method, such as GetWidth or SetColor
   type    - simple data type if required
   member  - class member variable such as m_width or mColor
   class   - the class name, such as Window or MyObject
   postfix - Method part after Get/Set such as "Width" for "GetWidth"
   
   Example:
   class Window {
   public:
     NS_IMPL_CLASS_GETSET(Width, int, m_width);
     NS_IMPL_CLASS_GETTER_STR(GetColor, m_color);
     NS_IMETHOD SetColor(char *color);
     
   private:
     int m_width;     // read/write
     char *m_color;   // readonly
   };

   // defined outside of class
   NS_IMPL_SETTER_STR(Window::GetColor, m_color);

   Questions/Comments to alecf@netscape.com
*/

   
/*
 * Getter/Setter implementation within a class definition
 */

/* simple data types */
#define NS_IMPL_CLASS_GETTER(_method, _type, _member) \
NS_IMETHOD NS_METHOD_GETTER(_method, _type, _member)

#define NS_IMPL_CLASS_SETTER(_method, _type, _member) \
NS_IMETHOD NS_METHOD_SETTER(_method, _type, _member)

#define NS_IMPL_CLASS_GETSET(_postfix, _type, _member) \
NS_IMPL_CLASS_GETTER(Get##_postfix, _type, _member) \
NS_IMPL_CLASS_SETTER(Set##_postfix, _type, _member)

/* strings */
#define NS_IMPL_CLASS_GETTER_STR(_method, _member) \
NS_IMETHOD NS_METHOD_GETTER_STR(_method, _member)

#define NS_IMPL_CLASS_SETTER_STR(_method, _member) \
NS_IMETHOD NS_METHOD_SETTER_STR(_method, _member)

#define NS_IMPL_CLASS_GETSET_STR(_postfix, _member) \
NS_IMPL_CLASS_GETTER_STR(Get##_postfix, _member) \
NS_IMPL_CLASS_SETTER_STR(Set##_postfix, _member)

/* Getter/Setter implementation outside of a class definition */

/* simple data types */
#define NS_IMPL_GETTER(_method, _type, _member) \
NS_IMETHODIMP NS_METHOD_GETTER(_method, _type, _member)

#define NS_IMPL_SETTER(_method, _type, _member) \
NS_IMETHODIMP NS_METHOD_SETTER(_method, _type, _member)

#define NS_IMPL_GETSET(_class, _postfix, _type, _member) \
NS_IMPL_GETTER(_class::Get##_postfix, _type, _member) \
NS_IMPL_SETTER(_class::Set##_postfix, _type, _member)

/* strings */
#define NS_IMPL_GETTER_STR(_method, _member) \
NS_IMETHODIMP NS_METHOD_GETTER_STR(_method, _member)

#define NS_IMPL_SETTER_STR(_method, _member) \
NS_IMETHODIMP NS_METHOD_SETTER_STR(_method, _member)

#define NS_IMPL_GETSET_STR(_class, _postfix, _member) \
NS_IMPL_GETTER_STR(_class::Get##_postfix, _member) \
NS_IMPL_SETTER_STR(_class::Set##_postfix, _member)



#endif


