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

#ifndef nsDebug_h___
#define nsDebug_h___

#include "nsCom.h"
#include "prtypes.h"

#ifdef DEBUG
#define NS_DEBUG
#endif

/**
 * Namespace for debugging methods. Note that your code must use the
 * macros defined later in this file so that the debug code can be
 * conditionally compiled out.
 */

/* in case this is included by a C file */
#ifdef __cplusplus

class nsDebug {
public:
  /**
   * When called, this will log a fatal abort message (to stderr and
   * to the NSPR log file) and then abort the program. This is
   * used by the NS_ABORT_IF_FALSE macro below when debugging is
   * enabled.
   */
  static NS_COM void AbortIfFalse(const char* aStr, const char* aExpr,
                                  const char* aFile, PRIntn aLine);

  /**
   * Log a warning message when an expression is not true. Used by
   * the NS_WARN_IF_FALSE macro below when debugging is enabled.
   *
   * The default behavior of this method is print a message to stderr
   * and to log an event in the NSPR log file.
   */
  static NS_COM PRBool WarnIfFalse(const char* aStr, const char* aExpr,
                                   const char* aFile, PRIntn aLine);

  /**
   * Enable flying a warning message box (if the platform supports
   * such a thing) when WarnIfFalse is called in addition to
   * the usual printing to stderr and the NSPR log file.
   *
   * If aOnOff is PR_TRUE then the message-box is enabled, otherwise
   * the message-box is disabled.
   *
   * The default state for the message-box enable is "off".
   *
   * Note also that the implementation looks at an environment
   * variable (for those platforms that have environment variables...)
   * called MOZ_WARNING_MESSAGE_BOX that when set enables the
   * warning message box by default.
   */
  static NS_COM void SetWarningMessageBoxEnable(PRBool aOnOff);

  /**
   * Get the current setting of the message-box enable.
   */
  static NS_COM PRBool GetWarningMessageBoxEnable(void);

  // Note: Methods below this line are the old ones; please start using
  // the new ones. The old ones will be removed eventually!

  //////////////////////////////////////////////////////////////////////

  // XXX add in log controls here
  // XXX probably want printf type arguments

  /**
   * Abort the executing program. This works on all architectures.
   */
  static NS_COM void Abort(const char* aFile, PRIntn aLine);

  /**
   * Break the executing program into the debugger. 
   */
  static NS_COM void Break(const char* aFile, PRIntn aLine);

  /**
   * Log a pre-condition message to the debug log
   */
  static NS_COM void PreCondition(const char* aStr, const char* aExpr,
                                  const char* aFile, PRIntn aLine);

  /**
   * Log a post-condition message to the debug log
   */
  static NS_COM void PostCondition(const char* aStr, const char* aExpr,
                                   const char* aFile, PRIntn aLine);

  /**
   * Log an assertion message to the debug log
   */
  static NS_COM void Assertion(const char* aStr, const char* aExpr,
                               const char* aFile, PRIntn aLine);

  /**
   * Log a not-yet-implemented message to the debug log
   */
  static NS_COM void NotYetImplemented(const char* aMessage,
                                       const char* aFile, PRIntn aLine);

  /**
   * Log a not-reached message to the debug log
   */
  static NS_COM void NotReached(const char* aMessage,
                                const char* aFile, PRIntn aLine);

  /**
   * Log an error message to the debug log. This call returns.
   */
  static NS_COM void Error(const char* aMessage,
                           const char* aFile, PRIntn aLine);

  /**
   * Log a warning message to the debug log.
   */
  static NS_COM void Warning(const char* aMessage,
                             const char* aFile, PRIntn aLine);
};

#ifdef DEBUG

/**
 * Abort the execution of the program if the expresion evaluates to
 * false.
 *
 * There is no status value returned from the macro.
 *
 * Note that the non-debug version of this macro does <b>not</b>
 * evaluate the expression argument. Hence side effect statements
 * as arguments to the macro will yield improper execution in a
 * non-debug build. For example:
 *
 *      NS_ABORT_IF_FALSE(0 == foo++, "yikes foo should be zero");
 *
 * Note also that the non-debug version of this macro does <b>not</b>
 * evaluate the message argument.
 */
#define NS_ABORT_IF_FALSE(_expr,_msg)                        \
PR_BEGIN_MACRO                                               \
  if (!(_expr)) {                                            \
    nsDebug::AbortIfFalse(_msg, #_expr, __FILE__, __LINE__); \
  }                                                          \
PR_END_MACRO

/**
 * Warn if a given condition is false.
 *
 * Program execution continues past the usage of this macro.
 *
 * The macro returns a status value that can be used in an "if"
 * statement. For example:
 *
 *      if (NS_WARN_IF_FALSE(aPtr, "null pointer")) {
 *        return NS_ERROR_NULL_POINTER;
 *      }
 *
 * Note that the non-debug version of this macro <b>does</b> evaluate
 * the expression as this macro continues to return a boolean
 * value. Therefore side effect expressions will work
 * correctly. However, they are still "poor form" so don't do it!
 *
 * Note also that the non-debug version of this macro does <b>not</b>
 * evaluate the message argument.
 */
#define NS_WARN_IF_FALSE(_e,_msg) \
  (!(_e) ? nsDebug::WarnIfFalse(_msg, #_e, __FILE__, __LINE__) : PR_FALSE)

// Note: Macros below this line are the old ones; please start using
// the new ones. The old ones will be removed eventually!

//////////////////////////////////////////////////////////////////////

/**
 * Test a precondition for truth. If the expression is not true then
 * trigger a program failure.
 */
#define NS_PRECONDITION(expr,str) \
if (!(expr))                      \
  nsDebug::PreCondition(str, #expr, __FILE__, __LINE__)

/**
 * Test an assertion for truth. If the expression is not true then
 * trigger a program failure.
 */
#define NS_ASSERTION(expr,str) \
if (!(expr))                   \
  nsDebug::Assertion(str, #expr, __FILE__, __LINE__)

/**
 * Test an assertion for truth. If the expression is not true then
 * trigger a program failure. The expression will still be
 * executed in release mode.
 */
#define NS_VERIFY(expr,str) \
if (!(expr))                \
  nsDebug::Assertion(str, #expr, __FILE__, __LINE__)

/**
 * Test a post-condition for truth. If the expression is not true then
 * trigger a program failure.
 */
#define NS_POSTCONDITION(expr,str) \
if (!(expr))                       \
  nsDebug::PostCondition(str, #expr, __FILE__, __LINE__)

/**
 * This macros triggers a program failure if executed. It indicates that
 * an attempt was made to execute some unimplimented functionality.
 */
#define NS_NOTYETIMPLEMENTED(str) \
  nsDebug::NotYetImplemented(str, __FILE__, __LINE__)

/**
 * This macros triggers a program failure if executed. It indicates that
 * an attempt was made to execute some unimplimented functionality.
 */
#define NS_NOTREACHED(str) \
  nsDebug::NotReached(str, __FILE__, __LINE__)

/**
 * Log an error message.
 */
#define NS_ERROR(str) \
  nsDebug::Error(str, __FILE__, __LINE__)

/**
 * Log a warning message.
 */
#define NS_WARNING(str) \
  nsDebug::Warning(str, __FILE__, __LINE__)

/**
 * Trigger an abort
 */
#define NS_ABORT() \
  nsDebug::Abort(__FILE__, __LINE__)

/**
 * Cause a break
 */
#define NS_BREAK() \
  nsDebug::Break(__FILE__, __LINE__)

#else /* NS_DEBUG */

/**
 * The non-debug version of this macro does not evaluate the
 * expression or the message arguments to the macro.
 */
#define NS_ABORT_IF_FALSE(_expr,_msg) {}

/**
 * Note that the non-debug version of this macro <b>does</b> evaluate
 * the expression as this macro continues to return a boolean
 * value.
 *
 * The non-debug version of this macro does <b>not</b> evaluate the
 * message argument.
 */
#define NS_WARN_IF_FALSE(_e,_msg) \
  (!(_e) ? PR_TRUE : PR_FALSE)

#define NS_PRECONDITION(expr,str)  {}
#define NS_ASSERTION(expr,str)     {}
#define NS_VERIFY(expr,str)        expr
#define NS_POSTCONDITION(expr,str) {}
#define NS_NOTYETIMPLEMENTED(str)  {}
#define NS_NOTREACHED(str)         {}
#define NS_ERROR(str)              {}
#define NS_WARNING(str)            {}
#define NS_ABORT()                 {}
#define NS_BREAK()                 {}

#endif /* ! NS_DEBUG */
#endif /* __cplusplus */
#endif /* nsDebug_h___ */
