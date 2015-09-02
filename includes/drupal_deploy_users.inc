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
  $items = array();
  $result = db_query("SELECT * FROM {users} ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $account = user_load($db_item->uid);
    print_r($user);
    print_r($db_item);
    $data = drupal_deploy_export_get_variables($account);
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

function drupal_deploy_export_get_variables($account){
  
  global $language;
  
  $language->language = 'en';
  $form_id = 'user_edit_form';
  $form_state = array();
  $form = user_edit_form($form_state, $account->uid, (array)$account);
  drupal_prepare_form($form_id, $form, $form_state);

  $return = array();
  
  unset($form['identity']);
  unset($form['submission']);
  unset($form['old_type']);
  unset($form['orig_type']);
  unset($form['module']);
  unset($form['custom']);
  unset($form['modified']);
  unset($form['locked']);
  unset($form['submit']);
  unset($form['reset']);  
  unset($form['delete']);
  unset($form['form_id']);

  $tmp_array = array();
  foreach($form as $key => $val){
    if($key[0] == '#')
    {
      continue;
    }else{
      $return[$key] = _process_form_variable($val);  
    }
  }
  
  
  return $return;
}

function _process_form_variable($array)
{
  if(isset($array['#type'])){
    switch($array['#type']){
      case 'fieldset':
      case 'item':
        $return = array();
        foreach($array as $key => $val){
          if($key[0] == '#')
          {
            continue;
          }else{
            $return[$key] = _process_form_variable($val);
          }
        }
        return $return;
        break;
      case 'checkbox':
      case 'textarea':
      case 'textfield':
        return $array['#default_value'];
        break;
      case 'radios':
      case 'select':
        return $array['#options'][$array['#default_value']];
        break;
      case 'checkboxes':
        $return = array();
        foreach($array['#default_value'] as $val){
           $return[] =  $array['#options'][$val];
        }
        return $return;
    }
  }
}