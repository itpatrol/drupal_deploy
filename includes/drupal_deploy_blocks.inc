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
        $box = array(
          'bid' => $item['delta'],
          'body' => $item['body'],
          'info' => $item['info'],
          'format' => $item['format'],
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
        $box = array(
          'body' => $item['body'],
          'info' => $item['info'],
          'format' => $item['format'],
        );
        drupal_write_record('boxes', $box);
        $item['delta'] = $box['bid'];
      }
      if(drupal_write_record('blocks', $item))
      {
        if(isset($item['rid']))
        {
          db_query("DELETE FROM {blocks_roles} WHERE module = '%s' AND delta = '%s'", $item['module'], $item['delta']);
          foreach($item['rid'] as $rid)
          {
            db_query("INSERT INTO {blocks_roles} (rid, module, delta) VALUES (%d, '%s', '%s')", $rid, $item['module'], $item['delta']);
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
  if(empty($theme_name)){
    $result = db_query("SELECT * FROM {blocks} WHERE status=%d ORDER BY module,delta", 1);
  }else{
    $result = db_query("SELECT * FROM {blocks} WHERE status=%d AND theme='%s' ORDER BY module,delta", 1, $theme_name);
  }

  $roles_result = db_query('SELECT * FROM {role}');
  $roles_names = array();
  while($role = db_fetch_object($roles_result)){
    $roles_names[$role->rid] = $role->name;
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
    
    if($db_item->module == 'block')
    {
      $boxes = db_fetch_object(db_query("SELECT * FROM {boxes} WHERE bid=%d", $db_item->delta));  
      if($boxes)
      {
        $item['body'] = $boxes->body;
        $item['info'] = $boxes->info;
        $item['format'] = $boxes->format;                
      }
    }

    $result_roles = db_query("SELECT * FROM {blocks_roles} WHERE module='%s' AND delta='%s'", $db_item->module, $db_item->delta);
    $roles = array();

    while($db_role = db_fetch_object($result_roles)){
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

