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
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_roles_items($data);
}

function drupal_deploy_import_roles_items($data)
{
  foreach($data as $item){
    $existing_item = db_fetch_object(db_query("SELECT * FROM {role} WHERE name = '%s'", $item['name']));
    if($existing_item)
    {
      $role_perms = array(
        'rid' => $existing_item->rid,
        'perm' => implode(", ", $item['permissions']),
      );
      
      if(drupal_write_record('permission', $role_perms, 'rid'))
      {
        drupal_set_message(t('Role !role permissions updated.', array('!role' => $item['name'])));
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
          drupal_set_message(t('Role !role permissions created.', array('!role' => $item['name'])));
        }
      }
    }
  }
}

function drupal_deploy_export_roles_fetch_data()
{
  $items = array();
   $role_names = user_roles();
  if (is_numeric($rid)) {
    $role_names = array($rid => $role_names[$rid]);
  }
  // Fetch permissions for all roles or the one selected role.
  $role_permissions = user_role_permissions($role_names);
  print_r($role_permissions);
  print_r($role_names);
  exit();
  $result = db_query("SELECT * FROM {role} r INNER JOIN {permission} p ON r.rid = p.rid ORDER BY r.name");
  while ($db_item = db_fetch_object($result)) {
    $permission = explode(", ", $db_item->perm);
    sort($permission, SORT_STRING);
    
    $item = array(
      'name' => $db_item->name,
      'permissions' => $permission,
    );

    $items[] = $item;
  }
  return $items;
}