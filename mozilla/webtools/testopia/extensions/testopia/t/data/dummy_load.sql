-- MySQL dump 10.11
--
-- Host: localhost    Database: bmo_test
-- ------------------------------------------------------
-- Server version	5.0.67-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `attach_data`
--

DROP TABLE IF EXISTS `attach_data`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `attach_data` (
  `id` mediumint(9) NOT NULL,
  `thedata` longblob NOT NULL,
  PRIMARY KEY  (`id`),
  CONSTRAINT `fk_attach_data_id_attachments_attach_id` FOREIGN KEY (`id`) REFERENCES `attachments` (`attach_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 MAX_ROWS=100000 AVG_ROW_LENGTH=1000000;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `attach_data`
--

LOCK TABLES `attach_data` WRITE;
/*!40000 ALTER TABLE `attach_data` DISABLE KEYS */;
INSERT INTO `attach_data` VALUES (1,'Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam pellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed iaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et metus. Nam varius, sapien nec egestas feugiat, mi libero dignissim orci, id fermentum quam nisl quis risus. Phasellus libero justo, aliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas sollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus placerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus, feugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus faucibus lectus eget felis. Nullam commodo tortor vitae turpis.\n\nSed mollis interdum risus. Pellentesque ante velit, facilisis vitae, fermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper nisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis ullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque nulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque dignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum tortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut, scelerisque vel, magna. Aenean nisl nulla, rutrum sit amet, sollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. In odio erat, bibendum eu, gravida nec, elementum sed, urna.\n\nAliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed tortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in purus. Sed at est non libero dignissim varius. Donec vestibulum odio ac felis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper eget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet, mattis nec, lacus. Nam tortor.\n\nNam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus velit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing enim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet sed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut sapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam non erat.\n\nSed feugiat, lacus in elementum egestas, sapien nulla sodales leo, nec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim pellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis quam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam erat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec posuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non dapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id, dapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam, fringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Sed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu, eros. \n');
/*!40000 ALTER TABLE `attach_data` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `attachments`
--

DROP TABLE IF EXISTS `attachments`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `attachments` (
  `attach_id` mediumint(9) NOT NULL auto_increment,
  `bug_id` mediumint(9) NOT NULL,
  `creation_ts` datetime NOT NULL,
  `description` tinytext NOT NULL,
  `mimetype` tinytext NOT NULL,
  `ispatch` tinyint(4) default NULL,
  `filename` varchar(100) NOT NULL,
  `submitter_id` mediumint(9) NOT NULL,
  `isobsolete` tinyint(4) NOT NULL default '0',
  `isprivate` tinyint(4) NOT NULL default '0',
  `isurl` tinyint(4) NOT NULL default '0',
  `modification_time` datetime NOT NULL,
  PRIMARY KEY  (`attach_id`),
  KEY `attachments_bug_id_idx` (`bug_id`),
  KEY `attachments_creation_ts_idx` (`creation_ts`),
  KEY `attachments_submitter_id_idx` (`submitter_id`,`bug_id`),
  KEY `attachments_modification_time_idx` (`modification_time`),
  CONSTRAINT `fk_attachments_submitter_id_profiles_userid` FOREIGN KEY (`submitter_id`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_attachments_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `attachments`
--

LOCK TABLES `attachments` WRITE;
/*!40000 ALTER TABLE `attachments` DISABLE KEYS */;
INSERT INTO `attachments` VALUES (1,3,'2008-05-02 15:10:00','LOREM','text/plain',0,'LOREM.TXT',1,0,0,0,'2008-05-02 15:10:00');
/*!40000 ALTER TABLE `attachments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bug_group_map`
--

DROP TABLE IF EXISTS `bug_group_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bug_group_map` (
  `bug_id` mediumint(9) NOT NULL,
  `group_id` mediumint(9) NOT NULL,
  UNIQUE KEY `bug_group_map_bug_id_idx` (`bug_id`,`group_id`),
  KEY `bug_group_map_group_id_idx` (`group_id`),
  CONSTRAINT `fk_bug_group_map_group_id_groups_id` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_bug_group_map_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bug_group_map`
--

LOCK TABLES `bug_group_map` WRITE;
/*!40000 ALTER TABLE `bug_group_map` DISABLE KEYS */;
INSERT INTO `bug_group_map` VALUES (4,15),(5,16),(6,16);
/*!40000 ALTER TABLE `bug_group_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bug_see_also`
--

DROP TABLE IF EXISTS `bug_see_also`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bug_see_also` (
  `bug_id` mediumint(9) NOT NULL,
  `value` varchar(255) NOT NULL,
  UNIQUE KEY `bug_see_also_bug_id_idx` (`bug_id`,`value`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bug_see_also`
--

LOCK TABLES `bug_see_also` WRITE;
/*!40000 ALTER TABLE `bug_see_also` DISABLE KEYS */;
/*!40000 ALTER TABLE `bug_see_also` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bug_severity`
--

DROP TABLE IF EXISTS `bug_severity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bug_severity` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `bug_severity_value_idx` (`value`),
  KEY `bug_severity_sortkey_idx` (`sortkey`,`value`),
  KEY `bug_severity_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bug_severity`
--

LOCK TABLES `bug_severity` WRITE;
/*!40000 ALTER TABLE `bug_severity` DISABLE KEYS */;
INSERT INTO `bug_severity` VALUES (1,'blocker',100,1,NULL),(2,'critical',200,1,NULL),(3,'major',300,1,NULL),(4,'normal',400,1,NULL),(5,'minor',500,1,NULL),(6,'trivial',600,1,NULL),(7,'enhancement',700,1,NULL);
/*!40000 ALTER TABLE `bug_severity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bug_status`
--

DROP TABLE IF EXISTS `bug_status`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bug_status` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `is_open` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `bug_status_value_idx` (`value`),
  KEY `bug_status_sortkey_idx` (`sortkey`,`value`),
  KEY `bug_status_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bug_status`
--

LOCK TABLES `bug_status` WRITE;
/*!40000 ALTER TABLE `bug_status` DISABLE KEYS */;
INSERT INTO `bug_status` VALUES (1,'UNCONFIRMED',100,1,1,NULL),(2,'NEW',200,1,1,NULL),(3,'ASSIGNED',300,1,1,NULL),(4,'REOPENED',400,1,1,NULL),(5,'RESOLVED',500,1,0,NULL),(6,'VERIFIED',600,1,0,NULL),(7,'CLOSED',700,1,0,NULL);
/*!40000 ALTER TABLE `bug_status` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bugs`
--

DROP TABLE IF EXISTS `bugs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bugs` (
  `bug_id` mediumint(9) NOT NULL auto_increment,
  `assigned_to` mediumint(9) NOT NULL,
  `bug_file_loc` mediumtext,
  `bug_severity` varchar(64) NOT NULL,
  `bug_status` varchar(64) NOT NULL,
  `creation_ts` datetime default NULL,
  `delta_ts` datetime NOT NULL,
  `short_desc` varchar(255) NOT NULL,
  `op_sys` varchar(64) NOT NULL,
  `priority` varchar(64) NOT NULL,
  `product_id` smallint(6) NOT NULL,
  `rep_platform` varchar(64) NOT NULL,
  `reporter` mediumint(9) NOT NULL,
  `version` varchar(64) NOT NULL,
  `component_id` smallint(6) NOT NULL,
  `resolution` varchar(64) NOT NULL default '',
  `target_milestone` varchar(20) NOT NULL default '---',
  `qa_contact` mediumint(9) default NULL,
  `status_whiteboard` mediumtext NOT NULL,
  `votes` mediumint(9) NOT NULL default '0',
  `keywords` mediumtext NOT NULL,
  `lastdiffed` datetime default NULL,
  `everconfirmed` tinyint(4) NOT NULL,
  `reporter_accessible` tinyint(4) NOT NULL default '1',
  `cclist_accessible` tinyint(4) NOT NULL default '1',
  `estimated_time` decimal(7,2) NOT NULL default '0.00',
  `remaining_time` decimal(7,2) NOT NULL default '0.00',
  `deadline` datetime default NULL,
  `alias` varchar(20) default NULL,
  `infoprovider` int(11) default NULL,
  PRIMARY KEY  (`bug_id`),
  UNIQUE KEY `bugs_alias_idx` (`alias`),
  KEY `bugs_assigned_to_idx` (`assigned_to`),
  KEY `bugs_creation_ts_idx` (`creation_ts`),
  KEY `bugs_delta_ts_idx` (`delta_ts`),
  KEY `bugs_bug_severity_idx` (`bug_severity`),
  KEY `bugs_bug_status_idx` (`bug_status`),
  KEY `bugs_op_sys_idx` (`op_sys`),
  KEY `bugs_priority_idx` (`priority`),
  KEY `bugs_product_id_idx` (`product_id`),
  KEY `bugs_reporter_idx` (`reporter`),
  KEY `bugs_version_idx` (`version`),
  KEY `bugs_component_id_idx` (`component_id`),
  KEY `bugs_resolution_idx` (`resolution`),
  KEY `bugs_target_milestone_idx` (`target_milestone`),
  KEY `bugs_qa_contact_idx` (`qa_contact`),
  KEY `bugs_votes_idx` (`votes`),
  CONSTRAINT `fk_bugs_component_id_components_id` FOREIGN KEY (`component_id`) REFERENCES `components` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_assigned_to_profiles_userid` FOREIGN KEY (`assigned_to`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_reporter_profiles_userid` FOREIGN KEY (`reporter`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bugs`
--

LOCK TABLES `bugs` WRITE;
/*!40000 ALTER TABLE `bugs` DISABLE KEYS */;
INSERT INTO `bugs` VALUES (1,4,'','normal','NEW','2008-03-27 15:48:39','2008-03-27 15:48:39','PUBLIC VISIBLE BUG','Linux','P5',1,'PC',1,'PUBLIC v1',1,'','PUBLIC M1',5,'',0,'','2008-03-27 15:48:39',1,1,1,'0.00','0.00',NULL,NULL,NULL),(2,4,'','enhancement','NEW','2008-05-01 17:24:10','2008-05-01 17:24:10','[Test Case 5] PUBLIC TEST CASE 3 - CONFIRMED','Linux','P5',1,'PC',1,'PUBLIC v1',1,'','PUBLIC M1',5,'',0,'','2008-05-01 17:24:11',1,1,1,'0.00','0.00',NULL,NULL,NULL),(3,4,'','enhancement','NEW','2008-05-02 15:10:00','2008-05-02 15:10:00','PUBLIC BUG','Linux','P5',1,'PC',1,'PUBLIC v1',1,'','PUBLIC M1',5,'',0,'','2008-05-02 15:10:01',1,1,1,'0.00','0.00',NULL,NULL,NULL),(4,2,'','enhancement','NEW','2008-05-02 15:19:36','2008-05-02 15:19:36','PARTNER VISIBLE BUG','Linux','P5',3,'PC',2,'PARTNER v2',4,'','PARTNER M1',6,'',0,'','2008-05-02 15:19:36',1,1,1,'0.00','0.00',NULL,NULL,NULL),(5,7,'','enhancement','NEW','2008-05-02 15:21:06','2008-05-02 15:21:06','PRIVATE BUG','Linux','P5',2,'PC',7,'PRIVATE v2',3,'','PRIVATE M1',8,'',0,'','2008-05-02 15:21:07',1,1,1,'0.00','0.00',NULL,NULL,NULL),(6,7,'','enhancement','NEW','2008-05-02 15:27:32','2008-05-02 15:27:32','[Test Case 15] PRIVATE CASE (RUN 3)','Linux','P5',2,'PC',3,'PRIVATE v2',3,'','PRIVATE M1',8,'',0,'','2008-05-02 15:27:32',1,1,1,'0.00','0.00',NULL,NULL,NULL);
/*!40000 ALTER TABLE `bugs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bugs_activity`
--

DROP TABLE IF EXISTS `bugs_activity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bugs_activity` (
  `bug_id` mediumint(9) NOT NULL,
  `attach_id` mediumint(9) default NULL,
  `who` mediumint(9) NOT NULL,
  `bug_when` datetime NOT NULL,
  `fieldid` mediumint(9) NOT NULL,
  `added` varchar(255) default NULL,
  `removed` tinytext,
  KEY `bugs_activity_bug_id_idx` (`bug_id`),
  KEY `bugs_activity_who_idx` (`who`),
  KEY `bugs_activity_bug_when_idx` (`bug_when`),
  KEY `bugs_activity_fieldid_idx` (`fieldid`),
  KEY `bugs_activity_added_idx` (`added`),
  KEY `fk_bugs_activity_attach_id_attachments_attach_id` (`attach_id`),
  CONSTRAINT `fk_bugs_activity_fieldid_fielddefs_id` FOREIGN KEY (`fieldid`) REFERENCES `fielddefs` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_activity_attach_id_attachments_attach_id` FOREIGN KEY (`attach_id`) REFERENCES `attachments` (`attach_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_activity_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_bugs_activity_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bugs_activity`
--

LOCK TABLES `bugs_activity` WRITE;
/*!40000 ALTER TABLE `bugs_activity` DISABLE KEYS */;
/*!40000 ALTER TABLE `bugs_activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bugs_fulltext`
--

DROP TABLE IF EXISTS `bugs_fulltext`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bugs_fulltext` (
  `bug_id` mediumint(9) NOT NULL,
  `short_desc` varchar(255) NOT NULL,
  `comments` mediumtext,
  `comments_noprivate` mediumtext,
  PRIMARY KEY  (`bug_id`),
  FULLTEXT KEY `bugs_fulltext_comments_noprivate_idx` (`comments_noprivate`),
  FULLTEXT KEY `bugs_fulltext_comments_idx` (`comments`),
  FULLTEXT KEY `bugs_fulltext_short_desc_idx` (`short_desc`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bugs_fulltext`
--

LOCK TABLES `bugs_fulltext` WRITE;
/*!40000 ALTER TABLE `bugs_fulltext` DISABLE KEYS */;
INSERT INTO `bugs_fulltext` VALUES (1,'PUBLIC VISIBLE BUG','PUBLIC VISIBLE BUG - basic','PUBLIC VISIBLE BUG - basic'),(2,'[Test Case 5] PUBLIC TEST CASE 3 - CONFIRMED','STATUS: IDLE\nBUILD: PUBLIC ACTIVE BUILD 1\nENVIRONMENT: PUBLIC ACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: \n\nPublic bug logged from test case 5 in run 1','STATUS: IDLE\nBUILD: PUBLIC ACTIVE BUILD 1\nENVIRONMENT: PUBLIC ACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: \n\nPublic bug logged from test case 5 in run 1'),(3,'PUBLIC BUG','THIS BUG IS PUBLIC','THIS BUG IS PUBLIC'),(4,'PARTNER VISIBLE BUG','PARTNER BUG','PARTNER BUG'),(5,'PRIVATE BUG','PRIVATE BUG','PRIVATE BUG'),(6,'[Test Case 15] PRIVATE CASE (RUN 3)','STATUS: IDLE\nBUILD: PRIVATE INACTIVE BUILD\nENVIRONMENT: PRIVATE INACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: Logged from run 3','STATUS: IDLE\nBUILD: PRIVATE INACTIVE BUILD\nENVIRONMENT: PRIVATE INACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: Logged from run 3');
/*!40000 ALTER TABLE `bugs_fulltext` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `bz_schema`
--

DROP TABLE IF EXISTS `bz_schema`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `bz_schema` (
  `schema_data` longblob NOT NULL,
  `version` decimal(3,2) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `bz_schema`
--

LOCK TABLES `bz_schema` WRITE;
/*!40000 ALTER TABLE `bz_schema` DISABLE KEYS */;
INSERT INTO `bz_schema` VALUES ('$VAR1 = {\n          \'attach_data\' => {\n                             \'FIELDS\' => [\n                                           \'id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'PRIMARYKEY\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'attach_id\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'attachments\'\n                                                             },\n                                             \'TYPE\' => \'INT3\'\n                                           },\n                                           \'thedata\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'LONGBLOB\'\n                                           }\n                                         ]\n                           },\n          \'attachments\' => {\n                             \'FIELDS\' => [\n                                           \'attach_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'PRIMARYKEY\' => 1,\n                                             \'TYPE\' => \'MEDIUMSERIAL\'\n                                           },\n                                           \'bug_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'bug_id\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'bugs\'\n                                                             },\n                                             \'TYPE\' => \'INT3\'\n                                           },\n                                           \'creation_ts\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'DATETIME\'\n                                           },\n                                           \'description\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'TINYTEXT\'\n                                           },\n                                           \'mimetype\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'TINYTEXT\'\n                                           },\n                                           \'ispatch\',\n                                           {\n                                             \'TYPE\' => \'BOOLEAN\'\n                                           },\n                                           \'filename\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'varchar(100)\'\n                                           },\n                                           \'submitter_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'userid\',\n                                                               \'TABLE\' => \'profiles\'\n                                                             },\n                                             \'TYPE\' => \'INT3\'\n                                           },\n                                           \'isobsolete\',\n                                           {\n                                             \'DEFAULT\' => \'FALSE\',\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'BOOLEAN\'\n                                           },\n                                           \'isprivate\',\n                                           {\n                                             \'DEFAULT\' => \'FALSE\',\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'BOOLEAN\'\n                                           },\n                                           \'isurl\',\n                                           {\n                                             \'DEFAULT\' => \'FALSE\',\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'BOOLEAN\'\n                                           },\n                                           \'modification_time\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'DATETIME\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'attachments_bug_id_idx\',\n                                            [\n                                              \'bug_id\'\n                                            ],\n                                            \'attachments_creation_ts_idx\',\n                                            [\n                                              \'creation_ts\'\n                                            ],\n                                            \'attachments_submitter_id_idx\',\n                                            [\n                                              \'submitter_id\',\n                                              \'bug_id\'\n                                            ],\n                                            \'attachments_modification_time_idx\',\n                                            [\n                                              \'modification_time\'\n                                            ]\n                                          ]\n                           },\n          \'bug_group_map\' => {\n                               \'FIELDS\' => [\n                                             \'bug_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'bug_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'bugs\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'group_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'groups\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'bug_group_map_bug_id_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'bug_id\',\n                                                              \'group_id\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              },\n                                              \'bug_group_map_group_id_idx\',\n                                              [\n                                                \'group_id\'\n                                              ]\n                                            ]\n                             },\n          \'bug_see_also\' => {\n                              \'FIELDS\' => [\n                                            \'bug_id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'value\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'varchar(255)\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'bug_see_also_bug_id_idx\',\n                                             {\n                                               \'FIELDS\' => [\n                                                             \'bug_id\',\n                                                             \'value\'\n                                                           ],\n                                               \'TYPE\' => \'UNIQUE\'\n                                             }\n                                           ]\n                            },\n          \'bug_severity\' => {\n                              \'FIELDS\' => [\n                                            \'id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'PRIMARYKEY\' => 1,\n                                              \'TYPE\' => \'SMALLSERIAL\'\n                                            },\n                                            \'value\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'varchar(64)\'\n                                            },\n                                            \'sortkey\',\n                                            {\n                                              \'DEFAULT\' => 0,\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'INT2\'\n                                            },\n                                            \'isactive\',\n                                            {\n                                              \'DEFAULT\' => \'TRUE\',\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'BOOLEAN\'\n                                            },\n                                            \'visibility_value_id\',\n                                            {\n                                              \'TYPE\' => \'INT2\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'bug_severity_value_idx\',\n                                             {\n                                               \'FIELDS\' => [\n                                                             \'value\'\n                                                           ],\n                                               \'TYPE\' => \'UNIQUE\'\n                                             },\n                                             \'bug_severity_sortkey_idx\',\n                                             [\n                                               \'sortkey\',\n                                               \'value\'\n                                             ],\n                                             \'bug_severity_visibility_value_id_idx\',\n                                             [\n                                               \'visibility_value_id\'\n                                             ]\n                                           ]\n                            },\n          \'bug_status\' => {\n                            \'FIELDS\' => [\n                                          \'id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'SMALLSERIAL\'\n                                          },\n                                          \'value\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(64)\'\n                                          },\n                                          \'sortkey\',\n                                          {\n                                            \'DEFAULT\' => 0,\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'isactive\',\n                                          {\n                                            \'DEFAULT\' => \'TRUE\',\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'BOOLEAN\'\n                                          },\n                                          \'is_open\',\n                                          {\n                                            \'DEFAULT\' => \'TRUE\',\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'BOOLEAN\'\n                                          },\n                                          \'visibility_value_id\',\n                                          {\n                                            \'TYPE\' => \'INT2\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'bug_status_value_idx\',\n                                           {\n                                             \'FIELDS\' => [\n                                                           \'value\'\n                                                         ],\n                                             \'TYPE\' => \'UNIQUE\'\n                                           },\n                                           \'bug_status_sortkey_idx\',\n                                           [\n                                             \'sortkey\',\n                                             \'value\'\n                                           ],\n                                           \'bug_status_visibility_value_id_idx\',\n                                           [\n                                             \'visibility_value_id\'\n                                           ]\n                                         ]\n                          },\n          \'bugs\' => {\n                      \'FIELDS\' => [\n                                    \'bug_id\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'PRIMARYKEY\' => 1,\n                                      \'TYPE\' => \'MEDIUMSERIAL\'\n                                    },\n                                    \'assigned_to\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'REFERENCES\' => {\n                                                        \'COLUMN\' => \'userid\',\n                                                        \'TABLE\' => \'profiles\'\n                                                      },\n                                      \'TYPE\' => \'INT3\'\n                                    },\n                                    \'bug_file_loc\',\n                                    {\n                                      \'TYPE\' => \'MEDIUMTEXT\'\n                                    },\n                                    \'bug_severity\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'bug_status\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'creation_ts\',\n                                    {\n                                      \'TYPE\' => \'DATETIME\'\n                                    },\n                                    \'delta_ts\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'DATETIME\'\n                                    },\n                                    \'short_desc\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(255)\'\n                                    },\n                                    \'op_sys\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'priority\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'product_id\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'REFERENCES\' => {\n                                                        \'COLUMN\' => \'id\',\n                                                        \'TABLE\' => \'products\'\n                                                      },\n                                      \'TYPE\' => \'INT2\'\n                                    },\n                                    \'rep_platform\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'reporter\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'REFERENCES\' => {\n                                                        \'COLUMN\' => \'userid\',\n                                                        \'TABLE\' => \'profiles\'\n                                                      },\n                                      \'TYPE\' => \'INT3\'\n                                    },\n                                    \'version\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'component_id\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'REFERENCES\' => {\n                                                        \'COLUMN\' => \'id\',\n                                                        \'TABLE\' => \'components\'\n                                                      },\n                                      \'TYPE\' => \'INT2\'\n                                    },\n                                    \'resolution\',\n                                    {\n                                      \'DEFAULT\' => \'\\\'\\\'\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(64)\'\n                                    },\n                                    \'target_milestone\',\n                                    {\n                                      \'DEFAULT\' => \'\\\'---\\\'\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'varchar(20)\'\n                                    },\n                                    \'qa_contact\',\n                                    {\n                                      \'TYPE\' => \'INT3\'\n                                    },\n                                    \'status_whiteboard\',\n                                    {\n                                      \'DEFAULT\' => \'\\\'\\\'\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'MEDIUMTEXT\'\n                                    },\n                                    \'votes\',\n                                    {\n                                      \'DEFAULT\' => \'0\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'INT3\'\n                                    },\n                                    \'keywords\',\n                                    {\n                                      \'DEFAULT\' => \'\\\'\\\'\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'MEDIUMTEXT\'\n                                    },\n                                    \'lastdiffed\',\n                                    {\n                                      \'TYPE\' => \'DATETIME\'\n                                    },\n                                    \'everconfirmed\',\n                                    {\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'BOOLEAN\'\n                                    },\n                                    \'reporter_accessible\',\n                                    {\n                                      \'DEFAULT\' => \'TRUE\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'BOOLEAN\'\n                                    },\n                                    \'cclist_accessible\',\n                                    {\n                                      \'DEFAULT\' => \'TRUE\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'BOOLEAN\'\n                                    },\n                                    \'estimated_time\',\n                                    {\n                                      \'DEFAULT\' => \'0\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'decimal(7,2)\'\n                                    },\n                                    \'remaining_time\',\n                                    {\n                                      \'DEFAULT\' => \'0\',\n                                      \'NOTNULL\' => 1,\n                                      \'TYPE\' => \'decimal(7,2)\'\n                                    },\n                                    \'deadline\',\n                                    {\n                                      \'TYPE\' => \'DATETIME\'\n                                    },\n                                    \'alias\',\n                                    {\n                                      \'TYPE\' => \'varchar(20)\'\n                                    }\n                                  ],\n                      \'INDEXES\' => [\n                                     \'bugs_alias_idx\',\n                                     {\n                                       \'FIELDS\' => [\n                                                     \'alias\'\n                                                   ],\n                                       \'TYPE\' => \'UNIQUE\'\n                                     },\n                                     \'bugs_assigned_to_idx\',\n                                     [\n                                       \'assigned_to\'\n                                     ],\n                                     \'bugs_creation_ts_idx\',\n                                     [\n                                       \'creation_ts\'\n                                     ],\n                                     \'bugs_delta_ts_idx\',\n                                     [\n                                       \'delta_ts\'\n                                     ],\n                                     \'bugs_bug_severity_idx\',\n                                     [\n                                       \'bug_severity\'\n                                     ],\n                                     \'bugs_bug_status_idx\',\n                                     [\n                                       \'bug_status\'\n                                     ],\n                                     \'bugs_op_sys_idx\',\n                                     [\n                                       \'op_sys\'\n                                     ],\n                                     \'bugs_priority_idx\',\n                                     [\n                                       \'priority\'\n                                     ],\n                                     \'bugs_product_id_idx\',\n                                     [\n                                       \'product_id\'\n                                     ],\n                                     \'bugs_reporter_idx\',\n                                     [\n                                       \'reporter\'\n                                     ],\n                                     \'bugs_version_idx\',\n                                     [\n                                       \'version\'\n                                     ],\n                                     \'bugs_component_id_idx\',\n                                     [\n                                       \'component_id\'\n                                     ],\n                                     \'bugs_resolution_idx\',\n                                     [\n                                       \'resolution\'\n                                     ],\n                                     \'bugs_target_milestone_idx\',\n                                     [\n                                       \'target_milestone\'\n                                     ],\n                                     \'bugs_qa_contact_idx\',\n                                     [\n                                       \'qa_contact\'\n                                     ],\n                                     \'bugs_votes_idx\',\n                                     [\n                                       \'votes\'\n                                     ]\n                                   ]\n                    },\n          \'bugs_activity\' => {\n                               \'FIELDS\' => [\n                                             \'bug_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'bug_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'bugs\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'attach_id\',\n                                             {\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'attach_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'attachments\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'who\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'userid\',\n                                                                 \'TABLE\' => \'profiles\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'bug_when\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'DATETIME\'\n                                             },\n                                             \'fieldid\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'id\',\n                                                                 \'TABLE\' => \'fielddefs\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'added\',\n                                             {\n                                               \'TYPE\' => \'varchar(255)\'\n                                             },\n                                             \'removed\',\n                                             {\n                                               \'TYPE\' => \'TINYTEXT\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'bugs_activity_bug_id_idx\',\n                                              [\n                                                \'bug_id\'\n                                              ],\n                                              \'bugs_activity_who_idx\',\n                                              [\n                                                \'who\'\n                                              ],\n                                              \'bugs_activity_bug_when_idx\',\n                                              [\n                                                \'bug_when\'\n                                              ],\n                                              \'bugs_activity_fieldid_idx\',\n                                              [\n                                                \'fieldid\'\n                                              ],\n                                              \'bugs_activity_added_idx\',\n                                              [\n                                                \'added\'\n                                              ]\n                                            ]\n                             },\n          \'bugs_fulltext\' => {\n                               \'FIELDS\' => [\n                                             \'bug_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'PRIMARYKEY\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'bug_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'bugs\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'short_desc\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'varchar(255)\'\n                                             },\n                                             \'comments\',\n                                             {\n                                               \'TYPE\' => \'LONGTEXT\'\n                                             },\n                                             \'comments_noprivate\',\n                                             {\n                                               \'TYPE\' => \'LONGTEXT\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'bugs_fulltext_comments_noprivate_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'comments_noprivate\'\n                                                            ],\n                                                \'TYPE\' => \'FULLTEXT\'\n                                              },\n                                              \'bugs_fulltext_comments_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'comments\'\n                                                            ],\n                                                \'TYPE\' => \'FULLTEXT\'\n                                              },\n                                              \'bugs_fulltext_short_desc_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'short_desc\'\n                                                            ],\n                                                \'TYPE\' => \'FULLTEXT\'\n                                              }\n                                            ]\n                             },\n          \'bz_schema\' => {\n                           \'FIELDS\' => [\n                                         \'schema_data\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'LONGBLOB\'\n                                         },\n                                         \'version\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'decimal(3,2)\'\n                                         }\n                                       ]\n                         },\n          \'category_group_map\' => {\n                                    \'FIELDS\' => [\n                                                  \'category_id\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'id\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'series_categories\'\n                                                                    },\n                                                    \'TYPE\' => \'INT2\'\n                                                  },\n                                                  \'group_id\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'id\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'groups\'\n                                                                    },\n                                                    \'TYPE\' => \'INT3\'\n                                                  }\n                                                ],\n                                    \'INDEXES\' => [\n                                                   \'category_group_map_category_id_idx\',\n                                                   {\n                                                     \'FIELDS\' => [\n                                                                   \'category_id\',\n                                                                   \'group_id\'\n                                                                 ],\n                                                     \'TYPE\' => \'UNIQUE\'\n                                                   }\n                                                 ]\n                                  },\n          \'cc\' => {\n                    \'FIELDS\' => [\n                                  \'bug_id\',\n                                  {\n                                    \'NOTNULL\' => 1,\n                                    \'REFERENCES\' => {\n                                                      \'COLUMN\' => \'bug_id\',\n                                                      \'DELETE\' => \'CASCADE\',\n                                                      \'TABLE\' => \'bugs\'\n                                                    },\n                                    \'TYPE\' => \'INT3\'\n                                  },\n                                  \'who\',\n                                  {\n                                    \'NOTNULL\' => 1,\n                                    \'REFERENCES\' => {\n                                                      \'COLUMN\' => \'userid\',\n                                                      \'DELETE\' => \'CASCADE\',\n                                                      \'TABLE\' => \'profiles\'\n                                                    },\n                                    \'TYPE\' => \'INT3\'\n                                  }\n                                ],\n                    \'INDEXES\' => [\n                                   \'cc_bug_id_idx\',\n                                   {\n                                     \'FIELDS\' => [\n                                                   \'bug_id\',\n                                                   \'who\'\n                                                 ],\n                                     \'TYPE\' => \'UNIQUE\'\n                                   },\n                                   \'cc_who_idx\',\n                                   [\n                                     \'who\'\n                                   ]\n                                 ]\n                  },\n          \'classifications\' => {\n                                 \'FIELDS\' => [\n                                               \'id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'PRIMARYKEY\' => 1,\n                                                 \'TYPE\' => \'SMALLSERIAL\'\n                                               },\n                                               \'name\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'varchar(64)\'\n                                               },\n                                               \'description\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               },\n                                               \'sortkey\',\n                                               {\n                                                 \'DEFAULT\' => \'0\',\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT2\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'classifications_name_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'name\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                }\n                                              ]\n                               },\n          \'component_cc\' => {\n                              \'FIELDS\' => [\n                                            \'user_id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'userid\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'profiles\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'component_id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'id\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'components\'\n                                                              },\n                                              \'TYPE\' => \'INT2\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'component_cc_user_id_idx\',\n                                             {\n                                               \'FIELDS\' => [\n                                                             \'component_id\',\n                                                             \'user_id\'\n                                                           ],\n                                               \'TYPE\' => \'UNIQUE\'\n                                             }\n                                           ]\n                            },\n          \'components\' => {\n                            \'FIELDS\' => [\n                                          \'id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'SMALLSERIAL\'\n                                          },\n                                          \'name\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(64)\'\n                                          },\n                                          \'product_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'products\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'initialowner\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'userid\',\n                                                              \'TABLE\' => \'profiles\'\n                                                            },\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'initialqacontact\',\n                                          {\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'userid\',\n                                                              \'DELETE\' => \'SET NULL\',\n                                                              \'TABLE\' => \'profiles\'\n                                                            },\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'description\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'MEDIUMTEXT\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'components_product_id_idx\',\n                                           {\n                                             \'FIELDS\' => [\n                                                           \'product_id\',\n                                                           \'name\'\n                                                         ],\n                                             \'TYPE\' => \'UNIQUE\'\n                                           },\n                                           \'components_name_idx\',\n                                           [\n                                             \'name\'\n                                           ]\n                                         ]\n                          },\n          \'dependencies\' => {\n                              \'FIELDS\' => [\n                                            \'blocked\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'bug_id\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'bugs\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'dependson\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'bug_id\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'bugs\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'dependencies_blocked_idx\',\n                                             [\n                                               \'blocked\'\n                                             ],\n                                             \'dependencies_dependson_idx\',\n                                             [\n                                               \'dependson\'\n                                             ]\n                                           ]\n                            },\n          \'duplicates\' => {\n                            \'FIELDS\' => [\n                                          \'dupe_of\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'bug_id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'bugs\'\n                                                            },\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'dupe\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'bug_id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'bugs\'\n                                                            },\n                                            \'TYPE\' => \'INT3\'\n                                          }\n                                        ]\n                          },\n          \'email_setting\' => {\n                               \'FIELDS\' => [\n                                             \'user_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'userid\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'profiles\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'relationship\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT1\'\n                                             },\n                                             \'event\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT1\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'email_setting_user_id_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'user_id\',\n                                                              \'relationship\',\n                                                              \'event\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              }\n                                            ]\n                             },\n          \'fielddefs\' => {\n                           \'FIELDS\' => [\n                                         \'id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'PRIMARYKEY\' => 1,\n                                           \'TYPE\' => \'MEDIUMSERIAL\'\n                                         },\n                                         \'name\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'varchar(64)\'\n                                         },\n                                         \'type\',\n                                         {\n                                           \'DEFAULT\' => 0,\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'INT2\'\n                                         },\n                                         \'custom\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'description\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'TINYTEXT\'\n                                         },\n                                         \'mailhead\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'sortkey\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'INT2\'\n                                         },\n                                         \'obsolete\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'enter_bug\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'visibility_field_id\',\n                                         {\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'id\',\n                                                             \'TABLE\' => \'fielddefs\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'visibility_value_id\',\n                                         {\n                                           \'TYPE\' => \'INT2\'\n                                         },\n                                         \'value_field_id\',\n                                         {\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'id\',\n                                                             \'TABLE\' => \'fielddefs\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'buglist\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         }\n                                       ],\n                           \'INDEXES\' => [\n                                          \'fielddefs_name_idx\',\n                                          {\n                                            \'FIELDS\' => [\n                                                          \'name\'\n                                                        ],\n                                            \'TYPE\' => \'UNIQUE\'\n                                          },\n                                          \'fielddefs_sortkey_idx\',\n                                          [\n                                            \'sortkey\'\n                                          ],\n                                          \'fielddefs_value_field_id_idx\',\n                                          [\n                                            \'value_field_id\'\n                                          ]\n                                        ]\n                         },\n          \'flagexclusions\' => {\n                                \'FIELDS\' => [\n                                              \'type_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'flagtypes\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              },\n                                              \'product_id\',\n                                              {\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'products\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              },\n                                              \'component_id\',\n                                              {\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'components\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'flagexclusions_type_id_idx\',\n                                               [\n                                                 \'type_id\',\n                                                 \'product_id\',\n                                                 \'component_id\'\n                                               ]\n                                             ]\n                              },\n          \'flaginclusions\' => {\n                                \'FIELDS\' => [\n                                              \'type_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'flagtypes\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              },\n                                              \'product_id\',\n                                              {\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'products\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              },\n                                              \'component_id\',\n                                              {\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'components\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'flaginclusions_type_id_idx\',\n                                               [\n                                                 \'type_id\',\n                                                 \'product_id\',\n                                                 \'component_id\'\n                                               ]\n                                             ]\n                              },\n          \'flags\' => {\n                       \'FIELDS\' => [\n                                     \'id\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'PRIMARYKEY\' => 1,\n                                       \'TYPE\' => \'MEDIUMSERIAL\'\n                                     },\n                                     \'type_id\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'id\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'flagtypes\'\n                                                       },\n                                       \'TYPE\' => \'INT2\'\n                                     },\n                                     \'status\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'TYPE\' => \'char(1)\'\n                                     },\n                                     \'bug_id\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'bug_id\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'bugs\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'attach_id\',\n                                     {\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'attach_id\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'attachments\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'creation_date\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'TYPE\' => \'DATETIME\'\n                                     },\n                                     \'modification_date\',\n                                     {\n                                       \'TYPE\' => \'DATETIME\'\n                                     },\n                                     \'setter_id\',\n                                     {\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'requestee_id\',\n                                     {\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     }\n                                   ],\n                       \'INDEXES\' => [\n                                      \'flags_bug_id_idx\',\n                                      [\n                                        \'bug_id\',\n                                        \'attach_id\'\n                                      ],\n                                      \'flags_setter_id_idx\',\n                                      [\n                                        \'setter_id\'\n                                      ],\n                                      \'flags_requestee_id_idx\',\n                                      [\n                                        \'requestee_id\'\n                                      ],\n                                      \'flags_type_id_idx\',\n                                      [\n                                        \'type_id\'\n                                      ]\n                                    ]\n                     },\n          \'flagtypes\' => {\n                           \'FIELDS\' => [\n                                         \'id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'PRIMARYKEY\' => 1,\n                                           \'TYPE\' => \'SMALLSERIAL\'\n                                         },\n                                         \'name\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'varchar(50)\'\n                                         },\n                                         \'description\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'MEDIUMTEXT\'\n                                         },\n                                         \'cc_list\',\n                                         {\n                                           \'TYPE\' => \'varchar(200)\'\n                                         },\n                                         \'target_type\',\n                                         {\n                                           \'DEFAULT\' => \'\\\'b\\\'\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'char(1)\'\n                                         },\n                                         \'is_active\',\n                                         {\n                                           \'DEFAULT\' => \'TRUE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'is_requestable\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'is_requesteeble\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'is_multiplicable\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'sortkey\',\n                                         {\n                                           \'DEFAULT\' => \'0\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'INT2\'\n                                         },\n                                         \'grant_group_id\',\n                                         {\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'id\',\n                                                             \'DELETE\' => \'SET NULL\',\n                                                             \'TABLE\' => \'groups\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'request_group_id\',\n                                         {\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'id\',\n                                                             \'DELETE\' => \'SET NULL\',\n                                                             \'TABLE\' => \'groups\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         }\n                                       ]\n                         },\n          \'group_control_map\' => {\n                                   \'FIELDS\' => [\n                                                 \'group_id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'id\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'groups\'\n                                                                   },\n                                                   \'TYPE\' => \'INT3\'\n                                                 },\n                                                 \'product_id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'id\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'products\'\n                                                                   },\n                                                   \'TYPE\' => \'INT2\'\n                                                 },\n                                                 \'entry\',\n                                                 {\n                                                   \'DEFAULT\' => \'FALSE\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'membercontrol\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'othercontrol\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'canedit\',\n                                                 {\n                                                   \'DEFAULT\' => \'FALSE\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'editcomponents\',\n                                                 {\n                                                   \'DEFAULT\' => \'FALSE\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'editbugs\',\n                                                 {\n                                                   \'DEFAULT\' => \'FALSE\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 },\n                                                 \'canconfirm\',\n                                                 {\n                                                   \'DEFAULT\' => \'FALSE\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 }\n                                               ],\n                                   \'INDEXES\' => [\n                                                  \'group_control_map_product_id_idx\',\n                                                  {\n                                                    \'FIELDS\' => [\n                                                                  \'product_id\',\n                                                                  \'group_id\'\n                                                                ],\n                                                    \'TYPE\' => \'UNIQUE\'\n                                                  },\n                                                  \'group_control_map_group_id_idx\',\n                                                  [\n                                                    \'group_id\'\n                                                  ]\n                                                ]\n                                 },\n          \'group_group_map\' => {\n                                 \'FIELDS\' => [\n                                               \'member_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'groups\'\n                                                                 },\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'grantor_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'groups\'\n                                                                 },\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'grant_type\',\n                                               {\n                                                 \'DEFAULT\' => \'0\',\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT1\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'group_group_map_member_id_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'member_id\',\n                                                                \'grantor_id\',\n                                                                \'grant_type\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                }\n                                              ]\n                               },\n          \'groups\' => {\n                        \'FIELDS\' => [\n                                      \'id\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'PRIMARYKEY\' => 1,\n                                        \'TYPE\' => \'MEDIUMSERIAL\'\n                                      },\n                                      \'name\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'varchar(255)\'\n                                      },\n                                      \'description\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'MEDIUMTEXT\'\n                                      },\n                                      \'isbuggroup\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'BOOLEAN\'\n                                      },\n                                      \'userregexp\',\n                                      {\n                                        \'DEFAULT\' => \'\\\'\\\'\',\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'TINYTEXT\'\n                                      },\n                                      \'isactive\',\n                                      {\n                                        \'DEFAULT\' => \'TRUE\',\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'BOOLEAN\'\n                                      },\n                                      \'icon_url\',\n                                      {\n                                        \'TYPE\' => \'TINYTEXT\'\n                                      }\n                                    ],\n                        \'INDEXES\' => [\n                                       \'groups_name_idx\',\n                                       {\n                                         \'FIELDS\' => [\n                                                       \'name\'\n                                                     ],\n                                         \'TYPE\' => \'UNIQUE\'\n                                       }\n                                     ]\n                      },\n          \'keyworddefs\' => {\n                             \'FIELDS\' => [\n                                           \'id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'PRIMARYKEY\' => 1,\n                                             \'TYPE\' => \'SMALLSERIAL\'\n                                           },\n                                           \'name\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'varchar(64)\'\n                                           },\n                                           \'description\',\n                                           {\n                                             \'TYPE\' => \'MEDIUMTEXT\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'keyworddefs_name_idx\',\n                                            {\n                                              \'FIELDS\' => [\n                                                            \'name\'\n                                                          ],\n                                              \'TYPE\' => \'UNIQUE\'\n                                            }\n                                          ]\n                           },\n          \'keywords\' => {\n                          \'FIELDS\' => [\n                                        \'bug_id\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'REFERENCES\' => {\n                                                            \'COLUMN\' => \'bug_id\',\n                                                            \'DELETE\' => \'CASCADE\',\n                                                            \'TABLE\' => \'bugs\'\n                                                          },\n                                          \'TYPE\' => \'INT3\'\n                                        },\n                                        \'keywordid\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'REFERENCES\' => {\n                                                            \'COLUMN\' => \'id\',\n                                                            \'DELETE\' => \'CASCADE\',\n                                                            \'TABLE\' => \'keyworddefs\'\n                                                          },\n                                          \'TYPE\' => \'INT2\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'keywords_bug_id_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'bug_id\',\n                                                         \'keywordid\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         },\n                                         \'keywords_keywordid_idx\',\n                                         [\n                                           \'keywordid\'\n                                         ]\n                                       ]\n                        },\n          \'login_failure\' => {\n                               \'FIELDS\' => [\n                                             \'user_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'userid\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'profiles\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'login_time\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'DATETIME\'\n                                             },\n                                             \'ip_addr\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'varchar(40)\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'login_failure_user_id_idx\',\n                                              [\n                                                \'user_id\'\n                                              ]\n                                            ]\n                             },\n          \'logincookies\' => {\n                              \'FIELDS\' => [\n                                            \'cookie\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'PRIMARYKEY\' => 1,\n                                              \'TYPE\' => \'varchar(16)\'\n                                            },\n                                            \'userid\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'userid\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'profiles\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'ipaddr\',\n                                            {\n                                              \'TYPE\' => \'varchar(40)\'\n                                            },\n                                            \'lastused\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'DATETIME\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'logincookies_lastused_idx\',\n                                             [\n                                               \'lastused\'\n                                             ]\n                                           ]\n                            },\n          \'longdescs\' => {\n                           \'FIELDS\' => [\n                                         \'comment_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'PRIMARYKEY\' => 1,\n                                           \'TYPE\' => \'MEDIUMSERIAL\'\n                                         },\n                                         \'bug_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'bug_id\',\n                                                             \'DELETE\' => \'CASCADE\',\n                                                             \'TABLE\' => \'bugs\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'who\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'userid\',\n                                                             \'TABLE\' => \'profiles\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'bug_when\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'DATETIME\'\n                                         },\n                                         \'work_time\',\n                                         {\n                                           \'DEFAULT\' => \'0\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'decimal(7,2)\'\n                                         },\n                                         \'thetext\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'LONGTEXT\'\n                                         },\n                                         \'isprivate\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'already_wrapped\',\n                                         {\n                                           \'DEFAULT\' => \'FALSE\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'BOOLEAN\'\n                                         },\n                                         \'type\',\n                                         {\n                                           \'DEFAULT\' => \'0\',\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'INT2\'\n                                         },\n                                         \'extra_data\',\n                                         {\n                                           \'TYPE\' => \'varchar(255)\'\n                                         }\n                                       ],\n                           \'INDEXES\' => [\n                                          \'longdescs_bug_id_idx\',\n                                          [\n                                            \'bug_id\'\n                                          ],\n                                          \'longdescs_who_idx\',\n                                          [\n                                            \'who\',\n                                            \'bug_id\'\n                                          ],\n                                          \'longdescs_bug_when_idx\',\n                                          [\n                                            \'bug_when\'\n                                          ]\n                                        ]\n                         },\n          \'milestones\' => {\n                            \'FIELDS\' => [\n                                          \'id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'MEDIUMSERIAL\'\n                                          },\n                                          \'product_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'products\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'value\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(20)\'\n                                          },\n                                          \'sortkey\',\n                                          {\n                                            \'DEFAULT\' => 0,\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'INT2\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'milestones_product_id_idx\',\n                                           {\n                                             \'FIELDS\' => [\n                                                           \'product_id\',\n                                                           \'value\'\n                                                         ],\n                                             \'TYPE\' => \'UNIQUE\'\n                                           }\n                                         ]\n                          },\n          \'namedqueries\' => {\n                              \'FIELDS\' => [\n                                            \'id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'PRIMARYKEY\' => 1,\n                                              \'TYPE\' => \'MEDIUMSERIAL\'\n                                            },\n                                            \'userid\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'userid\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'profiles\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'name\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'varchar(64)\'\n                                            },\n                                            \'query\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'LONGTEXT\'\n                                            },\n                                            \'query_type\',\n                                            {\n                                              \'DEFAULT\' => 0,\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'BOOLEAN\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'namedqueries_userid_idx\',\n                                             {\n                                               \'FIELDS\' => [\n                                                             \'userid\',\n                                                             \'name\'\n                                                           ],\n                                               \'TYPE\' => \'UNIQUE\'\n                                             }\n                                           ]\n                            },\n          \'namedqueries_link_in_footer\' => {\n                                             \'FIELDS\' => [\n                                                           \'namedquery_id\',\n                                                           {\n                                                             \'NOTNULL\' => 1,\n                                                             \'REFERENCES\' => {\n                                                                               \'COLUMN\' => \'id\',\n                                                                               \'DELETE\' => \'CASCADE\',\n                                                                               \'TABLE\' => \'namedqueries\'\n                                                                             },\n                                                             \'TYPE\' => \'INT3\'\n                                                           },\n                                                           \'user_id\',\n                                                           {\n                                                             \'NOTNULL\' => 1,\n                                                             \'REFERENCES\' => {\n                                                                               \'COLUMN\' => \'userid\',\n                                                                               \'DELETE\' => \'CASCADE\',\n                                                                               \'TABLE\' => \'profiles\'\n                                                                             },\n                                                             \'TYPE\' => \'INT3\'\n                                                           }\n                                                         ],\n                                             \'INDEXES\' => [\n                                                            \'namedqueries_link_in_footer_id_idx\',\n                                                            {\n                                                              \'FIELDS\' => [\n                                                                            \'namedquery_id\',\n                                                                            \'user_id\'\n                                                                          ],\n                                                              \'TYPE\' => \'UNIQUE\'\n                                                            },\n                                                            \'namedqueries_link_in_footer_userid_idx\',\n                                                            [\n                                                              \'user_id\'\n                                                            ]\n                                                          ]\n                                           },\n          \'namedquery_group_map\' => {\n                                      \'FIELDS\' => [\n                                                    \'namedquery_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'namedqueries\'\n                                                                      },\n                                                      \'TYPE\' => \'INT3\'\n                                                    },\n                                                    \'group_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'groups\'\n                                                                      },\n                                                      \'TYPE\' => \'INT3\'\n                                                    }\n                                                  ],\n                                      \'INDEXES\' => [\n                                                     \'namedquery_group_map_namedquery_id_idx\',\n                                                     {\n                                                       \'FIELDS\' => [\n                                                                     \'namedquery_id\'\n                                                                   ],\n                                                       \'TYPE\' => \'UNIQUE\'\n                                                     },\n                                                     \'namedquery_group_map_group_id_idx\',\n                                                     [\n                                                       \'group_id\'\n                                                     ]\n                                                   ]\n                                    },\n          \'op_sys\' => {\n                        \'FIELDS\' => [\n                                      \'id\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'PRIMARYKEY\' => 1,\n                                        \'TYPE\' => \'SMALLSERIAL\'\n                                      },\n                                      \'value\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'varchar(64)\'\n                                      },\n                                      \'sortkey\',\n                                      {\n                                        \'DEFAULT\' => 0,\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'INT2\'\n                                      },\n                                      \'isactive\',\n                                      {\n                                        \'DEFAULT\' => \'TRUE\',\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'BOOLEAN\'\n                                      },\n                                      \'visibility_value_id\',\n                                      {\n                                        \'TYPE\' => \'INT2\'\n                                      }\n                                    ],\n                        \'INDEXES\' => [\n                                       \'op_sys_value_idx\',\n                                       {\n                                         \'FIELDS\' => [\n                                                       \'value\'\n                                                     ],\n                                         \'TYPE\' => \'UNIQUE\'\n                                       },\n                                       \'op_sys_sortkey_idx\',\n                                       [\n                                         \'sortkey\',\n                                         \'value\'\n                                       ],\n                                       \'op_sys_visibility_value_id_idx\',\n                                       [\n                                         \'visibility_value_id\'\n                                       ]\n                                     ]\n                      },\n          \'priority\' => {\n                          \'FIELDS\' => [\n                                        \'id\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'PRIMARYKEY\' => 1,\n                                          \'TYPE\' => \'SMALLSERIAL\'\n                                        },\n                                        \'value\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(64)\'\n                                        },\n                                        \'sortkey\',\n                                        {\n                                          \'DEFAULT\' => 0,\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT2\'\n                                        },\n                                        \'isactive\',\n                                        {\n                                          \'DEFAULT\' => \'TRUE\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'BOOLEAN\'\n                                        },\n                                        \'visibility_value_id\',\n                                        {\n                                          \'TYPE\' => \'INT2\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'priority_value_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'value\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         },\n                                         \'priority_sortkey_idx\',\n                                         [\n                                           \'sortkey\',\n                                           \'value\'\n                                         ],\n                                         \'priority_visibility_value_id_idx\',\n                                         [\n                                           \'visibility_value_id\'\n                                         ]\n                                       ]\n                        },\n          \'products\' => {\n                          \'FIELDS\' => [\n                                        \'id\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'PRIMARYKEY\' => 1,\n                                          \'TYPE\' => \'SMALLSERIAL\'\n                                        },\n                                        \'name\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(64)\'\n                                        },\n                                        \'classification_id\',\n                                        {\n                                          \'DEFAULT\' => \'1\',\n                                          \'NOTNULL\' => 1,\n                                          \'REFERENCES\' => {\n                                                            \'COLUMN\' => \'id\',\n                                                            \'DELETE\' => \'CASCADE\',\n                                                            \'TABLE\' => \'classifications\'\n                                                          },\n                                          \'TYPE\' => \'INT2\'\n                                        },\n                                        \'description\',\n                                        {\n                                          \'TYPE\' => \'MEDIUMTEXT\'\n                                        },\n                                        \'votesperuser\',\n                                        {\n                                          \'DEFAULT\' => 0,\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT2\'\n                                        },\n                                        \'maxvotesperbug\',\n                                        {\n                                          \'DEFAULT\' => \'10000\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT2\'\n                                        },\n                                        \'votestoconfirm\',\n                                        {\n                                          \'DEFAULT\' => 0,\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT2\'\n                                        },\n                                        \'defaultmilestone\',\n                                        {\n                                          \'DEFAULT\' => \'\\\'---\\\'\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(20)\'\n                                        },\n                                        \'isactive\',\n                                        {\n                                          \'DEFAULT\' => \'TRUE\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'BOOLEAN\'\n                                        },\n                                        \'allows_unconfirmed\',\n                                        {\n                                          \'DEFAULT\' => \'FALSE\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'BOOLEAN\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'products_name_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'name\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         }\n                                       ]\n                        },\n          \'profile_setting\' => {\n                                 \'FIELDS\' => [\n                                               \'user_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'userid\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'profiles\'\n                                                                 },\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'setting_name\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'name\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'setting\'\n                                                                 },\n                                                 \'TYPE\' => \'varchar(32)\'\n                                               },\n                                               \'setting_value\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'varchar(32)\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'profile_setting_value_unique_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'user_id\',\n                                                                \'setting_name\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                }\n                                              ]\n                               },\n          \'profiles\' => {\n                          \'FIELDS\' => [\n                                        \'userid\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'PRIMARYKEY\' => 1,\n                                          \'TYPE\' => \'MEDIUMSERIAL\'\n                                        },\n                                        \'login_name\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(255)\'\n                                        },\n                                        \'cryptpassword\',\n                                        {\n                                          \'TYPE\' => \'varchar(128)\'\n                                        },\n                                        \'realname\',\n                                        {\n                                          \'DEFAULT\' => \'\\\'\\\'\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(255)\'\n                                        },\n                                        \'disabledtext\',\n                                        {\n                                          \'DEFAULT\' => \'\\\'\\\'\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'MEDIUMTEXT\'\n                                        },\n                                        \'disable_mail\',\n                                        {\n                                          \'DEFAULT\' => \'FALSE\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'BOOLEAN\'\n                                        },\n                                        \'mybugslink\',\n                                        {\n                                          \'DEFAULT\' => \'TRUE\',\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'BOOLEAN\'\n                                        },\n                                        \'extern_id\',\n                                        {\n                                          \'TYPE\' => \'varchar(64)\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'profiles_login_name_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'login_name\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         },\n                                         \'profiles_extern_id_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'extern_id\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         }\n                                       ]\n                        },\n          \'profiles_activity\' => {\n                                   \'FIELDS\' => [\n                                                 \'userid\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'userid\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'profiles\'\n                                                                   },\n                                                   \'TYPE\' => \'INT3\'\n                                                 },\n                                                 \'who\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'userid\',\n                                                                     \'TABLE\' => \'profiles\'\n                                                                   },\n                                                   \'TYPE\' => \'INT3\'\n                                                 },\n                                                 \'profiles_when\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'DATETIME\'\n                                                 },\n                                                 \'fieldid\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'id\',\n                                                                     \'TABLE\' => \'fielddefs\'\n                                                                   },\n                                                   \'TYPE\' => \'INT3\'\n                                                 },\n                                                 \'oldvalue\',\n                                                 {\n                                                   \'TYPE\' => \'TINYTEXT\'\n                                                 },\n                                                 \'newvalue\',\n                                                 {\n                                                   \'TYPE\' => \'TINYTEXT\'\n                                                 }\n                                               ],\n                                   \'INDEXES\' => [\n                                                  \'profiles_activity_userid_idx\',\n                                                  [\n                                                    \'userid\'\n                                                  ],\n                                                  \'profiles_activity_profiles_when_idx\',\n                                                  [\n                                                    \'profiles_when\'\n                                                  ],\n                                                  \'profiles_activity_fieldid_idx\',\n                                                  [\n                                                    \'fieldid\'\n                                                  ]\n                                                ]\n                                 },\n          \'quips\' => {\n                       \'FIELDS\' => [\n                                     \'quipid\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'PRIMARYKEY\' => 1,\n                                       \'TYPE\' => \'MEDIUMSERIAL\'\n                                     },\n                                     \'userid\',\n                                     {\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'DELETE\' => \'SET NULL\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'quip\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'TYPE\' => \'MEDIUMTEXT\'\n                                     },\n                                     \'approved\',\n                                     {\n                                       \'DEFAULT\' => \'TRUE\',\n                                       \'NOTNULL\' => 1,\n                                       \'TYPE\' => \'BOOLEAN\'\n                                     }\n                                   ]\n                     },\n          \'rep_platform\' => {\n                              \'FIELDS\' => [\n                                            \'id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'PRIMARYKEY\' => 1,\n                                              \'TYPE\' => \'SMALLSERIAL\'\n                                            },\n                                            \'value\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'varchar(64)\'\n                                            },\n                                            \'sortkey\',\n                                            {\n                                              \'DEFAULT\' => 0,\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'INT2\'\n                                            },\n                                            \'isactive\',\n                                            {\n                                              \'DEFAULT\' => \'TRUE\',\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'BOOLEAN\'\n                                            },\n                                            \'visibility_value_id\',\n                                            {\n                                              \'TYPE\' => \'INT2\'\n                                            }\n                                          ],\n                              \'INDEXES\' => [\n                                             \'rep_platform_value_idx\',\n                                             {\n                                               \'FIELDS\' => [\n                                                             \'value\'\n                                                           ],\n                                               \'TYPE\' => \'UNIQUE\'\n                                             },\n                                             \'rep_platform_sortkey_idx\',\n                                             [\n                                               \'sortkey\',\n                                               \'value\'\n                                             ],\n                                             \'rep_platform_visibility_value_id_idx\',\n                                             [\n                                               \'visibility_value_id\'\n                                             ]\n                                           ]\n                            },\n          \'resolution\' => {\n                            \'FIELDS\' => [\n                                          \'id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'SMALLSERIAL\'\n                                          },\n                                          \'value\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(64)\'\n                                          },\n                                          \'sortkey\',\n                                          {\n                                            \'DEFAULT\' => 0,\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'isactive\',\n                                          {\n                                            \'DEFAULT\' => \'TRUE\',\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'BOOLEAN\'\n                                          },\n                                          \'visibility_value_id\',\n                                          {\n                                            \'TYPE\' => \'INT2\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'resolution_value_idx\',\n                                           {\n                                             \'FIELDS\' => [\n                                                           \'value\'\n                                                         ],\n                                             \'TYPE\' => \'UNIQUE\'\n                                           },\n                                           \'resolution_sortkey_idx\',\n                                           [\n                                             \'sortkey\',\n                                             \'value\'\n                                           ],\n                                           \'resolution_visibility_value_id_idx\',\n                                           [\n                                             \'visibility_value_id\'\n                                           ]\n                                         ]\n                          },\n          \'series\' => {\n                        \'FIELDS\' => [\n                                      \'series_id\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'PRIMARYKEY\' => 1,\n                                        \'TYPE\' => \'MEDIUMSERIAL\'\n                                      },\n                                      \'creator\',\n                                      {\n                                        \'REFERENCES\' => {\n                                                          \'COLUMN\' => \'userid\',\n                                                          \'DELETE\' => \'CASCADE\',\n                                                          \'TABLE\' => \'profiles\'\n                                                        },\n                                        \'TYPE\' => \'INT3\'\n                                      },\n                                      \'category\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'REFERENCES\' => {\n                                                          \'COLUMN\' => \'id\',\n                                                          \'DELETE\' => \'CASCADE\',\n                                                          \'TABLE\' => \'series_categories\'\n                                                        },\n                                        \'TYPE\' => \'INT2\'\n                                      },\n                                      \'subcategory\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'REFERENCES\' => {\n                                                          \'COLUMN\' => \'id\',\n                                                          \'DELETE\' => \'CASCADE\',\n                                                          \'TABLE\' => \'series_categories\'\n                                                        },\n                                        \'TYPE\' => \'INT2\'\n                                      },\n                                      \'name\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'varchar(64)\'\n                                      },\n                                      \'frequency\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'INT2\'\n                                      },\n                                      \'query\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'MEDIUMTEXT\'\n                                      },\n                                      \'is_public\',\n                                      {\n                                        \'DEFAULT\' => \'FALSE\',\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'BOOLEAN\'\n                                      }\n                                    ],\n                        \'INDEXES\' => [\n                                       \'series_creator_idx\',\n                                       {\n                                         \'FIELDS\' => [\n                                                       \'creator\',\n                                                       \'category\',\n                                                       \'subcategory\',\n                                                       \'name\'\n                                                     ],\n                                         \'TYPE\' => \'UNIQUE\'\n                                       }\n                                     ]\n                      },\n          \'series_categories\' => {\n                                   \'FIELDS\' => [\n                                                 \'id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'PRIMARYKEY\' => 1,\n                                                   \'TYPE\' => \'SMALLSERIAL\'\n                                                 },\n                                                 \'name\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'varchar(64)\'\n                                                 }\n                                               ],\n                                   \'INDEXES\' => [\n                                                  \'series_categories_name_idx\',\n                                                  {\n                                                    \'FIELDS\' => [\n                                                                  \'name\'\n                                                                ],\n                                                    \'TYPE\' => \'UNIQUE\'\n                                                  }\n                                                ]\n                                 },\n          \'series_data\' => {\n                             \'FIELDS\' => [\n                                           \'series_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'series_id\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'series\'\n                                                             },\n                                             \'TYPE\' => \'INT3\'\n                                           },\n                                           \'series_date\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'DATETIME\'\n                                           },\n                                           \'series_value\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'INT3\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'series_data_series_id_idx\',\n                                            {\n                                              \'FIELDS\' => [\n                                                            \'series_id\',\n                                                            \'series_date\'\n                                                          ],\n                                              \'TYPE\' => \'UNIQUE\'\n                                            }\n                                          ]\n                           },\n          \'setting\' => {\n                         \'FIELDS\' => [\n                                       \'name\',\n                                       {\n                                         \'NOTNULL\' => 1,\n                                         \'PRIMARYKEY\' => 1,\n                                         \'TYPE\' => \'varchar(32)\'\n                                       },\n                                       \'default_value\',\n                                       {\n                                         \'NOTNULL\' => 1,\n                                         \'TYPE\' => \'varchar(32)\'\n                                       },\n                                       \'is_enabled\',\n                                       {\n                                         \'DEFAULT\' => \'TRUE\',\n                                         \'NOTNULL\' => 1,\n                                         \'TYPE\' => \'BOOLEAN\'\n                                       },\n                                       \'subclass\',\n                                       {\n                                         \'TYPE\' => \'varchar(32)\'\n                                       }\n                                     ]\n                       },\n          \'setting_value\' => {\n                               \'FIELDS\' => [\n                                             \'name\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'name\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'setting\'\n                                                               },\n                                               \'TYPE\' => \'varchar(32)\'\n                                             },\n                                             \'value\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'varchar(32)\'\n                                             },\n                                             \'sortindex\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT2\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'setting_value_nv_unique_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'name\',\n                                                              \'value\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              },\n                                              \'setting_value_ns_unique_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'name\',\n                                                              \'sortindex\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              }\n                                            ]\n                             },\n          \'status_workflow\' => {\n                                 \'FIELDS\' => [\n                                               \'old_status\',\n                                               {\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'bug_status\'\n                                                                 },\n                                                 \'TYPE\' => \'INT2\'\n                                               },\n                                               \'new_status\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'bug_status\'\n                                                                 },\n                                                 \'TYPE\' => \'INT2\'\n                                               },\n                                               \'require_comment\',\n                                               {\n                                                 \'DEFAULT\' => 0,\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT1\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'status_workflow_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'old_status\',\n                                                                \'new_status\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                }\n                                              ]\n                               },\n          \'test_attachment_data\' => {\n                                      \'FIELDS\' => [\n                                                    \'attachment_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'attachment_id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'test_attachments\'\n                                                                      },\n                                                      \'TYPE\' => \'INT4\'\n                                                    },\n                                                    \'contents\',\n                                                    {\n                                                      \'TYPE\' => \'LONGBLOB\'\n                                                    }\n                                                  ],\n                                      \'INDEXES\' => [\n                                                     \'test_attachment_data_primary_idx\',\n                                                     [\n                                                       \'attachment_id\'\n                                                     ]\n                                                   ]\n                                    },\n          \'test_attachments\' => {\n                                  \'FIELDS\' => [\n                                                \'attachment_id\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'PRIMARYKEY\' => 1,\n                                                  \'TYPE\' => \'INTSERIAL\'\n                                                },\n                                                \'submitter_id\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'REFERENCES\' => {\n                                                                    \'COLUMN\' => \'userid\',\n                                                                    \'TABLE\' => \'profiles\'\n                                                                  },\n                                                  \'TYPE\' => \'INT3\'\n                                                },\n                                                \'description\',\n                                                {\n                                                  \'TYPE\' => \'MEDIUMTEXT\'\n                                                },\n                                                \'filename\',\n                                                {\n                                                  \'TYPE\' => \'MEDIUMTEXT\'\n                                                },\n                                                \'creation_ts\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'TYPE\' => \'DATETIME\'\n                                                },\n                                                \'mime_type\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'TYPE\' => \'varchar(100)\'\n                                                }\n                                              ],\n                                  \'INDEXES\' => [\n                                                 \'test_attachments_submitter_idx\',\n                                                 [\n                                                   \'submitter_id\'\n                                                 ]\n                                               ]\n                                },\n          \'test_builds\' => {\n                             \'FIELDS\' => [\n                                           \'build_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'PRIMARYKEY\' => 1,\n                                             \'TYPE\' => \'INTSERIAL\'\n                                           },\n                                           \'product_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'id\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'products\'\n                                                             },\n                                             \'TYPE\' => \'INT2\'\n                                           },\n                                           \'milestone\',\n                                           {\n                                             \'TYPE\' => \'varchar(20)\'\n                                           },\n                                           \'name\',\n                                           {\n                                             \'TYPE\' => \'varchar(255)\'\n                                           },\n                                           \'description\',\n                                           {\n                                             \'TYPE\' => \'TEXT\'\n                                           },\n                                           \'isactive\',\n                                           {\n                                             \'DEFAULT\' => \'1\',\n                                             \'NOTNULL\' => 1,\n                                             \'TYPE\' => \'BOOLEAN\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'build_name_idx\',\n                                            [\n                                              \'name\'\n                                            ],\n                                            \'build_milestone_idx\',\n                                            [\n                                              \'milestone\'\n                                            ],\n                                            \'build_product_id_name_idx\',\n                                            {\n                                              \'FIELDS\' => [\n                                                            \'product_id\',\n                                                            \'name\'\n                                                          ],\n                                              \'TYPE\' => \'UNIQUE\'\n                                            },\n                                            \'build_prod_idx\',\n                                            {\n                                              \'FIELDS\' => [\n                                                            \'build_id\',\n                                                            \'product_id\'\n                                                          ],\n                                              \'TYPE\' => \'UNIQUE\'\n                                            }\n                                          ]\n                           },\n          \'test_case_activity\' => {\n                                    \'FIELDS\' => [\n                                                  \'case_id\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'case_id\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'test_cases\'\n                                                                    },\n                                                    \'TYPE\' => \'INT4\'\n                                                  },\n                                                  \'fieldid\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'fieldid\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'test_fielddefs\'\n                                                                    },\n                                                    \'TYPE\' => \'INT2\',\n                                                    \'UNSIGNED\' => 1\n                                                  },\n                                                  \'who\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'userid\',\n                                                                      \'TABLE\' => \'profiles\'\n                                                                    },\n                                                    \'TYPE\' => \'INT3\'\n                                                  },\n                                                  \'changed\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'DATETIME\'\n                                                  },\n                                                  \'oldvalue\',\n                                                  {\n                                                    \'TYPE\' => \'MEDIUMTEXT\'\n                                                  },\n                                                  \'newvalue\',\n                                                  {\n                                                    \'TYPE\' => \'MEDIUMTEXT\'\n                                                  }\n                                                ],\n                                    \'INDEXES\' => [\n                                                   \'case_activity_case_id_idx\',\n                                                   [\n                                                     \'case_id\'\n                                                   ],\n                                                   \'case_activity_who_idx\',\n                                                   [\n                                                     \'who\'\n                                                   ],\n                                                   \'case_activity_when_idx\',\n                                                   [\n                                                     \'changed\'\n                                                   ],\n                                                   \'case_activity_field_idx\',\n                                                   [\n                                                     \'fieldid\'\n                                                   ]\n                                                 ]\n                                  },\n          \'test_case_attachments\' => {\n                                       \'FIELDS\' => [\n                                                     \'attachment_id\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'attachment_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_attachments\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\'\n                                                     },\n                                                     \'case_id\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'case_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_cases\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\',\n                                                       \'UNSIGNED\' => 1\n                                                     },\n                                                     \'case_run_id\',\n                                                     {\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'case_run_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_case_runs\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\',\n                                                       \'UNSIGNED\' => 1\n                                                     }\n                                                   ],\n                                       \'INDEXES\' => [\n                                                      \'test_case_attachments_primary_idx\',\n                                                      [\n                                                        \'attachment_id\'\n                                                      ],\n                                                      \'attachment_case_id_idx\',\n                                                      [\n                                                        \'case_id\'\n                                                      ],\n                                                      \'attachment_caserun_id_idx\',\n                                                      [\n                                                        \'case_run_id\'\n                                                      ]\n                                                    ]\n                                     },\n          \'test_case_bugs\' => {\n                                \'FIELDS\' => [\n                                              \'bug_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'bug_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'bugs\'\n                                                                },\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'case_run_id\',\n                                              {\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'case_run_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_case_runs\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'case_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'case_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_cases\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'case_bugs_bug_id_idx\',\n                                               [\n                                                 \'bug_id\'\n                                               ],\n                                               \'case_bugs_case_id_idx\',\n                                               [\n                                                 \'case_id\'\n                                               ],\n                                               \'case_bugs_case_run_id_idx\',\n                                               [\n                                                 \'case_run_id\'\n                                               ]\n                                             ]\n                              },\n          \'test_case_categories\' => {\n                                      \'FIELDS\' => [\n                                                    \'category_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'PRIMARYKEY\' => 1,\n                                                      \'TYPE\' => \'SMALLSERIAL\'\n                                                    },\n                                                    \'product_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'products\'\n                                                                      },\n                                                      \'TYPE\' => \'INT2\'\n                                                    },\n                                                    \'name\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'TYPE\' => \'varchar(240)\'\n                                                    },\n                                                    \'description\',\n                                                    {\n                                                      \'TYPE\' => \'MEDIUMTEXT\'\n                                                    }\n                                                  ],\n                                      \'INDEXES\' => [\n                                                     \'category_product_id_name_idx\',\n                                                     {\n                                                       \'FIELDS\' => [\n                                                                     \'product_id\',\n                                                                     \'name\'\n                                                                   ],\n                                                       \'TYPE\' => \'UNIQUE\'\n                                                     },\n                                                     \'category_product_idx\',\n                                                     {\n                                                       \'FIELDS\' => [\n                                                                     \'category_id\',\n                                                                     \'product_id\'\n                                                                   ],\n                                                       \'TYPE\' => \'UNIQUE\'\n                                                     },\n                                                     \'category_name_idx_v2\',\n                                                     [\n                                                       \'name\'\n                                                     ]\n                                                   ]\n                                    },\n          \'test_case_components\' => {\n                                      \'FIELDS\' => [\n                                                    \'case_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'case_id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'test_cases\'\n                                                                      },\n                                                      \'TYPE\' => \'INT4\'\n                                                    },\n                                                    \'component_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'components\'\n                                                                      },\n                                                      \'TYPE\' => \'INT2\'\n                                                    }\n                                                  ],\n                                      \'INDEXES\' => [\n                                                     \'components_case_id_idx\',\n                                                     {\n                                                       \'FIELDS\' => [\n                                                                     \'case_id\',\n                                                                     \'component_id\'\n                                                                   ],\n                                                       \'TYPE\' => \'UNIQUE\'\n                                                     },\n                                                     \'components_component_id_idx\',\n                                                     [\n                                                       \'component_id\'\n                                                     ]\n                                                   ]\n                                    },\n          \'test_case_dependencies\' => {\n                                        \'FIELDS\' => [\n                                                      \'dependson\',\n                                                      {\n                                                        \'NOTNULL\' => 1,\n                                                        \'TYPE\' => \'INT4\'\n                                                      },\n                                                      \'blocked\',\n                                                      {\n                                                        \'NOTNULL\' => 1,\n                                                        \'TYPE\' => \'INT4\'\n                                                      }\n                                                    ],\n                                        \'INDEXES\' => [\n                                                       \'case_dependencies_primary_idx\',\n                                                       {\n                                                         \'FIELDS\' => [\n                                                                       \'dependson\',\n                                                                       \'blocked\'\n                                                                     ],\n                                                         \'TYPE\' => \'UNIQUE\'\n                                                       },\n                                                       \'case_dependencies_blocked_idx\',\n                                                       [\n                                                         \'blocked\'\n                                                       ]\n                                                     ]\n                                      },\n          \'test_case_plans\' => {\n                                 \'FIELDS\' => [\n                                               \'plan_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'plan_id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'test_plans\'\n                                                                 },\n                                                 \'TYPE\' => \'INT4\'\n                                               },\n                                               \'case_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'case_id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'test_cases\'\n                                                                 },\n                                                 \'TYPE\' => \'INT4\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'test_case_plans_primary_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'plan_id\',\n                                                                \'case_id\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                },\n                                                \'test_case_plans_case_idx\',\n                                                [\n                                                  \'case_id\'\n                                                ]\n                                              ]\n                               },\n          \'test_case_run_status\' => {\n                                      \'FIELDS\' => [\n                                                    \'case_run_status_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'PRIMARYKEY\' => 1,\n                                                      \'TYPE\' => \'SMALLSERIAL\'\n                                                    },\n                                                    \'name\',\n                                                    {\n                                                      \'TYPE\' => \'varchar(20)\'\n                                                    },\n                                                    \'sortkey\',\n                                                    {\n                                                      \'TYPE\' => \'INT4\'\n                                                    },\n                                                    \'description\',\n                                                    {\n                                                      \'TYPE\' => \'TEXT\'\n                                                    }\n                                                  ]\n                                    },\n          \'test_case_runs\' => {\n                                \'FIELDS\' => [\n                                              \'case_run_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'PRIMARYKEY\' => 1,\n                                                \'TYPE\' => \'INTSERIAL\'\n                                              },\n                                              \'run_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'run_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_runs\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'case_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'case_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_cases\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'assignee\',\n                                              {\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'testedby\',\n                                              {\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'case_run_status_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'case_run_status_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_case_run_status\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              },\n                                              \'case_text_version\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'build_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'build_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_builds\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'running_date\',\n                                              {\n                                                \'TYPE\' => \'DATETIME\'\n                                              },\n                                              \'close_date\',\n                                              {\n                                                \'TYPE\' => \'DATETIME\'\n                                              },\n                                              \'notes\',\n                                              {\n                                                \'TYPE\' => \'TEXT\'\n                                              },\n                                              \'iscurrent\',\n                                              {\n                                                \'DEFAULT\' => \'0\',\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'BOOLEAN\'\n                                              },\n                                              \'sortkey\',\n                                              {\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'environment_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'environment_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_environments\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'priority_id\',\n                                              {\n                                                \'DEFAULT\' => 0,\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'RESTRICT\',\n                                                                  \'TABLE\' => \'priority\'\n                                                                },\n                                                \'TYPE\' => \'INT2\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'case_run_case_id_idx\',\n                                               [\n                                                 \'case_id\'\n                                               ],\n                                               \'case_run_assignee_idx\',\n                                               [\n                                                 \'assignee\'\n                                               ],\n                                               \'case_run_testedby_idx\',\n                                               [\n                                                 \'testedby\'\n                                               ],\n                                               \'case_run_close_date_idx\',\n                                               [\n                                                 \'close_date\'\n                                               ],\n                                               \'case_run_build_env_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'run_id\',\n                                                               \'case_id\',\n                                                               \'build_id\',\n                                                               \'environment_id\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               },\n                                               \'case_run_status_idx\',\n                                               [\n                                                 \'case_run_status_id\'\n                                               ],\n                                               \'case_run_text_ver_idx\',\n                                               [\n                                                 \'case_text_version\'\n                                               ],\n                                               \'case_run_build_idx_v2\',\n                                               [\n                                                 \'build_id\'\n                                               ],\n                                               \'case_run_env_idx_v2\',\n                                               [\n                                                 \'environment_id\'\n                                               ],\n                                               \'case_run_priority_idx\',\n                                               [\n                                                 \'priority_id\'\n                                               ]\n                                             ]\n                              },\n          \'test_case_status\' => {\n                                  \'FIELDS\' => [\n                                                \'case_status_id\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'PRIMARYKEY\' => 1,\n                                                  \'TYPE\' => \'SMALLSERIAL\'\n                                                },\n                                                \'name\',\n                                                {\n                                                  \'NOTNULL\' => 1,\n                                                  \'TYPE\' => \'varchar(255)\'\n                                                },\n                                                \'description\',\n                                                {\n                                                  \'TYPE\' => \'TEXT\'\n                                                }\n                                              ]\n                                },\n          \'test_case_tags\' => {\n                                \'FIELDS\' => [\n                                              \'tag_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'tag_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_tags\'\n                                                                },\n                                                \'TYPE\' => \'INT4\',\n                                                \'UNSIGNED\' => 1\n                                              },\n                                              \'case_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'case_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_cases\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'userid\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'INT3\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'case_tags_primary_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'tag_id\',\n                                                               \'case_id\',\n                                                               \'userid\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               },\n                                               \'case_tags_secondary_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'tag_id\',\n                                                               \'case_id\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               },\n                                               \'case_tags_case_id_idx_v3\',\n                                               [\n                                                 \'case_id\'\n                                               ],\n                                               \'case_tags_userid_idx\',\n                                               [\n                                                 \'userid\'\n                                               ]\n                                             ]\n                              },\n          \'test_case_texts\' => {\n                                 \'FIELDS\' => [\n                                               \'case_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'case_id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'test_cases\'\n                                                                 },\n                                                 \'TYPE\' => \'INT4\'\n                                               },\n                                               \'case_text_version\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'who\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'creation_ts\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'DATETIME\'\n                                               },\n                                               \'action\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               },\n                                               \'effect\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               },\n                                               \'setup\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               },\n                                               \'breakdown\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'case_versions_idx\',\n                                                {\n                                                  \'FIELDS\' => [\n                                                                \'case_id\',\n                                                                \'case_text_version\'\n                                                              ],\n                                                  \'TYPE\' => \'UNIQUE\'\n                                                },\n                                                \'case_versions_who_idx\',\n                                                [\n                                                  \'who\'\n                                                ],\n                                                \'case_versions_creation_ts_idx\',\n                                                [\n                                                  \'creation_ts\'\n                                                ]\n                                              ]\n                               },\n          \'test_cases\' => {\n                            \'FIELDS\' => [\n                                          \'case_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'INTSERIAL\'\n                                          },\n                                          \'case_status_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'case_status_id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'test_case_status\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'category_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'category_id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'test_case_categories\'\n                                                            },\n                                            \'TYPE\' => \'INT2\',\n                                            \'UNSIGNED\' => 1\n                                          },\n                                          \'priority_id\',\n                                          {\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'id\',\n                                                              \'DELETE\' => \'RESTRICT\',\n                                                              \'TABLE\' => \'priority\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'author_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'userid\',\n                                                              \'TABLE\' => \'profiles\'\n                                                            },\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'default_tester_id\',\n                                          {\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'creation_date\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'DATETIME\'\n                                          },\n                                          \'estimated_time\',\n                                          {\n                                            \'TYPE\' => \'TIME\'\n                                          },\n                                          \'isautomated\',\n                                          {\n                                            \'DEFAULT\' => \'0\',\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'BOOLEAN\'\n                                          },\n                                          \'sortkey\',\n                                          {\n                                            \'TYPE\' => \'INT4\'\n                                          },\n                                          \'script\',\n                                          {\n                                            \'TYPE\' => \'MEDIUMTEXT\'\n                                          },\n                                          \'arguments\',\n                                          {\n                                            \'TYPE\' => \'MEDIUMTEXT\'\n                                          },\n                                          \'summary\',\n                                          {\n                                            \'TYPE\' => \'varchar(255)\'\n                                          },\n                                          \'requirement\',\n                                          {\n                                            \'TYPE\' => \'varchar(255)\'\n                                          },\n                                          \'alias\',\n                                          {\n                                            \'TYPE\' => \'varchar(255)\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'test_case_category_idx\',\n                                           [\n                                             \'category_id\'\n                                           ],\n                                           \'test_case_author_idx\',\n                                           [\n                                             \'author_id\'\n                                           ],\n                                           \'test_case_creation_date_idx\',\n                                           [\n                                             \'creation_date\'\n                                           ],\n                                           \'test_case_sortkey_idx\',\n                                           [\n                                             \'sortkey\'\n                                           ],\n                                           \'test_case_shortname_idx\',\n                                           [\n                                             \'alias\'\n                                           ],\n                                           \'test_case_requirement_idx\',\n                                           [\n                                             \'requirement\'\n                                           ],\n                                           \'test_case_status_idx\',\n                                           [\n                                             \'case_status_id\'\n                                           ],\n                                           \'test_case_tester_idx\',\n                                           [\n                                             \'default_tester_id\'\n                                           ]\n                                         ]\n                          },\n          \'test_email_settings\' => {\n                                     \'FIELDS\' => [\n                                                   \'userid\',\n                                                   {\n                                                     \'NOTNULL\' => 1,\n                                                     \'REFERENCES\' => {\n                                                                       \'COLUMN\' => \'userid\',\n                                                                       \'TABLE\' => \'profiles\'\n                                                                     },\n                                                     \'TYPE\' => \'INT3\'\n                                                   },\n                                                   \'eventid\',\n                                                   {\n                                                     \'NOTNULL\' => 1,\n                                                     \'REFERENCES\' => {\n                                                                       \'COLUMN\' => \'eventid\',\n                                                                       \'DELETE\' => \'CASCADE\',\n                                                                       \'TABLE\' => \'test_events\'\n                                                                     },\n                                                     \'TYPE\' => \'INT1\',\n                                                     \'UNSIGNED\' => 1\n                                                   },\n                                                   \'relationship_id\',\n                                                   {\n                                                     \'NOTNULL\' => 1,\n                                                     \'REFERENCES\' => {\n                                                                       \'COLUMN\' => \'relationship_id\',\n                                                                       \'DELETE\' => \'CASCADE\',\n                                                                       \'TABLE\' => \'test_relationships\'\n                                                                     },\n                                                     \'TYPE\' => \'INT1\',\n                                                     \'UNSIGNED\' => 1\n                                                   }\n                                                 ],\n                                     \'INDEXES\' => [\n                                                    \'test_email_setting_user_id_idx\',\n                                                    {\n                                                      \'FIELDS\' => [\n                                                                    \'userid\',\n                                                                    \'relationship_id\',\n                                                                    \'eventid\'\n                                                                  ],\n                                                      \'TYPE\' => \'UNIQUE\'\n                                                    }\n                                                  ]\n                                   },\n          \'test_environment_category\' => {\n                                           \'FIELDS\' => [\n                                                         \'env_category_id\',\n                                                         {\n                                                           \'NOTNULL\' => 1,\n                                                           \'PRIMARYKEY\' => 1,\n                                                           \'TYPE\' => \'INTSERIAL\'\n                                                         },\n                                                         \'product_id\',\n                                                         {\n                                                           \'NOTNULL\' => 1,\n                                                           \'TYPE\' => \'INT2\'\n                                                         },\n                                                         \'name\',\n                                                         {\n                                                           \'TYPE\' => \'varchar(255)\'\n                                                         }\n                                                       ],\n                                           \'INDEXES\' => [\n                                                          \'test_environment_category_key1\',\n                                                          {\n                                                            \'FIELDS\' => [\n                                                                          \'env_category_id\',\n                                                                          \'product_id\'\n                                                                        ],\n                                                            \'TYPE\' => \'UNIQUE\'\n                                                          },\n                                                          \'test_environment_category_key2\',\n                                                          {\n                                                            \'FIELDS\' => [\n                                                                          \'product_id\',\n                                                                          \'name\'\n                                                                        ],\n                                                            \'TYPE\' => \'UNIQUE\'\n                                                          }\n                                                        ]\n                                         },\n          \'test_environment_element\' => {\n                                          \'FIELDS\' => [\n                                                        \'element_id\',\n                                                        {\n                                                          \'NOTNULL\' => 1,\n                                                          \'PRIMARYKEY\' => 1,\n                                                          \'TYPE\' => \'INTSERIAL\'\n                                                        },\n                                                        \'env_category_id\',\n                                                        {\n                                                          \'NOTNULL\' => 1,\n                                                          \'REFERENCES\' => {\n                                                                            \'COLUMN\' => \'env_category_id\',\n                                                                            \'DELETE\' => \'CASCADE\',\n                                                                            \'TABLE\' => \'test_environment_category\'\n                                                                          },\n                                                          \'TYPE\' => \'INT4\',\n                                                          \'UNSIGNED\' => 1\n                                                        },\n                                                        \'name\',\n                                                        {\n                                                          \'TYPE\' => \'varchar(255)\'\n                                                        },\n                                                        \'parent_id\',\n                                                        {\n                                                          \'TYPE\' => \'INT4\',\n                                                          \'UNSIGNED\' => 1\n                                                        },\n                                                        \'isprivate\',\n                                                        {\n                                                          \'DEFAULT\' => 0,\n                                                          \'NOTNULL\' => 1,\n                                                          \'TYPE\' => \'BOOLEAN\'\n                                                        }\n                                                      ],\n                                          \'INDEXES\' => [\n                                                         \'test_environment_element_key1\',\n                                                         {\n                                                           \'FIELDS\' => [\n                                                                         \'element_id\',\n                                                                         \'env_category_id\'\n                                                                       ],\n                                                           \'TYPE\' => \'UNIQUE\'\n                                                         },\n                                                         \'test_environment_element_key2\',\n                                                         {\n                                                           \'FIELDS\' => [\n                                                                         \'env_category_id\',\n                                                                         \'name\'\n                                                                       ],\n                                                           \'TYPE\' => \'UNIQUE\'\n                                                         }\n                                                       ]\n                                        },\n          \'test_environment_map\' => {\n                                      \'FIELDS\' => [\n                                                    \'environment_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'case_id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'test_cases\'\n                                                                      },\n                                                      \'TYPE\' => \'INT4\'\n                                                    },\n                                                    \'property_id\',\n                                                    {\n                                                      \'TYPE\' => \'INT4\'\n                                                    },\n                                                    \'element_id\',\n                                                    {\n                                                      \'NOTNULL\' => 1,\n                                                      \'REFERENCES\' => {\n                                                                        \'COLUMN\' => \'element_id\',\n                                                                        \'DELETE\' => \'CASCADE\',\n                                                                        \'TABLE\' => \'test_environment_element\'\n                                                                      },\n                                                      \'TYPE\' => \'INT4\',\n                                                      \'UNSIGNED\' => 1\n                                                    },\n                                                    \'value_selected\',\n                                                    {\n                                                      \'TYPE\' => \'TINYTEXT\'\n                                                    }\n                                                  ],\n                                      \'INDEXES\' => [\n                                                     \'env_map_env_element_idx\',\n                                                     [\n                                                       \'environment_id\',\n                                                       \'element_id\'\n                                                     ],\n                                                     \'env_map_property_idx\',\n                                                     [\n                                                       \'environment_id\',\n                                                       \'property_id\'\n                                                     ],\n                                                     \'test_environment_map_key3\',\n                                                     {\n                                                       \'FIELDS\' => [\n                                                                     \'environment_id\',\n                                                                     \'element_id\',\n                                                                     \'property_id\'\n                                                                   ],\n                                                       \'TYPE\' => \'UNIQUE\'\n                                                     }\n                                                   ]\n                                    },\n          \'test_environment_property\' => {\n                                           \'FIELDS\' => [\n                                                         \'property_id\',\n                                                         {\n                                                           \'NOTNULL\' => 1,\n                                                           \'PRIMARYKEY\' => 1,\n                                                           \'TYPE\' => \'INTSERIAL\'\n                                                         },\n                                                         \'element_id\',\n                                                         {\n                                                           \'NOTNULL\' => 1,\n                                                           \'REFERENCES\' => {\n                                                                             \'COLUMN\' => \'element_id\',\n                                                                             \'DELETE\' => \'CASCADE\',\n                                                                             \'TABLE\' => \'test_environment_element\'\n                                                                           },\n                                                           \'TYPE\' => \'INT4\',\n                                                           \'UNSIGNED\' => 1\n                                                         },\n                                                         \'name\',\n                                                         {\n                                                           \'TYPE\' => \'varchar(255)\'\n                                                         },\n                                                         \'validexp\',\n                                                         {\n                                                           \'TYPE\' => \'TEXT\'\n                                                         }\n                                                       ],\n                                           \'INDEXES\' => [\n                                                          \'test_environment_property_key1\',\n                                                          {\n                                                            \'FIELDS\' => [\n                                                                          \'property_id\',\n                                                                          \'element_id\'\n                                                                        ],\n                                                            \'TYPE\' => \'UNIQUE\'\n                                                          },\n                                                          \'test_environment_property_key2\',\n                                                          {\n                                                            \'FIELDS\' => [\n                                                                          \'element_id\',\n                                                                          \'name\'\n                                                                        ],\n                                                            \'TYPE\' => \'UNIQUE\'\n                                                          }\n                                                        ]\n                                         },\n          \'test_environments\' => {\n                                   \'FIELDS\' => [\n                                                 \'environment_id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'PRIMARYKEY\' => 1,\n                                                   \'TYPE\' => \'INTSERIAL\'\n                                                 },\n                                                 \'product_id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'id\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'products\'\n                                                                   },\n                                                   \'TYPE\' => \'INT2\'\n                                                 },\n                                                 \'name\',\n                                                 {\n                                                   \'TYPE\' => \'varchar(255)\'\n                                                 },\n                                                 \'isactive\',\n                                                 {\n                                                   \'DEFAULT\' => \'1\',\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'BOOLEAN\'\n                                                 }\n                                               ],\n                                   \'INDEXES\' => [\n                                                  \'test_environments_key1\',\n                                                  {\n                                                    \'FIELDS\' => [\n                                                                  \'environment_id\',\n                                                                  \'product_id\'\n                                                                ],\n                                                    \'TYPE\' => \'UNIQUE\'\n                                                  },\n                                                  \'test_environments_key2\',\n                                                  {\n                                                    \'FIELDS\' => [\n                                                                  \'product_id\',\n                                                                  \'name\'\n                                                                ],\n                                                    \'TYPE\' => \'UNIQUE\'\n                                                  },\n                                                  \'environment_name_idx_v2\',\n                                                  [\n                                                    \'name\'\n                                                  ]\n                                                ]\n                                 },\n          \'test_events\' => {\n                             \'FIELDS\' => [\n                                           \'eventid\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'PRIMARYKEY\' => 1,\n                                             \'TYPE\' => \'INT1\',\n                                             \'UNSIGNED\' => 1\n                                           },\n                                           \'name\',\n                                           {\n                                             \'TYPE\' => \'varchar(50)\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'test_event_name_idx\',\n                                            [\n                                              \'name\'\n                                            ]\n                                          ]\n                           },\n          \'test_fielddefs\' => {\n                                \'FIELDS\' => [\n                                              \'fieldid\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'PRIMARYKEY\' => 1,\n                                                \'TYPE\' => \'SMALLSERIAL\'\n                                              },\n                                              \'name\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'varchar(100)\'\n                                              },\n                                              \'description\',\n                                              {\n                                                \'TYPE\' => \'MEDIUMTEXT\'\n                                              },\n                                              \'table_name\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'varchar(100)\'\n                                              }\n                                            ]\n                              },\n          \'test_named_queries\' => {\n                                    \'FIELDS\' => [\n                                                  \'userid\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'userid\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'profiles\'\n                                                                    },\n                                                    \'TYPE\' => \'INT3\'\n                                                  },\n                                                  \'name\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'varchar(64)\'\n                                                  },\n                                                  \'isvisible\',\n                                                  {\n                                                    \'DEFAULT\' => 1,\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'BOOLEAN\'\n                                                  },\n                                                  \'query\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'MEDIUMTEXT\'\n                                                  },\n                                                  \'type\',\n                                                  {\n                                                    \'DEFAULT\' => 0,\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'INT3\'\n                                                  }\n                                                ],\n                                    \'INDEXES\' => [\n                                                   \'test_namedquery_primary_idx\',\n                                                   {\n                                                     \'FIELDS\' => [\n                                                                   \'userid\',\n                                                                   \'name\'\n                                                                 ],\n                                                     \'TYPE\' => \'UNIQUE\'\n                                                   },\n                                                   \'test_namedquery_name_idx\',\n                                                   [\n                                                     \'name\'\n                                                   ]\n                                                 ]\n                                  },\n          \'test_plan_activity\' => {\n                                    \'FIELDS\' => [\n                                                  \'plan_id\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'plan_id\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'test_plans\'\n                                                                    },\n                                                    \'TYPE\' => \'INT4\'\n                                                  },\n                                                  \'fieldid\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'fieldid\',\n                                                                      \'DELETE\' => \'CASCADE\',\n                                                                      \'TABLE\' => \'test_fielddefs\'\n                                                                    },\n                                                    \'TYPE\' => \'INT2\',\n                                                    \'UNSIGNED\' => 1\n                                                  },\n                                                  \'who\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'REFERENCES\' => {\n                                                                      \'COLUMN\' => \'userid\',\n                                                                      \'TABLE\' => \'profiles\'\n                                                                    },\n                                                    \'TYPE\' => \'INT3\'\n                                                  },\n                                                  \'changed\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'TYPE\' => \'DATETIME\'\n                                                  },\n                                                  \'oldvalue\',\n                                                  {\n                                                    \'TYPE\' => \'MEDIUMTEXT\'\n                                                  },\n                                                  \'newvalue\',\n                                                  {\n                                                    \'TYPE\' => \'MEDIUMTEXT\'\n                                                  }\n                                                ],\n                                    \'INDEXES\' => [\n                                                   \'plan_activity_primary_idx\',\n                                                   [\n                                                     \'plan_id\'\n                                                   ],\n                                                   \'plan_activity_field_idx\',\n                                                   [\n                                                     \'fieldid\'\n                                                   ],\n                                                   \'plan_activity_who_idx\',\n                                                   [\n                                                     \'who\'\n                                                   ],\n                                                   \'plan_activity_changed_idx\',\n                                                   [\n                                                     \'changed\'\n                                                   ]\n                                                 ]\n                                  },\n          \'test_plan_attachments\' => {\n                                       \'FIELDS\' => [\n                                                     \'attachment_id\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'attachment_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_attachments\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\'\n                                                     },\n                                                     \'plan_id\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'plan_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_plans\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\',\n                                                       \'UNSIGNED\' => 1\n                                                     }\n                                                   ],\n                                       \'INDEXES\' => [\n                                                      \'test_plan_attachments_primary_idx\',\n                                                      [\n                                                        \'attachment_id\'\n                                                      ],\n                                                      \'attachment_plan_id_idx\',\n                                                      [\n                                                        \'plan_id\'\n                                                      ]\n                                                    ]\n                                     },\n          \'test_plan_permissions\' => {\n                                       \'FIELDS\' => [\n                                                     \'userid\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'userid\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'profiles\'\n                                                                       },\n                                                       \'TYPE\' => \'INT3\'\n                                                     },\n                                                     \'plan_id\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'REFERENCES\' => {\n                                                                         \'COLUMN\' => \'plan_id\',\n                                                                         \'DELETE\' => \'CASCADE\',\n                                                                         \'TABLE\' => \'test_plans\'\n                                                                       },\n                                                       \'TYPE\' => \'INT4\',\n                                                       \'UNSIGNED\' => 1\n                                                     },\n                                                     \'permissions\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'TYPE\' => \'INT1\'\n                                                     },\n                                                     \'grant_type\',\n                                                     {\n                                                       \'NOTNULL\' => 1,\n                                                       \'TYPE\' => \'INT1\'\n                                                     }\n                                                   ],\n                                       \'INDEXES\' => [\n                                                      \'testers_plan_user_idx\',\n                                                      {\n                                                        \'FIELDS\' => [\n                                                                      \'userid\',\n                                                                      \'plan_id\',\n                                                                      \'grant_type\'\n                                                                    ],\n                                                        \'TYPE\' => \'UNIQUE\'\n                                                      },\n                                                      \'testers_plan_user_plan_idx\',\n                                                      [\n                                                        \'plan_id\'\n                                                      ],\n                                                      \'testers_plan_grant_idx\',\n                                                      [\n                                                        \'grant_type\'\n                                                      ]\n                                                    ]\n                                     },\n          \'test_plan_permissions_regexp\' => {\n                                              \'FIELDS\' => [\n                                                            \'plan_id\',\n                                                            {\n                                                              \'NOTNULL\' => 1,\n                                                              \'REFERENCES\' => {\n                                                                                \'COLUMN\' => \'plan_id\',\n                                                                                \'DELETE\' => \'CASCADE\',\n                                                                                \'TABLE\' => \'test_plans\'\n                                                                              },\n                                                              \'TYPE\' => \'INT4\',\n                                                              \'UNSIGNED\' => 1\n                                                            },\n                                                            \'user_regexp\',\n                                                            {\n                                                              \'NOTNULL\' => 1,\n                                                              \'TYPE\' => \'TEXT\'\n                                                            },\n                                                            \'permissions\',\n                                                            {\n                                                              \'NOTNULL\' => 1,\n                                                              \'TYPE\' => \'INT1\'\n                                                            }\n                                                          ],\n                                              \'INDEXES\' => [\n                                                             \'testers_plan_regexp_idx\',\n                                                             {\n                                                               \'FIELDS\' => [\n                                                                             \'plan_id\'\n                                                                           ],\n                                                               \'TYPE\' => \'UNIQUE\'\n                                                             }\n                                                           ]\n                                            },\n          \'test_plan_tags\' => {\n                                \'FIELDS\' => [\n                                              \'tag_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'tag_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_tags\'\n                                                                },\n                                                \'TYPE\' => \'INT4\',\n                                                \'UNSIGNED\' => 1\n                                              },\n                                              \'plan_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'tag_id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'test_tags\'\n                                                                },\n                                                \'TYPE\' => \'INT4\'\n                                              },\n                                              \'userid\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'INT3\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'plan_tags_primary_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'tag_id\',\n                                                               \'plan_id\',\n                                                               \'userid\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               },\n                                               \'plan_tags_secondary_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'tag_id\',\n                                                               \'plan_id\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               },\n                                               \'plan_tags_plan_id_idx\',\n                                               [\n                                                 \'plan_id\'\n                                               ],\n                                               \'plan_tags_userid_idx\',\n                                               [\n                                                 \'userid\'\n                                               ]\n                                             ]\n                              },\n          \'test_plan_texts\' => {\n                                 \'FIELDS\' => [\n                                               \'plan_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'plan_id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'test_plans\'\n                                                                 },\n                                                 \'TYPE\' => \'INT4\'\n                                               },\n                                               \'plan_text_version\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT4\'\n                                               },\n                                               \'who\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'creation_ts\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'DATETIME\'\n                                               },\n                                               \'plan_text\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'test_plan_text_version_idx\',\n                                                [\n                                                  \'plan_id\',\n                                                  \'plan_text_version\'\n                                                ],\n                                                \'test_plan_text_who_idx\',\n                                                [\n                                                  \'who\'\n                                                ]\n                                              ]\n                               },\n          \'test_plan_types\' => {\n                                 \'FIELDS\' => [\n                                               \'type_id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'PRIMARYKEY\' => 1,\n                                                 \'TYPE\' => \'SMALLSERIAL\'\n                                               },\n                                               \'name\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'varchar(64)\'\n                                               },\n                                               \'description\',\n                                               {\n                                                 \'TYPE\' => \'MEDIUMTEXT\'\n                                               }\n                                             ]\n                               },\n          \'test_plans\' => {\n                            \'FIELDS\' => [\n                                          \'plan_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'INTSERIAL\'\n                                          },\n                                          \'product_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'products\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'author_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'INT3\'\n                                          },\n                                          \'type_id\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'REFERENCES\' => {\n                                                              \'COLUMN\' => \'type_id\',\n                                                              \'DELETE\' => \'CASCADE\',\n                                                              \'TABLE\' => \'test_plan_types\'\n                                                            },\n                                            \'TYPE\' => \'INT2\'\n                                          },\n                                          \'default_product_version\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'MEDIUMTEXT\'\n                                          },\n                                          \'name\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(255)\'\n                                          },\n                                          \'creation_date\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'DATETIME\'\n                                          },\n                                          \'isactive\',\n                                          {\n                                            \'DEFAULT\' => \'1\',\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'BOOLEAN\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'plan_product_plan_id_idx\',\n                                           [\n                                             \'product_id\',\n                                             \'plan_id\'\n                                           ],\n                                           \'plan_author_idx\',\n                                           [\n                                             \'author_id\'\n                                           ],\n                                           \'plan_type_idx\',\n                                           [\n                                             \'type_id\'\n                                           ],\n                                           \'plan_isactive_idx\',\n                                           [\n                                             \'isactive\'\n                                           ],\n                                           \'plan_name_idx\',\n                                           [\n                                             \'name\'\n                                           ]\n                                         ]\n                          },\n          \'test_relationships\' => {\n                                    \'FIELDS\' => [\n                                                  \'relationship_id\',\n                                                  {\n                                                    \'NOTNULL\' => 1,\n                                                    \'PRIMARYKEY\' => 1,\n                                                    \'TYPE\' => \'INT1\',\n                                                    \'UNSIGNED\' => 1\n                                                  },\n                                                  \'name\',\n                                                  {\n                                                    \'TYPE\' => \'varchar(50)\'\n                                                  }\n                                                ]\n                                  },\n          \'test_run_activity\' => {\n                                   \'FIELDS\' => [\n                                                 \'run_id\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'run_id\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'test_runs\'\n                                                                   },\n                                                   \'TYPE\' => \'INT4\'\n                                                 },\n                                                 \'fieldid\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'fieldid\',\n                                                                     \'DELETE\' => \'CASCADE\',\n                                                                     \'TABLE\' => \'test_fielddefs\'\n                                                                   },\n                                                   \'TYPE\' => \'INT2\',\n                                                   \'UNSIGNED\' => 1\n                                                 },\n                                                 \'who\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'REFERENCES\' => {\n                                                                     \'COLUMN\' => \'userid\',\n                                                                     \'TABLE\' => \'profiles\'\n                                                                   },\n                                                   \'TYPE\' => \'INT3\'\n                                                 },\n                                                 \'changed\',\n                                                 {\n                                                   \'NOTNULL\' => 1,\n                                                   \'TYPE\' => \'DATETIME\'\n                                                 },\n                                                 \'oldvalue\',\n                                                 {\n                                                   \'TYPE\' => \'MEDIUMTEXT\'\n                                                 },\n                                                 \'newvalue\',\n                                                 {\n                                                   \'TYPE\' => \'MEDIUMTEXT\'\n                                                 }\n                                               ],\n                                   \'INDEXES\' => [\n                                                  \'run_activity_run_id_idx\',\n                                                  [\n                                                    \'run_id\'\n                                                  ],\n                                                  \'run_activity_field_idx\',\n                                                  [\n                                                    \'fieldid\'\n                                                  ],\n                                                  \'run_activity_who_idx\',\n                                                  [\n                                                    \'who\'\n                                                  ],\n                                                  \'run_activity_when_idx\',\n                                                  [\n                                                    \'changed\'\n                                                  ]\n                                                ]\n                                 },\n          \'test_run_cc\' => {\n                             \'FIELDS\' => [\n                                           \'run_id\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'run_id\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'test_runs\'\n                                                             },\n                                             \'TYPE\' => \'INT4\'\n                                           },\n                                           \'who\',\n                                           {\n                                             \'NOTNULL\' => 1,\n                                             \'REFERENCES\' => {\n                                                               \'COLUMN\' => \'userid\',\n                                                               \'DELETE\' => \'CASCADE\',\n                                                               \'TABLE\' => \'profiles\'\n                                                             },\n                                             \'TYPE\' => \'INT3\'\n                                           }\n                                         ],\n                             \'INDEXES\' => [\n                                            \'test_run_cc_primary_idx\',\n                                            {\n                                              \'FIELDS\' => [\n                                                            \'run_id\',\n                                                            \'who\'\n                                                          ],\n                                              \'TYPE\' => \'UNIQUE\'\n                                            },\n                                            \'test_run_cc_who_idx\',\n                                            [\n                                              \'who\'\n                                            ]\n                                          ]\n                           },\n          \'test_run_tags\' => {\n                               \'FIELDS\' => [\n                                             \'tag_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'tag_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'test_tags\'\n                                                               },\n                                               \'TYPE\' => \'INT4\',\n                                               \'UNSIGNED\' => 1\n                                             },\n                                             \'run_id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'run_id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'test_runs\'\n                                                               },\n                                               \'TYPE\' => \'INT4\'\n                                             },\n                                             \'userid\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT3\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'run_tags_primary_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'tag_id\',\n                                                              \'run_id\',\n                                                              \'userid\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              },\n                                              \'run_tags_secondary_idx\',\n                                              {\n                                                \'FIELDS\' => [\n                                                              \'tag_id\',\n                                                              \'run_id\'\n                                                            ],\n                                                \'TYPE\' => \'UNIQUE\'\n                                              },\n                                              \'run_tags_run_id_idx\',\n                                              [\n                                                \'run_id\'\n                                              ],\n                                              \'run_tags_userid_idx\',\n                                              [\n                                                \'userid\'\n                                              ]\n                                            ]\n                             },\n          \'test_runs\' => {\n                           \'FIELDS\' => [\n                                         \'run_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'PRIMARYKEY\' => 1,\n                                           \'TYPE\' => \'INTSERIAL\'\n                                         },\n                                         \'plan_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'plan_id\',\n                                                             \'DELETE\' => \'CASCADE\',\n                                                             \'TABLE\' => \'test_plans\'\n                                                           },\n                                           \'TYPE\' => \'INT4\'\n                                         },\n                                         \'environment_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'environment_id\',\n                                                             \'DELETE\' => \'CASCADE\',\n                                                             \'TABLE\' => \'test_environments\'\n                                                           },\n                                           \'TYPE\' => \'INT4\'\n                                         },\n                                         \'product_version\',\n                                         {\n                                           \'TYPE\' => \'MEDIUMTEXT\'\n                                         },\n                                         \'build_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'build_id\',\n                                                             \'DELETE\' => \'CASCADE\',\n                                                             \'TABLE\' => \'test_builds\'\n                                                           },\n                                           \'TYPE\' => \'INT4\'\n                                         },\n                                         \'plan_text_version\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'INT4\'\n                                         },\n                                         \'manager_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'REFERENCES\' => {\n                                                             \'COLUMN\' => \'userid\',\n                                                             \'DELETE\' => \'CASCADE\',\n                                                             \'TABLE\' => \'profiles\'\n                                                           },\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'default_tester_id\',\n                                         {\n                                           \'TYPE\' => \'INT3\'\n                                         },\n                                         \'start_date\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'DATETIME\'\n                                         },\n                                         \'stop_date\',\n                                         {\n                                           \'TYPE\' => \'DATETIME\'\n                                         },\n                                         \'summary\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'TINYTEXT\'\n                                         },\n                                         \'notes\',\n                                         {\n                                           \'TYPE\' => \'MEDIUMTEXT\'\n                                         },\n                                         \'target_pass\',\n                                         {\n                                           \'TYPE\' => \'INT1\'\n                                         },\n                                         \'target_completion\',\n                                         {\n                                           \'TYPE\' => \'INT1\'\n                                         }\n                                       ],\n                           \'INDEXES\' => [\n                                          \'test_run_plan_id_run_id_idx\',\n                                          [\n                                            \'plan_id\',\n                                            \'run_id\'\n                                          ],\n                                          \'test_run_manager_idx\',\n                                          [\n                                            \'manager_id\'\n                                          ],\n                                          \'test_run_start_date_idx\',\n                                          [\n                                            \'start_date\'\n                                          ],\n                                          \'test_run_stop_date_idx\',\n                                          [\n                                            \'stop_date\'\n                                          ],\n                                          \'test_run_env_idx\',\n                                          [\n                                            \'environment_id\'\n                                          ],\n                                          \'test_run_build_idx\',\n                                          [\n                                            \'build_id\'\n                                          ],\n                                          \'test_run_plan_ver_idx\',\n                                          [\n                                            \'plan_text_version\'\n                                          ],\n                                          \'test_run_tester_idx\',\n                                          [\n                                            \'default_tester_id\'\n                                          ]\n                                        ]\n                         },\n          \'test_tags\' => {\n                           \'FIELDS\' => [\n                                         \'tag_id\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'PRIMARYKEY\' => 1,\n                                           \'TYPE\' => \'INTSERIAL\'\n                                         },\n                                         \'tag_name\',\n                                         {\n                                           \'NOTNULL\' => 1,\n                                           \'TYPE\' => \'varchar(255)\'\n                                         }\n                                       ],\n                           \'INDEXES\' => [\n                                          \'test_tag_name_idx_v2\',\n                                          [\n                                            \'tag_name\'\n                                          ]\n                                        ]\n                         },\n          \'tokens\' => {\n                        \'FIELDS\' => [\n                                      \'userid\',\n                                      {\n                                        \'REFERENCES\' => {\n                                                          \'COLUMN\' => \'userid\',\n                                                          \'DELETE\' => \'CASCADE\',\n                                                          \'TABLE\' => \'profiles\'\n                                                        },\n                                        \'TYPE\' => \'INT3\'\n                                      },\n                                      \'issuedate\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'DATETIME\'\n                                      },\n                                      \'token\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'PRIMARYKEY\' => 1,\n                                        \'TYPE\' => \'varchar(16)\'\n                                      },\n                                      \'tokentype\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'varchar(8)\'\n                                      },\n                                      \'eventdata\',\n                                      {\n                                        \'TYPE\' => \'TINYTEXT\'\n                                      }\n                                    ],\n                        \'INDEXES\' => [\n                                       \'tokens_userid_idx\',\n                                       [\n                                         \'userid\'\n                                       ]\n                                     ]\n                      },\n          \'ts_error\' => {\n                          \'FIELDS\' => [\n                                        \'error_time\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT4\'\n                                        },\n                                        \'jobid\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT4\'\n                                        },\n                                        \'message\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(255)\'\n                                        },\n                                        \'funcid\',\n                                        {\n                                          \'DEFAULT\' => 0,\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'INT4\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'ts_error_funcid_idx\',\n                                         [\n                                           \'funcid\',\n                                           \'error_time\'\n                                         ],\n                                         \'ts_error_error_time_idx\',\n                                         [\n                                           \'error_time\'\n                                         ],\n                                         \'ts_error_jobid_idx\',\n                                         [\n                                           \'jobid\'\n                                         ]\n                                       ]\n                        },\n          \'ts_exitstatus\' => {\n                               \'FIELDS\' => [\n                                             \'jobid\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'PRIMARYKEY\' => 1,\n                                               \'TYPE\' => \'INTSERIAL\'\n                                             },\n                                             \'funcid\',\n                                             {\n                                               \'DEFAULT\' => 0,\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT4\'\n                                             },\n                                             \'status\',\n                                             {\n                                               \'TYPE\' => \'INT2\'\n                                             },\n                                             \'completion_time\',\n                                             {\n                                               \'TYPE\' => \'INT4\'\n                                             },\n                                             \'delete_after\',\n                                             {\n                                               \'TYPE\' => \'INT4\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'ts_exitstatus_funcid_idx\',\n                                              [\n                                                \'funcid\'\n                                              ],\n                                              \'ts_exitstatus_delete_after_idx\',\n                                              [\n                                                \'delete_after\'\n                                              ]\n                                            ]\n                             },\n          \'ts_funcmap\' => {\n                            \'FIELDS\' => [\n                                          \'funcid\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'PRIMARYKEY\' => 1,\n                                            \'TYPE\' => \'INTSERIAL\'\n                                          },\n                                          \'funcname\',\n                                          {\n                                            \'NOTNULL\' => 1,\n                                            \'TYPE\' => \'varchar(255)\'\n                                          }\n                                        ],\n                            \'INDEXES\' => [\n                                           \'ts_funcmap_funcname_idx\',\n                                           {\n                                             \'FIELDS\' => [\n                                                           \'funcname\'\n                                                         ],\n                                             \'TYPE\' => \'UNIQUE\'\n                                           }\n                                         ]\n                          },\n          \'ts_job\' => {\n                        \'FIELDS\' => [\n                                      \'jobid\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'PRIMARYKEY\' => 1,\n                                        \'TYPE\' => \'INTSERIAL\'\n                                      },\n                                      \'funcid\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'INT4\'\n                                      },\n                                      \'arg\',\n                                      {\n                                        \'TYPE\' => \'LONGBLOB\'\n                                      },\n                                      \'uniqkey\',\n                                      {\n                                        \'TYPE\' => \'varchar(255)\'\n                                      },\n                                      \'insert_time\',\n                                      {\n                                        \'TYPE\' => \'INT4\'\n                                      },\n                                      \'run_after\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'INT4\'\n                                      },\n                                      \'grabbed_until\',\n                                      {\n                                        \'NOTNULL\' => 1,\n                                        \'TYPE\' => \'INT4\'\n                                      },\n                                      \'priority\',\n                                      {\n                                        \'TYPE\' => \'INT2\'\n                                      },\n                                      \'coalesce\',\n                                      {\n                                        \'TYPE\' => \'varchar(255)\'\n                                      }\n                                    ],\n                        \'INDEXES\' => [\n                                       \'ts_job_funcid_idx\',\n                                       {\n                                         \'FIELDS\' => [\n                                                       \'funcid\',\n                                                       \'uniqkey\'\n                                                     ],\n                                         \'TYPE\' => \'UNIQUE\'\n                                       },\n                                       \'ts_job_run_after_idx\',\n                                       [\n                                         \'run_after\',\n                                         \'funcid\'\n                                       ],\n                                       \'ts_job_coalesce_idx\',\n                                       [\n                                         \'coalesce\',\n                                         \'funcid\'\n                                       ]\n                                     ]\n                      },\n          \'ts_note\' => {\n                         \'FIELDS\' => [\n                                       \'jobid\',\n                                       {\n                                         \'NOTNULL\' => 1,\n                                         \'TYPE\' => \'INT4\'\n                                       },\n                                       \'notekey\',\n                                       {\n                                         \'TYPE\' => \'varchar(255)\'\n                                       },\n                                       \'value\',\n                                       {\n                                         \'TYPE\' => \'LONGBLOB\'\n                                       }\n                                     ],\n                         \'INDEXES\' => [\n                                        \'ts_note_jobid_idx\',\n                                        {\n                                          \'FIELDS\' => [\n                                                        \'jobid\',\n                                                        \'notekey\'\n                                                      ],\n                                          \'TYPE\' => \'UNIQUE\'\n                                        }\n                                      ]\n                       },\n          \'user_group_map\' => {\n                                \'FIELDS\' => [\n                                              \'user_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'userid\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'profiles\'\n                                                                },\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'group_id\',\n                                              {\n                                                \'NOTNULL\' => 1,\n                                                \'REFERENCES\' => {\n                                                                  \'COLUMN\' => \'id\',\n                                                                  \'DELETE\' => \'CASCADE\',\n                                                                  \'TABLE\' => \'groups\'\n                                                                },\n                                                \'TYPE\' => \'INT3\'\n                                              },\n                                              \'isbless\',\n                                              {\n                                                \'DEFAULT\' => \'FALSE\',\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'BOOLEAN\'\n                                              },\n                                              \'grant_type\',\n                                              {\n                                                \'DEFAULT\' => 0,\n                                                \'NOTNULL\' => 1,\n                                                \'TYPE\' => \'INT1\'\n                                              }\n                                            ],\n                                \'INDEXES\' => [\n                                               \'user_group_map_user_id_idx\',\n                                               {\n                                                 \'FIELDS\' => [\n                                                               \'user_id\',\n                                                               \'group_id\',\n                                                               \'grant_type\',\n                                                               \'isbless\'\n                                                             ],\n                                                 \'TYPE\' => \'UNIQUE\'\n                                               }\n                                             ]\n                              },\n          \'versions\' => {\n                          \'FIELDS\' => [\n                                        \'id\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'PRIMARYKEY\' => 1,\n                                          \'TYPE\' => \'MEDIUMSERIAL\'\n                                        },\n                                        \'value\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'TYPE\' => \'varchar(64)\'\n                                        },\n                                        \'product_id\',\n                                        {\n                                          \'NOTNULL\' => 1,\n                                          \'REFERENCES\' => {\n                                                            \'COLUMN\' => \'id\',\n                                                            \'DELETE\' => \'CASCADE\',\n                                                            \'TABLE\' => \'products\'\n                                                          },\n                                          \'TYPE\' => \'INT2\'\n                                        }\n                                      ],\n                          \'INDEXES\' => [\n                                         \'versions_product_id_idx\',\n                                         {\n                                           \'FIELDS\' => [\n                                                         \'product_id\',\n                                                         \'value\'\n                                                       ],\n                                           \'TYPE\' => \'UNIQUE\'\n                                         }\n                                       ]\n                        },\n          \'votes\' => {\n                       \'FIELDS\' => [\n                                     \'who\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'bug_id\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'bug_id\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'bugs\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'vote_count\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'TYPE\' => \'INT2\'\n                                     }\n                                   ],\n                       \'INDEXES\' => [\n                                      \'votes_who_idx\',\n                                      [\n                                        \'who\'\n                                      ],\n                                      \'votes_bug_id_idx\',\n                                      [\n                                        \'bug_id\'\n                                      ]\n                                    ]\n                     },\n          \'watch\' => {\n                       \'FIELDS\' => [\n                                     \'watcher\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     },\n                                     \'watched\',\n                                     {\n                                       \'NOTNULL\' => 1,\n                                       \'REFERENCES\' => {\n                                                         \'COLUMN\' => \'userid\',\n                                                         \'DELETE\' => \'CASCADE\',\n                                                         \'TABLE\' => \'profiles\'\n                                                       },\n                                       \'TYPE\' => \'INT3\'\n                                     }\n                                   ],\n                       \'INDEXES\' => [\n                                      \'watch_watcher_idx\',\n                                      {\n                                        \'FIELDS\' => [\n                                                      \'watcher\',\n                                                      \'watched\'\n                                                    ],\n                                        \'TYPE\' => \'UNIQUE\'\n                                      },\n                                      \'watch_watched_idx\',\n                                      [\n                                        \'watched\'\n                                      ]\n                                    ]\n                     },\n          \'whine_events\' => {\n                              \'FIELDS\' => [\n                                            \'id\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'PRIMARYKEY\' => 1,\n                                              \'TYPE\' => \'MEDIUMSERIAL\'\n                                            },\n                                            \'owner_userid\',\n                                            {\n                                              \'NOTNULL\' => 1,\n                                              \'REFERENCES\' => {\n                                                                \'COLUMN\' => \'userid\',\n                                                                \'DELETE\' => \'CASCADE\',\n                                                                \'TABLE\' => \'profiles\'\n                                                              },\n                                              \'TYPE\' => \'INT3\'\n                                            },\n                                            \'subject\',\n                                            {\n                                              \'TYPE\' => \'varchar(128)\'\n                                            },\n                                            \'body\',\n                                            {\n                                              \'TYPE\' => \'MEDIUMTEXT\'\n                                            },\n                                            \'mailifnobugs\',\n                                            {\n                                              \'DEFAULT\' => \'FALSE\',\n                                              \'NOTNULL\' => 1,\n                                              \'TYPE\' => \'BOOLEAN\'\n                                            }\n                                          ]\n                            },\n          \'whine_queries\' => {\n                               \'FIELDS\' => [\n                                             \'id\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'PRIMARYKEY\' => 1,\n                                               \'TYPE\' => \'MEDIUMSERIAL\'\n                                             },\n                                             \'eventid\',\n                                             {\n                                               \'NOTNULL\' => 1,\n                                               \'REFERENCES\' => {\n                                                                 \'COLUMN\' => \'id\',\n                                                                 \'DELETE\' => \'CASCADE\',\n                                                                 \'TABLE\' => \'whine_events\'\n                                                               },\n                                               \'TYPE\' => \'INT3\'\n                                             },\n                                             \'query_name\',\n                                             {\n                                               \'DEFAULT\' => \'\\\'\\\'\',\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'varchar(64)\'\n                                             },\n                                             \'sortkey\',\n                                             {\n                                               \'DEFAULT\' => \'0\',\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'INT2\'\n                                             },\n                                             \'onemailperbug\',\n                                             {\n                                               \'DEFAULT\' => \'FALSE\',\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'BOOLEAN\'\n                                             },\n                                             \'title\',\n                                             {\n                                               \'DEFAULT\' => \'\\\'\\\'\',\n                                               \'NOTNULL\' => 1,\n                                               \'TYPE\' => \'varchar(128)\'\n                                             }\n                                           ],\n                               \'INDEXES\' => [\n                                              \'whine_queries_eventid_idx\',\n                                              [\n                                                \'eventid\'\n                                              ]\n                                            ]\n                             },\n          \'whine_schedules\' => {\n                                 \'FIELDS\' => [\n                                               \'id\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'PRIMARYKEY\' => 1,\n                                                 \'TYPE\' => \'MEDIUMSERIAL\'\n                                               },\n                                               \'eventid\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'REFERENCES\' => {\n                                                                   \'COLUMN\' => \'id\',\n                                                                   \'DELETE\' => \'CASCADE\',\n                                                                   \'TABLE\' => \'whine_events\'\n                                                                 },\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'run_day\',\n                                               {\n                                                 \'TYPE\' => \'varchar(32)\'\n                                               },\n                                               \'run_time\',\n                                               {\n                                                 \'TYPE\' => \'varchar(32)\'\n                                               },\n                                               \'run_next\',\n                                               {\n                                                 \'TYPE\' => \'DATETIME\'\n                                               },\n                                               \'mailto\',\n                                               {\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT3\'\n                                               },\n                                               \'mailto_type\',\n                                               {\n                                                 \'DEFAULT\' => \'0\',\n                                                 \'NOTNULL\' => 1,\n                                                 \'TYPE\' => \'INT2\'\n                                               }\n                                             ],\n                                 \'INDEXES\' => [\n                                                \'whine_schedules_run_next_idx\',\n                                                [\n                                                  \'run_next\'\n                                                ],\n                                                \'whine_schedules_eventid_idx\',\n                                                [\n                                                  \'eventid\'\n                                                ]\n                                              ]\n                               }\n        };\n','2.00');
/*!40000 ALTER TABLE `bz_schema` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `category_group_map`
--

DROP TABLE IF EXISTS `category_group_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `category_group_map` (
  `category_id` smallint(6) NOT NULL,
  `group_id` mediumint(9) NOT NULL,
  UNIQUE KEY `category_group_map_category_id_idx` (`category_id`,`group_id`),
  KEY `fk_category_group_map_group_id_groups_id` (`group_id`),
  CONSTRAINT `fk_category_group_map_group_id_groups_id` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_category_group_map_category_id_series_categories_id` FOREIGN KEY (`category_id`) REFERENCES `series_categories` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `category_group_map`
--

LOCK TABLES `category_group_map` WRITE;
/*!40000 ALTER TABLE `category_group_map` DISABLE KEYS */;
/*!40000 ALTER TABLE `category_group_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `cc`
--

DROP TABLE IF EXISTS `cc`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `cc` (
  `bug_id` mediumint(9) NOT NULL,
  `who` mediumint(9) NOT NULL,
  UNIQUE KEY `cc_bug_id_idx` (`bug_id`,`who`),
  KEY `cc_who_idx` (`who`),
  CONSTRAINT `fk_cc_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_cc_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `cc`
--

LOCK TABLES `cc` WRITE;
/*!40000 ALTER TABLE `cc` DISABLE KEYS */;
/*!40000 ALTER TABLE `cc` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `classifications`
--

DROP TABLE IF EXISTS `classifications`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `classifications` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `description` mediumtext,
  `sortkey` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `classifications_name_idx` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `classifications`
--

LOCK TABLES `classifications` WRITE;
/*!40000 ALTER TABLE `classifications` DISABLE KEYS */;
INSERT INTO `classifications` VALUES (1,'PUBLIC','Publicly available products',0),(2,'PARTNER','Products visible to partners',0),(3,'PRIVATE','Products that are only visible internally.',0);
/*!40000 ALTER TABLE `classifications` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `component_cc`
--

DROP TABLE IF EXISTS `component_cc`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `component_cc` (
  `user_id` mediumint(9) NOT NULL,
  `component_id` smallint(6) NOT NULL,
  UNIQUE KEY `component_cc_user_id_idx` (`component_id`,`user_id`),
  KEY `fk_component_cc_user_id_profiles_userid` (`user_id`),
  CONSTRAINT `fk_component_cc_component_id_components_id` FOREIGN KEY (`component_id`) REFERENCES `components` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_component_cc_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `component_cc`
--

LOCK TABLES `component_cc` WRITE;
/*!40000 ALTER TABLE `component_cc` DISABLE KEYS */;
/*!40000 ALTER TABLE `component_cc` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `components`
--

DROP TABLE IF EXISTS `components`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `components` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `product_id` smallint(6) NOT NULL,
  `initialowner` mediumint(9) NOT NULL,
  `initialqacontact` mediumint(9) default NULL,
  `description` mediumtext NOT NULL,
  `disallownew` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `components_product_id_idx` (`product_id`,`name`),
  KEY `components_name_idx` (`name`),
  KEY `fk_components_initialowner_profiles_userid` (`initialowner`),
  KEY `fk_components_initialqacontact_profiles_userid` (`initialqacontact`),
  CONSTRAINT `fk_components_initialqacontact_profiles_userid` FOREIGN KEY (`initialqacontact`) REFERENCES `profiles` (`userid`) ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_components_initialowner_profiles_userid` FOREIGN KEY (`initialowner`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_components_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `components`
--

LOCK TABLES `components` WRITE;
/*!40000 ALTER TABLE `components` DISABLE KEYS */;
INSERT INTO `components` VALUES (1,'PUBLIC ONE COMP 1',1,4,5,'PUBLIC ONE COMP 1',0),(2,'PUBLIC ONE COMP 2',1,4,5,'PUBLIC ONE COMP 2',0),(3,'PRIVATE ONE COMP 1',2,7,8,'PRIVATE ONE COMP 1',0),(4,'PARTNER ONE COMP 1',3,2,6,'PARTNER ONE COMP 1',0);
/*!40000 ALTER TABLE `components` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `dependencies`
--

DROP TABLE IF EXISTS `dependencies`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `dependencies` (
  `blocked` mediumint(9) NOT NULL,
  `dependson` mediumint(9) NOT NULL,
  KEY `dependencies_blocked_idx` (`blocked`),
  KEY `dependencies_dependson_idx` (`dependson`),
  CONSTRAINT `fk_dependencies_dependson_bugs_bug_id` FOREIGN KEY (`dependson`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_dependencies_blocked_bugs_bug_id` FOREIGN KEY (`blocked`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `dependencies`
--

LOCK TABLES `dependencies` WRITE;
/*!40000 ALTER TABLE `dependencies` DISABLE KEYS */;
/*!40000 ALTER TABLE `dependencies` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `duplicates`
--

DROP TABLE IF EXISTS `duplicates`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `duplicates` (
  `dupe_of` mediumint(9) NOT NULL,
  `dupe` mediumint(9) NOT NULL,
  PRIMARY KEY  (`dupe`),
  KEY `fk_duplicates_dupe_of_bugs_bug_id` (`dupe_of`),
  CONSTRAINT `fk_duplicates_dupe_bugs_bug_id` FOREIGN KEY (`dupe`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_duplicates_dupe_of_bugs_bug_id` FOREIGN KEY (`dupe_of`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `duplicates`
--

LOCK TABLES `duplicates` WRITE;
/*!40000 ALTER TABLE `duplicates` DISABLE KEYS */;
/*!40000 ALTER TABLE `duplicates` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `email_setting`
--

DROP TABLE IF EXISTS `email_setting`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `email_setting` (
  `user_id` mediumint(9) NOT NULL,
  `relationship` tinyint(4) NOT NULL,
  `event` tinyint(4) NOT NULL,
  UNIQUE KEY `email_setting_user_id_idx` (`user_id`,`relationship`,`event`),
  CONSTRAINT `fk_email_setting_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `email_setting`
--

LOCK TABLES `email_setting` WRITE;
/*!40000 ALTER TABLE `email_setting` DISABLE KEYS */;
INSERT INTO `email_setting` VALUES (1,0,0),(1,0,1),(1,0,2),(1,0,3),(1,0,4),(1,0,5),(1,0,6),(1,0,7),(1,0,9),(1,0,50),(1,1,0),(1,1,1),(1,1,2),(1,1,3),(1,1,4),(1,1,5),(1,1,6),(1,1,7),(1,1,9),(1,1,50),(1,2,0),(1,2,1),(1,2,2),(1,2,3),(1,2,4),(1,2,5),(1,2,6),(1,2,7),(1,2,8),(1,2,9),(1,2,50),(1,3,0),(1,3,1),(1,3,2),(1,3,3),(1,3,4),(1,3,5),(1,3,6),(1,3,7),(1,3,9),(1,3,50),(1,4,0),(1,4,1),(1,4,2),(1,4,3),(1,4,4),(1,4,5),(1,4,6),(1,4,7),(1,4,9),(1,4,50),(1,5,0),(1,5,1),(1,5,2),(1,5,3),(1,5,4),(1,5,5),(1,5,6),(1,5,7),(1,5,9),(1,5,50),(1,100,100),(1,100,101),(2,0,0),(2,0,1),(2,0,2),(2,0,3),(2,0,4),(2,0,5),(2,0,6),(2,0,7),(2,0,9),(2,0,50),(2,1,0),(2,1,1),(2,1,2),(2,1,3),(2,1,4),(2,1,5),(2,1,6),(2,1,7),(2,1,9),(2,1,50),(2,2,0),(2,2,1),(2,2,2),(2,2,3),(2,2,4),(2,2,5),(2,2,6),(2,2,7),(2,2,8),(2,2,9),(2,2,50),(2,3,0),(2,3,1),(2,3,2),(2,3,3),(2,3,4),(2,3,5),(2,3,6),(2,3,7),(2,3,9),(2,3,50),(2,4,0),(2,4,1),(2,4,2),(2,4,3),(2,4,4),(2,4,5),(2,4,6),(2,4,7),(2,4,9),(2,4,50),(2,5,0),(2,5,1),(2,5,2),(2,5,3),(2,5,4),(2,5,5),(2,5,6),(2,5,7),(2,5,9),(2,5,50),(2,100,100),(2,100,101),(3,0,0),(3,0,1),(3,0,2),(3,0,3),(3,0,4),(3,0,5),(3,0,6),(3,0,7),(3,0,9),(3,0,50),(3,1,0),(3,1,1),(3,1,2),(3,1,3),(3,1,4),(3,1,5),(3,1,6),(3,1,7),(3,1,9),(3,1,50),(3,2,0),(3,2,1),(3,2,2),(3,2,3),(3,2,4),(3,2,5),(3,2,6),(3,2,7),(3,2,8),(3,2,9),(3,2,50),(3,3,0),(3,3,1),(3,3,2),(3,3,3),(3,3,4),(3,3,5),(3,3,6),(3,3,7),(3,3,9),(3,3,50),(3,4,0),(3,4,1),(3,4,2),(3,4,3),(3,4,4),(3,4,5),(3,4,6),(3,4,7),(3,4,9),(3,4,50),(3,5,0),(3,5,1),(3,5,2),(3,5,3),(3,5,4),(3,5,5),(3,5,6),(3,5,7),(3,5,9),(3,5,50),(3,100,100),(3,100,101),(4,0,0),(4,0,1),(4,0,2),(4,0,3),(4,0,4),(4,0,5),(4,0,6),(4,0,7),(4,0,9),(4,0,50),(4,1,0),(4,1,1),(4,1,2),(4,1,3),(4,1,4),(4,1,5),(4,1,6),(4,1,7),(4,1,9),(4,1,50),(4,2,0),(4,2,1),(4,2,2),(4,2,3),(4,2,4),(4,2,5),(4,2,6),(4,2,7),(4,2,8),(4,2,9),(4,2,50),(4,3,0),(4,3,1),(4,3,2),(4,3,3),(4,3,4),(4,3,5),(4,3,6),(4,3,7),(4,3,9),(4,3,50),(4,4,0),(4,4,1),(4,4,2),(4,4,3),(4,4,4),(4,4,5),(4,4,6),(4,4,7),(4,4,9),(4,4,50),(4,5,0),(4,5,1),(4,5,2),(4,5,3),(4,5,4),(4,5,5),(4,5,6),(4,5,7),(4,5,9),(4,5,50),(4,100,100),(4,100,101),(5,0,0),(5,0,1),(5,0,2),(5,0,3),(5,0,4),(5,0,5),(5,0,6),(5,0,7),(5,0,9),(5,0,50),(5,1,0),(5,1,1),(5,1,2),(5,1,3),(5,1,4),(5,1,5),(5,1,6),(5,1,7),(5,1,9),(5,1,50),(5,2,0),(5,2,1),(5,2,2),(5,2,3),(5,2,4),(5,2,5),(5,2,6),(5,2,7),(5,2,8),(5,2,9),(5,2,50),(5,3,0),(5,3,1),(5,3,2),(5,3,3),(5,3,4),(5,3,5),(5,3,6),(5,3,7),(5,3,9),(5,3,50),(5,4,0),(5,4,1),(5,4,2),(5,4,3),(5,4,4),(5,4,5),(5,4,6),(5,4,7),(5,4,9),(5,4,50),(5,5,0),(5,5,1),(5,5,2),(5,5,3),(5,5,4),(5,5,5),(5,5,6),(5,5,7),(5,5,9),(5,5,50),(5,100,100),(5,100,101),(6,0,0),(6,0,1),(6,0,2),(6,0,3),(6,0,4),(6,0,5),(6,0,6),(6,0,7),(6,0,9),(6,0,50),(6,1,0),(6,1,1),(6,1,2),(6,1,3),(6,1,4),(6,1,5),(6,1,6),(6,1,7),(6,1,9),(6,1,50),(6,2,0),(6,2,1),(6,2,2),(6,2,3),(6,2,4),(6,2,5),(6,2,6),(6,2,7),(6,2,8),(6,2,9),(6,2,50),(6,3,0),(6,3,1),(6,3,2),(6,3,3),(6,3,4),(6,3,5),(6,3,6),(6,3,7),(6,3,9),(6,3,50),(6,4,0),(6,4,1),(6,4,2),(6,4,3),(6,4,4),(6,4,5),(6,4,6),(6,4,7),(6,4,9),(6,4,50),(6,5,0),(6,5,1),(6,5,2),(6,5,3),(6,5,4),(6,5,5),(6,5,6),(6,5,7),(6,5,9),(6,5,50),(6,100,100),(6,100,101),(7,0,0),(7,0,1),(7,0,2),(7,0,3),(7,0,4),(7,0,5),(7,0,6),(7,0,7),(7,0,9),(7,0,50),(7,1,0),(7,1,1),(7,1,2),(7,1,3),(7,1,4),(7,1,5),(7,1,6),(7,1,7),(7,1,9),(7,1,50),(7,2,0),(7,2,1),(7,2,2),(7,2,3),(7,2,4),(7,2,5),(7,2,6),(7,2,7),(7,2,8),(7,2,9),(7,2,50),(7,3,0),(7,3,1),(7,3,2),(7,3,3),(7,3,4),(7,3,5),(7,3,6),(7,3,7),(7,3,9),(7,3,50),(7,4,0),(7,4,1),(7,4,2),(7,4,3),(7,4,4),(7,4,5),(7,4,6),(7,4,7),(7,4,9),(7,4,50),(7,5,0),(7,5,1),(7,5,2),(7,5,3),(7,5,4),(7,5,5),(7,5,6),(7,5,7),(7,5,9),(7,5,50),(7,100,100),(7,100,101),(8,0,0),(8,0,1),(8,0,2),(8,0,3),(8,0,4),(8,0,5),(8,0,6),(8,0,7),(8,0,9),(8,0,50),(8,1,0),(8,1,1),(8,1,2),(8,1,3),(8,1,4),(8,1,5),(8,1,6),(8,1,7),(8,1,9),(8,1,50),(8,2,0),(8,2,1),(8,2,2),(8,2,3),(8,2,4),(8,2,5),(8,2,6),(8,2,7),(8,2,8),(8,2,9),(8,2,50),(8,3,0),(8,3,1),(8,3,2),(8,3,3),(8,3,4),(8,3,5),(8,3,6),(8,3,7),(8,3,9),(8,3,50),(8,4,0),(8,4,1),(8,4,2),(8,4,3),(8,4,4),(8,4,5),(8,4,6),(8,4,7),(8,4,9),(8,4,50),(8,5,0),(8,5,1),(8,5,2),(8,5,3),(8,5,4),(8,5,5),(8,5,6),(8,5,7),(8,5,9),(8,5,50),(8,100,100),(8,100,101);
/*!40000 ALTER TABLE `email_setting` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `fielddefs`
--

DROP TABLE IF EXISTS `fielddefs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `fielddefs` (
  `id` mediumint(9) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `type` smallint(6) NOT NULL default '0',
  `custom` tinyint(4) NOT NULL default '0',
  `description` tinytext NOT NULL,
  `mailhead` tinyint(4) NOT NULL default '0',
  `sortkey` smallint(6) NOT NULL,
  `obsolete` tinyint(4) NOT NULL default '0',
  `enter_bug` tinyint(4) NOT NULL default '0',
  `visibility_field_id` mediumint(9) default NULL,
  `visibility_value_id` smallint(6) default NULL,
  `value_field_id` mediumint(9) default NULL,
  `buglist` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `fielddefs_name_idx` (`name`),
  KEY `fielddefs_sortkey_idx` (`sortkey`),
  KEY `fielddefs_value_field_id_idx` (`value_field_id`),
  KEY `fk_fielddefs_visibility_field_id_fielddefs_id` (`visibility_field_id`),
  CONSTRAINT `fk_fielddefs_value_field_id_fielddefs_id` FOREIGN KEY (`value_field_id`) REFERENCES `fielddefs` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_fielddefs_visibility_field_id_fielddefs_id` FOREIGN KEY (`visibility_field_id`) REFERENCES `fielddefs` (`id`) ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=55 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `fielddefs`
--

LOCK TABLES `fielddefs` WRITE;
/*!40000 ALTER TABLE `fielddefs` DISABLE KEYS */;
INSERT INTO `fielddefs` VALUES (1,'bug_id',0,0,'Bug #',1,100,0,0,NULL,NULL,NULL,1),(2,'short_desc',0,0,'Summary',1,200,0,0,NULL,NULL,NULL,1),(3,'classification',0,0,'Classification',1,300,0,0,NULL,NULL,NULL,1),(4,'product',2,0,'Product',1,400,0,0,NULL,NULL,NULL,1),(5,'version',0,0,'Version',1,500,0,0,NULL,NULL,NULL,1),(6,'rep_platform',2,0,'Platform',1,600,0,0,NULL,NULL,NULL,1),(7,'bug_file_loc',0,0,'URL',1,700,0,0,NULL,NULL,NULL,0),(8,'op_sys',2,0,'OS/Version',1,800,0,0,NULL,NULL,NULL,1),(9,'bug_status',2,0,'Status',1,900,0,0,NULL,NULL,NULL,1),(10,'status_whiteboard',0,0,'Status Whiteboard',1,1000,0,0,NULL,NULL,NULL,1),(11,'keywords',0,0,'Keywords',1,1100,0,0,NULL,NULL,NULL,1),(12,'resolution',2,0,'Resolution',0,1200,0,0,NULL,NULL,NULL,1),(13,'bug_severity',2,0,'Severity',1,1300,0,0,NULL,NULL,NULL,1),(14,'priority',2,0,'Priority',1,1400,0,0,NULL,NULL,NULL,1),(15,'component',0,0,'Component',1,1500,0,0,NULL,NULL,NULL,1),(16,'assigned_to',0,0,'AssignedTo',1,1600,0,0,NULL,NULL,NULL,1),(17,'reporter',0,0,'ReportedBy',1,1700,0,0,NULL,NULL,NULL,1),(18,'votes',0,0,'Votes',0,1800,0,0,NULL,NULL,NULL,1),(19,'qa_contact',0,0,'QAContact',1,1900,0,0,NULL,NULL,NULL,1),(20,'cc',0,0,'CC',1,2000,0,0,NULL,NULL,NULL,0),(21,'dependson',0,0,'Depends on',1,2100,0,0,NULL,NULL,NULL,0),(22,'blocked',0,0,'Blocks',1,2200,0,0,NULL,NULL,NULL,0),(23,'attachments.description',0,0,'Attachment description',0,2300,0,0,NULL,NULL,NULL,0),(24,'attachments.filename',0,0,'Attachment filename',0,2400,0,0,NULL,NULL,NULL,0),(25,'attachments.mimetype',0,0,'Attachment mime type',0,2500,0,0,NULL,NULL,NULL,0),(26,'attachments.ispatch',0,0,'Attachment is patch',0,2600,0,0,NULL,NULL,NULL,0),(27,'attachments.isobsolete',0,0,'Attachment is obsolete',0,2700,0,0,NULL,NULL,NULL,0),(28,'attachments.isprivate',0,0,'Attachment is private',0,2800,0,0,NULL,NULL,NULL,0),(29,'attachments.submitter',0,0,'Attachment creator',0,2900,0,0,NULL,NULL,NULL,0),(30,'target_milestone',0,0,'Target Milestone',0,3000,0,0,NULL,NULL,NULL,1),(31,'creation_ts',0,0,'Creation date',1,3100,0,0,NULL,NULL,NULL,1),(32,'delta_ts',0,0,'Last changed date',1,3200,0,0,NULL,NULL,NULL,1),(33,'longdesc',0,0,'Comment',0,3300,0,0,NULL,NULL,NULL,0),(34,'longdescs.isprivate',0,0,'Comment is private',0,3400,0,0,NULL,NULL,NULL,0),(35,'alias',0,0,'Alias',0,3500,0,0,NULL,NULL,NULL,1),(36,'everconfirmed',0,0,'Ever Confirmed',0,3600,0,0,NULL,NULL,NULL,0),(37,'reporter_accessible',0,0,'Reporter Accessible',0,3700,0,0,NULL,NULL,NULL,0),(38,'cclist_accessible',0,0,'CC Accessible',0,3800,0,0,NULL,NULL,NULL,0),(39,'bug_group',0,0,'Group',1,3900,0,0,NULL,NULL,NULL,0),(40,'estimated_time',0,0,'Estimated Hours',1,4000,0,0,NULL,NULL,NULL,1),(41,'remaining_time',0,0,'Remaining Hours',0,4100,0,0,NULL,NULL,NULL,1),(42,'deadline',0,0,'Deadline',1,4200,0,0,NULL,NULL,NULL,1),(43,'commenter',0,0,'Commenter',0,4300,0,0,NULL,NULL,NULL,0),(44,'flagtypes.name',0,0,'Flags',0,4400,0,0,NULL,NULL,NULL,1),(45,'requestees.login_name',0,0,'Flag Requestee',0,4500,0,0,NULL,NULL,NULL,0),(46,'setters.login_name',0,0,'Flag Setter',0,4600,0,0,NULL,NULL,NULL,0),(47,'work_time',0,0,'Hours Worked',0,4700,0,0,NULL,NULL,NULL,1),(48,'percentage_complete',0,0,'Percentage Complete',0,4800,0,0,NULL,NULL,NULL,1),(49,'content',0,0,'Content',0,4900,0,0,NULL,NULL,NULL,0),(50,'attach_data.thedata',0,0,'Attachment data',0,5000,0,0,NULL,NULL,NULL,0),(51,'attachments.isurl',0,0,'Attachment is a URL',0,5100,0,0,NULL,NULL,NULL,0),(52,'owner_idle_time',0,0,'Time Since Assignee Touched',0,5200,0,0,NULL,NULL,NULL,0),(53,'days_elapsed',0,0,'Days since bug changed',0,5300,0,0,NULL,NULL,NULL,0),(54,'see_also',7,0,'See Also',0,5400,0,0,NULL,NULL,NULL,0);
/*!40000 ALTER TABLE `fielddefs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flagexclusions`
--

DROP TABLE IF EXISTS `flagexclusions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `flagexclusions` (
  `type_id` smallint(6) NOT NULL,
  `product_id` smallint(6) default NULL,
  `component_id` smallint(6) default NULL,
  KEY `flagexclusions_type_id_idx` (`type_id`,`product_id`,`component_id`),
  KEY `fk_flagexclusions_product_id_products_id` (`product_id`),
  KEY `fk_flagexclusions_component_id_components_id` (`component_id`),
  CONSTRAINT `fk_flagexclusions_component_id_components_id` FOREIGN KEY (`component_id`) REFERENCES `components` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flagexclusions_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flagexclusions_type_id_flagtypes_id` FOREIGN KEY (`type_id`) REFERENCES `flagtypes` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `flagexclusions`
--

LOCK TABLES `flagexclusions` WRITE;
/*!40000 ALTER TABLE `flagexclusions` DISABLE KEYS */;
/*!40000 ALTER TABLE `flagexclusions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flaginclusions`
--

DROP TABLE IF EXISTS `flaginclusions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `flaginclusions` (
  `type_id` smallint(6) NOT NULL,
  `product_id` smallint(6) default NULL,
  `component_id` smallint(6) default NULL,
  KEY `flaginclusions_type_id_idx` (`type_id`,`product_id`,`component_id`),
  KEY `fk_flaginclusions_product_id_products_id` (`product_id`),
  KEY `fk_flaginclusions_component_id_components_id` (`component_id`),
  CONSTRAINT `fk_flaginclusions_component_id_components_id` FOREIGN KEY (`component_id`) REFERENCES `components` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flaginclusions_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flaginclusions_type_id_flagtypes_id` FOREIGN KEY (`type_id`) REFERENCES `flagtypes` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `flaginclusions`
--

LOCK TABLES `flaginclusions` WRITE;
/*!40000 ALTER TABLE `flaginclusions` DISABLE KEYS */;
/*!40000 ALTER TABLE `flaginclusions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flags`
--

DROP TABLE IF EXISTS `flags`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `flags` (
  `id` mediumint(9) NOT NULL auto_increment,
  `type_id` smallint(6) NOT NULL,
  `status` char(1) NOT NULL,
  `bug_id` mediumint(9) NOT NULL,
  `attach_id` mediumint(9) default NULL,
  `creation_date` datetime NOT NULL,
  `modification_date` datetime default NULL,
  `setter_id` mediumint(9) default NULL,
  `requestee_id` mediumint(9) default NULL,
  PRIMARY KEY  (`id`),
  KEY `flags_bug_id_idx` (`bug_id`,`attach_id`),
  KEY `flags_setter_id_idx` (`setter_id`),
  KEY `flags_requestee_id_idx` (`requestee_id`),
  KEY `flags_type_id_idx` (`type_id`),
  KEY `fk_flags_attach_id_attachments_attach_id` (`attach_id`),
  CONSTRAINT `fk_flags_requestee_id_profiles_userid` FOREIGN KEY (`requestee_id`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_flags_attach_id_attachments_attach_id` FOREIGN KEY (`attach_id`) REFERENCES `attachments` (`attach_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flags_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_flags_setter_id_profiles_userid` FOREIGN KEY (`setter_id`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_flags_type_id_flagtypes_id` FOREIGN KEY (`type_id`) REFERENCES `flagtypes` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `flags`
--

LOCK TABLES `flags` WRITE;
/*!40000 ALTER TABLE `flags` DISABLE KEYS */;
/*!40000 ALTER TABLE `flags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flagtypes`
--

DROP TABLE IF EXISTS `flagtypes`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `flagtypes` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(50) NOT NULL,
  `description` mediumtext NOT NULL,
  `cc_list` varchar(200) default NULL,
  `target_type` char(1) NOT NULL default 'b',
  `is_active` tinyint(4) NOT NULL default '1',
  `is_requestable` tinyint(4) NOT NULL default '0',
  `is_requesteeble` tinyint(4) NOT NULL default '0',
  `is_multiplicable` tinyint(4) NOT NULL default '0',
  `sortkey` smallint(6) NOT NULL default '0',
  `grant_group_id` mediumint(9) default NULL,
  `request_group_id` mediumint(9) default NULL,
  PRIMARY KEY  (`id`),
  KEY `fk_flagtypes_grant_group_id_groups_id` (`grant_group_id`),
  KEY `fk_flagtypes_request_group_id_groups_id` (`request_group_id`),
  CONSTRAINT `fk_flagtypes_request_group_id_groups_id` FOREIGN KEY (`request_group_id`) REFERENCES `groups` (`id`) ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_flagtypes_grant_group_id_groups_id` FOREIGN KEY (`grant_group_id`) REFERENCES `groups` (`id`) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `flagtypes`
--

LOCK TABLES `flagtypes` WRITE;
/*!40000 ALTER TABLE `flagtypes` DISABLE KEYS */;
/*!40000 ALTER TABLE `flagtypes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `group_control_map`
--

DROP TABLE IF EXISTS `group_control_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `group_control_map` (
  `group_id` mediumint(9) NOT NULL,
  `product_id` smallint(6) NOT NULL,
  `entry` tinyint(4) NOT NULL default '0',
  `membercontrol` tinyint(4) NOT NULL,
  `othercontrol` tinyint(4) NOT NULL,
  `canedit` tinyint(4) NOT NULL default '0',
  `editcomponents` tinyint(4) NOT NULL default '0',
  `editbugs` tinyint(4) NOT NULL default '0',
  `canconfirm` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `group_control_map_product_id_idx` (`product_id`,`group_id`),
  KEY `group_control_map_group_id_idx` (`group_id`),
  CONSTRAINT `fk_group_control_map_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_group_control_map_group_id_groups_id` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `group_control_map`
--

LOCK TABLES `group_control_map` WRITE;
/*!40000 ALTER TABLE `group_control_map` DISABLE KEYS */;
INSERT INTO `group_control_map` VALUES (16,2,1,3,3,0,0,0,0),(15,3,1,3,3,0,0,0,0);
/*!40000 ALTER TABLE `group_control_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `group_group_map`
--

DROP TABLE IF EXISTS `group_group_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `group_group_map` (
  `member_id` mediumint(9) NOT NULL,
  `grantor_id` mediumint(9) NOT NULL,
  `grant_type` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `group_group_map_member_id_idx` (`member_id`,`grantor_id`,`grant_type`),
  KEY `fk_group_group_map_grantor_id_groups_id` (`grantor_id`),
  CONSTRAINT `fk_group_group_map_grantor_id_groups_id` FOREIGN KEY (`grantor_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_group_group_map_member_id_groups_id` FOREIGN KEY (`member_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `group_group_map`
--

LOCK TABLES `group_group_map` WRITE;
/*!40000 ALTER TABLE `group_group_map` DISABLE KEYS */;
INSERT INTO `group_group_map` VALUES (2,2,0),(2,2,1),(2,2,2),(2,3,0),(2,3,1),(2,3,2),(2,4,0),(2,4,1),(2,4,2),(2,5,0),(2,5,1),(2,5,2),(2,6,0),(2,6,1),(2,6,2),(2,7,0),(2,7,1),(2,7,2),(2,8,0),(2,8,1),(2,8,2),(2,9,0),(2,9,1),(2,9,2),(2,10,0),(2,10,1),(2,10,2),(2,11,0),(2,11,1),(2,11,2),(13,11,0),(2,12,0),(2,12,1),(2,12,2),(2,13,0),(2,13,1),(2,13,2),(2,14,0),(2,14,1),(2,14,2),(12,14,0),(1,15,0),(2,15,0),(2,15,1),(2,15,2),(16,15,0),(1,16,0),(2,16,0),(2,16,1),(2,16,2);
/*!40000 ALTER TABLE `group_group_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `groups`
--

DROP TABLE IF EXISTS `groups`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `groups` (
  `id` mediumint(9) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL,
  `description` mediumtext NOT NULL,
  `isbuggroup` tinyint(4) NOT NULL,
  `userregexp` tinytext NOT NULL,
  `isactive` tinyint(4) NOT NULL default '1',
  `icon_url` tinytext,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `groups_name_idx` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=17 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `groups`
--

LOCK TABLES `groups` WRITE;
/*!40000 ALTER TABLE `groups` DISABLE KEYS */;
INSERT INTO `groups` VALUES (1,'Testers','Can read and write all test plans, runs, and cases.',0,'',1,NULL),(2,'admin','Administrators',0,'',1,NULL),(3,'tweakparams','Can change Parameters',0,'',1,NULL),(4,'editusers','Can edit or disable users',0,'',1,NULL),(5,'creategroups','Can create and destroy groups',0,'',1,NULL),(6,'editclassifications','Can create, destroy, and edit classifications',0,'',1,NULL),(7,'editcomponents','Can create, destroy, and edit components',0,'',1,NULL),(8,'editkeywords','Can create, destroy, and edit keywords',0,'',1,NULL),(9,'editbugs','Can edit all bug fields',0,'.*',1,NULL),(10,'canconfirm','Can confirm a bug or mark it a duplicate',0,'',1,NULL),(11,'bz_canusewhines','User can configure whine reports for self',0,'',1,NULL),(12,'bz_sudoers','Can perform actions as other users',0,'',1,NULL),(13,'bz_canusewhineatothers','Can configure whine reports for other users',0,'',1,NULL),(14,'bz_sudo_protect','Can not be impersonated by other users',0,'',1,NULL),(15,'partners','Full access to certain products',1,'',1,NULL),(16,'private','access restricted to insiders',1,'',1,NULL);
/*!40000 ALTER TABLE `groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `keyworddefs`
--

DROP TABLE IF EXISTS `keyworddefs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `keyworddefs` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `description` mediumtext,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `keyworddefs_name_idx` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `keyworddefs`
--

LOCK TABLES `keyworddefs` WRITE;
/*!40000 ALTER TABLE `keyworddefs` DISABLE KEYS */;
/*!40000 ALTER TABLE `keyworddefs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `keywords`
--

DROP TABLE IF EXISTS `keywords`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `keywords` (
  `bug_id` mediumint(9) NOT NULL,
  `keywordid` smallint(6) NOT NULL,
  UNIQUE KEY `keywords_bug_id_idx` (`bug_id`,`keywordid`),
  KEY `keywords_keywordid_idx` (`keywordid`),
  CONSTRAINT `fk_keywords_keywordid_keyworddefs_id` FOREIGN KEY (`keywordid`) REFERENCES `keyworddefs` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_keywords_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `keywords`
--

LOCK TABLES `keywords` WRITE;
/*!40000 ALTER TABLE `keywords` DISABLE KEYS */;
/*!40000 ALTER TABLE `keywords` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `login_failure`
--

DROP TABLE IF EXISTS `login_failure`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `login_failure` (
  `user_id` mediumint(9) NOT NULL,
  `login_time` datetime NOT NULL,
  `ip_addr` varchar(40) NOT NULL,
  KEY `login_failure_user_id_idx` (`user_id`),
  CONSTRAINT `fk_login_failure_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `login_failure`
--

LOCK TABLES `login_failure` WRITE;
/*!40000 ALTER TABLE `login_failure` DISABLE KEYS */;
/*!40000 ALTER TABLE `login_failure` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `logincookies`
--

DROP TABLE IF EXISTS `logincookies`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `logincookies` (
  `cookie` varchar(16) NOT NULL,
  `userid` mediumint(9) NOT NULL,
  `ipaddr` varchar(40) default NULL,
  `lastused` datetime NOT NULL,
  PRIMARY KEY  (`cookie`),
  KEY `logincookies_lastused_idx` (`lastused`),
  KEY `fk_logincookies_userid_profiles_userid` (`userid`),
  CONSTRAINT `fk_logincookies_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `logincookies`
--

LOCK TABLES `logincookies` WRITE;
/*!40000 ALTER TABLE `logincookies` DISABLE KEYS */;
INSERT INTO `logincookies` VALUES ('21zme1piZv',1,'127.0.0.2','2010-07-21 10:52:00');
/*!40000 ALTER TABLE `logincookies` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `longdescs`
--

DROP TABLE IF EXISTS `longdescs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `longdescs` (
  `comment_id` mediumint(9) NOT NULL auto_increment,
  `bug_id` mediumint(9) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `bug_when` datetime NOT NULL,
  `work_time` decimal(7,2) NOT NULL default '0.00',
  `thetext` mediumtext NOT NULL,
  `isprivate` tinyint(4) NOT NULL default '0',
  `already_wrapped` tinyint(4) NOT NULL default '0',
  `type` smallint(6) NOT NULL default '0',
  `extra_data` varchar(255) default NULL,
  PRIMARY KEY  (`comment_id`),
  KEY `longdescs_bug_id_idx` (`bug_id`),
  KEY `longdescs_who_idx` (`who`,`bug_id`),
  KEY `longdescs_bug_when_idx` (`bug_when`),
  CONSTRAINT `fk_longdescs_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_longdescs_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `longdescs`
--

LOCK TABLES `longdescs` WRITE;
/*!40000 ALTER TABLE `longdescs` DISABLE KEYS */;
INSERT INTO `longdescs` VALUES (1,1,1,'2008-03-27 15:48:39','0.00','PUBLIC VISIBLE BUG - basic',0,0,0,NULL),(2,2,1,'2008-05-01 17:24:10','0.00','STATUS: IDLE\nBUILD: PUBLIC ACTIVE BUILD 1\nENVIRONMENT: PUBLIC ACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: \n\nPublic bug logged from test case 5 in run 1',0,0,0,NULL),(3,3,1,'2008-05-02 15:10:00','0.00','THIS BUG IS PUBLIC',0,0,5,'1'),(4,4,2,'2008-05-02 15:19:36','0.00','PARTNER BUG',0,0,0,NULL),(5,5,7,'2008-05-02 15:21:06','0.00','PRIVATE BUG',0,0,0,NULL),(6,6,3,'2008-05-02 15:27:32','0.00','STATUS: IDLE\nBUILD: PRIVATE INACTIVE BUILD\nENVIRONMENT: PRIVATE INACTIVE ENVIRONMENT\nNOTES: \nSTEPS TO REPRODUCE: Logged from run 3',0,0,0,NULL);
/*!40000 ALTER TABLE `longdescs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `milestones`
--

DROP TABLE IF EXISTS `milestones`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `milestones` (
  `id` mediumint(9) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `value` varchar(20) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `disallownew` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `milestones_product_id_idx` (`product_id`,`value`),
  CONSTRAINT `fk_milestones_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `milestones`
--

LOCK TABLES `milestones` WRITE;
/*!40000 ALTER TABLE `milestones` DISABLE KEYS */;
INSERT INTO `milestones` VALUES (1,1,'PUBLIC M1',0,0),(2,2,'PRIVATE M1',0,0),(3,3,'PARTNER M1',0,0),(4,3,'PARTNER M2',0,0),(5,1,'PUBLIC M2',0,0),(6,2,'PRIVATE M2',0,0);
/*!40000 ALTER TABLE `milestones` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `namedqueries`
--

DROP TABLE IF EXISTS `namedqueries`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `namedqueries` (
  `id` mediumint(9) NOT NULL auto_increment,
  `userid` mediumint(9) NOT NULL,
  `name` varchar(64) NOT NULL,
  `query` mediumtext NOT NULL,
  `query_type` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `namedqueries_userid_idx` (`userid`,`name`),
  CONSTRAINT `fk_namedqueries_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `namedqueries`
--

LOCK TABLES `namedqueries` WRITE;
/*!40000 ALTER TABLE `namedqueries` DISABLE KEYS */;
/*!40000 ALTER TABLE `namedqueries` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `namedqueries_link_in_footer`
--

DROP TABLE IF EXISTS `namedqueries_link_in_footer`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `namedqueries_link_in_footer` (
  `namedquery_id` mediumint(9) NOT NULL,
  `user_id` mediumint(9) NOT NULL,
  UNIQUE KEY `namedqueries_link_in_footer_id_idx` (`namedquery_id`,`user_id`),
  KEY `namedqueries_link_in_footer_userid_idx` (`user_id`),
  CONSTRAINT `fk_namedqueries_link_in_footer_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_namedqueries_link_in_footer_namedquery_id_namedqueries_id` FOREIGN KEY (`namedquery_id`) REFERENCES `namedqueries` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `namedqueries_link_in_footer`
--

LOCK TABLES `namedqueries_link_in_footer` WRITE;
/*!40000 ALTER TABLE `namedqueries_link_in_footer` DISABLE KEYS */;
/*!40000 ALTER TABLE `namedqueries_link_in_footer` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `namedquery_group_map`
--

DROP TABLE IF EXISTS `namedquery_group_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `namedquery_group_map` (
  `namedquery_id` mediumint(9) NOT NULL,
  `group_id` mediumint(9) NOT NULL,
  UNIQUE KEY `namedquery_group_map_namedquery_id_idx` (`namedquery_id`),
  KEY `namedquery_group_map_group_id_idx` (`group_id`),
  CONSTRAINT `fk_namedquery_group_map_group_id_groups_id` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_namedquery_group_map_namedquery_id_namedqueries_id` FOREIGN KEY (`namedquery_id`) REFERENCES `namedqueries` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `namedquery_group_map`
--

LOCK TABLES `namedquery_group_map` WRITE;
/*!40000 ALTER TABLE `namedquery_group_map` DISABLE KEYS */;
/*!40000 ALTER TABLE `namedquery_group_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `op_sys`
--

DROP TABLE IF EXISTS `op_sys`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `op_sys` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `op_sys_value_idx` (`value`),
  KEY `op_sys_sortkey_idx` (`sortkey`,`value`),
  KEY `op_sys_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `op_sys`
--

LOCK TABLES `op_sys` WRITE;
/*!40000 ALTER TABLE `op_sys` DISABLE KEYS */;
INSERT INTO `op_sys` VALUES (1,'All',100,1,NULL),(2,'Windows',200,1,NULL),(3,'Mac OS',300,1,NULL),(4,'Linux',400,1,NULL),(5,'Other',500,1,NULL);
/*!40000 ALTER TABLE `op_sys` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `priority`
--

DROP TABLE IF EXISTS `priority`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `priority` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `priority_value_idx` (`value`),
  KEY `priority_sortkey_idx` (`sortkey`,`value`),
  KEY `priority_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `priority`
--

LOCK TABLES `priority` WRITE;
/*!40000 ALTER TABLE `priority` DISABLE KEYS */;
INSERT INTO `priority` VALUES (1,'P1',100,1,NULL),(2,'P2',200,1,NULL),(3,'P3',300,1,NULL),(4,'P4',400,1,NULL),(5,'P5',500,1,NULL);
/*!40000 ALTER TABLE `priority` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `products`
--

DROP TABLE IF EXISTS `products`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `products` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `classification_id` smallint(6) NOT NULL default '1',
  `description` mediumtext,
  `votesperuser` smallint(6) NOT NULL default '0',
  `maxvotesperbug` smallint(6) NOT NULL default '10000',
  `votestoconfirm` smallint(6) NOT NULL default '0',
  `defaultmilestone` varchar(20) NOT NULL default '---',
  `isactive` tinyint(4) NOT NULL default '1',
  `allows_unconfirmed` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `products_name_idx` (`name`),
  KEY `fk_products_classification_id_classifications_id` (`classification_id`),
  CONSTRAINT `fk_products_classification_id_classifications_id` FOREIGN KEY (`classification_id`) REFERENCES `classifications` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `products`
--

LOCK TABLES `products` WRITE;
/*!40000 ALTER TABLE `products` DISABLE KEYS */;
INSERT INTO `products` VALUES (1,'PUBLIC PRODUCT ONE',1,'PUBLIC PRODUCT',0,10000,0,'PUBLIC M1',1,0),(2,'PRIVATE PRODUCT ONE',3,'PRIVATE PRODUCT',0,10000,0,'PRIVATE M1',1,0),(3,'PARTNER PRODUCT ONE',2,'PARTNER PRODUCT',0,10000,0,'PARTNER M1',1,0);
/*!40000 ALTER TABLE `products` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `profile_setting`
--

DROP TABLE IF EXISTS `profile_setting`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `profile_setting` (
  `user_id` mediumint(9) NOT NULL,
  `setting_name` varchar(32) NOT NULL,
  `setting_value` varchar(32) NOT NULL,
  UNIQUE KEY `profile_setting_value_unique_idx` (`user_id`,`setting_name`),
  KEY `fk_profile_setting_setting_name_setting_name` (`setting_name`),
  CONSTRAINT `fk_profile_setting_setting_name_setting_name` FOREIGN KEY (`setting_name`) REFERENCES `setting` (`name`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_profile_setting_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `profile_setting`
--

LOCK TABLES `profile_setting` WRITE;
/*!40000 ALTER TABLE `profile_setting` DISABLE KEYS */;
/*!40000 ALTER TABLE `profile_setting` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `profiles`
--

DROP TABLE IF EXISTS `profiles`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `profiles` (
  `userid` mediumint(9) NOT NULL auto_increment,
  `login_name` varchar(255) NOT NULL,
  `cryptpassword` varchar(128) default NULL,
  `realname` varchar(255) NOT NULL default '',
  `disabledtext` mediumtext NOT NULL,
  `disable_mail` tinyint(4) NOT NULL default '0',
  `mybugslink` tinyint(4) NOT NULL default '1',
  `extern_id` varchar(64) default NULL,
  PRIMARY KEY  (`userid`),
  UNIQUE KEY `profiles_login_name_idx` (`login_name`),
  UNIQUE KEY `profiles_extern_id_idx` (`extern_id`)
) ENGINE=InnoDB AUTO_INCREMENT=9 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `profiles`
--

LOCK TABLES `profiles` WRITE;
/*!40000 ALTER TABLE `profiles` DISABLE KEYS */;
INSERT INTO `profiles` VALUES (1,'admin@testopia.com','FegzLO4zJ0VRb3tOzFRbzwQ3OUNWZ5xNRD1MxDMtfeYFhqhEOHE{SHA-256}','Administrator','',0,1,NULL),(2,'partner@testopia.com','tXDoNaxzga5/g','partner','',0,1,NULL),(3,'tester@testopia.com','RxCZ93TxwnHFw','tester','',0,1,NULL),(4,'public@testopia.com','CDLQmFTbMGUI.','public','',0,1,NULL),(5,'public_qa@testopia.com','j.kGVdj9j1Nes','public_qa','',0,1,NULL),(6,'partner_qa@testopia.com','cpDcIQV.8fcEI','partner_qa','',0,1,NULL),(7,'private@testopia.com','rchARuyeCjm8o','private','',0,1,NULL),(8,'private_qa@testopia.com','nU.nkYvKgijzA','private_qa','',0,1,NULL);
/*!40000 ALTER TABLE `profiles` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `profiles_activity`
--

DROP TABLE IF EXISTS `profiles_activity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `profiles_activity` (
  `userid` mediumint(9) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `profiles_when` datetime NOT NULL,
  `fieldid` mediumint(9) NOT NULL,
  `oldvalue` tinytext,
  `newvalue` tinytext,
  KEY `profiles_activity_userid_idx` (`userid`),
  KEY `profiles_activity_profiles_when_idx` (`profiles_when`),
  KEY `profiles_activity_fieldid_idx` (`fieldid`),
  KEY `fk_profiles_activity_who_profiles_userid` (`who`),
  CONSTRAINT `fk_profiles_activity_fieldid_fielddefs_id` FOREIGN KEY (`fieldid`) REFERENCES `fielddefs` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_profiles_activity_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_profiles_activity_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `profiles_activity`
--

LOCK TABLES `profiles_activity` WRITE;
/*!40000 ALTER TABLE `profiles_activity` DISABLE KEYS */;
INSERT INTO `profiles_activity` VALUES (1,1,'2008-03-17 15:53:06',31,NULL,'2008-03-17 15:53:06'),(2,1,'2008-03-17 15:56:26',31,NULL,'2008-03-17 15:56:26'),(3,1,'2008-03-17 15:56:59',31,NULL,'2008-03-17 15:56:59'),(3,1,'2008-03-17 15:57:09',39,'','Testers'),(4,1,'2008-03-17 15:57:45',31,NULL,'2008-03-17 15:57:45'),(5,1,'2008-03-17 16:05:26',31,NULL,'2008-03-17 16:05:26'),(6,1,'2008-03-17 16:17:42',31,NULL,'2008-03-17 16:17:42'),(7,1,'2008-03-17 16:18:30',31,NULL,'2008-03-17 16:18:30'),(7,1,'2008-03-17 16:18:40',39,'','Testers'),(8,1,'2008-03-17 16:19:08',31,NULL,'2008-03-17 16:19:08'),(1,1,'2008-03-17 16:49:15',39,'','Testers'),(2,1,'2008-05-02 14:43:11',39,'','partners'),(6,1,'2008-05-02 14:43:40',39,'','partners'),(7,1,'2008-05-02 14:44:29',39,'Testers','private'),(8,1,'2008-05-02 14:44:49',39,'','private');
/*!40000 ALTER TABLE `profiles_activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `quips`
--

DROP TABLE IF EXISTS `quips`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `quips` (
  `quipid` mediumint(9) NOT NULL auto_increment,
  `userid` mediumint(9) default NULL,
  `quip` mediumtext NOT NULL,
  `approved` tinyint(4) NOT NULL default '1',
  PRIMARY KEY  (`quipid`),
  KEY `fk_quips_userid_profiles_userid` (`userid`),
  CONSTRAINT `fk_quips_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `quips`
--

LOCK TABLES `quips` WRITE;
/*!40000 ALTER TABLE `quips` DISABLE KEYS */;
/*!40000 ALTER TABLE `quips` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rep_platform`
--

DROP TABLE IF EXISTS `rep_platform`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `rep_platform` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `rep_platform_value_idx` (`value`),
  KEY `rep_platform_sortkey_idx` (`sortkey`,`value`),
  KEY `rep_platform_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `rep_platform`
--

LOCK TABLES `rep_platform` WRITE;
/*!40000 ALTER TABLE `rep_platform` DISABLE KEYS */;
INSERT INTO `rep_platform` VALUES (1,'All',100,1,NULL),(2,'PC',200,1,NULL),(3,'Macintosh',300,1,NULL),(4,'Other',400,1,NULL);
/*!40000 ALTER TABLE `rep_platform` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `resolution`
--

DROP TABLE IF EXISTS `resolution`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `resolution` (
  `id` smallint(6) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `sortkey` smallint(6) NOT NULL default '0',
  `isactive` tinyint(4) NOT NULL default '1',
  `visibility_value_id` smallint(6) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `resolution_value_idx` (`value`),
  KEY `resolution_sortkey_idx` (`sortkey`,`value`),
  KEY `resolution_visibility_value_id_idx` (`visibility_value_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `resolution`
--

LOCK TABLES `resolution` WRITE;
/*!40000 ALTER TABLE `resolution` DISABLE KEYS */;
INSERT INTO `resolution` VALUES (1,'',100,1,NULL),(2,'FIXED',200,1,NULL),(3,'INVALID',300,1,NULL),(4,'WONTFIX',400,1,NULL),(5,'DUPLICATE',500,1,NULL),(6,'WORKSFORME',600,1,NULL),(7,'MOVED',700,1,NULL);
/*!40000 ALTER TABLE `resolution` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `series`
--

DROP TABLE IF EXISTS `series`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `series` (
  `series_id` mediumint(9) NOT NULL auto_increment,
  `creator` mediumint(9) default NULL,
  `category` smallint(6) NOT NULL,
  `subcategory` smallint(6) NOT NULL,
  `name` varchar(64) NOT NULL,
  `frequency` smallint(6) NOT NULL,
  `query` mediumtext NOT NULL,
  `is_public` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`series_id`),
  UNIQUE KEY `series_creator_idx` (`creator`,`category`,`subcategory`,`name`),
  KEY `fk_series_category_series_categories_id` (`category`),
  KEY `fk_series_subcategory_series_categories_id` (`subcategory`),
  CONSTRAINT `fk_series_subcategory_series_categories_id` FOREIGN KEY (`subcategory`) REFERENCES `series_categories` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_series_category_series_categories_id` FOREIGN KEY (`category`) REFERENCES `series_categories` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_series_creator_profiles_userid` FOREIGN KEY (`creator`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `series`
--

LOCK TABLES `series` WRITE;
/*!40000 ALTER TABLE `series` DISABLE KEYS */;
INSERT INTO `series` VALUES (1,1,1,2,'All Open',1,'field0-0-0=resolution&type0-0-0=notregexp&value0-0-0=.&product=PUBLIC%20PRODUCT%20ONE&component=PUBLIC%20ONE%20COMP%202',1),(2,1,1,2,'All Closed',1,'field0-0-0=resolution&type0-0-0=regexp&value0-0-0=.&product=PUBLIC%20PRODUCT%20ONE&component=PUBLIC%20ONE%20COMP%202',1),(3,1,3,4,'All Open',1,'field0-0-0=resolution&type0-0-0=notregexp&value0-0-0=.&product=PRIVATE%20ONE&component=PRIVATE%20ONE%20COMP%201',1),(4,1,3,4,'All Closed',1,'field0-0-0=resolution&type0-0-0=regexp&value0-0-0=.&product=PRIVATE%20ONE&component=PRIVATE%20ONE%20COMP%201',1),(5,1,5,6,'All Open',1,'field0-0-0=resolution&type0-0-0=notregexp&value0-0-0=.&product=PARTNER%20ONE&component=PARTNER%20ONE%20COMP%201',1),(6,1,5,6,'All Closed',1,'field0-0-0=resolution&type0-0-0=regexp&value0-0-0=.&product=PARTNER%20ONE&component=PARTNER%20ONE%20COMP%201',1);
/*!40000 ALTER TABLE `series` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `series_categories`
--

DROP TABLE IF EXISTS `series_categories`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `series_categories` (
  `id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `series_categories_name_idx` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `series_categories`
--

LOCK TABLES `series_categories` WRITE;
/*!40000 ALTER TABLE `series_categories` DISABLE KEYS */;
INSERT INTO `series_categories` VALUES (5,'PARTNER ONE'),(6,'PARTNER ONE COMP 1'),(3,'PRIVATE ONE'),(4,'PRIVATE ONE COMP 1'),(2,'PUBLIC ONE COMP 2'),(1,'PUBLIC PRODUCT ONE');
/*!40000 ALTER TABLE `series_categories` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `series_data`
--

DROP TABLE IF EXISTS `series_data`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `series_data` (
  `series_id` mediumint(9) NOT NULL,
  `series_date` datetime NOT NULL,
  `series_value` mediumint(9) NOT NULL,
  UNIQUE KEY `series_data_series_id_idx` (`series_id`,`series_date`),
  CONSTRAINT `fk_series_data_series_id_series_series_id` FOREIGN KEY (`series_id`) REFERENCES `series` (`series_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `series_data`
--

LOCK TABLES `series_data` WRITE;
/*!40000 ALTER TABLE `series_data` DISABLE KEYS */;
/*!40000 ALTER TABLE `series_data` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `setting`
--

DROP TABLE IF EXISTS `setting`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `setting` (
  `name` varchar(32) NOT NULL,
  `default_value` varchar(32) NOT NULL,
  `is_enabled` tinyint(4) NOT NULL default '1',
  `subclass` varchar(32) default NULL,
  PRIMARY KEY  (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `setting`
--

LOCK TABLES `setting` WRITE;
/*!40000 ALTER TABLE `setting` DISABLE KEYS */;
INSERT INTO `setting` VALUES ('comment_sort_order','oldest_to_newest',1,NULL),('csv_colsepchar',',',1,NULL),('display_quips','on',1,NULL),('lang','en',1,'Lang'),('per_bug_queries','off',1,NULL),('post_bug_submit_action','next_bug',1,NULL),('quote_replies','quoted_reply',1,NULL),('skin','Dusk',1,'Skin'),('state_addselfcc','cc_unless_role',1,NULL),('timezone','local',1,'Timezone'),('view_testopia','on',1,NULL),('zoom_textareas','on',1,NULL);
/*!40000 ALTER TABLE `setting` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `setting_value`
--

DROP TABLE IF EXISTS `setting_value`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `setting_value` (
  `name` varchar(32) NOT NULL,
  `value` varchar(32) NOT NULL,
  `sortindex` smallint(6) NOT NULL,
  UNIQUE KEY `setting_value_nv_unique_idx` (`name`,`value`),
  UNIQUE KEY `setting_value_ns_unique_idx` (`name`,`sortindex`),
  CONSTRAINT `fk_setting_value_name_setting_name` FOREIGN KEY (`name`) REFERENCES `setting` (`name`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `setting_value`
--

LOCK TABLES `setting_value` WRITE;
/*!40000 ALTER TABLE `setting_value` DISABLE KEYS */;
INSERT INTO `setting_value` VALUES ('comment_sort_order','oldest_to_newest',5),('comment_sort_order','newest_to_oldest',10),('comment_sort_order','newest_to_oldest_desc_first',15),('csv_colsepchar',',',5),('csv_colsepchar',';',10),('display_quips','on',5),('display_quips','off',10),('per_bug_queries','on',5),('per_bug_queries','off',10),('post_bug_submit_action','next_bug',5),('post_bug_submit_action','same_bug',10),('post_bug_submit_action','nothing',15),('quote_replies','quoted_reply',5),('quote_replies','simple_reply',10),('quote_replies','off',15),('state_addselfcc','always',5),('state_addselfcc','never',10),('state_addselfcc','cc_unless_role',15),('view_testopia','on',5),('view_testopia','off',10),('zoom_textareas','on',5),('zoom_textareas','off',10);
/*!40000 ALTER TABLE `setting_value` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `status_workflow`
--

DROP TABLE IF EXISTS `status_workflow`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `status_workflow` (
  `old_status` smallint(6) default NULL,
  `new_status` smallint(6) NOT NULL,
  `require_comment` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `status_workflow_idx` (`old_status`,`new_status`),
  KEY `fk_status_workflow_new_status_bug_status_id` (`new_status`),
  CONSTRAINT `fk_status_workflow_new_status_bug_status_id` FOREIGN KEY (`new_status`) REFERENCES `bug_status` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_status_workflow_old_status_bug_status_id` FOREIGN KEY (`old_status`) REFERENCES `bug_status` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `status_workflow`
--

LOCK TABLES `status_workflow` WRITE;
/*!40000 ALTER TABLE `status_workflow` DISABLE KEYS */;
INSERT INTO `status_workflow` VALUES (NULL,1,0),(NULL,2,0),(NULL,3,0),(1,2,0),(1,3,0),(1,5,0),(2,3,0),(2,5,0),(3,2,0),(3,5,0),(4,2,0),(4,3,0),(4,5,0),(5,1,0),(5,4,0),(5,6,0),(5,7,0),(6,1,0),(6,4,0),(6,7,0),(7,1,0),(7,4,0),(6,5,0),(7,5,0);
/*!40000 ALTER TABLE `status_workflow` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_attachment_data`
--

DROP TABLE IF EXISTS `test_attachment_data`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_attachment_data` (
  `attachment_id` int(11) NOT NULL,
  `contents` longblob,
  KEY `test_attachment_data_primary_idx` (`attachment_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_attachment_data`
--

LOCK TABLES `test_attachment_data` WRITE;
/*!40000 ALTER TABLE `test_attachment_data` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_attachment_data` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_attachments`
--

DROP TABLE IF EXISTS `test_attachments`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_attachments` (
  `attachment_id` int(11) NOT NULL auto_increment,
  `submitter_id` mediumint(9) NOT NULL,
  `description` mediumtext,
  `filename` mediumtext,
  `creation_ts` datetime NOT NULL,
  `mime_type` varchar(100) NOT NULL,
  PRIMARY KEY  (`attachment_id`),
  KEY `test_attachments_submitter_idx` (`submitter_id`),
  CONSTRAINT `fk_test_attachments_submitter_id_profiles_userid` FOREIGN KEY (`submitter_id`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=13 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_attachments`
--

LOCK TABLES `test_attachments` WRITE;
/*!40000 ALTER TABLE `test_attachments` DISABLE KEYS */;
INSERT INTO `test_attachments` VALUES (1,1,'PUBLIC PLAN ATTACHMENT ASCII','LOREM.TXT','2008-05-01 16:54:37','text/plain'),(2,1,'PUBLIC PLAN ATTACHMENT BINARY','testopia_city_512.png','2008-05-01 16:56:44','image/png'),(3,1,'Attachment','LOREM.TXT','2008-05-02 14:20:00','text/plain'),(4,1,'Attachment','LOREM.TXT','2008-05-02 14:20:08','text/plain'),(5,1,'Attachment','LOREM.TXT','2008-05-02 14:20:12','text/plain'),(6,1,'Attachment','LOREM.TXT','2008-05-02 14:20:20','text/plain'),(7,1,'Attachment','LOREM.TXT','2008-05-02 14:20:23','text/plain'),(8,1,'Attachment','LOREM.TXT','2008-05-02 14:20:26','text/plain'),(9,1,'Attachment','LOREM.TXT','2008-05-02 14:20:36','text/plain'),(10,1,'Attachment','LOREM.TXT','2008-05-02 14:21:02','text/plain'),(11,1,'Attachment','LOREM.TXT','2008-05-02 14:21:06','text/plain'),(12,1,'Attachment','LOREM.TXT','2008-05-02 14:21:14','text/plain');
/*!40000 ALTER TABLE `test_attachments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_builds`
--

DROP TABLE IF EXISTS `test_builds`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_builds` (
  `build_id` int(11) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `milestone` varchar(20) default NULL,
  `name` varchar(255) default NULL,
  `description` text,
  `isactive` tinyint(4) NOT NULL default '1',
  PRIMARY KEY  (`build_id`),
  UNIQUE KEY `build_prod_idx` (`build_id`,`product_id`),
  UNIQUE KEY `build_product_id_name_idx` (`product_id`,`name`),
  KEY `build_name_idx` (`name`),
  KEY `build_milestone_idx` (`milestone`),
  CONSTRAINT `fk_test_builds_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_builds`
--

LOCK TABLES `test_builds` WRITE;
/*!40000 ALTER TABLE `test_builds` DISABLE KEYS */;
INSERT INTO `test_builds` VALUES (1,2,'PRIVATE M1','PRIVATE ACTIVE BUILD 1','Private Visible Build',1),(2,2,'PRIVATE M1','PRIVATE INACTIVE BUILD','Private Visible Build',0),(3,3,'PARTNER M1','PARTNER ACTIVE BUILD 1','Partner Visible Build',1),(4,1,'PUBLIC M1','PUBLIC ACTIVE BUILD 1','Publicly Visible Build',1),(5,1,'PUBLIC M1','PUBLIC INACTIVE BUILD','Publicly Visible Build',0),(6,3,'PARTNER M1','PARTNER INACTIVE BUILD','Partner Visible Build',1);
/*!40000 ALTER TABLE `test_builds` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_activity`
--

DROP TABLE IF EXISTS `test_case_activity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_activity` (
  `case_id` int(11) NOT NULL,
  `fieldid` smallint(6) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `changed` datetime NOT NULL,
  `oldvalue` mediumtext,
  `newvalue` mediumtext,
  KEY `case_activity_case_id_idx` (`case_id`),
  KEY `case_activity_who_idx` (`who`),
  KEY `case_activity_when_idx` (`changed`),
  KEY `case_activity_field_idx` (`fieldid`),
  CONSTRAINT `fk_test_case_activity_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_test_case_activity_fieldid_test_fielddefs_fieldid` FOREIGN KEY (`fieldid`) REFERENCES `test_fielddefs` (`fieldid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_activity`
--

LOCK TABLES `test_case_activity` WRITE;
/*!40000 ALTER TABLE `test_case_activity` DISABLE KEYS */;
INSERT INTO `test_case_activity` VALUES (2,6,1,'2008-05-01 17:07:15','1','2'),(2,7,1,'2008-05-01 17:07:31','PUBLIC TEST CASE 1 - PROPOSED','PUBLIC TEST CASE 1 - DISABLED'),(2,4,1,'2008-05-01 17:07:40','1','3'),(3,7,1,'2008-05-01 17:18:05','PUBLIC TEST CASE 1 - PROPOSED','PUBLIC TEST CASE 1 - CONFIRMED'),(4,7,1,'2008-05-01 17:18:10','PUBLIC TEST CASE - CONFIRMED','PUBLIC TEST CASE 2 - CONFIRMED'),(3,4,1,'2008-05-01 17:18:23','1','2');
/*!40000 ALTER TABLE `test_case_activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_attachments`
--

DROP TABLE IF EXISTS `test_case_attachments`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_attachments` (
  `attachment_id` int(11) NOT NULL,
  `case_id` int(11) NOT NULL,
  `case_run_id` int(11) default NULL,
  KEY `test_case_attachments_primary_idx` (`attachment_id`),
  KEY `attachment_case_id_idx` (`case_id`),
  KEY `attachment_caserun_id_idx` (`case_run_id`),
  CONSTRAINT `fk_test_case_attachments_case_run_id_test_case_runs_case_run_id` FOREIGN KEY (`case_run_id`) REFERENCES `test_case_runs` (`case_run_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_00a12e9da829a588e7c5d82e70525432` FOREIGN KEY (`attachment_id`) REFERENCES `test_attachments` (`attachment_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_case_attachments_case_id_test_cases_case_id` FOREIGN KEY (`case_id`) REFERENCES `test_cases` (`case_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_attachments`
--

LOCK TABLES `test_case_attachments` WRITE;
/*!40000 ALTER TABLE `test_case_attachments` DISABLE KEYS */;
INSERT INTO `test_case_attachments` VALUES (3,6,NULL),(4,7,NULL),(5,8,NULL),(6,9,NULL),(7,10,NULL),(8,11,NULL),(9,12,NULL),(10,13,NULL),(11,14,NULL),(12,15,NULL);
/*!40000 ALTER TABLE `test_case_attachments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_bugs`
--

DROP TABLE IF EXISTS `test_case_bugs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_bugs` (
  `bug_id` mediumint(9) NOT NULL,
  `case_run_id` int(11) default NULL,
  `case_id` int(11) NOT NULL,
  KEY `case_bugs_bug_id_idx` (`bug_id`),
  KEY `case_bugs_case_id_idx` (`case_id`),
  KEY `case_bugs_case_run_id_idx` (`case_run_id`),
  CONSTRAINT `fk_test_case_bugs_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_bugs`
--

LOCK TABLES `test_case_bugs` WRITE;
/*!40000 ALTER TABLE `test_case_bugs` DISABLE KEYS */;
INSERT INTO `test_case_bugs` VALUES (2,2,5);
/*!40000 ALTER TABLE `test_case_bugs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_categories`
--

DROP TABLE IF EXISTS `test_case_categories`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_categories` (
  `category_id` smallint(6) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `name` varchar(240) NOT NULL,
  `description` mediumtext,
  PRIMARY KEY  (`category_id`),
  UNIQUE KEY `category_product_id_name_idx` (`product_id`,`name`),
  UNIQUE KEY `category_product_idx` (`category_id`,`product_id`),
  KEY `category_name_idx_v2` (`name`),
  CONSTRAINT `fk_test_case_categories_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_categories`
--

LOCK TABLES `test_case_categories` WRITE;
/*!40000 ALTER TABLE `test_case_categories` DISABLE KEYS */;
INSERT INTO `test_case_categories` VALUES (1,1,'PUBLIC CATEGORY 1','PUBLIC CATEGORY'),(2,2,'PRIVATE CATEGORY 1','PRIVATE CATEGORY'),(3,3,'PARTNER CATEGORY 1','PARTNER CATEGORY'),(4,3,'PARTNER CATEGORY 2','PARTNER CATEGORY'),(5,2,'PRIVATE CATEGORY 2','PRIVATE CATEOGRY'),(6,1,'PUBLIC CATEGORY 2','PUBLIC CATEGORY');
/*!40000 ALTER TABLE `test_case_categories` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_components`
--

DROP TABLE IF EXISTS `test_case_components`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_components` (
  `case_id` int(11) NOT NULL,
  `component_id` smallint(6) NOT NULL,
  UNIQUE KEY `components_case_id_idx` (`case_id`,`component_id`),
  KEY `components_component_id_idx` (`component_id`),
  CONSTRAINT `fk_test_case_components_component_id_components_id` FOREIGN KEY (`component_id`) REFERENCES `components` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_components`
--

LOCK TABLES `test_case_components` WRITE;
/*!40000 ALTER TABLE `test_case_components` DISABLE KEYS */;
INSERT INTO `test_case_components` VALUES (5,1),(29,1),(6,3),(7,3),(8,3),(9,3),(10,3),(11,3),(12,3),(13,3),(14,3),(15,3),(16,4),(17,4),(18,4),(19,4),(20,4),(21,4),(22,4),(23,4),(24,4);
/*!40000 ALTER TABLE `test_case_components` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_dependencies`
--

DROP TABLE IF EXISTS `test_case_dependencies`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_dependencies` (
  `dependson` int(11) NOT NULL,
  `blocked` int(11) NOT NULL,
  UNIQUE KEY `case_dependencies_primary_idx` (`dependson`,`blocked`),
  KEY `case_dependencies_blocked_idx` (`blocked`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_dependencies`
--

LOCK TABLES `test_case_dependencies` WRITE;
/*!40000 ALTER TABLE `test_case_dependencies` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_case_dependencies` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_plans`
--

DROP TABLE IF EXISTS `test_case_plans`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_plans` (
  `plan_id` int(11) NOT NULL,
  `case_id` int(11) NOT NULL,
  UNIQUE KEY `test_case_plans_primary_idx` (`plan_id`,`case_id`),
  KEY `test_case_plans_case_idx` (`case_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_plans`
--

LOCK TABLES `test_case_plans` WRITE;
/*!40000 ALTER TABLE `test_case_plans` DISABLE KEYS */;
INSERT INTO `test_case_plans` VALUES (1,1),(1,2),(1,3),(1,4),(1,5),(2,6),(2,7),(2,8),(2,9),(2,10),(2,11),(2,12),(2,13),(2,14),(2,15),(3,16),(3,17),(3,18),(3,19),(3,20),(3,21),(3,22),(3,23),(3,24),(5,25),(5,26),(5,27),(5,28),(5,29);
/*!40000 ALTER TABLE `test_case_plans` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_run_status`
--

DROP TABLE IF EXISTS `test_case_run_status`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_run_status` (
  `case_run_status_id` smallint(6) NOT NULL auto_increment,
  `name` varchar(20) default NULL,
  `sortkey` int(11) default NULL,
  `description` text,
  PRIMARY KEY  (`case_run_status_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_run_status`
--

LOCK TABLES `test_case_run_status` WRITE;
/*!40000 ALTER TABLE `test_case_run_status` DISABLE KEYS */;
INSERT INTO `test_case_run_status` VALUES (1,'IDLE',1,NULL),(2,'PASSED',4,NULL),(3,'FAILED',5,NULL),(4,'RUNNING',2,NULL),(5,'PAUSED',3,NULL),(6,'BLOCKED',6,NULL),(7,'ERROR',7,NULL);
/*!40000 ALTER TABLE `test_case_run_status` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_runs`
--

DROP TABLE IF EXISTS `test_case_runs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_runs` (
  `case_run_id` int(11) NOT NULL auto_increment,
  `run_id` int(11) NOT NULL,
  `case_id` int(11) NOT NULL,
  `assignee` mediumint(9) default NULL,
  `testedby` mediumint(9) default NULL,
  `case_run_status_id` smallint(6) NOT NULL,
  `case_text_version` mediumint(9) NOT NULL,
  `build_id` int(11) NOT NULL,
  `running_date` datetime default NULL,
  `close_date` datetime default NULL,
  `notes` text,
  `iscurrent` tinyint(4) NOT NULL default '0',
  `sortkey` int(11) default NULL,
  `environment_id` int(11) NOT NULL,
  `priority_id` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`case_run_id`),
  UNIQUE KEY `case_run_build_env_idx` (`run_id`,`case_id`,`build_id`,`environment_id`),
  KEY `case_run_case_id_idx` (`case_id`),
  KEY `case_run_assignee_idx` (`assignee`),
  KEY `case_run_testedby_idx` (`testedby`),
  KEY `case_run_close_date_idx` (`close_date`),
  KEY `case_run_status_idx` (`case_run_status_id`),
  KEY `case_run_text_ver_idx` (`case_text_version`),
  KEY `case_run_build_idx_v2` (`build_id`),
  KEY `case_run_env_idx_v2` (`environment_id`),
  KEY `case_run_priority_idx` (`priority_id`),
  CONSTRAINT `fk_test_case_runs_priority_id_priority_id` FOREIGN KEY (`priority_id`) REFERENCES `priority` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_db6dbaa3c97a63af1ce5b40c788046a5` FOREIGN KEY (`case_run_status_id`) REFERENCES `test_case_run_status` (`case_run_status_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=19 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_runs`
--

LOCK TABLES `test_case_runs` WRITE;
/*!40000 ALTER TABLE `test_case_runs` DISABLE KEYS */;
INSERT INTO `test_case_runs` VALUES (1,1,4,0,1,3,1,4,NULL,'2010-07-21 10:42:19','07/21/10 10:42:19: Status changed from IDLE to FAILED by admin@testopia.com for build \'PUBLIC ACTIVE BUILD 1\' and environment \'PUBLIC ACTIVE ENVIRONMENT',1,NULL,3,2),(2,1,5,5,1,6,1,4,NULL,'2010-07-21 10:42:26','07/21/10 10:42:26: Status changed from IDLE to BLOCKED by admin@testopia.com for build \'PUBLIC ACTIVE BUILD 1\' and environment \'PUBLIC ACTIVE ENVIRONMENT',1,NULL,3,3),(3,1,3,5,1,2,1,4,NULL,'2010-07-21 10:42:10','07/21/10 10:42:10: Status changed from IDLE to PASSED by admin@testopia.com for build \'PUBLIC ACTIVE BUILD 1\' and environment \'PUBLIC ACTIVE ENVIRONMENT',1,NULL,3,1),(4,2,6,8,NULL,1,1,1,NULL,NULL,NULL,1,NULL,1,3),(5,2,7,8,NULL,1,1,1,NULL,NULL,NULL,1,NULL,1,3),(6,2,8,8,NULL,1,1,1,NULL,NULL,NULL,1,NULL,1,3),(7,2,15,8,NULL,1,1,1,NULL,NULL,NULL,1,NULL,1,1),(8,3,6,8,NULL,1,1,2,NULL,NULL,NULL,1,NULL,2,3),(9,3,7,8,NULL,1,1,2,NULL,NULL,NULL,1,NULL,2,3),(10,3,8,8,NULL,1,1,2,NULL,NULL,NULL,1,NULL,2,3),(11,3,15,8,NULL,1,1,2,NULL,NULL,NULL,1,NULL,2,1),(12,4,17,6,1,2,1,3,NULL,'2010-07-20 15:50:09','07/20/10 15:50:09: Status changed from IDLE to PASSED by admin@testopia.com for build \'PARTNER ACTIVE BUILD 1\' and environment \'PARTNER ACTIVE ENVIRONMENT',0,NULL,4,3),(13,4,18,6,NULL,1,1,3,NULL,NULL,NULL,1,NULL,4,3),(14,4,21,6,NULL,1,1,3,NULL,NULL,NULL,1,NULL,4,5),(15,4,17,6,NULL,1,1,6,NULL,NULL,'07/20/10 15:50:36: Build Changed by admin@testopia.com. Old build: \'PARTNER ACTIVE BUILD 1\' New build: \'PARTNER INACTIVE BUILD\'. Resetting to IDLE.',1,NULL,4,3),(16,5,27,5,NULL,1,1,4,NULL,NULL,NULL,1,1,3,1),(17,5,28,NULL,NULL,1,1,4,NULL,NULL,NULL,1,2,3,2),(18,5,29,5,NULL,1,1,4,NULL,NULL,NULL,1,3,3,3);
/*!40000 ALTER TABLE `test_case_runs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_status`
--

DROP TABLE IF EXISTS `test_case_status`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_status` (
  `case_status_id` smallint(6) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL,
  `description` text,
  PRIMARY KEY  (`case_status_id`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_status`
--

LOCK TABLES `test_case_status` WRITE;
/*!40000 ALTER TABLE `test_case_status` DISABLE KEYS */;
INSERT INTO `test_case_status` VALUES (1,'PROPOSED',NULL),(2,'CONFIRMED',NULL),(3,'DISABLED',NULL);
/*!40000 ALTER TABLE `test_case_status` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_tags`
--

DROP TABLE IF EXISTS `test_case_tags`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_tags` (
  `tag_id` int(11) NOT NULL,
  `case_id` int(11) NOT NULL,
  `userid` mediumint(9) NOT NULL,
  UNIQUE KEY `case_tags_primary_idx` (`tag_id`,`case_id`,`userid`),
  UNIQUE KEY `case_tags_secondary_idx` (`tag_id`,`case_id`),
  KEY `case_tags_case_id_idx_v3` (`case_id`),
  KEY `case_tags_userid_idx` (`userid`),
  CONSTRAINT `fk_test_case_tags_tag_id_test_tags_tag_id` FOREIGN KEY (`tag_id`) REFERENCES `test_tags` (`tag_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_tags`
--

LOCK TABLES `test_case_tags` WRITE;
/*!40000 ALTER TABLE `test_case_tags` DISABLE KEYS */;
INSERT INTO `test_case_tags` VALUES (1,5,1),(1,29,1),(2,6,1),(2,7,1),(2,8,1),(2,9,1),(2,10,1),(2,11,1),(2,12,1),(2,13,1),(2,14,1),(2,15,1),(3,16,1),(3,17,1),(3,18,1),(3,19,1),(3,20,1),(3,21,1),(3,22,1),(3,23,1),(3,24,1);
/*!40000 ALTER TABLE `test_case_tags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_case_texts`
--

DROP TABLE IF EXISTS `test_case_texts`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_case_texts` (
  `case_id` int(11) NOT NULL,
  `case_text_version` mediumint(9) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `creation_ts` datetime NOT NULL,
  `action` mediumtext,
  `effect` mediumtext,
  `setup` mediumtext,
  `breakdown` mediumtext,
  UNIQUE KEY `case_versions_idx` (`case_id`,`case_text_version`),
  KEY `case_versions_who_idx` (`who`),
  KEY `case_versions_creation_ts_idx` (`creation_ts`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_case_texts`
--

LOCK TABLES `test_case_texts` WRITE;
/*!40000 ALTER TABLE `test_case_texts` DISABLE KEYS */;
INSERT INTO `test_case_texts` VALUES (1,1,1,'2008-04-16 13:57:05','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(2,1,1,'2008-04-16 14:01:58','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(3,1,1,'2008-04-16 14:02:11','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(4,1,1,'2008-04-17 09:51:33','<ol>\r\n  <li>PUBLIC TEST CASE - CONFIRMED P2<br></li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(5,1,1,'2008-05-01 17:20:41','<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div><ol>\r\n  <li><br></li>\r\n</ol>','<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div><ol>\r\n  <li><br></li>\r\n</ol>','&nbsp;<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div>','&nbsp;<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div>'),(6,1,1,'2008-05-02 14:20:00','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(7,1,1,'2008-05-02 14:20:08','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(8,1,1,'2008-05-02 14:20:12','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(9,1,1,'2008-05-02 14:20:20','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(10,1,1,'2008-05-02 14:20:23','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(11,1,1,'2008-05-02 14:20:26','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(12,1,1,'2008-05-02 14:20:36','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(13,1,1,'2008-05-02 14:21:02','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(14,1,1,'2008-05-02 14:21:06','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(15,1,1,'2008-05-02 14:21:14','<ol>\r\n  <li>STEP ONE</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>REACTION ONE</li><li>REACTION TWO</li><li>REACTION THREE<br></li>\r\n</ol>','&nbsp;SETUP<br>','&nbsp;BREAKDOWN'),(16,1,1,'2008-05-02 14:25:41','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(17,1,1,'2008-05-02 14:25:56','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(18,1,1,'2008-05-02 14:25:59','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(19,1,1,'2008-05-02 14:26:11','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(20,1,1,'2008-05-02 14:26:22','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(21,1,1,'2008-05-02 14:26:33','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(22,1,1,'2008-05-02 14:26:41','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(23,1,1,'2008-05-02 14:26:48','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(24,1,1,'2008-05-02 14:26:55','<ol>\r\n  <li>STEP ONE FOR PARTNER</li><li>STEP TWO</li><li>STEP THREE<br></li>\r\n</ol>','<ol>\r\n  <li>RESULTS FOR PARTNER<br></li>\r\n</ol>','&nbsp;SETTING UP FOR PARTNER <br>','&nbsp;BREAKING DOWN FOR PARTNER<br>'),(25,1,1,'2010-07-21 10:14:55','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(26,1,1,'2010-07-21 10:14:55','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(27,1,1,'2010-07-21 10:14:55','<ol>\r\n  <li>PUBLIC TEST CASE 1 - PROPOSED</li><li>PUBLIC CATEGORY 1</li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(28,1,1,'2010-07-21 10:14:55','<ol>\r\n  <li>PUBLIC TEST CASE - CONFIRMED P2<br></li>\r\n</ol>','<ol>\r\n  <li></li>\r\n</ol>','',''),(29,1,1,'2010-07-21 10:14:55','<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div><ol>\r\n  <li><br></li>\r\n</ol>','<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div><ol>\r\n  <li><br></li>\r\n</ol>','&nbsp;<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div>','&nbsp;<div id=\"lipsum\">\r\n<p>\r\nLorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\r\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\r\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\r\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\r\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\r\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\r\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\r\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\r\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\r\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\r\n</p>\r\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\r\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\r\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\r\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\r\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\r\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\r\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\r\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\r\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\r\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\r\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\r\nbibendum eu, gravida nec, elementum sed, urna.\r\n</p>\r\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\r\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\r\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\r\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\r\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\r\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\r\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\r\nmattis nec, lacus. Nam tortor.\r\n</p>\r\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\r\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\r\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\r\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\r\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\r\nnon erat.\r\n</p>\r\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\r\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\r\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\r\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\r\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\r\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\r\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\r\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\r\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\r\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\r\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\r\neros.\r\n</p></div>');
/*!40000 ALTER TABLE `test_case_texts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_cases`
--

DROP TABLE IF EXISTS `test_cases`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_cases` (
  `case_id` int(11) NOT NULL auto_increment,
  `case_status_id` smallint(6) NOT NULL,
  `category_id` smallint(6) NOT NULL,
  `priority_id` smallint(6) default NULL,
  `author_id` mediumint(9) NOT NULL,
  `default_tester_id` mediumint(9) default NULL,
  `creation_date` datetime NOT NULL,
  `estimated_time` time default NULL,
  `isautomated` tinyint(4) NOT NULL default '0',
  `sortkey` int(11) default NULL,
  `script` mediumtext,
  `arguments` mediumtext,
  `summary` varchar(255) default NULL,
  `requirement` varchar(255) default NULL,
  `alias` varchar(255) default NULL,
  PRIMARY KEY  (`case_id`),
  KEY `test_case_category_idx` (`category_id`),
  KEY `test_case_author_idx` (`author_id`),
  KEY `test_case_creation_date_idx` (`creation_date`),
  KEY `test_case_sortkey_idx` (`sortkey`),
  KEY `test_case_shortname_idx` (`alias`),
  KEY `test_case_requirement_idx` (`requirement`),
  KEY `test_case_status_idx` (`case_status_id`),
  KEY `test_case_tester_idx` (`default_tester_id`),
  KEY `fk_test_cases_priority_id_priority_id` (`priority_id`),
  CONSTRAINT `fk_test_cases_author_id_profiles_userid` FOREIGN KEY (`author_id`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_test_cases_case_status_id_test_case_status_case_status_id` FOREIGN KEY (`case_status_id`) REFERENCES `test_case_status` (`case_status_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_cases_category_id_test_case_categories_category_id` FOREIGN KEY (`category_id`) REFERENCES `test_case_categories` (`category_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_cases_priority_id_priority_id` FOREIGN KEY (`priority_id`) REFERENCES `priority` (`id`) ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=30 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_cases`
--

LOCK TABLES `test_cases` WRITE;
/*!40000 ALTER TABLE `test_cases` DISABLE KEYS */;
INSERT INTO `test_cases` VALUES (1,1,1,1,1,5,'2008-04-16 13:57:05','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - PROPOSED','',NULL),(2,3,1,2,1,5,'2008-04-16 14:01:58','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - DISABLED','',NULL),(3,2,1,1,1,5,'2008-04-16 14:02:11','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - CONFIRMED','',NULL),(4,2,1,2,1,NULL,'2008-04-17 09:51:33','00:00:00',1,NULL,'PUBLIC SCRIPT','PUBLIC ARG','PUBLIC TEST CASE 2 - CONFIRMED','PUBLIC REQUIREMENT 1',NULL),(5,2,6,3,1,5,'2008-05-01 17:20:41','00:00:00',1,NULL,'script','arg1','PUBLIC TEST CASE 3 - CONFIRMED','',NULL),(6,2,2,3,1,8,'2008-05-02 14:20:00','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(7,2,2,3,1,8,'2008-05-02 14:20:08','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(8,2,2,3,1,8,'2008-05-02 14:20:12','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(9,1,2,3,1,8,'2008-05-02 14:20:20','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(10,1,2,3,1,8,'2008-05-02 14:20:23','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(11,1,2,3,1,8,'2008-05-02 14:20:26','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(12,3,2,3,1,8,'2008-05-02 14:20:36','12:00:00',0,NULL,'','','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(13,3,2,1,1,8,'2008-05-02 14:21:02','12:00:00',1,NULL,'auto script','-a -b -c','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(14,3,2,1,1,8,'2008-05-02 14:21:06','12:00:00',1,NULL,'auto script','-a -b -c','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(15,2,2,1,1,8,'2008-05-02 14:21:14','12:00:00',1,NULL,'auto script','-a -b -c','PRIVATE CASE','PUBLIC REQUIREMENT 1',NULL),(16,2,4,3,1,6,'2008-05-02 14:25:41','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(17,2,4,3,1,6,'2008-05-02 14:25:56','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(18,2,4,3,1,6,'2008-05-02 14:25:59','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(19,1,4,2,1,6,'2008-05-02 14:26:11','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(20,3,4,4,1,6,'2008-05-02 14:26:22','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(21,2,4,5,1,6,'2008-05-02 14:26:33','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(22,2,3,5,1,6,'2008-05-02 14:26:41','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(23,2,3,3,1,6,'2008-05-02 14:26:48','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(24,2,3,1,1,6,'2008-05-02 14:26:55','00:00:30',0,NULL,'','','PARTNER CASE','PARTNER REQUIREMENT',NULL),(25,1,1,1,1,5,'2010-07-21 10:14:55','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - PROPOSED','',NULL),(26,3,1,2,1,5,'2010-07-21 10:14:55','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - DISABLED','',NULL),(27,2,1,1,1,5,'2010-07-21 10:14:55','00:00:00',0,NULL,'','','PUBLIC TEST CASE 1 - CONFIRMED','',NULL),(28,2,1,2,1,0,'2010-07-21 10:14:55','00:00:00',1,NULL,'PUBLIC SCRIPT','PUBLIC ARG','PUBLIC TEST CASE 2 - CONFIRMED','PUBLIC REQUIREMENT 1',NULL),(29,2,6,3,1,5,'2010-07-21 10:14:55','00:00:00',1,NULL,'script','arg1','PUBLIC TEST CASE 3 - CONFIRMED','',NULL);
/*!40000 ALTER TABLE `test_cases` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_email_settings`
--

DROP TABLE IF EXISTS `test_email_settings`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_email_settings` (
  `userid` mediumint(9) NOT NULL,
  `eventid` tinyint(4) NOT NULL,
  `relationship_id` tinyint(4) NOT NULL,
  UNIQUE KEY `test_email_setting_user_id_idx` (`userid`,`relationship_id`,`eventid`),
  KEY `fk_test_email_settings_eventid_test_events_eventid` (`eventid`),
  KEY `fk_1e39f0c93071aaa7f9855d4cdc672d32` (`relationship_id`),
  CONSTRAINT `fk_1e39f0c93071aaa7f9855d4cdc672d32` FOREIGN KEY (`relationship_id`) REFERENCES `test_relationships` (`relationship_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_email_settings_eventid_test_events_eventid` FOREIGN KEY (`eventid`) REFERENCES `test_events` (`eventid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_email_settings_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_email_settings`
--

LOCK TABLES `test_email_settings` WRITE;
/*!40000 ALTER TABLE `test_email_settings` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_email_settings` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_environment_category`
--

DROP TABLE IF EXISTS `test_environment_category`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_environment_category` (
  `env_category_id` int(11) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `name` varchar(255) default NULL,
  PRIMARY KEY  (`env_category_id`),
  UNIQUE KEY `test_environment_category_key1` (`env_category_id`,`product_id`),
  UNIQUE KEY `test_environment_category_key2` (`product_id`,`name`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_environment_category`
--

LOCK TABLES `test_environment_category` WRITE;
/*!40000 ALTER TABLE `test_environment_category` DISABLE KEYS */;
INSERT INTO `test_environment_category` VALUES (2,0,'Hardware'),(1,0,'Operating System'),(3,3,'New category 1');
/*!40000 ALTER TABLE `test_environment_category` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_environment_element`
--

DROP TABLE IF EXISTS `test_environment_element`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_environment_element` (
  `element_id` int(11) NOT NULL auto_increment,
  `env_category_id` int(11) NOT NULL,
  `name` varchar(255) default NULL,
  `parent_id` int(11) default NULL,
  `isprivate` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`element_id`),
  UNIQUE KEY `test_environment_element_key1` (`element_id`,`env_category_id`),
  UNIQUE KEY `test_environment_element_key2` (`env_category_id`,`name`),
  CONSTRAINT `fk_3b94189e0a3284e963e61ec0650d4d58` FOREIGN KEY (`env_category_id`) REFERENCES `test_environment_category` (`env_category_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_environment_element`
--

LOCK TABLES `test_environment_element` WRITE;
/*!40000 ALTER TABLE `test_environment_element` DISABLE KEYS */;
INSERT INTO `test_environment_element` VALUES (1,1,'All',0,0),(2,1,'Linux',0,0),(3,1,'Mac OS',0,0),(4,1,'Other',0,0),(5,1,'Windows',0,0),(6,2,'All',0,0),(7,2,'Macintosh',0,0),(8,2,'Other',0,0),(9,2,'PC',0,0),(10,3,'New element 1',0,0);
/*!40000 ALTER TABLE `test_environment_element` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_environment_map`
--

DROP TABLE IF EXISTS `test_environment_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_environment_map` (
  `environment_id` int(11) NOT NULL,
  `property_id` int(11) default NULL,
  `element_id` int(11) NOT NULL,
  `value_selected` tinytext,
  UNIQUE KEY `test_environment_map_key3` (`environment_id`,`element_id`,`property_id`),
  KEY `env_map_env_element_idx` (`environment_id`,`element_id`),
  KEY `env_map_property_idx` (`environment_id`,`property_id`),
  KEY `fk_b3fe3d67a1278c9422b59eca1a8502d9` (`element_id`),
  CONSTRAINT `fk_b3fe3d67a1278c9422b59eca1a8502d9` FOREIGN KEY (`element_id`) REFERENCES `test_environment_element` (`element_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_environment_map`
--

LOCK TABLES `test_environment_map` WRITE;
/*!40000 ALTER TABLE `test_environment_map` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_environment_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_environment_property`
--

DROP TABLE IF EXISTS `test_environment_property`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_environment_property` (
  `property_id` int(11) NOT NULL auto_increment,
  `element_id` int(11) NOT NULL,
  `name` varchar(255) default NULL,
  `validexp` text,
  PRIMARY KEY  (`property_id`),
  UNIQUE KEY `test_environment_property_key1` (`property_id`,`element_id`),
  UNIQUE KEY `test_environment_property_key2` (`element_id`,`name`),
  CONSTRAINT `fk_6cbb3bc5952fb4c9ed8b6dfee61acb43` FOREIGN KEY (`element_id`) REFERENCES `test_environment_element` (`element_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_environment_property`
--

LOCK TABLES `test_environment_property` WRITE;
/*!40000 ALTER TABLE `test_environment_property` DISABLE KEYS */;
INSERT INTO `test_environment_property` VALUES (1,10,'New property 1','');
/*!40000 ALTER TABLE `test_environment_property` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_environments`
--

DROP TABLE IF EXISTS `test_environments`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_environments` (
  `environment_id` int(11) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `name` varchar(255) default NULL,
  `isactive` tinyint(4) NOT NULL default '1',
  PRIMARY KEY  (`environment_id`),
  UNIQUE KEY `test_environments_key1` (`environment_id`,`product_id`),
  UNIQUE KEY `test_environments_key2` (`product_id`,`name`),
  KEY `environment_name_idx_v2` (`name`),
  CONSTRAINT `fk_test_environments_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_environments`
--

LOCK TABLES `test_environments` WRITE;
/*!40000 ALTER TABLE `test_environments` DISABLE KEYS */;
INSERT INTO `test_environments` VALUES (1,2,'PRIVATE ACTIVE ENVIRONMENT',1),(2,2,'PRIVATE INACTIVE ENVIRONMENT',0),(3,1,'PUBLIC ACTIVE ENVIRONMENT',1),(4,3,'PARTNER ACTIVE ENVIRONMENT',1),(5,3,'PARTNER INACTIVE ENVIRONMENT',0),(6,1,'PUBLIC INACTIVE ENVIRONMENT',0);
/*!40000 ALTER TABLE `test_environments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_events`
--

DROP TABLE IF EXISTS `test_events`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_events` (
  `eventid` tinyint(4) NOT NULL,
  `name` varchar(50) default NULL,
  PRIMARY KEY  (`eventid`),
  KEY `test_event_name_idx` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_events`
--

LOCK TABLES `test_events` WRITE;
/*!40000 ALTER TABLE `test_events` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_events` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_fielddefs`
--

DROP TABLE IF EXISTS `test_fielddefs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_fielddefs` (
  `fieldid` smallint(6) NOT NULL auto_increment,
  `name` varchar(100) NOT NULL,
  `description` mediumtext,
  `table_name` varchar(100) NOT NULL,
  PRIMARY KEY  (`fieldid`)
) ENGINE=InnoDB AUTO_INCREMENT=27 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_fielddefs`
--

LOCK TABLES `test_fielddefs` WRITE;
/*!40000 ALTER TABLE `test_fielddefs` DISABLE KEYS */;
INSERT INTO `test_fielddefs` VALUES (1,'isactive','Archived','test_plans'),(2,'name','Plan Name','test_plans'),(3,'type_id','Plan Type','test_plans'),(4,'case_status_id','Case Status','test_cases'),(5,'category_id','Category','test_cases'),(6,'priority_id','Priority','test_cases'),(7,'summary','Run Summary','test_cases'),(8,'isautomated','Automated','test_cases'),(9,'alias','Alias','test_cases'),(10,'requirement','Requirement','test_cases'),(11,'script','Script','test_cases'),(12,'arguments','Argument','test_cases'),(13,'product_id','Product','test_plans'),(14,'default_product_version','Default Product Version','test_plans'),(15,'environment_id','Environment','test_runs'),(16,'product_version','Product Version','test_runs'),(17,'build_id','Default Build','test_runs'),(18,'plan_text_version','Plan Text Version','test_runs'),(19,'manager_id','Manager','test_runs'),(20,'default_tester_id','Default Tester','test_cases'),(21,'stop_date','Stop Date','test_runs'),(22,'summary','Run Summary','test_runs'),(23,'notes','Notes','test_runs'),(24,'estimated_time','Estimated Time','test_cases'),(25,'target_pass','Target Pass Rate','test_runs'),(26,'target_completion','Target Completion Rate','test_runs');
/*!40000 ALTER TABLE `test_fielddefs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_named_queries`
--

DROP TABLE IF EXISTS `test_named_queries`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_named_queries` (
  `userid` mediumint(9) NOT NULL,
  `name` varchar(64) NOT NULL,
  `isvisible` tinyint(4) NOT NULL default '1',
  `query` mediumtext NOT NULL,
  `type` mediumint(9) NOT NULL default '0',
  UNIQUE KEY `test_namedquery_primary_idx` (`userid`,`name`),
  KEY `test_namedquery_name_idx` (`name`),
  CONSTRAINT `fk_test_named_queries_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_named_queries`
--

LOCK TABLES `test_named_queries` WRITE;
/*!40000 ALTER TABLE `test_named_queries` DISABLE KEYS */;
INSERT INTO `test_named_queries` VALUES (1,'__case_run__',0,'16,17,18',0),(1,'__case__',0,'25,26,27,28,29',0),(1,'__environment__',0,'3',0),(1,'__plan__',0,'1,4,5',0),(1,'__run_id_4_Foo',0,'assignee=&assignee_type=substring&case_summary=&case_summary_type=allwordssubstr&current_tab=case_run&isautomated=0&query_name=Foo&requirement=&requirement_type=substring&run_id=4&tags=&tags_type=anyexact&testedby=&testedby_type=substring',2),(1,'__run__',0,'1,5',0),(3,'__case_run__',0,'8,9,10,11',0),(3,'__case__',0,'6,7,8,9,10,11,12,13,14,15',0),(3,'__environment__',0,'1,2',0),(3,'__plan__',0,'1,2,3',0),(3,'__run__',0,'1',0);
/*!40000 ALTER TABLE `test_named_queries` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_activity`
--

DROP TABLE IF EXISTS `test_plan_activity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_activity` (
  `plan_id` int(11) NOT NULL,
  `fieldid` smallint(6) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `changed` datetime NOT NULL,
  `oldvalue` mediumtext,
  `newvalue` mediumtext,
  KEY `plan_activity_primary_idx` (`plan_id`),
  KEY `plan_activity_field_idx` (`fieldid`),
  KEY `plan_activity_who_idx` (`who`),
  KEY `plan_activity_changed_idx` (`changed`),
  CONSTRAINT `fk_test_plan_activity_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_test_plan_activity_fieldid_test_fielddefs_fieldid` FOREIGN KEY (`fieldid`) REFERENCES `test_fielddefs` (`fieldid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_activity`
--

LOCK TABLES `test_plan_activity` WRITE;
/*!40000 ALTER TABLE `test_plan_activity` DISABLE KEYS */;
INSERT INTO `test_plan_activity` VALUES (2,1,3,'2008-05-15 14:49:31','1','0'),(1,1,3,'2008-05-15 14:49:51','1','0'),(1,1,3,'2008-05-15 14:50:06','0','1'),(4,2,1,'2010-07-21 10:13:28','CLONE','CLONE OF PLAN 1 NO CASES NO RUNS');
/*!40000 ALTER TABLE `test_plan_activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_attachments`
--

DROP TABLE IF EXISTS `test_plan_attachments`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_attachments` (
  `attachment_id` int(11) NOT NULL,
  `plan_id` int(11) NOT NULL,
  KEY `test_plan_attachments_primary_idx` (`attachment_id`),
  KEY `attachment_plan_id_idx` (`plan_id`),
  CONSTRAINT `fk_test_plan_attachments_plan_id_test_plans_plan_id` FOREIGN KEY (`plan_id`) REFERENCES `test_plans` (`plan_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_59a5d901636ca2b3dc1a7baf7b6b2f15` FOREIGN KEY (`attachment_id`) REFERENCES `test_attachments` (`attachment_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_attachments`
--

LOCK TABLES `test_plan_attachments` WRITE;
/*!40000 ALTER TABLE `test_plan_attachments` DISABLE KEYS */;
INSERT INTO `test_plan_attachments` VALUES (1,1),(2,1),(1,5),(2,5);
/*!40000 ALTER TABLE `test_plan_attachments` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_permissions`
--

DROP TABLE IF EXISTS `test_plan_permissions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_permissions` (
  `userid` mediumint(9) NOT NULL,
  `plan_id` int(11) NOT NULL,
  `permissions` tinyint(4) NOT NULL,
  `grant_type` tinyint(4) NOT NULL,
  UNIQUE KEY `testers_plan_user_idx` (`userid`,`plan_id`,`grant_type`),
  KEY `testers_plan_user_plan_idx` (`plan_id`),
  KEY `testers_plan_grant_idx` (`grant_type`),
  CONSTRAINT `fk_test_plan_permissions_plan_id_test_plans_plan_id` FOREIGN KEY (`plan_id`) REFERENCES `test_plans` (`plan_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_plan_permissions_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_permissions`
--

LOCK TABLES `test_plan_permissions` WRITE;
/*!40000 ALTER TABLE `test_plan_permissions` DISABLE KEYS */;
INSERT INTO `test_plan_permissions` VALUES (1,1,15,0),(1,2,15,0),(1,3,15,0),(1,4,15,0),(1,5,15,0);
/*!40000 ALTER TABLE `test_plan_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_permissions_regexp`
--

DROP TABLE IF EXISTS `test_plan_permissions_regexp`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_permissions_regexp` (
  `plan_id` int(11) NOT NULL,
  `user_regexp` text NOT NULL,
  `permissions` tinyint(4) NOT NULL,
  UNIQUE KEY `testers_plan_regexp_idx` (`plan_id`),
  CONSTRAINT `fk_test_plan_permissions_regexp_plan_id_test_plans_plan_id` FOREIGN KEY (`plan_id`) REFERENCES `test_plans` (`plan_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_permissions_regexp`
--

LOCK TABLES `test_plan_permissions_regexp` WRITE;
/*!40000 ALTER TABLE `test_plan_permissions_regexp` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_plan_permissions_regexp` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_tags`
--

DROP TABLE IF EXISTS `test_plan_tags`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_tags` (
  `tag_id` int(11) NOT NULL,
  `plan_id` int(11) NOT NULL,
  `userid` mediumint(9) NOT NULL,
  UNIQUE KEY `plan_tags_primary_idx` (`tag_id`,`plan_id`,`userid`),
  UNIQUE KEY `plan_tags_secondary_idx` (`tag_id`,`plan_id`),
  KEY `plan_tags_plan_id_idx` (`plan_id`),
  KEY `plan_tags_userid_idx` (`userid`),
  CONSTRAINT `fk_test_plan_tags_tag_id_test_tags_tag_id` FOREIGN KEY (`tag_id`) REFERENCES `test_tags` (`tag_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_tags`
--

LOCK TABLES `test_plan_tags` WRITE;
/*!40000 ALTER TABLE `test_plan_tags` DISABLE KEYS */;
INSERT INTO `test_plan_tags` VALUES (1,1,1),(1,4,1),(1,5,1);
/*!40000 ALTER TABLE `test_plan_tags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_texts`
--

DROP TABLE IF EXISTS `test_plan_texts`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_texts` (
  `plan_id` int(11) NOT NULL,
  `plan_text_version` int(11) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `creation_ts` datetime NOT NULL,
  `plan_text` mediumtext,
  KEY `test_plan_text_version_idx` (`plan_id`,`plan_text_version`),
  KEY `test_plan_text_who_idx` (`who`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_texts`
--

LOCK TABLES `test_plan_texts` WRITE;
/*!40000 ALTER TABLE `test_plan_texts` DISABLE KEYS */;
INSERT INTO `test_plan_texts` VALUES (1,1,1,'2008-03-17 16:51:01','&nbsp;This is a public test plan<br>'),(2,1,1,'2008-03-17 16:52:14','&nbsp;This is a PRIVATE test plan<br>'),(3,1,1,'2008-03-17 16:53:29','&nbsp;This is a PARTNER plan<br>'),(1,2,1,'2008-05-01 17:08:47','&nbsp;<b>This is a public test plan<br></b>\n<br>Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\n<div id=\"lipsum\">\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\nbibendum eu, gravida nec, elementum sed, urna.\n</p>\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\nmattis nec, lacus. Nam tortor.\n</p>\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\nnon erat.\n</p>\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\neros.\n</p></div><br>'),(4,3,1,'2010-07-20 16:58:35','&nbsp;<b>This is a public test plan<br></b>\n<br>Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\n<div id=\"lipsum\">\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\nbibendum eu, gravida nec, elementum sed, urna.\n</p>\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\nmattis nec, lacus. Nam tortor.\n</p>\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\nnon erat.\n</p>\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\neros.\n</p></div><br>'),(5,3,1,'2010-07-21 10:14:55','&nbsp;<b>This is a public test plan<br></b>\n<br>Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Nam\npellentesque odio et elit. Nam lobortis sem suscipit sapien. Sed\niaculis aliquam sapien. Maecenas ut lectus. Aenean fringilla massa et\nmetus. Nam varius, sapien nec egestas feugiat, mi libero dignissim\norci, id fermentum quam nisl quis risus. Phasellus libero justo,\naliquet quis, pellentesque vitae, porttitor quis, orci. Maecenas\nsollicitudin. Donec bibendum, ante quis sodales fermentum, quam risus\nplacerat pede, nec aliquam lorem odio sit amet nisi. Ut sem tellus,\nfeugiat vitae, lobortis nec, dapibus at, est. Aenean cursus. Vivamus\nfaucibus lectus eget felis. Nullam commodo tortor vitae turpis.\n<div id=\"lipsum\">\n<p>Sed mollis interdum risus. Pellentesque ante velit, facilisis vitae,\nfermentum eu, feugiat sit amet, dui. Suspendisse tempus ullamcorper\nnisl. Suspendisse ullamcorper, velit non luctus gravida, massa turpis\nullamcorper eros, sed dictum risus neque ut augue. Vestibulum neque\nnulla, pretium fermentum, rutrum vehicula, pulvinar at, est. Quisque\ndignissim. Nullam placerat neque vel urna. Quisque cursus lacus rutrum\ntortor. Nunc ut elit. Vestibulum mi nunc, volutpat id, tempor ut,\nscelerisque vel, magna. Aenean nisl nulla, rutrum sit amet,\nsollicitudin sed, molestie eget, nisi. Lorem ipsum dolor sit amet,\nconsectetuer adipiscing elit. Class aptent taciti sociosqu ad litora\ntorquent per conubia nostra, per inceptos himenaeos. In odio erat,\nbibendum eu, gravida nec, elementum sed, urna.\n</p>\n<p>Aliquam ultricies viverra mi. Ut convallis urna quis urna. Sed sed\ntortor. Suspendisse quis tellus. Ut gravida. Ut facilisis lectus in\npurus. Sed at est non libero dignissim varius. Donec vestibulum odio ac\nfelis. Duis interdum pellentesque nisl. Aenean leo. Curabitur lectus.\nCum sociis natoque penatibus et magnis dis parturient montes, nascetur\nridiculus mus. Duis nisl ligula, elementum vitae, posuere eu, semper\neget, augue. Maecenas metus nulla, ullamcorper id, malesuada sit amet,\nmattis nec, lacus. Nam tortor.\n</p>\n<p>Nam sollicitudin, lacus sit amet aliquam tempus, nulla tellus tempus\nvelit, eu sollicitudin dolor dui et velit. In ac sem. Mauris adipiscing\nenim in felis. Morbi porttitor laoreet sapien. Nam felis dolor, laoreet\nsed, iaculis eu, vulputate eu, nunc. Nullam egestas ligula. Fusce ut\nsapien. Aliquam erat volutpat. Proin tristique scelerisque sem. Nullam\nnon erat.\n</p>\n<p>Sed feugiat, lacus in elementum egestas, sapien nulla sodales leo,\nnec scelerisque diam eros eu arcu. Phasellus ut magna. Cras dignissim\npellentesque tellus. Curabitur sapien. Suspendisse a risus lobortis\nquam consectetuer placerat. Aliquam ultricies pretium tortor. Aliquam\nerat volutpat. Mauris nunc. Etiam vitae diam. Aenean a felis. Donec\nposuere, lacus in lacinia commodo, ligula lectus rutrum nibh, non\ndapibus sapien enim eu mauris. Pellentesque arcu risus, condimentum id,\ndapibus in, blandit ut, pede. Nulla facilisi. Vestibulum elit quam,\nfringilla convallis, congue lacinia, dictum at, velit. Vestibulum ante\nipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae;\nSed augue mauris, commodo vel, tincidunt hendrerit, consectetuer eu,\neros.\n</p></div><br>');
/*!40000 ALTER TABLE `test_plan_texts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plan_types`
--

DROP TABLE IF EXISTS `test_plan_types`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plan_types` (
  `type_id` smallint(6) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `description` mediumtext,
  PRIMARY KEY  (`type_id`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plan_types`
--

LOCK TABLES `test_plan_types` WRITE;
/*!40000 ALTER TABLE `test_plan_types` DISABLE KEYS */;
INSERT INTO `test_plan_types` VALUES (1,'Unit',NULL),(2,'Integration',NULL),(3,'Function',NULL),(4,'System',NULL),(5,'Acceptance',NULL),(6,'Installation',NULL),(7,'Performance',NULL),(8,'Product',NULL),(9,'Interoperability',NULL);
/*!40000 ALTER TABLE `test_plan_types` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_plans`
--

DROP TABLE IF EXISTS `test_plans`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_plans` (
  `plan_id` int(11) NOT NULL auto_increment,
  `product_id` smallint(6) NOT NULL,
  `author_id` mediumint(9) NOT NULL,
  `type_id` smallint(6) NOT NULL,
  `default_product_version` mediumtext NOT NULL,
  `name` varchar(255) NOT NULL,
  `creation_date` datetime NOT NULL,
  `isactive` tinyint(4) NOT NULL default '1',
  PRIMARY KEY  (`plan_id`),
  KEY `plan_product_plan_id_idx` (`product_id`,`plan_id`),
  KEY `plan_author_idx` (`author_id`),
  KEY `plan_type_idx` (`type_id`),
  KEY `plan_isactive_idx` (`isactive`),
  KEY `plan_name_idx` (`name`),
  CONSTRAINT `fk_test_plans_type_id_test_plan_types_type_id` FOREIGN KEY (`type_id`) REFERENCES `test_plan_types` (`type_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_test_plans_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_plans`
--

LOCK TABLES `test_plans` WRITE;
/*!40000 ALTER TABLE `test_plans` DISABLE KEYS */;
INSERT INTO `test_plans` VALUES (1,1,1,8,'PUBLIC v1','PUBLIC PLAN 1','2008-03-17 16:51:01',1),(2,2,1,8,'PRIVATE v2','PRIVATE PLAN 1','2008-03-17 16:52:14',0),(3,3,1,8,'PARTNER v1','PARTNER PLAN 1','2008-03-17 16:53:29',1),(4,1,1,8,'PUBLIC v2','CLONE OF PLAN 1 NO CASES NO RUNS','2010-07-20 16:58:35',1),(5,1,1,8,'PUBLIC v3','CLONE OF PLAN 1 FULL','2010-07-21 10:14:55',1);
/*!40000 ALTER TABLE `test_plans` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_relationships`
--

DROP TABLE IF EXISTS `test_relationships`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_relationships` (
  `relationship_id` tinyint(4) NOT NULL,
  `name` varchar(50) default NULL,
  PRIMARY KEY  (`relationship_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_relationships`
--

LOCK TABLES `test_relationships` WRITE;
/*!40000 ALTER TABLE `test_relationships` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_relationships` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_run_activity`
--

DROP TABLE IF EXISTS `test_run_activity`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_run_activity` (
  `run_id` int(11) NOT NULL,
  `fieldid` smallint(6) NOT NULL,
  `who` mediumint(9) NOT NULL,
  `changed` datetime NOT NULL,
  `oldvalue` mediumtext,
  `newvalue` mediumtext,
  KEY `run_activity_run_id_idx` (`run_id`),
  KEY `run_activity_field_idx` (`fieldid`),
  KEY `run_activity_who_idx` (`who`),
  KEY `run_activity_when_idx` (`changed`),
  CONSTRAINT `fk_test_run_activity_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON UPDATE CASCADE,
  CONSTRAINT `fk_test_run_activity_fieldid_test_fielddefs_fieldid` FOREIGN KEY (`fieldid`) REFERENCES `test_fielddefs` (`fieldid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_run_activity`
--

LOCK TABLES `test_run_activity` WRITE;
/*!40000 ALTER TABLE `test_run_activity` DISABLE KEYS */;
INSERT INTO `test_run_activity` VALUES (5,16,1,'2010-07-21 10:14:55','PUBLIC v1','PUBLIC v3'),(1,26,1,'2010-07-21 10:48:08',NULL,'95'),(1,25,1,'2010-07-21 10:48:12',NULL,'90');
/*!40000 ALTER TABLE `test_run_activity` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_run_cc`
--

DROP TABLE IF EXISTS `test_run_cc`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_run_cc` (
  `run_id` int(11) NOT NULL,
  `who` mediumint(9) NOT NULL,
  UNIQUE KEY `test_run_cc_primary_idx` (`run_id`,`who`),
  KEY `test_run_cc_who_idx` (`who`),
  CONSTRAINT `fk_test_run_cc_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_run_cc`
--

LOCK TABLES `test_run_cc` WRITE;
/*!40000 ALTER TABLE `test_run_cc` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_run_cc` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_run_tags`
--

DROP TABLE IF EXISTS `test_run_tags`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_run_tags` (
  `tag_id` int(11) NOT NULL,
  `run_id` int(11) NOT NULL,
  `userid` mediumint(9) NOT NULL,
  UNIQUE KEY `run_tags_primary_idx` (`tag_id`,`run_id`,`userid`),
  UNIQUE KEY `run_tags_secondary_idx` (`tag_id`,`run_id`),
  KEY `run_tags_run_id_idx` (`run_id`),
  KEY `run_tags_userid_idx` (`userid`),
  CONSTRAINT `fk_test_run_tags_tag_id_test_tags_tag_id` FOREIGN KEY (`tag_id`) REFERENCES `test_tags` (`tag_id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_run_tags`
--

LOCK TABLES `test_run_tags` WRITE;
/*!40000 ALTER TABLE `test_run_tags` DISABLE KEYS */;
INSERT INTO `test_run_tags` VALUES (3,3,1);
/*!40000 ALTER TABLE `test_run_tags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_runs`
--

DROP TABLE IF EXISTS `test_runs`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_runs` (
  `run_id` int(11) NOT NULL auto_increment,
  `plan_id` int(11) NOT NULL,
  `environment_id` int(11) NOT NULL,
  `product_version` mediumtext,
  `build_id` int(11) NOT NULL,
  `plan_text_version` int(11) NOT NULL,
  `manager_id` mediumint(9) NOT NULL,
  `default_tester_id` mediumint(9) default NULL,
  `start_date` datetime NOT NULL,
  `stop_date` datetime default NULL,
  `summary` tinytext NOT NULL,
  `notes` mediumtext,
  `target_pass` tinyint(4) default NULL,
  `target_completion` tinyint(4) default NULL,
  PRIMARY KEY  (`run_id`),
  KEY `test_run_plan_id_run_id_idx` (`plan_id`,`run_id`),
  KEY `test_run_manager_idx` (`manager_id`),
  KEY `test_run_start_date_idx` (`start_date`),
  KEY `test_run_stop_date_idx` (`stop_date`),
  KEY `test_run_env_idx` (`environment_id`),
  KEY `test_run_build_idx` (`build_id`),
  KEY `test_run_plan_ver_idx` (`plan_text_version`),
  KEY `test_run_tester_idx` (`default_tester_id`),
  CONSTRAINT `fk_test_runs_manager_id_profiles_userid` FOREIGN KEY (`manager_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_runs`
--

LOCK TABLES `test_runs` WRITE;
/*!40000 ALTER TABLE `test_runs` DISABLE KEYS */;
INSERT INTO `test_runs` VALUES (1,1,3,'PUBLIC v1',4,1,5,NULL,'2008-04-17 09:54:24',NULL,'PUBLIC TEST RUN 1','PUBLIC TEST RUN 1',90,95),(2,2,1,'PRIVATE v1',1,1,2,NULL,'2008-05-02 14:29:02',NULL,'PARTNER RUN','PARTNER RUN NOTES',NULL,NULL),(3,2,2,'PRIVATE v2',2,1,2,NULL,'2008-05-02 14:29:18',NULL,'PARTNER RUN','PARTNER RUN NOTES',NULL,NULL),(4,3,4,'PARTNER v2',3,1,1,NULL,'2010-07-20 15:49:01',NULL,'PARTNER TEST RUN','',88,99),(5,5,3,'PUBLIC v3',4,1,1,NULL,'2010-07-21 10:14:55',NULL,'PUBLIC TEST RUN 1',NULL,NULL,NULL);
/*!40000 ALTER TABLE `test_runs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `test_tags`
--

DROP TABLE IF EXISTS `test_tags`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `test_tags` (
  `tag_id` int(11) NOT NULL auto_increment,
  `tag_name` varchar(255) NOT NULL,
  PRIMARY KEY  (`tag_id`),
  KEY `test_tag_name_idx_v2` (`tag_name`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `test_tags`
--

LOCK TABLES `test_tags` WRITE;
/*!40000 ALTER TABLE `test_tags` DISABLE KEYS */;
INSERT INTO `test_tags` VALUES (3,'Partner'),(2,'Private'),(1,'Public');
/*!40000 ALTER TABLE `test_tags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tokens`
--

DROP TABLE IF EXISTS `tokens`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `tokens` (
  `userid` mediumint(9) default NULL,
  `issuedate` datetime NOT NULL,
  `token` varchar(16) NOT NULL,
  `tokentype` varchar(8) NOT NULL,
  `eventdata` tinytext,
  PRIMARY KEY  (`token`),
  KEY `tokens_userid_idx` (`userid`),
  CONSTRAINT `fk_tokens_userid_profiles_userid` FOREIGN KEY (`userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `tokens`
--

LOCK TABLES `tokens` WRITE;
/*!40000 ALTER TABLE `tokens` DISABLE KEYS */;
INSERT INTO `tokens` VALUES (1,'2008-05-02 15:15:52','0YkcZVp6yg','session','edit_group'),(1,'2008-05-02 14:43:11','35AW14OH17','session','edit_user'),(1,'2008-05-02 14:36:48','4BSWEvYiW1','session','edit_group_controls'),(1,'2008-05-02 14:47:10','5Hc6oyGIoX','session','edit_user'),(1,'2008-05-02 15:15:40','6gZDTD1r2M','session','edit_group'),(1,'2008-05-02 14:45:14','8A59PcqkAU','session','edit_user'),(1,'2008-05-02 15:13:13','8D0fE2L5Km','session','edit_group'),(1,'2008-05-02 15:12:40','8HYIPmGWgc','session','edit_group'),(1,'2008-05-02 14:44:31','8q3B4Ls9nI','session','edit_user'),(1,'2008-05-02 14:31:17','8U7sSamWVd','session','edit_product'),(1,'2008-05-02 14:36:49','9A3HVCYod2','session','edit_group_controls'),(1,'2008-05-02 15:14:49','9ibRoQm4Z2','session','edit_product'),(1,'2008-05-02 15:13:11','9XM243wzya','session','edit_group'),(1,'2008-05-02 14:43:13','ABeKirlvVr','session','edit_user'),(1,'2008-05-02 15:15:08','b3viUsc6mm','session','edit_group'),(1,'2008-05-02 14:44:50','bGXTZDkMrR','session','edit_user'),(1,'2008-05-02 14:44:01','bx5POwjtfX','session','edit_user'),(1,'2008-05-02 14:44:29','CYL1QV0LVZ','session','edit_user'),(1,'2010-07-20 16:41:08','DdcklOKVRW','session','edit_parameters'),(1,'2008-05-02 14:43:41','dmNHdSUzM9','session','edit_user'),(1,'2008-05-02 14:37:12','DMyUzF5irS','session','edit_product'),(1,'2008-05-02 14:46:07','EfQ4HncVJI','session','edit_user'),(1,'2008-05-02 14:46:34','eKvbg8Ctde','session','edit_group'),(1,'2008-05-02 14:45:07','eNUBOfAU3g','session','edit_user'),(1,'2008-05-02 15:09:13','ERWmkpUOPF','session','createbug:3'),(1,'2008-05-02 14:45:16','EuVzmBMNIP','session','edit_user'),(1,'2008-05-02 15:15:32','EYZoBDiLSr','session','edit_group'),(1,'2008-05-02 14:43:27','g8s0nUB0ys','session','edit_user'),(1,'2008-05-02 14:52:09','GjEdx8s6OM','session','edit_group_controls'),(1,'2008-05-02 14:51:33','gv3Cnn1YHG','session','edit_group'),(1,'2008-05-02 14:46:36','HGqWaqFjYv','session','edit_group'),(1,'2010-07-20 16:41:36','hprxr4gRKu','session','edit_parameters'),(1,'2008-05-02 14:51:45','i2LheuBigV','session','edit_product'),(1,'2008-05-02 14:36:58','IdbtGEzC6T','session','edit_product'),(1,'2008-05-02 14:42:52','ig1kaNGOt0','session','edit_user'),(1,'2010-07-20 15:21:21','iUJmI5cPQU','session','edit_parameters'),(1,'2008-05-02 14:36:44','jc9XK8DTf3','session','edit_product'),(1,'2010-07-21 10:52:01','ju7WO9dzQL','session','edit_settings'),(1,'2008-05-02 14:45:06','jXHHM2xgGv','session','edit_user'),(1,'2008-05-02 15:01:26','kdmDhrsu6s','session','edit_product'),(3,'2008-05-02 15:26:26','Ko1sjx9tyG','session','createbug:6'),(1,'2008-05-02 14:52:11','KsuUmfN4gR','session','edit_group_controls'),(2,'2008-05-02 15:19:13','lYoujnHHxw','session','createbug:4'),(1,'2008-05-02 14:46:08','mioaP1BwE9','session','edit_user'),(2,'2008-05-02 15:14:14','NdF08sPHDO','session','createbug:'),(1,'2008-05-01 17:22:58','NlD32ZB5Jc','session','createbug:2'),(1,'2008-05-02 14:31:16','NTKyBgowwc','session','edit_product'),(1,'2008-05-02 14:37:14','nXMVzUPuzC','session','edit_product'),(1,'2008-05-02 14:44:49','oonmgKM6Up','session','edit_user'),(1,'2008-05-02 14:47:08','OU0Fk0XmP2','session','edit_user'),(1,'2008-05-02 15:01:22','pRJuLwYw21','session','edit_product'),(1,'2008-05-02 14:44:42','qac4kmeRnn','session','edit_user'),(1,'2008-05-02 15:12:42','RP12jUBF94','session','edit_group'),(1,'2008-05-02 14:51:46','rPYOLH1vpN','session','edit_product'),(1,'2010-07-20 15:21:33','RTcRCAb0Md','session','edit_parameters'),(1,'2010-07-20 15:21:36','SypNsA2uQy','session','edit_parameters'),(1,'2008-05-02 14:31:24','TAMHWO9GGS','session','edit_group_controls'),(1,'2008-05-02 15:14:50','tGvDQ7xIqb','session','edit_product'),(1,'2008-05-02 14:43:40','tUj6K7JMQX','session','edit_user'),(1,'2008-05-02 14:33:49','u2NvNqrqeK','session','edit_group_controls'),(1,'2008-05-02 14:37:26','UojzfCQ4cq','session','edit_product'),(1,'2010-07-21 10:51:30','vapUARBrq4','session','edit_user_prefs'),(1,'2008-05-02 14:45:48','VyKqtm1n14','session','edit_user'),(1,'2008-05-02 14:37:24','wzHivUGror','session','edit_product'),(7,'2008-05-02 15:20:38','XYEf5r4pHB','session','createbug:5'),(1,'2008-05-02 14:36:45','zGpvba8saL','session','edit_product'),(1,'2008-05-02 14:51:02','Zzog07sUwU','session','edit_group');
/*!40000 ALTER TABLE `tokens` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ts_error`
--

DROP TABLE IF EXISTS `ts_error`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ts_error` (
  `error_time` int(11) NOT NULL,
  `jobid` int(11) NOT NULL,
  `message` varchar(255) NOT NULL,
  `funcid` int(11) NOT NULL default '0',
  KEY `ts_error_funcid_idx` (`funcid`,`error_time`),
  KEY `ts_error_error_time_idx` (`error_time`),
  KEY `ts_error_jobid_idx` (`jobid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `ts_error`
--

LOCK TABLES `ts_error` WRITE;
/*!40000 ALTER TABLE `ts_error` DISABLE KEYS */;
/*!40000 ALTER TABLE `ts_error` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ts_exitstatus`
--

DROP TABLE IF EXISTS `ts_exitstatus`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ts_exitstatus` (
  `jobid` int(11) NOT NULL auto_increment,
  `funcid` int(11) NOT NULL default '0',
  `status` smallint(6) default NULL,
  `completion_time` int(11) default NULL,
  `delete_after` int(11) default NULL,
  PRIMARY KEY  (`jobid`),
  KEY `ts_exitstatus_funcid_idx` (`funcid`),
  KEY `ts_exitstatus_delete_after_idx` (`delete_after`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `ts_exitstatus`
--

LOCK TABLES `ts_exitstatus` WRITE;
/*!40000 ALTER TABLE `ts_exitstatus` DISABLE KEYS */;
/*!40000 ALTER TABLE `ts_exitstatus` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ts_funcmap`
--

DROP TABLE IF EXISTS `ts_funcmap`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ts_funcmap` (
  `funcid` int(11) NOT NULL auto_increment,
  `funcname` varchar(255) NOT NULL,
  PRIMARY KEY  (`funcid`),
  UNIQUE KEY `ts_funcmap_funcname_idx` (`funcname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `ts_funcmap`
--

LOCK TABLES `ts_funcmap` WRITE;
/*!40000 ALTER TABLE `ts_funcmap` DISABLE KEYS */;
/*!40000 ALTER TABLE `ts_funcmap` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ts_job`
--

DROP TABLE IF EXISTS `ts_job`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ts_job` (
  `jobid` int(11) NOT NULL auto_increment,
  `funcid` int(11) NOT NULL,
  `arg` longblob,
  `uniqkey` varchar(255) default NULL,
  `insert_time` int(11) default NULL,
  `run_after` int(11) NOT NULL,
  `grabbed_until` int(11) NOT NULL,
  `priority` smallint(6) default NULL,
  `coalesce` varchar(255) default NULL,
  PRIMARY KEY  (`jobid`),
  UNIQUE KEY `ts_job_funcid_idx` (`funcid`,`uniqkey`),
  KEY `ts_job_run_after_idx` (`run_after`,`funcid`),
  KEY `ts_job_coalesce_idx` (`coalesce`,`funcid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `ts_job`
--

LOCK TABLES `ts_job` WRITE;
/*!40000 ALTER TABLE `ts_job` DISABLE KEYS */;
/*!40000 ALTER TABLE `ts_job` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ts_note`
--

DROP TABLE IF EXISTS `ts_note`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `ts_note` (
  `jobid` int(11) NOT NULL,
  `notekey` varchar(255) default NULL,
  `value` longblob,
  UNIQUE KEY `ts_note_jobid_idx` (`jobid`,`notekey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `ts_note`
--

LOCK TABLES `ts_note` WRITE;
/*!40000 ALTER TABLE `ts_note` DISABLE KEYS */;
/*!40000 ALTER TABLE `ts_note` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_group_map`
--

DROP TABLE IF EXISTS `user_group_map`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `user_group_map` (
  `user_id` mediumint(9) NOT NULL,
  `group_id` mediumint(9) NOT NULL,
  `isbless` tinyint(4) NOT NULL default '0',
  `grant_type` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `user_group_map_user_id_idx` (`user_id`,`group_id`,`grant_type`,`isbless`),
  KEY `fk_user_group_map_group_id_groups_id` (`group_id`),
  CONSTRAINT `fk_user_group_map_group_id_groups_id` FOREIGN KEY (`group_id`) REFERENCES `groups` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_user_group_map_user_id_profiles_userid` FOREIGN KEY (`user_id`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `user_group_map`
--

LOCK TABLES `user_group_map` WRITE;
/*!40000 ALTER TABLE `user_group_map` DISABLE KEYS */;
INSERT INTO `user_group_map` VALUES (1,1,0,0),(1,1,1,0),(3,1,0,0),(3,1,1,0),(1,2,0,0),(1,2,1,0),(1,4,0,0),(1,4,1,0),(1,9,0,2),(2,9,0,2),(3,9,0,2),(4,9,0,2),(5,9,0,2),(6,9,0,2),(7,9,0,2),(8,9,0,2),(2,15,0,0),(2,15,1,0),(6,15,0,0),(6,15,1,0),(7,16,0,0),(7,16,1,0),(8,16,0,0),(8,16,1,0);
/*!40000 ALTER TABLE `user_group_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `versions`
--

DROP TABLE IF EXISTS `versions`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `versions` (
  `id` mediumint(9) NOT NULL auto_increment,
  `value` varchar(64) NOT NULL,
  `product_id` smallint(6) NOT NULL,
  `disallownew` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `versions_product_id_idx` (`product_id`,`value`),
  CONSTRAINT `fk_versions_product_id_products_id` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `versions`
--

LOCK TABLES `versions` WRITE;
/*!40000 ALTER TABLE `versions` DISABLE KEYS */;
INSERT INTO `versions` VALUES (1,'PUBLIC v1',1,0),(2,'PRIVATE v1',2,0),(3,'PARTNER v1',3,0),(4,'PARTNER v2',3,0),(5,'PUBLIC v2',1,0),(6,'PUBLIC v3',1,0),(7,'PRIVATE v2',2,0);
/*!40000 ALTER TABLE `versions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `votes`
--

DROP TABLE IF EXISTS `votes`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `votes` (
  `who` mediumint(9) NOT NULL,
  `bug_id` mediumint(9) NOT NULL,
  `vote_count` smallint(6) NOT NULL,
  KEY `votes_who_idx` (`who`),
  KEY `votes_bug_id_idx` (`bug_id`),
  CONSTRAINT `fk_votes_bug_id_bugs_bug_id` FOREIGN KEY (`bug_id`) REFERENCES `bugs` (`bug_id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_votes_who_profiles_userid` FOREIGN KEY (`who`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `votes`
--

LOCK TABLES `votes` WRITE;
/*!40000 ALTER TABLE `votes` DISABLE KEYS */;
/*!40000 ALTER TABLE `votes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `watch`
--

DROP TABLE IF EXISTS `watch`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `watch` (
  `watcher` mediumint(9) NOT NULL,
  `watched` mediumint(9) NOT NULL,
  UNIQUE KEY `watch_watcher_idx` (`watcher`,`watched`),
  KEY `watch_watched_idx` (`watched`),
  CONSTRAINT `fk_watch_watched_profiles_userid` FOREIGN KEY (`watched`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_watch_watcher_profiles_userid` FOREIGN KEY (`watcher`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `watch`
--

LOCK TABLES `watch` WRITE;
/*!40000 ALTER TABLE `watch` DISABLE KEYS */;
/*!40000 ALTER TABLE `watch` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `whine_events`
--

DROP TABLE IF EXISTS `whine_events`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `whine_events` (
  `id` mediumint(9) NOT NULL auto_increment,
  `owner_userid` mediumint(9) NOT NULL,
  `subject` varchar(128) default NULL,
  `body` mediumtext,
  `mailifnobugs` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `fk_whine_events_owner_userid_profiles_userid` (`owner_userid`),
  CONSTRAINT `fk_whine_events_owner_userid_profiles_userid` FOREIGN KEY (`owner_userid`) REFERENCES `profiles` (`userid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `whine_events`
--

LOCK TABLES `whine_events` WRITE;
/*!40000 ALTER TABLE `whine_events` DISABLE KEYS */;
/*!40000 ALTER TABLE `whine_events` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `whine_queries`
--

DROP TABLE IF EXISTS `whine_queries`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `whine_queries` (
  `id` mediumint(9) NOT NULL auto_increment,
  `eventid` mediumint(9) NOT NULL,
  `query_name` varchar(64) NOT NULL default '',
  `sortkey` smallint(6) NOT NULL default '0',
  `onemailperbug` tinyint(4) NOT NULL default '0',
  `title` varchar(128) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `whine_queries_eventid_idx` (`eventid`),
  CONSTRAINT `fk_whine_queries_eventid_whine_events_id` FOREIGN KEY (`eventid`) REFERENCES `whine_events` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `whine_queries`
--

LOCK TABLES `whine_queries` WRITE;
/*!40000 ALTER TABLE `whine_queries` DISABLE KEYS */;
/*!40000 ALTER TABLE `whine_queries` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `whine_schedules`
--

DROP TABLE IF EXISTS `whine_schedules`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `whine_schedules` (
  `id` mediumint(9) NOT NULL auto_increment,
  `eventid` mediumint(9) NOT NULL,
  `run_day` varchar(32) default NULL,
  `run_time` varchar(32) default NULL,
  `run_next` datetime default NULL,
  `mailto` mediumint(9) NOT NULL,
  `mailto_type` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `whine_schedules_run_next_idx` (`run_next`),
  KEY `whine_schedules_eventid_idx` (`eventid`),
  CONSTRAINT `fk_whine_schedules_eventid_whine_events_id` FOREIGN KEY (`eventid`) REFERENCES `whine_events` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `whine_schedules`
--

LOCK TABLES `whine_schedules` WRITE;
/*!40000 ALTER TABLE `whine_schedules` DISABLE KEYS */;
/*!40000 ALTER TABLE `whine_schedules` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2010-07-21 16:53:13
