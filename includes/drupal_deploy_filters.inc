<?php
  
function drupal_deploy_export_filters($filename = ''){
  $items = drupal_deploy_export_filters_fetch_data();
  
  if(empty($filename)){
    $filename = $theme_name . '.' . 'filters.export';
    if(empty($theme_name)){
      $filename = 'filters.export';
    }
  }
  drupal_deploy_export_save($items, $filename);
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