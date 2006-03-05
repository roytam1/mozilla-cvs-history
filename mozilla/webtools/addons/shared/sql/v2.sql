-- SQL specific to v2 needed for migration of v1 production database
-- --------------------------------------------------------

-- 
-- Table structure for table `session_data`
-- 

DROP TABLE IF EXISTS `session_data`;
CREATE TABLE `session_data` (
  `sess_id` varchar(255) NOT NULL default '',
  `sess_user_id` int(11) NOT NULL default '0',
  `sess_expires` int(11) unsigned NOT NULL default '0',
  `sess_data` text,
  PRIMARY KEY  (`sess_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;


ALTER TABLE `feedback` ADD `UserID` INT( 11 ) AFTER `ID`;
ALTER TABLE `feedback` ADD INDEX ( `UserID` );
ALTER TABLE `feedback` ADD CONSTRAINT `feedback_ibfk_2` FOREIGN KEY (`UserID`) REFERENCES `userprofiles` (`UserID`) ON DELETE CASCADE ON UPDATE CASCADE;

-- If you want the app to show up on the "supported applications" table for the
-- extension this has to be '1'
ALTER TABLE `applications` ADD `supported` TINYINT( 1 ) DEFAULT '0' NOT NULL ;

-- Set the "supported" applications:
UPDATE `applications` SET `supported`=1 WHERE `AppName` IN('Firefox','Thunderbird','Mozilla');
