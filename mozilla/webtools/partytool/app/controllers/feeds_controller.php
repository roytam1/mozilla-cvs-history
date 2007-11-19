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
vendor('webServices');
class FeedsController extends AppController {
  var $name = 'Feeds';
  var $components = array('Unicode');

  function index() {
    header('Content-type: application/rss+xml');
    $this->layout = 'ajax';
    $this->set('count', $this->Feed->findCount());
  }

  function latest() {
    header('Content-type: application/rss+xml');
    $this->layout = 'ajax';
    $this->set('latest', $this->Feed->findAll('', '', 'id DESC', 10, 1));
  }

  function users() {
    header('Content-type: application/rss+xml');
    $this->layout = 'ajax';
    $this->set('count', $this->Feed->getUserCount());
  }

  function comments($id = null) {
    $this->layout = 'ajax';
    header('Content-type: application/rss+xml');
    if (!is_numeric($id))
      return;

    $this->set('comments', $this->Feed->getComments($id));
    $this->set('pid', $id);
  }
  
  function photos($id = null) {
    $this->layout = 'ajax';
    header('Content-type: application/atom+xml');
    if (!is_numeric($id))
      return;

    $party = $this->Feed->findById($id);
    $this->set('party', $party);

    if (FLICKR_API_KEY != null && !$party['Feeds']['canceled']) {
      if ($party['Feeds']['useflickr'] == 1) {
        $data = array('type' => 'flickr', 'userid' => $party['Feeds']['flickrid'], 'randomize' => false);
        $flickr = new webServices($data);
        $photoset = $flickr->fetchPhotos(FLICKR_TAG_PREFIX.$id, 30, !$party['Feeds']['flickrperms']);
        $this->set('flickr', $photoset);
      }
    }
  }

  function upcoming($limit = null) {
    $this->layout = 'ajax';
    header('Content-type: application/rss+xml');

    ($limit == null) ? $limit = 25 : $limit = intval($limit);

    $this->set('latest', $this->Feed->findAll('WHERE date > '. time(), '', 'date ASC', $limit, 1));
  }

  function ical() {
    $this->layout = 'ajax';
    header('Content-type: text/calendar');
    header("Content-Disposition: inline; filename=partylist.ics");
    $back = time() - 172800;
    $events = $this->Feed->findAll('WHERE date > '. $back, '', 'date ASC', null, 1);
    
    $cal = array();
    
    foreach($events as $event) {
      $event['Feeds']['name'] = 
        preg_replace(array("/&#(\d{2,5});/e", "/(\n|\r|\f)/", "/\,/"),
                     array('$this->Unicode->unicode2utf(${1})', ' ', '\,'),
                     html_entity_decode($event['Feeds']['name']));
      $event['Feeds']['address'] = 
        preg_replace(array("/&#(\d{2,5});/e", "/(\n|\r|\f)/", "/\,/"),
                     array('$this->Unicode->unicode2utf(${1})', ' ', '\,'),
                     html_entity_decode($event['Feeds']['address']));
      $event['Feeds']['notes'] = 
        preg_replace(array("/&#(\d{2,5});/e", "/(\n|\r|\f)/", "/\,/"),
                     array('$this->Unicode->unicode2utf(${1})', ' ', '\,'),
                     html_entity_decode($event['Feeds']['notes']));
      array_push($cal, $event);
    }
    $this->set('events', $cal);
  }
  
  function topguests($limit = null) {
    $this->layout = 'ajax';
    header('Content-type: application/rss+xml');

    ($limit == null) ? $limit = 25 : $limit = intval($limit);

    $rv = $this->Feed->query("SELECT parties.name AS name,
                                guests.pid AS id,
                                COUNT(guests.pid) AS count
                              FROM guests
                              LEFT JOIN parties
                                ON guests.pid = parties.id
                              GROUP BY guests.pid
                              ORDER BY count DESC
                              LIMIT $limit");
    $this->set('items', $rv);
  }
}
?>
