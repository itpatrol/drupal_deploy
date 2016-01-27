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

function drupal_deploy_import_blocks($filename = '', $theme_name = ''){
  if(empty($filename)){
    $filename = $menu_name . '.' . 'blocks.export';
    if(empty($menu_name)){
      $filename = 'blocks.export';
    }
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_blocks_items($data, $theme_name);
}

function  drupal_deploy_import_blocks_items($data, $theme_name = ''){

  $result = db_query('SELECT * FROM {role}');
  $roles = array();
  while($role = db_fetch_object($result)){
    $roles[$role->name] = $role->rid;
  }
  
  $formats_result = db_query('SELECT * FROM {filter_formats}');
  $format_names = array();
  while($format = db_fetch_object($formats_result)){
    $format_names[$format->name] = $format->format;
  }

  foreach($data as $item){
    if(!empty($menu_name))
    {
      if($item['theme'] != $theme_name)
      {
        continue;
      }
    }

    $existing_item = db_fetch_array(db_query("SELECT * FROM {blocks} WHERE module = '%s' and theme='%s' AND delta='%s'", $item['module'], $item['theme'], $item['delta']));
    if($existing_item)
    {
      // Update existing block.
      if($item['module'] == 'block'){
        // Update Boxes
        $format = 1;
        if($format_names[$item['format']]){
          $format = $format_names[$item['format']];
        }
        $box = array(
          'bid' => $item['delta'],
          'body' => $item['body'],
          'info' => $item['info'],
          'format' => $format,
        );
        drupal_write_record('boxes', $box, 'bid');
      }
      $item['bid'] = $existing_item['bid'];
      if(drupal_write_record('blocks', $item, 'bid')){
        if(isset($item['roles']))
        {
          db_query("DELETE FROM {blocks_roles} WHERE module = '%s' AND delta = '%s'", $item['module'], $item['delta']);
          foreach($item['roles'] as $role_name)
          {
            if(isset($roles[$role_name]))
            {
              db_query("INSERT INTO {blocks_roles} (rid, module, delta) VALUES (%d, '%s', '%s')", $roles[$role_name], $item['module'], $item['delta']);
            }
          }
        }
      
        $block_list = module_invoke($item['module'], 'block', 'list');
        $block_title = $block_list[$item['delta']]['info'];
        drupal_set_message(t('Block !title updated.', array('!title' => $block_title)));
      }
    }
    else
    {
      if($item['module'] == 'block'){
        // Update Boxes
        $format = 1;
        if(isset($format_names[$item['format']])){
          $format = $format_names[$item['format']];
        }
        $box = array(
          'body' => $item['body'],
          'info' => $item['info'],
          'format' => $format,
        );
        drupal_write_record('boxes', $box);
        $item['delta'] = $box['bid'];
      }
      if(drupal_write_record('blocks', $item))
      {
        if(isset($item['roles']))
        {
          db_query("DELETE FROM {blocks_roles} WHERE module = '%s' AND delta = '%s'", $item['module'], $item['delta']);
          foreach($item['roles'] as $role_name)
          {
            if(isset($roles[$role_name]))
            {
              db_query("INSERT INTO {blocks_roles} (rid, module, delta) VALUES (%d, '%s', '%s')", $roles[$role_name], $item['module'], $item['delta']);
            }
          }
        }
        $block_list = module_invoke($item['module'], 'block', 'list');
        $block_title = $block_list[$item['delta']]['info'];
        drupal_set_message(t('Block !title updated.', array('!title' => $block_title)));
      }
    }
  }
}

function drupal_deploy_export_blocks_fetch_data($theme_name = ''){
  $items = array();
  
  $query = db_select('block', 'b')
    ->fields('b');

  if(is_string($theme_name)){
    $query->condition('b.theme', $theme_name);
  }

  $query->orderBy('b.bid');
  $blocks = $query->execute()->fetchAll();
  
  $format_names = array();
  $filter_formats = db_select('filter_format', 'ff')
    ->fields('ff')
    ->orderBy('ff.format')
    ->execute()
    ->fetchAll();
    
  foreach($filter_formats as $db_item){
    $format_names[$db_item->format] = $db_item->name;
  }
  
  $roles_names = array();
  
  $roles_result = db_select('role', 'rr')
    ->fields('rr')
    ->orderBy('rr.rid')
    ->execute()
    ->fetchAll();
    
  foreach($roles_result as $db_item){
    $roles_names[$db_item->rid] = $db_item->name;
  }

  foreach ($blocks as $db_item) {
    $item = array(
      'module' => $db_item->module,
      'delta' => $db_item->delta,
      'theme' => $db_item->theme,
      'status' => $db_item->status,
      'weight' => $db_item->weight,
      'region' => $db_item->region,
      'custom' => $db_item->custom,
      'visibility' => $db_item->visibility,
      'pages' => $db_item->pages,
      'title' => $db_item->title,
      'cache' => $db_item->cache,
    );
    
    if($db_item->module == 'block')
    {
      $custom = db_select('block_custom', 'bc')
        ->fields('bc')
        ->condition('bc.bid', $db_item->bid)
        ->execute()
        ->fetchObject();
      
      if($custom)
      {
        $item['body'] = $custom->body;
        $item['info'] = $custom->info;
        if(isset($format_names[$custom->format]))
        {
          $item['format'] = $format_names[$custom->format];
        }                
      }
    }
    
    $block_roles = db_select('block_role', 'br')
      ->fields('br')
      ->condition('br.module', $db_item->module)
      ->condition('br.delta', $db_item->delta)
      ->execute()
      ->fetchAll();
    
    $roles = array();

    foreach ($block_roles as $db_role) {
      if(isset($roles_names[$db_role->rid])){
        $roles[] = $roles_names[$db_role->rid];
      }
    }
    if(!empty($roles))
    {
      $item['roles'] = $roles;
    }
    
    $items[] = $item;
  }
  return $items;
}

