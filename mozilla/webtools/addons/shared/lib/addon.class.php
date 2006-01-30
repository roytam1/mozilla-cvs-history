<?php
/**
 * Addon super class.  The class to end all classes.
 * @package amo
 * @subpackage lib
 * @todo properly separate accessors and mutators.
 */
class AddOn extends AMO_Object {
    // AddOn metadata.
    var $ID;
    var $GUID;
    var $Name;
    var $Type;
    var $DateAdded;
    var $DateUpdated;
    var $Homepage;
    var $Description;
    var $Rating;
    var $downloadcount;
    var $TotalDownloads;
    var $devcomments;
    var $db;
    var $tpl;

    // AddOn author metadata.
    var $UserID;
    var $UserName;
    var $UserEmail;
    var $UserWebsite;
    var $UserEmailHide;

    // Current version information.
    var $vID;
    var $Version;
    var $MinAppVer;
    var $MaxAppVer;
    var $Size;
    var $URI;
    var $Notes;
    var $VersionDateAdded;
    var $AppName;
    var $OSName;

    // Preview information.
    var $PreviewID;
    var $PreviewURI;
    var $PreviewHeight;
    var $PreviewWidth;
    var $Caption;
    var $Previews = array(); // Store the information for previews

    // Comments.
    var $Comments;

    // Categories.
    var $AddonCats;
    
    // History of releases
    var $History;

    /**
    * Class constructor.
    * 
    * @param int $ID AddOn ID
    */
    function AddOn($ID=null) {
        // Our DB and Smarty objects are global to save cycles.
        global $db, $tpl;

        // Pass by reference in order to save memory.
        $this->db =& $db;
        $this->tpl =& $tpl;

        // If $ID is set, attempt to retrieve data.
        if (!empty($ID)) {
            $this->ID = $ID;
            $this->getAddOn();
        }
    }

    /**
     * Get all commonly used AddOn information.
     */
    function getAddOn() {
        $this->getAddonCats();
        $this->getComments('3');
        $this->getCurrentVersion();
        $this->getMainPreview();
        $this->getUserInfo();
    }
    
    /**
     * Get the "highlight" for the current AddOn.
     */
    function getMainPreview() {
        // Gather previews information.
        $this->db->query(" 
            SELECT
                PreviewID,
                PreviewURI,
                Caption
            FROM
                previews
            WHERE
                ID = '{$this->ID}' AND
                preview = 'YES'
            LIMIT 1
        ", SQL_INIT, SQL_ASSOC);

        if (!empty($this->db->record)) {
            $this->setVars($this->db->record);

            if (file_exists(ROOT_PATH.'/htdocs'.$this->PreviewURI)) {
                $size = getimagesize(ROOT_PATH.'/htdocs'.$this->PreviewURI);
                $this->setVar('PreviewWidth',$size[0]);
                $this->setVar('PreviewHeight',$size[1]);
            }
        }

    }

    /**
     * Get all preview information attached to the current AddOn.
     */
    function getPreviews() {
        // Gather preview information
        $this->db->query("
            SELECT
                PreviewURI,
                caption
            FROM
                previews
            WHERE
                ID = {$this->ID}
            ORDER BY
                PreviewID ASC
        ", SQL_NONE);

        while ($this->db->next(SQL_ASSOC)) {
            $result = $this->db->record;
            $uri = $result['PreviewURI'];
            list($src_width, $src_height, $type, $attr) = getimagesize(ROOT_PATH.'/htdocs'.$uri);
            $this->Previews[] = array(
                'PreviewURI' => $uri,
                'caption' => $result['caption'],
                'width' => $src_width,
                'height' => $src_height
            );
        }
    }
     
    /**
     * Get all previous versions of the current AddOn.
     */
    function getHistory() {
        $this->db->query("
             SELECT DISTINCT
                 TV.vID,
                 TV.Version,
                 TV.MinAppVer,
                 TV.MaxAppVer,
                 TV.Size,
                 TV.URI,
                 TV.Notes,
                 UNIX_TIMESTAMP(TV.DateAdded) AS VerDateAdded,
                 TA.AppName,
                 TOS.OSName
            FROM
                version TV
            INNER JOIN applications TA ON TV.AppID = TA.AppID
            INNER JOIN os TOS ON TV.OSID = TOS.OSID
            WHERE
                TV.ID = {$this->ID} AND
                approved = 'YES'
            GROUP BY
                TV.Version
            ORDER BY
                TV.vID DESC
        ", SQL_ALL, SQL_ASSOC);

        $this->History = $this->db->record;
    }

    /**
     * Get information about the most recent verison of the current AddOn.
     */
    function getCurrentVersion() {
        $this->db->query("
            SELECT 
                version.vID, 
                version.Version, 
                version.MinAppVer, 
                version.MaxAppVer, 
                version.Size, 
                version.URI, 
                version.Notes, 
                version.DateAdded as VersionDateAdded,
                applications.AppName, 
                os.OSName
            FROM  
                version
            INNER JOIN applications ON version.AppID = applications.AppID
            INNER JOIN os ON version.OSID = os.OSID
            WHERE 
                version.ID = '{$this->ID}' AND 
                version.approved = 'YES'
            ORDER BY
                version.DateAdded DESC
            LIMIT 1
        ", SQL_INIT, SQL_ASSOC);

        if (!empty($this->db->record)) {
            $this->setVars($this->db->record);
        }
    }

    /**
     * Retrieve user information.
     *
     * @todo have this function set a User object instead
     */ 
    function getUserInfo() {
        // Gather addons metadata, user info.
        $this->db->query("
            SELECT 
                main.*,
                userprofiles.UserID,
                userprofiles.UserName,
                userprofiles.UserEmail,
                userprofiles.UserWebsite,
                userprofiles.UserEmailHide
            FROM 
                main 
            INNER JOIN authorxref ON authorxref.ID = main.ID
            INNER JOIN userprofiles ON userprofiles.UserID = authorxref.UserID
            WHERE 
                main.ID = '{$this->ID}'
        ", SQL_INIT, SQL_ASSOC);

        if (!empty($this->db->record)) {
            $this->setVars($this->db->record);
        }
    }

    /**
     * Get comments attached to this Addon.
     *
     * @param int $limit number of rows to limit by.
     * @todo add left/right limit clauses i.e. LIMIT 10,20 to work with pagination
     */
    function getComments($limit=null) {

        // Set the LIMIT if it is not null.
        $_limitSql = !empty($limit) ? " LIMIT {$limit} " : null;
    
        // Gather 10 latest comments.
        $this->db->query("
            SELECT
	        CommentID,
                CommentName,
                CommentTitle,
                CommentNote,
                CommentDate,
                CommentVote,
                `helpful-yes` as helpful_yes,
                `helpful-no` as helpful_no,
                `helpful-yes` + `helpful-no` as helpful_total
            FROM
                feedback
            WHERE
                ID = '{$this->ID}' AND
                CommentNote IS NOT NULL
            ORDER BY
                CommentDate DESC
            {$_limitSql}
        ", SQL_ALL, SQL_ASSOC);
        
        $this->setVar('Comments',$this->db->record);
    }

    /**
     * Retrieve all categories attached to the current AddOn.
     */
    function getAddonCats() {
        // Gather addon categories.
        $this->db->query("
            SELECT DISTINCT
                categories.CatName,
                categories.CategoryID
            FROM
                categoryxref
            INNER JOIN categories ON categoryxref.CategoryID = categories.CategoryID 
            INNER JOIN main ON categoryxref.ID = main.ID
            WHERE
                categoryxref.ID = {$this->ID}
            GROUP BY
                categories.CatName
            ORDER BY
                categories.CatName
        ", SQL_ALL, SQL_ASSOC);

        $this->setVar('AddonCats',$this->db->record);
    }
}
?>
