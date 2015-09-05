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
      'drush dde forum' => dt('Export forum container links to taxonomy tid.'),
      'drush dde cck' => dt('Export cck fields.'),
      'drush dde users' => dt('Export users.'),
      'drush dde imagecache' => dt('Export imagecache presets.'),
      'drush dde nodes --nid=1,2,3' => dt('Export nodes 1,2,3 with comments and revisions.'),
      'drush dde nodes --type=forum' => dt('Export forum nodes with comments and revisions.'),
      'drush dde pathauto' => dt('Export pathauto settings.'),
      'drush dde language --name=ru --group=default' => dt('Export RU language translation.'),
    ),
    'aliases' => array('dde'),
    'arguments' => array(
      'type' => 'The name of export type. Supports menu, blocks, filters, roles, node_types.',
    ),
    'options' => array(
      'file' => 'File to export data. File will be overwritten.',
      'name' => 'Language, Menu or Vocabulary name',
      'theme' => 'Theme name',
      'type' => 'Node type',
      'nid' => 'Node id or ids',
      'group' => 'Translation group name',
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
      'drush ddi forum' => dt('Import forum container links to taxonomy tid.'),
      'drush ddi cck' => dt('Import cck fields.'),
      'drush ddi users' => dt('Import users.'),
      'drush ddi imagecache' => dt('Import imagecache presets.'),
      'drush ddi nodes' => dt('Import nodes with comments and revisions.'),
      'drush ddi pathauto' => dt('Import pathauto settings.'),  
      'drush ddi language --name=ru --mode=replace' => dt('Import RU language translation. Replace existing ones, new ones are added.'),
      'drush ddi language --name=ru --set-default=true' => dt('Import RU language translation. Replace existing ones, new ones are added.'),
          
    ),
    'aliases' => array('ddi'),
    'arguments' => array(
      'type' => 'The name of import type. Supports menu, blocks, filters.',
    ),
    'options' => array(
      'file' => 'File to import data from.',
      'name' => 'Language, Menu or Vocabulary name',
      'theme' => 'Theme name',
      'type' => 'Node type',
      'nid' => 'Node id or ids', 
      'mode' => 'Import language mode. Values: add, replace',
      'group' => 'Translation group name',
      'set-default' => 'Set language as default',
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
    case 'forum':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_forum.inc');
      drupal_deploy_export_forum($filename);
      break;
    case 'cck':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_cck.inc');
      drupal_deploy_export_cck($filename);
      break;
    case 'users':
      include_once(drupal_get_path('module', 'user') .'/user.pages.inc');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_users.inc');
      drupal_deploy_export_users($filename);
      break;
    case 'imagecache':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_imagecache.inc');
      drupal_deploy_export_imagecache($filename);
      break;
    case 'nodes':
      include_once(drupal_get_path('module', 'node') .'/node.pages.inc');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_nodes.inc');
      drupal_deploy_export_nodes($filename);
      break;
    case 'pathauto':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_pathauto.inc');
      drupal_deploy_export_pathauto($filename);
      break;
    case 'language':
      include_once('includes/locale.inc');
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_language.inc');
      drupal_deploy_export_language($filename);
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
    case 'forum':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_forum.inc');
      drupal_deploy_import_forum($filename);
      break;
    case 'cck':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_cck.inc');
      drupal_deploy_import_cck($filename);
      break;  
    case 'users':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_users.inc');
      drupal_deploy_import_users($filename);
      break;
    case 'imagecache':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_imagecache.inc');
      drupal_deploy_import_imagecache($filename);
      break;
    case 'nodes':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_nodes.inc');
      drupal_deploy_import_nodes($filename);
      break;
    case 'pathauto':
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_pathauto.inc');
      drupal_deploy_import_pathauto($filename);
      break;
    case 'language':
      include_once('includes/locale.inc');    
      include_once(drupal_get_path('module', 'drupal_deploy') .'/includes/drupal_deploy_language.inc');
      drupal_deploy_import_language($filename);
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