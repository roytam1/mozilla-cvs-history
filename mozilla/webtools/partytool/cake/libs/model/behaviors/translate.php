<?php
/* SVN FILE: $Id$ */
/**
 * Short description for file.
 *
 * Long description for file
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
 * @link				http://www.cakefoundation.org/projects/info/cakephp CakePHP(tm) Project
 * @package			cake
 * @subpackage		cake.cake.libs.model.behaviors
 * @since			CakePHP(tm) v 1.2.0.4525
 * @version			$Revision$
 * @modifiedby		$LastChangedBy: phpnut $
 * @lastmodified	$Date$
 * @license			http://www.opensource.org/licenses/mit-license.php The MIT License
 */
/**
 * Short description for file.
 *
 * Long description for file
 *
 * @package	 	cake
 * @subpackage	cake.cake.libs.model.behaviors
 */
class TranslateBehavior extends ModelBehavior {
/**
 * Used for runtime configuration of model
 */
	var $runtime = array();
/**
 * Callback
 *
 * $config for TranslateBehavior should be
 * array( 'fields' => array('field_one',
 * 'field_two' => 'FieldAssoc', 'field_three'))
 *
 * With above example only one permanent hasMany will be joined (for field_two
 * as FieldAssoc)
 *
 * $config could be empty - and translations configured dynamically by
 * bindTranslation() method
 */
	function setup(&$model, $config = array()) {
		$db =& ConnectionManager::getDataSource($model->useDbConfig);
		if (!$db->connected) {
			trigger_error('Datasource '.$model->useDbConfig.' for TranslateBehavior of model '.$model->name.' is not connected', E_USER_ERROR);
			return false;
		}

		$this->settings[$model->name] = array();
		$this->runtime[$model->name] = array('fields' => array());
		$this->translateModel($model);
		return $this->bindTranslation($model, null, false);
	}
/**
 * Callback
 */
	function beforeFind(&$model, $query) {
		$locale = $this->_getLocale($model);
		$db =& ConnectionManager::getDataSource($model->useDbConfig);
		$tablePrefix = $db->config['prefix'];
		$RuntimeModel =& $this->translateModel($model);

		if (is_string($query['fields']) && 'COUNT(*) AS '.$db->name('count') == $query['fields']) {
			$this->runtime[$model->name]['count'] = true;

			if (empty($locale)) {
				return $query;
			}
			$query['fields'] = 'COUNT(DISTINCT('.$db->name($model->name).'.'.$db->name($model->primaryKey).')) ' . $db->alias . 'count';
			$query['joins'][] = array(
						'type' => 'INNER',
						'alias' => $RuntimeModel->name,
						'table' => $db->name($tablePrefix . 'i18n'),
						'conditions' => array(
								$model->name.'.id' => '{$__cakeIdentifier['.$RuntimeModel->name.'.foreign_key]__$}',
								$RuntimeModel->name.'.model' => $model->name,
								$RuntimeModel->name.'.locale' => $locale));
			return $query;
		}

		if (empty($locale)) {
			return $query;
		}
		$autoFields = false;

		if (empty($query['fields'])) {
			$query['fields'] = array($model->name.'.*');

			foreach (array('hasOne', 'belongsTo') as $type) {
				foreach ($model->{$type} as $key => $value) {

					if (empty($value['fields'])) {
						$query['fields'][] = $key.'.*';
					} else {
						foreach ($value['fields'] as $field) {
							$query['fields'][] = $key.'.'.$field;
						}
					}
				}
			}
			$autoFields = true;
		}
		$fields = am($this->settings[$model->name], $this->runtime[$model->name]['fields']);
		$addFields = array();
		if (is_array($query['fields'])) {
			foreach ($fields as $key => $value) {
				$field = ife(is_numeric($key), $value, $key);

				if (in_array($model->name.'.*', $query['fields']) || $autoFields || in_array($model->name.'.'.$field, $query['fields']) || in_array($field, $query['fields'])) {
					$addFields[] = $field;
				}
			}
		}

		if ($addFields) {
			foreach ($addFields as $field) {
				foreach (array($field, $model->name.'.'.$field) as $_field) {
					$key = array_search($_field, $query['fields']);

					if ($key !== false) {
						unset($query['fields'][$key]);
					}
				}

				if (is_array($locale)) {
					foreach ($locale as $_locale) {
						$query['fields'][] = 'I18n__'.$field.'__'.$_locale.'.content';
						$query['joins'][] = array(
								'type' => 'LEFT',
								'alias' => 'I18n__'.$field.'__'.$_locale,
								'table' => $db->name($tablePrefix . 'i18n'),
								'conditions' => array(
										$model->name.'.id' => '{$__cakeIdentifier[I18n__'.$field.'__'.$_locale.'.foreign_key]__$}',
										'I18n__'.$field.'__'.$_locale.'.model' => $model->name,
										'I18n__'.$field.'__'.$_locale.'.'.$RuntimeModel->displayField => $field,
										'I18n__'.$field.'__'.$_locale.'.locale' => $_locale));
					}
				} else {
					$query['fields'][] = 'I18n__'.$field.'.content';
					$query['joins'][] = array(
							'type' => 'LEFT',
							'alias' => 'I18n__'.$field,
							'table' => $db->name($tablePrefix . 'i18n'),
							'conditions' => array(
									$model->name.'.id' => '{$__cakeIdentifier[I18n__'.$field.'.foreign_key]__$}',
									'I18n__'.$field.'.model' => $model->name,
									'I18n__'.$field.'.'.$RuntimeModel->displayField => $field));
					$query['conditions'][$db->name('I18n__'.$field.'.locale')] = $locale;
				}
			}
		}
		$query['fields'] = am($query['fields']);
		$this->runtime[$model->name]['beforeFind'] = $addFields;
		return $query;
	}
/**
 * Callback
 */
	function afterFind(&$model, $results, $primary) {
		$this->runtime[$model->name]['fields'] = array();
		$locale = $this->_getLocale($model);

		if (empty($locale) || empty($results) || empty($this->runtime[$model->name]['beforeFind'])) {
			return $results;
		}
		$beforeFind = $this->runtime[$model->name]['beforeFind'];

		foreach ($results as $key => $row) {
			$results[$key][$model->name]['locale'] = ife(is_array($locale), @$locale[0], $locale);

			foreach ($beforeFind as $field) {
				if (is_array($locale)) {
					foreach ($locale as $_locale) {
						if (!isset($results[$key][$model->name][$field]) && !empty($results[$key]['I18n__'.$field.'__'.$_locale]['content'])) {
							$results[$key][$model->name][$field] = $results[$key]['I18n__'.$field.'__'.$_locale]['content'];
						}
						unset($results[$key]['I18n__'.$field.'__'.$_locale]);
					}

					if (!isset($results[$key][$model->name][$field])) {
						$results[$key][$model->name][$field] = '';
					}
				} else {
					$value = ife(empty($results[$key]['I18n__'.$field]['content']), '', $results[$key]['I18n__'.$field]['content']);
					$results[$key][$model->name][$field] = $value;
					unset($results[$key]['I18n__'.$field]);
				}
			}
		}
		return $results;
    }
/**
 * Callback
 */
	function beforeSave(&$model) {
		$locale = $this->_getLocale($model);

		if (empty($locale) || is_array($locale)) {
			return true;
		}
		$fields = am($this->settings[$model->name], $this->runtime[$model->name]['fields']);
		$tempData = array();

		foreach ($fields as $key => $value) {
			$field = ife(is_numeric($key), $value, $key);

			if (isset($model->data[$model->name][$field])) {
				$tempData[$field] = $model->data[$model->name][$field];
				unset($model->data[$model->name][$field]);
			}
		}
		$this->runtime[$model->name]['beforeSave'] = $tempData;
		return true;
	}
/**
 * Callback
 */
	function afterSave(&$model, $created) {
		if (!isset($this->runtime[$model->name]['beforeSave'])) {
			return true;
		}
		$locale = $this->_getLocale($model);
		$tempData = $this->runtime[$model->name]['beforeSave'];
		unset($this->runtime[$model->name]['beforeSave']);
		$conditions = array('locale' => $locale, 'model' => $model->name, 'foreign_key' => $model->id);
		$RuntimeModel =& $this->translateModel($model);

		if (empty($created)) {
			$translations = $RuntimeModel->generateList(am($conditions, array($RuntimeModel->displayField => array_keys($tempData))));

			if ($translations) {
				foreach ($translations as $id => $field) {
					$RuntimeModel->create();
					$RuntimeModel->save(array($RuntimeModel->name => array('id' => $id, 'content' => $tempData[$field])));
					unset($tempData[$field]);
				}
			}
		}

		if (!empty($tempData)) {
			foreach ($tempData as $field => $value) {
				$RuntimeModel->create(am($conditions, array($RuntimeModel->displayField => $field, 'content' => $value)));
				$RuntimeModel->save();
			}
		}
	}
/**
 * Callback
 */
	function afterDelete(&$model) {
		$RuntimeModel =& $this->translateModel($model);
		$conditions = array('model' => $model->name, 'foreign_key' => $model->id);
		$RuntimeModel->deleteAll($conditions);
	}
/**
 * Get selected locale for model
 *
 * @return mixed string or false
 */
	function _getLocale(&$model) {
		if (!isset($model->locale) || is_null($model->locale)) {
			if (!class_exists('I18n')) {
				uses('i18n');
			}
			$I18n =& I18n::getInstance();
			$model->locale = $I18n->l10n->locale;
		}
		return $model->locale;
	}
/**
 * Get instance of model for translations
 *
 * @return object
 */
	function &translateModel(&$model) {
		if (!isset($this->runtime[$model->name]['model'])) {
			if (!isset($model->translateModel) || empty($model->translateModel)) {
				$className = 'I18nModel';
			} else {
				$className = $model->translateModel;
				if (!class_exists($className) && !loadModel($className)) {
					return $this->cakeError('missingModel', array(array('className' => $className)));
				}
			}

			if (ClassRegistry::isKeySet($className)) {
				if (PHP5) {
					$this->runtime[$model->name]['model'] = ClassRegistry::getObject($className);
				} else {
					$this->runtime[$model->name]['model'] =& ClassRegistry::getObject($className);
				}
			} else {
				if (PHP5) {
					$this->runtime[$model->name]['model'] = new $className();
				} else {
					$this->runtime[$model->name]['model'] =& new $className();
				}
				ClassRegistry::addObject($className, $this->runtime[$model->name]['model']);
				ClassRegistry::map($className, $className);
			}
		}
		return $this->runtime[$model->name]['model'];
	}
/**
 * Bind translation for fields, optionally with hasMany association for
 * fake field
 *
 * @param object instance of model
 * @param mixed string with field, or array(field1, field2=>AssocName, field3), or null for bind all original translations
 * @param boolean $reset
 * @return bool
 */
	function bindTranslation(&$model, $fields = null, $reset = true) {
		if (empty($fields)) {
			return $this->bindTranslation($model, $model->actsAs['Translate'], $reset);
		}

		if (is_string($fields)) {
			$fields = array($fields);
		}
		$associations = array();
		$RuntimeModel =& $this->translateModel($model);
		$default = array('className' => $RuntimeModel->name, 'foreignKey' => 'foreign_key');

		foreach ($fields as $key => $value) {
			if (is_numeric($key)) {
				$field = $value;
				$association = null;
			} else {
				$field = $key;
				$association = $value;
			}

			if (array_key_exists($field, $this->settings[$model->name])) {
				unset($this->settings[$model->name][$field]);

			} elseif (in_array($field, $this->settings[$model->name])) {
				$this->settings[$model->name] = am(array_diff_assoc($this->settings[$model->name], array($field)));
			}

			if (array_key_exists($field, $this->runtime[$model->name]['fields'])) {
				unset($this->runtime[$model->name]['fields'][$field]);

			} elseif (in_array($field, $this->runtime[$model->name]['fields'])) {
				$this->runtime[$model->name]['fields'] = am(array_diff_assoc($this->runtime[$model->name]['fields'], array($field)));
			}

			if (is_null($association)) {
				if ($reset) {
					$this->runtime[$model->name]['fields'][] = $field;
				} else {
					$this->settings[$model->name][] = $field;
				}
			} else {

				if ($reset) {
					$this->runtime[$model->name]['fields'][$field] = $association;
				} else {
					$this->settings[$model->name][$field] = $association;
				}

				foreach (array('hasOne', 'hasMany', 'belongsTo', 'hasAndBelongsToMany') as $type) {
					if (isset($model->{$type}[$association]) || isset($model->__backAssociation[$type][$association])) {
						trigger_error('Association '.$association.' is already binded to model '.$model->name, E_USER_ERROR);
						return false;
					}
				}
				$associations[$association] = am($default, array('conditions' => array(
						'model' => $model->name,
						$RuntimeModel->displayField => $field)));
			}
		}

		if (!empty($associations)) {
			$model->bindModel(array('hasMany' => $associations), $reset);
		}
		return true;
	}
/**
 * Unbind translation for fields, optionally unbinds hasMany association for
 * fake field
 *
 * @param object instance of model
 * @param mixed string with field, or array(field1, field2=>AssocName, field3), or null for unbind all original translations
 * @return bool
 */
	function unbindTranslation(&$model, $fields = null) {
		if (empty($fields)) {
			return $this->unbindTranslation($model, $model->actsAs['Translate']);
		}

		if (is_string($fields)) {
			$fields = array($fields);
		}
		$RuntimeModel =& $this->translateModel($model);
		$default = array('className' => $RuntimeModel->name, 'foreignKey' => 'foreign_key');
		$associations = array();

		foreach ($fields as $key => $value) {
			if (is_numeric($key)) {
				$field = $value;
				$association = null;
			} else {
				$field = $key;
				$association = $value;
			}

			if (array_key_exists($field, $this->settings[$model->name])) {
				unset($this->settings[$model->name][$field]);

			} elseif (in_array($field, $this->settings[$model->name])) {
				$this->settings[$model->name] = am(array_diff_assoc($this->settings[$model->name], array($field)));
			}

			if (array_key_exists($field, $this->runtime[$model->name]['fields'])) {
				unset($this->runtime[$model->name]['fields'][$field]);

			} elseif (in_array($field, $this->runtime[$model->name]['fields'])) {
				$this->runtime[$model->name]['fields'] = am(array_diff_assoc($this->runtime[$model->name]['fields'], array($field)));
			}

			if (!is_null($association) && (isset($model->hasMany[$association]) || isset($model->__backAssociation['hasMany'][$association]))) {
				$associations[] = $association;
			}
		}

		if (!empty($associations)) {
			$model->unbindModel(array('hasMany' => $associations), false);
		}
		return true;
	}
}
if (!defined('CAKEPHP_UNIT_TEST_EXECUTION')) {
/**
 * @package	 	cake
 * @subpackage	cake.cake.libs.model.behaviors
 */
	class I18nModel extends AppModel {
		var $name = 'I18nModel';
		var $useTable = 'i18n';
		var $displayField = 'field';
	}
}
?>