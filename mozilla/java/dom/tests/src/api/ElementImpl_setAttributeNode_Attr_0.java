/**
 *
 *  @version 1.00 
 *  @author Raju Pallath
 *
 *  TESTID 
 * 
 *  Tests out the ElementImpl->setAttributeNode(null) method.
 *
 */
package org.mozilla.dom.test;

import java.util.*;
import java.io.*;
import org.mozilla.dom.test.*;
import org.mozilla.dom.*;
import org.w3c.dom.*;

public class ElementImpl_setAttributeNode_Attr_0 extends BWBaseTest implements Execution
{

   /**
    *
    ***********************************************************
    *  Constructor
    ***********************************************************
    *
    */
   public ElementImpl_setAttributeNode_Attr_0()
   {
   }


   /**
    *
    ***********************************************************
    *  Starting point of application
    *
    *  @param   args    Array of command line arguments
    *
    ***********************************************************
    */
   public static void main(String[] args)
   {
   }

   /**
    ***********************************************************
    *
    *  Execute Method 
    *
    *  @param   tobj    Object reference (Node/Document/...)
    *  @return          true or false  depending on whether test passed or failed.
    *
    ***********************************************************
    */
   public boolean execute(Object tobj)
   {
      if (tobj == null)  {
           TestLoader.logPrint("Object is NULL...");
           return BWBaseTest.FAILED;
      }

      String os = System.getProperty("OS");
      osRoutine(os);

      Document d = (Document)tobj;
      if (d != null)
      {
        try {
             Element e = d.getDocumentElement();
	     if (e == null) {
                TestLoader.logErrPrint("Document Element is  NULL..");
                return BWBaseTest.FAILED;
             } else  {
                Attr anode = e.setAttributeNode(null);
		if (anode != null)
                {
                  TestLoader.logErrPrint("Document setAttribute Node is  not NULL..");
                  return BWBaseTest.FAILED;
                }
           }
         } catch (DOMException e) {
                TestLoader.logErrPrint("Caught DOMException");
                return BWBaseTest.PASSED;
         }
      } else {
             System.out.println("Document is  NULL..");
             return BWBaseTest.FAILED;
      }

      return BWBaseTest.PASSED;
   }

   /**
    *
    ***********************************************************
    *  Routine where OS specific checks are made. 
    *
    *  @param   os      OS Name (SunOS/Linus/MacOS/...)
    ***********************************************************
    *
    */
   private void osRoutine(String os)
   {
     if(os == null) return;

     os = os.trim();
     if(os.compareTo("SunOS") == 0) {}
     else if(os.compareTo("Linux") == 0) {}
     else if(os.compareTo("Windows") == 0) {}
     else if(os.compareTo("MacOS") == 0) {}
     else {}
   }
}
