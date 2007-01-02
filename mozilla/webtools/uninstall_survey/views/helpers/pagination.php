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
 * The Original Code is survey.mozilla.com site.
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Wil Clouser <clouserw@mozilla.com> (Original Author)
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

class PaginationHelper {
    
    var $helpers = array('Html','Ajax');
    var $_pageDetails = array();
    var $link = '';
    var $show = array();
    var $page;
    var $style;
    
    /**
     * Sets the default pagination options. 
     *
     * @param array $paging an array detailing the page options
     */
    function setPaging($paging)
    {
        if(!empty($paging))
        {
    
            $this->link = $paging['link'];
            $this->show = $paging['show'];
            $this->page = $paging['page'];
            $this->style = $paging['style'];
            
            $pageCount = ceil($paging['count'] / $paging['limit'] );
                
            $this->_pageDetails = array(
                        'page'=>$paging['page'],
                        'recordCount'=>$paging['count'],
                        'pageCount' =>$pageCount,
                        'nextPage'=> ($paging['page'] < $pageCount) ? $paging['page']+1 : '',
                        'previousPage'=> ($paging['page']>1) ? $paging['page']-1 : '',
                        'limit'=>$paging['limit']
                        );
                        
            return true;         
        }
        return false;
    }
    /**
    * Displays limits for the query 
    *
    * @param string $text - text to display before limits
    * @param string $separator - display a separate between limits
    *
    **/
    function show($text=null, $separator=null)
    {
        if (empty($this->_pageDetails)) { return false; }
        if ( !empty($this->_pageDetails['recordCount']) )
        {
            $t = '';
            if(is_array($this->show))
            {
                $t = $text.$separator;
                foreach($this->show as $value)
                {
                    $link = preg_replace('/show=(.*?)&/','show='.$value.'&',$this->link);
                    if($this->_pageDetails['limit'] == $value)
                    {
                        $t .= '<em>'.$value.'</em>'.$separator;
                    }
                    else
                    {   
                        if($this->style == 'ajax')
                        {   
                            $t .= $this->Ajax->linkToRemote($value, array("fallback"=>$this->action."#","url" => $link.$this->_pageDetails['page'],"update" => "ajax_update","method"=>"get")).$separator;
                        }
                        else
                        {
                            $t .= $this->Html->link($value,$link.$this->_pageDetails['page']).$separator;
                        }
                    }
                }
            }
            return $t;
        }
        return false;
        
    }
    /**
    * Displays current result information 
    *
    * @param string $text - text to preceeding the number of results
    *
    **/
    function result($text)
    {
        if (empty($this->_pageDetails)) { return false; }
        if ( !empty($this->_pageDetails['recordCount']) )
        {
            if($this->_pageDetails['recordCount'] > $this->_pageDetails['limit'])
            {
                $start_row = $this->_pageDetails['page'] > 1 ? (($this->_pageDetails['page']-1)*$this->_pageDetails['limit'])+1:'1';
                $end_row = ($this->_pageDetails['recordCount'] < ($start_row + $this->_pageDetails['limit']-1)) ? $this->_pageDetails['recordCount'] : ($start_row + $this->_pageDetails['limit']-1);
                $t = $text.$start_row.'-'.$end_row.' of '.$this->_pageDetails['recordCount'];
            }
            else
            {
                $t = $text.$this->_pageDetails['recordCount'];
            }
            return $t;
        }
        return false;
    }
    /**
    * Returns a "Google style" list of page numbers
    *
    * @param string $separator - defaults to null
    * @param string $prefix - defaults to null. If set, displays prefix before page links.
    * @param int $pageSetLength - defaults to 10. Maximum number of pages to show.
    * @param string $prevLabel - defaults to null. If set, displays previous link.
    * @param string $nextLabel - defaults to null. If set, displays next link.
    *
    **/
    function pageNumbers($separator=null, $prefix=null, $pageSetLength=10, $prevLabel=null, $nextLabel=null)
    {
  
        if (empty($this->_pageDetails) || $this->_pageDetails['pageCount'] == 1) { return false; }
     
        $t = array();
     
        $modulo = $this->_pageDetails['page'] % $pageSetLength;
        if ($modulo)
        { // any number > 0
          $prevSetLastPage = $this->_pageDetails['page'] - $modulo;
        } else { // 0, last page of set
          $prevSetLastPage = $this->_pageDetails['page'] - $pageSetLength;
        }
        //$nextSetFirstPage = $prevSetLastPage + $pageSetLength + 1;
     
        if ($prevLabel) $t[] = $this->prevPage($prevLabel);
     
        // loops through each page number
        $pageSet = $prevSetLastPage + $pageSetLength;
        if ($pageSet > $this->_pageDetails['pageCount']) $pageSet = $this->_pageDetails['pageCount'];
        for ($pageIndex = $prevSetLastPage+1; $pageIndex <= $pageSet; $pageIndex++)
        {
          if ($pageIndex == $this->_pageDetails['page'])
          {
            $t[] = '<em>'.$pageIndex.'</em>';
          }
          else
          {
            if($this->style == 'ajax')
            {
              $t[] = $this->Ajax->linkToRemote($pageIndex, array("fallback"=>$this->action."#","url" =>$this->link.$pageIndex,"update" => "ajax_update","method"=>"get"));
            } else {
              $t[] = $this->Html->link($pageIndex,$this->link.$pageIndex);
            }
          }
        }
     
        if ($nextLabel) $t[] = $this->nextPage($nextLabel);
     
        $t = implode($separator, $t);
     
        return $prefix.$t;
    }
    /**
    * Displays a link to the previous page, where the page doesn't exist then
    * display the $text
    *
    * @param string $text - text display: defaults to next
    *
    **/
    function prevPage($text='prev')
    {
        if (empty($this->_pageDetails)) { return false; }
        if ( !empty($this->_pageDetails['previousPage']) )
        {
            if($this->style == 'ajax')
            {   
                $t = $this->Ajax->linkToRemote($text, array("fallback"=>$this->action."#","url" => $this->link.$this->_pageDetails['previousPage'],"update" => "ajax_update","method"=>"get"));
            }
            else
            {
                $t = $this->Html->link($text,$this->link.$this->_pageDetails['previousPage']);
            }
            return $t;
        }   
        return false;
    }
    /**
    * Displays a link to the next page, where the page doesn't exist then
    * display the $text
    *
    * @param string $text - text to display: defaults to next
    *
    **/
    function nextPage($text='next')
    {
        if (empty($this->_pageDetails)) { return false; }
        if (!empty($this->_pageDetails['nextPage']))
        {
            if($this->style == 'ajax')
            {   
                $t = $this->Ajax->linkToRemote($text, array("fallback"=>$this->action."#","url" => $this->link.$this->_pageDetails['nextPage'],"update" => "ajax_update","method"=>"get"));
            }
            else
            {
                $t = $this->Html->link($text,$this->link.$this->_pageDetails['nextPage']);
            }
            return $t;
        }   
        return false;
    }
 
}
?>
