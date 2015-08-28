<?php
  
/**
 * Implementation of hook_drush_command().
 */
function drupal_deploy_drush_command() {
  $items = array();

  $items['deploy-export'] = array(
    'callback' => 'drupal_deploy_export',
    'description' => dt('Export data from database.'),
    'examples' => array(
      'drush deploy-export menu navigation navigation.export' => dt('Export navigation menu from database.'),
    ),
    'aliases' => array('dde'),
  );

  $items['deploy-import'] = array(
    'callback' => 'drupal_deploy_import',
    'description' => dt('Import data to database.'),
    'examples' => array(
      'drush deploy-import menu navigation' => dt('Import navigation menu to database.'),
    ),
    'aliases' => array('ddi'),
  );

  $items['deploy-sync'] = array(
    'callback' => 'drupal_deploy_sync',
    'description' => dt('Sync database state from files.'),
    'examples' => array(
      'drush deploy-sync menu' => dt('Import menu changes from file storage to database.'),
    ),
    'aliases' => array('dds'),
  );

  $items['deploy-revert'] = array(
    'callback' => 'drupal_deploy_revert',
    'description' => dt('Sync database state from files.'),
    'examples' => array(
      'drush deploy-revert menu TIMESTAMP' => dt('Revert changes back to specified timestamp.'),
    ),
    'aliases' => array('dds'),
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function drupal_deploy_drush_help($section) {
  switch ($section) {
    case 'drush:deploy-export':
      return dt('Export data from database.');
    case 'drush:deploy-import':
      return dt('Import navigation menu to database.');
    case 'drush:deploy-sync':
      return dt('Sync database state from files.');
    case 'drush:deploy-revert':
      return dt('Sync database state from files.');
  }
}

/**
 * Drush callback to perform export.
 */
function drupal_deploy_export() {
  $args = func_get_args();
  switch($args[0]){
    case 'menu':
      $menu_name = '';
      $dir = '';
      if(isset($args[1])){
        $menu_name = $args[1];
      }
      if(isset($args[2])){
        $dir = $args[2];
      }
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_menu.inc');
      drupal_deploy_export_menu($menu_name, $dir);
  }
}

/**
 * Drush callback to perform import.
 */
function drupal_deploy_import() {
  $args = func_get_args();
}

/**
 * Drush callback to perform sync.
 */
function drupal_deploy_sync() {
  $args = func_get_args();
}

/**
 * Drush callback to perform revert.
 */
function drupal_deploy_revert() {
  $args = func_get_args();
}