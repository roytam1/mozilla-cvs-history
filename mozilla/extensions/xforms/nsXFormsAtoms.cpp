/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsXFormsAtoms.h"
#include "nsMemory.h"

nsIAtom* nsXFormsAtoms::schema;
nsIAtom* nsXFormsAtoms::instance;
nsIAtom* nsXFormsAtoms::src;
nsIAtom* nsXFormsAtoms::bind;
nsIAtom* nsXFormsAtoms::nodeset;
nsIAtom* nsXFormsAtoms::type;
nsIAtom* nsXFormsAtoms::readonly;
nsIAtom* nsXFormsAtoms::required;
nsIAtom* nsXFormsAtoms::relevant;
nsIAtom* nsXFormsAtoms::calculate;
nsIAtom* nsXFormsAtoms::constraint;
nsIAtom* nsXFormsAtoms::p3ptype;
nsIAtom* nsXFormsAtoms::model;
nsIAtom* nsXFormsAtoms::modelListProperty;
nsIAtom *nsXFormsAtoms::ref;
nsIAtom *nsXFormsAtoms::action;
nsIAtom *nsXFormsAtoms::method;
nsIAtom *nsXFormsAtoms::replace;
nsIAtom *nsXFormsAtoms::separator;

const nsStaticAtom nsXFormsAtoms::Atoms_info[] = {
  { "schema",            &nsXFormsAtoms::schema },
  { "instance",          &nsXFormsAtoms::instance },
  { "src",               &nsXFormsAtoms::src },
  { "bind",              &nsXFormsAtoms::bind },
  { "nodeset",           &nsXFormsAtoms::nodeset },
  { "type",              &nsXFormsAtoms::type },
  { "readonly",          &nsXFormsAtoms::readonly },
  { "required",          &nsXFormsAtoms::required },
  { "relevant",          &nsXFormsAtoms::relevant },
  { "calculate",         &nsXFormsAtoms::calculate },
  { "constraint",        &nsXFormsAtoms::constraint },
  { "p3ptype",           &nsXFormsAtoms::p3ptype },
  { "model",             &nsXFormsAtoms::model },
  { "ModelListProperty", &nsXFormsAtoms::modelListProperty },
  { "ref",               &nsXFormsAtoms::ref },
  { "action",            &nsXFormsAtoms::action },
  { "method",            &nsXFormsAtoms::method },
  { "replace",           &nsXFormsAtoms::replace },
  { "separator",         &nsXFormsAtoms::separator }
};

void
nsXFormsAtoms::InitAtoms()
{
  NS_RegisterStaticAtoms(Atoms_info, NS_ARRAY_LENGTH(Atoms_info));
}
