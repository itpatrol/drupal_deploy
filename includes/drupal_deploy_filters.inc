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
  db_query("TRUNCATE {filter_format}");
  db_query("TRUNCATE {filter}");
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
    
    if(drupal_write_record('filter_format', $format))
    {
      drupal_set_message(t('Filter !title restored.', array('!title' => $format['name'])));
      if(!empty($item['filters']))
      {
        foreach($item['filters'] as $filter)
        {
          $filter['format'] = $format['format'];
          drupal_write_record('filter', $filter);
        }
      }
    }
  }
}

function drupal_deploy_export_filters_fetch_data()
{
  $items = array();
  $filter_formats = db_select('filter_format', 'ff')
    ->fields('ff')
    ->orderBy('ff.format')
    ->execute()
    ->fetchAll();
    
  foreach($filter_formats as $db_item){
    $item = array(
      'name' => $db_item->name,
      'status' => $db_item->status,
      'weight' => $db_item->weight,
      'cache' => $db_item->cache,
    );
    
    $roles = filter_get_roles_by_format($db_item);

    foreach($roles as $role){
      $item['roles'][] = $role;
    }    
    
    $filters = db_select('filter', 'f')
    ->fields('f')
    ->condition('format', $db_item->format)
    ->orderBy('f.module')
    ->orderBy('f.name')
    ->execute()
    ->fetchAll();
    
    foreach($filters as $filter){
      unset($filter->format);
      $filter->settings = unserialize($filter->settings);
      $item['filters'][] = $filter;
    }

    $items[] = $item;
  }
  return $items;
}