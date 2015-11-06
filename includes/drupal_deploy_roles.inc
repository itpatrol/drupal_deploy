<?php
  
function drupal_deploy_export_roles($filename = ''){
  $items = drupal_deploy_export_roles_fetch_data();
  
  if(empty($filename)){
    $filename = 'roles.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_roles($filename){
  if(empty($filename)){
      $filename = 'roles.export';
  }
  if(!is_file($filename)){
    drush_log(dt('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drush_log(dt('Json decode error!'), 'error');
  }
  drupal_deploy_import_roles_items($data);
}

function drupal_deploy_import_roles_items($data)
{
  foreach($data as $item){
    
    $existing_item = db_select('role', 'r')
      ->fields('r')
      ->condition('r.name', $item['name'])
      ->execute()
      ->fetchObject();
    
    if($existing_item)
    {
      $role_permissions = array();
      
      foreach($item['permissions'] as $perm){
        $role_permissions[$perm] = TRUE; 
      }
      user_role_change_permissions($existing_item->rid, $role_permissions) {
      
      drush_log(dt('Role !role permissions updated.', array('!role' => $item['name'])));
      }
    }
    else
    {
      $role = array(
        'name' => $item['name'],
      );
      if(drupal_write_record('role', $role))
      {
        $role_perms = array(
          'rid' => $role['rid'],
          'perm' => implode(", ", $item['permissions']),
        );
        if(drupal_write_record('permission', $role_perms))
        {
          drush_log(dt('Role !role permissions created.', array('!role' => $item['name'])));
        }
      }
    }
  }
}

function drupal_deploy_export_roles_fetch_data()
{
  $items = array();
  $role_names = user_roles();
  $role_permissions = user_role_permissions($role_names);

  foreach($role_names as $rid => $role){
    $item = array(
      'name' => $role,
      'permissions' => array_keys($role_permissions[$rid]),
    );

    $items[] = $item;
  }
  return $items;
}