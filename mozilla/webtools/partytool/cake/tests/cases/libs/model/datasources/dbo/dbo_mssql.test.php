<?php
/* SVN FILE: $Id$ */
/**
 * DboMssql test
 *
 * PHP versions 4 and 5
 *
 * CakePHP(tm) :  Rapid Development Framework <http://www.cakephp.org/>
 * Copyright 2005-2007, Cake Software Foundation, Inc.
 *								1785 E. Sahara Avenue, Suite 490-204
 *								Las Vegas, Nevada 89104
 *
 * Licensed under The MIT License
 * Redistributions of files must retain the above copyright notice.
 *
 * @filesource
 * @copyright		Copyright 2005-2007, Cake Software Foundation, Inc.
 * @link			http://www.cakefoundation.org/projects/info/cakephp CakePHP(tm) Project
 * @package			cake
 * @subpackage		cake.cake.libs
 * @since			CakePHP(tm) v 1.2.0
 * @version			$Revision$
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date$
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */
	if (!defined('CAKEPHP_UNIT_TEST_EXECUTION')) {
		define('CAKEPHP_UNIT_TEST_EXECUTION', 1);
	}
	require_once LIBS.'model'.DS.'model.php';
	require_once LIBS.'model'.DS.'datasources'.DS.'datasource.php';
	require_once LIBS.'model'.DS.'datasources'.DS.'dbo_source.php';
	require_once LIBS.'model'.DS.'datasources'.DS.'dbo'.DS.'dbo_mssql.php';

	/**
	 * Short description for class.
	 *
	 * @package		cake.tests
	 * @subpackage	cake.tests.cases.libs.model.datasources
	 */
	class DboMssqlTestDb extends DboMssql {

		var $simulated = array();

		function _execute($sql) {
			$this->simulated[] = $sql;
			return null;
		}

		function getLastQuery() {
			return $this->simulated[count($this->simulated) - 1];
		}
	}

	/**
	 * Short description for class.
	 *
	 * @package		cake.tests
	 * @subpackage	cake.tests.cases.libs.model.datasources
	 */
	class MssqlTestModel extends Model {

		var $name = 'MssqlTestModel';
		var $useTable = false;

		function find($conditions = null, $fields = null, $order = null, $recursive = null) {
			return $conditions;
		}

		function findAll($conditions = null, $fields = null, $order = null, $recursive = null) {
			return $conditions;
		}

		function schema() {
			return new Set(array(
				'id'		=> array('type' => 'integer', 'null' => '', 'default' => '', 'length' => '8'),
				'client_id'	=> array('type' => 'integer', 'null' => '', 'default' => '0', 'length' => '11'),
				'name'		=> array('type' => 'string', 'null' => '', 'default' => '', 'length' => '255'),
				'login'		=> array('type' => 'string', 'null' => '', 'default' => '', 'length' => '255'),
				'passwd'	=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '255'),
				'addr_1'	=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '255'),
				'addr_2'	=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '25'),
				'zip_code'	=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'city'		=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'country'	=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'phone'		=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'fax'		=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'url'		=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '255'),
				'email'		=> array('type' => 'string', 'null' => '1', 'default' => '', 'length' => '155'),
				'comments'	=> array('type' => 'text', 'null' => '1', 'default' => '', 'length' => ''),
				'last_login'=> array('type' => 'datetime', 'null' => '1', 'default' => '', 'length' => ''),
				'created'	=> array('type' => 'date', 'null' => '1', 'default' => '', 'length' => ''),
				'updated'	=> array('type' => 'datetime', 'null' => '1', 'default' => '', 'length' => null)
			));
		}
	}

/**
 * The test class for the DboMssql
 *
 * @package		cake.tests
 * @subpackage	cake.tests.cases.libs.model.datasources.dbo
 */
class DboMssqlTest extends UnitTestCase {
/**
 * The Dbo instance to be tested
 *
 * @var object
 * @access public
 */
	var $Db = null;
/**
 * Sets up a Dbo class instance for testing
 *
 * @access public
 */
	function setUp() {
		require_once APP . 'config' . DS . 'database.php';
		$config = new DATABASE_CONFIG();
		$this->db =& new DboMssqlTestDb($config->default, false);
		$this->db->fullDebug = false;
		$this->model = new MssqlTestModel();
	}

	/**
	 * Test Dbo value method
	 *
	 * @access public
	 */

	function testQuoting() {
		$result = $this->db->fields($this->model);
		$expected = array(
			'[MssqlTestModel].[id] AS [MssqlTestModel__0]',
			'[MssqlTestModel].[client_id] AS [MssqlTestModel__1]',
			'[MssqlTestModel].[name] AS [MssqlTestModel__2]',
			'[MssqlTestModel].[login] AS [MssqlTestModel__3]',
			'[MssqlTestModel].[passwd] AS [MssqlTestModel__4]',
			'[MssqlTestModel].[addr_1] AS [MssqlTestModel__5]',
			'[MssqlTestModel].[addr_2] AS [MssqlTestModel__6]',
			'[MssqlTestModel].[zip_code] AS [MssqlTestModel__7]',
			'[MssqlTestModel].[city] AS [MssqlTestModel__8]',
			'[MssqlTestModel].[country] AS [MssqlTestModel__9]',
			'[MssqlTestModel].[phone] AS [MssqlTestModel__10]',
			'[MssqlTestModel].[fax] AS [MssqlTestModel__11]',
			'[MssqlTestModel].[url] AS [MssqlTestModel__12]',
			'[MssqlTestModel].[email] AS [MssqlTestModel__13]',
			'[MssqlTestModel].[comments] AS [MssqlTestModel__14]',
			'[MssqlTestModel].[last_login] AS [MssqlTestModel__15]',
			'[MssqlTestModel].[created] AS [MssqlTestModel__16]',
			'[MssqlTestModel].[updated] AS [MssqlTestModel__17]'
		);
		$this->assertEqual($result, $expected);

		$expected = "1.2";
		$result = $this->db->value(1.2, 'float');
		$this->assertIdentical($expected, $result);

		$expected = "'1,2'";
		$result = $this->db->value('1,2', 'float');
		$this->assertIdentical($expected, $result);
	}
}
?>