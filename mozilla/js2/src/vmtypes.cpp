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
 * Communications Corporation.  Portions created by Netscape are
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

#include "utilities.h"
#include "vmtypes.h"

namespace JavaScript {
namespace VM {

using namespace JSTypes;

#define OPCODE_NAMES
#include "icode.h"

Formatter& operator<< (Formatter& f, Instruction& i)
{
    return i.print(f);
}

Formatter& operator<< (Formatter& f, ArgumentList* rl)
{
    Argument* e = rl->end();
    
    f << "(";
    for (ArgumentList::iterator r = rl->begin(); r != e; r++) {
        f << "R" << r->first.first; 
        if ((r + 1) != e)
            f << ", ";
    }
    f << ")";
    
    return f;
}

Formatter& operator<< (Formatter& f, const ArgList& al)
{
    const ArgumentList* rl = al.mList;
    const JSValues& registers = al.mRegisters;
    f << "(";
    ArgumentList::const_iterator i = rl->begin(), e = rl->end();
    if (i != e) {
        Argument r = *i++;
        f << getRegisterValue(registers, r.first.first);
        while (i != e) {
            r = *i++;
            f << ", " << getRegisterValue(registers, r.first.first);
        }
    }
    f << ")";
    
    return f;
}

Formatter& operator<< (Formatter& f, TypedRegister& r)
{
    if (r.first == NotARegister)
        f << "<NaR>";
    else
        f << "R" << r.first;
    return f;
}

Formatter& operator<< (Formatter &f, InstructionStream &is)
{
    for (InstructionIterator i = is.begin(); 
         i != is.end(); i++) {
        /*
        bool isLabel = false;

        for (LabelList::iterator k = labels.begin(); 
             k != labels.end(); k++)
            if ((ptrdiff_t)(*k)->mOffset == (i - is.begin())) {
                f << "#" << (uint32)(i - is.begin()) << "\t";
                isLabel = true;
                break;
            }

        if (!isLabel)
            f << "\t";

        f << **i << "\n";
        */

        printFormat(stdOut, "%04u", (uint32)(i - is.begin()));
        f << ": " << **i << "\n";
    
    }

    return f;
}


Formatter& operator<< (Formatter& f, JSTypes::Operator& op)
{
    switch (op) {
    case JSTypes::None:
    f << "None"; break;
    case JSTypes::Posate:
    f << "Posate"; break;
    case JSTypes::Negate:
    f << "Negate"; break;
    case JSTypes::Complement:
    f << "Complement"; break;
    case JSTypes::Increment:
    f << "Increment"; break;
    case JSTypes::Decrement:
    f << "Decrement"; break;
    case JSTypes::Const:
    f << "Const"; break;
    case JSTypes::Call:
    f << "Call"; break;
    case JSTypes::New:
    f << "New"; break;
    case JSTypes::NewArgs:
    f << "NewArgs"; break;
    case JSTypes::Index:
    f << "Index"; break;
    case JSTypes::IndexEqual:
    f << "IndexEqual"; break;
    case JSTypes::DeleteIndex:
    f << "DeleteIndex"; break;
    case JSTypes::Plus:
    f << "Plus"; break;
    case JSTypes::Minus:
    f << "Minus"; break;
    case JSTypes::Multiply:
    f << "Multiply"; break;
    case JSTypes::Divide:
    f << "Divide"; break;
    case JSTypes::Remainder:
    f << "Remainder"; break;
    case JSTypes::ShiftLeft:
    f << "ShiftLeft"; break;
    case JSTypes::ShiftRight:
    f << "ShiftRight"; break;
    case JSTypes::UShiftRight:
    f << "UShiftRight"; break;
    case JSTypes::Less:
    f << "Less"; break;
    case JSTypes::LessEqual:
    f << "LessEqual"; break;
    case JSTypes::In:
    f << "In"; break;
    case JSTypes::Equal:
    f << "Equal"; break;
    case JSTypes::SpittingImage:
    f << "SpittingImage"; break;
    case JSTypes::BitAnd:
    f << "BitAnd"; break;
    case JSTypes::BitXor:
    f << "BitXor"; break;
    case JSTypes::BitOr:
    f << "BitOr"; break;
    default:
        NOT_REACHED("Bad op");
        break;
    }
    return f;
}

} /* namespace VM */
} /* namespace JavaScript */


