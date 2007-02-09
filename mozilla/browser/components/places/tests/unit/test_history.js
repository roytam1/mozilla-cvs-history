/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *  Dietrich Ayala <dietrich@mozilla.com>
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

// Get history service
try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 

// main
function run_test() {
  // test URI
  var testURI = uri("http://mozilla.com/");

  // add a visit
  var placeID = histsvc.addVisit(testURI,
                                 Date.now(),
                                 0, // no referrer
                                 histsvc.TRANSITION_TYPED, // user typed in URL bar
                                 false, // not redirect
                                 0);
  // test for valid place ID
  do_check_true(placeID > 0);

  // query for the visit
  var options = histsvc.getNewQueryOptions();
  // set sorting and limit such that we should retrieve only the visit we just added
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.maxResults = 1;
  // TODO: using full visit crashes in xpcshell test
  //options.resultType = options.RESULTS_AS_FULL_VISIT;
  options.resultType = options.RESULTS_AS_VISIT;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  for (var i=0; i < cc; ++i) {
    var node = root.getChild(i);
    // test node properties in RESULTS_AS_VISIT
    do_check_eq(node.uri, testURI.spec);
    do_check_eq(node.type, options.RESULTS_AS_VISIT);
    // TODO: change query type to RESULTS_AS_FULL_VISIT and test this
    //do_check_eq(node.transitionType, histsvc.TRANSITION_TYPED);
  }
  root.containerOpen = false;

  // by default, browser.history_expire_days is 9
  do_check_true(!histsvc.historyDisabled);
}
