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
      'drush dde menu --name=navigation --file=navigation.file' => dt('Export navigation menu.'),
      'drush dde blocks --theme=garland' => dt('Export blocks.'),
      'drush dde filters' => dt('Export filters.'),
      'drush dde roles' => dt('Export roles.'),
      'drush dde node_types --type=blog' => dt('Export node type blog.'),
      'drush dde taxonomy --name=Forums' => dt('Export taxonomy vocabulary Forums.'),
    ),
    'aliases' => array('dde'),
    'arguments' => array(
      'type' => 'The name of export type. Supports menu, blocks, filters, roles, node_types.',
    ),
    'options' => array(
      'file' => 'File to export data. File will be overwritten.',
      'name' => 'Menu or Vocabulary name',
      'theme' => 'Theme name',
      'type' => 'Node type',
    ),
  );

  $items['deploy-import'] = array(
    'callback' => 'drupal_deploy_import',
    'description' => dt('Import data to database.'),
    'examples' => array(
      'drush ddi menu --name=navigation --file=navigation.file' => dt('Import navigation menu.'),
      'drush ddi blocks' => dt('Import blocks.'),
      'drush ddi filters' => dt('Import filters.'),
      'drush ddi roles' => dt('Import roles.'),
      'drush ddi node_types --type=blog' => dt('Import node type blog.'),
      'drush ddi taxonomy --name=Forums' => dt('Import taxonomy vocabulary Forums.'),      
      
    ),
    'aliases' => array('ddi'),
    'arguments' => array(
      'type' => 'The name of import type. Supports menu, blocks, filters.',
    ),
    'options' => array(
      'file' => 'File to import data from.',
      'name' => 'Menu or Vocabulary name',
      'theme' => 'Theme name',
      'type' => 'Node type',          
    ),
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
function drupal_deploy_export($type = '') {
  $filename = drush_get_option('file');
  
  switch($type){
    case 'menu':
      $menu_name = drush_get_option('name');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_menu.inc');
      drupal_deploy_export_menu($menu_name, $filename);
      break;
    case 'blocks':
      $theme_name = drush_get_option('theme');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_blocks.inc');
      drupal_deploy_export_blocks($theme_name, $filename);
      break;
    case 'filters':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_filters.inc');
      drupal_deploy_export_filters($filename);
      break;
    case 'roles':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_roles.inc');
      drupal_deploy_export_roles($filename);
      break;
    case 'node_types':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_node_types.inc');
      include_once(drupal_get_path('module', 'node') .'/content_types.inc');
      drupal_deploy_export_node_types($filename);
      break;      
    case 'taxonomy':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_taxonomy.inc');
      drupal_deploy_export_taxonomy($filename);
      break;
  }
}

/**
 * Drush callback to perform import.
 */
function drupal_deploy_import($type = '', $option = '') {
  $filename = drush_get_option('file');
    
  switch($type){
    case 'menu':
      $menu_name = drush_get_option('name');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_menu.inc');
      drupal_deploy_import_menu($filename, $menu_name);
      break;
    case 'blocks':
      $theme_name = drush_get_option('theme');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_blocks.inc');
      drupal_deploy_import_blocks($filename, $theme_name);
      break; 
    case 'filters':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_filters.inc');
      drupal_deploy_import_filters($filename);
      break;      
    case 'roles':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_roles.inc');
      drupal_deploy_import_roles($filename);
      break;
    case 'node_types':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_node_types.inc');
      include_once(drupal_get_path('module', 'node') .'/content_types.inc');
      drupal_deploy_import_node_types($filename);
    case 'taxonomy':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_taxonomy.inc');
      drupal_deploy_import_taxonomy($filename);
      break;
  }
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