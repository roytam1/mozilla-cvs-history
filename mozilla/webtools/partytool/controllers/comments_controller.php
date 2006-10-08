<?php
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
 * The Original Code is Mozilla Party Tool
 *
 * The Initial Developer of the Original Code is
 * Ryan Flint <rflint@dslr.net>
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
uses('sanitize');

class CommentsController extends AppController {
  var $name = 'Comments';
  var $components = array('Security');
  
  function beforeFilter() {
    $this->Security->requirePost('add');
  }

  function add($pid, $uid) {
    if (!$this->Session->check('User') || $uid != $_SESSION['User']['id'])
      $this->redirect('/');

    if (!empty($this->data) && $this->Comment->canComment($pid, $uid)) {
      // Explictly destroy the last model to avoid an edit instead of an insert
      $this->Comment->create();

      $clean = new Sanitize();
      $text = $clean->html($this->data['Comment']['text']);
      $this->data['Comment']['text'] = nl2br($text);
      $this->data['Comment']['owner'] = $uid;
      $this->data['Comment']['assoc'] = $pid;
      $this->data['Comment']['time'] = gmmktime();

      if ($this->Comment->save($this->data)) {
        $this->redirect('/parties/view/'.$pid.'#c'.$this->Comment->getLastInsertID());
      }
    }

    else
      $this->redirect('/parties/view/'.$pid);
  }
}
?>