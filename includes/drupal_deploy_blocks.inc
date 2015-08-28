<?php
  
function drupal_deploy_export_blocks($theme_name = '', $filename = ''){
  $items = drupal_deploy_export_blocks_fetch_data($theme_name);
  
  if(empty($filename)){
    $filename = $theme_name . '.' . 'blocks.export';
    if(empty($theme_name)){
      $filename = 'blocks.export';
    }
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_export_blocks_fetch_data($theme_name = ''){
  $items = array();
  if(empty($theme_name)){
    $result = db_query("SELECT * FROM {blocks} WHERE status=%d ORDER BY module,delta", 1);
  }else{
    $result = db_query("SELECT * FROM {blocks} WHERE status=%d AND theme='%s' ORDER BY module,delta", 1, $theme_name);
  }
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'module' => $db_item->module,
      'delta' => $db_item->delta,
      'theme' => $db_item->theme,
      'status' => $db_item->status,
      'weight' => $db_item->weight,
      'region' => $db_item->region,
      'custom' => $db_item->custom,
      'throttle' => $db_item->throttle,
      'visibility' => $db_item->visibility,
      'pages' => $db_item->pages,
      'title' => $db_item->title,
    );
    
    if($db_item->module == 'block' && $db_item->delta == 1)
    {
      $boxes = db_fetch_object(db_query("SELECT * FROM {boxes} WHERE bid=%d", $db_item->delta));  
      if($boxes)
      {
        $item['body'] = $boxes->body;
        $item['info'] = $boxes->info;
        $item['format'] = $boxes->format;                
      }
    }
    $block_roles = db_fetch_object(db_query("SELECT * FROM {blocks_roles} WHERE module='%s' AND delta=%d", $db_item->module, $db_item->delta));
    if($block_roles)
    {
      $item['rid'] = $block_roles->rid;
    }
    
    $items[] = $item;
  }
  return $items;
}

