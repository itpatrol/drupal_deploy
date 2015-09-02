<?php
  
function drupal_deploy_export_users($filename = ''){
  $items = drupal_deploy_export_users_fetch_data();
  
  if(empty($filename)){
    $filename = 'users.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_users($filename){
  if(empty($filename)){
      $filename = 'users.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_users_items($data);
}

function drupal_deploy_import_users_items($data)
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
          'rid' => $role->rid,
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

function drupal_deploy_export_users_fetch_data()
{
  
  global $user;
  if ($user->uid != 1) {
    drush_log('dde users really only works as user #1. Use the "drush -u 1 " option on the commandline, or mysterious things may fail due to permissions during the node creation process. User was ' . $user->uid . ' - switching to uid1 now', 'notice');
    $user = user_load(1);
  }
  
  $items = array();
  $result = db_query("SELECT * FROM {users} ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $account = user_load($db_item->uid);
    print_r($user);
    print_r($db_item);
    $data = drupal_deploy_export_get_user_forms($account);
    print_r($data);
    $item = array(
      'name' => $db_item->name,
      'permissions' => $permission,
    );

    $items[] = $item;
  }
    exit();  
  return $items;
}

function drupal_deploy_export_get_user_forms($account){
  $groups = array();
  foreach (module_implements('user') as $module) {
    // $edit and $account need to be passed by reference.
    $function = $module . '_user';
    if (function_exists($function) && ($data = $function('form', (array)$account, $account, 'account'))) {
      $groups = array_merge_recursive($data, $groups);
      
    }
  }
  return $groups;
}
