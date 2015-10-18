<?php
  
function drupal_deploy_export_filters($filename = ''){
  $items = drupal_deploy_export_filters_fetch_data();
  
  if(empty($filename)){
    $filename = 'filters.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_filters($filename){
  if(empty($filename)){
      $filename = 'filters.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_filters_items($data);
}

function drupal_deploy_import_filters_items($data)
{
  db_query("TRUNCATE {filter_formats}");
  db_query("TRUNCATE {filters}");
  $result = db_query('SELECT * FROM {role}');
  $roles = array();
  while($role = db_fetch_object($result)){
    $roles[$role->name] = $role->rid;
  }
  foreach($data as $item){
    
    $item_roles_ids = array();
    if(!empty($item['roles'])){
      foreach($item['roles'] as $role_name){
        if(isset($roles[$role_name])){
          $item_roles_ids[] = $roles[$role_name];
        }
      }
    }
    
    $format = array(
      'name' => $item['name'],
      'roles' => ',' . implode(",", $item_roles_ids) . ','
    );
    
    if(drupal_write_record('filter_formats', $format))
    {
      drupal_set_message(t('Filter !title restored.', array('!title' => $format['name'])));
      if(!empty($item['filters']))
      {
        foreach($item['filters'] as $filter)
        {
          $filter['format'] = $format['format'];
          drupal_write_record('filters', $filter);
        }
      }
    }
  }
}

function drupal_deploy_export_filters_fetch_data()
{
  $items = array();
  $result = db_query("SELECT * FROM {filter_formats} ORDER BY format");
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'name' => $db_item->name,
    );
    
    $roles = explode(",", $db_item->roles);
    $rids = array();
    foreach($roles as $role){
      if(!empty($role)){
        $rids[] = $role;
      }
    }
    if(!empty($rids)){
      $roles_result = db_query("SELECT * FROM {role} WHERE rid IN (%s)", implode(",", $rids));
      while ($role = db_fetch_array($roles_result)) {
        $item['roles'][] = $role['name'];
      }
    }
    
    $filters_result = db_query("SELECT * FROM {filters} WHERE format=%d ORDER BY format", $db_item->format);
    while ($filter = db_fetch_array($filters_result)) {
      unset($filter['fid']);
      unset($filter['format']);      
      $item['filters'][] = $filter;
    }

    $items[] = $item;
  }
  return $items;
}