<?php
  
function drupal_deploy_export_menu($menu_name = '', $filename = ''){
  $items = drupal_deploy_export_menu_fetch_data($menu_name);
  
  if(empty($filename)){
    $filename = $menu_name . '.' . 'menu_links.export';
    if(empty($menu_name)){
      $filename = 'menu_links.export';
    }
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_menu($filename = '', $menu_name = ''){
  if(empty($filename)){
    $filename = $menu_name . '.' . 'menu_links.export';
    if(empty($menu_name)){
      $filename = 'menu_links.export';
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
  drupal_deploy_import_menu_links($data, $menu_name);
}

function drupal_deploy_import_menu_links($data, $menu_name = ''){
  foreach($data as $item){
    if(!empty($menu_name))
    {
      if($item['menu_name'] != $menu_name)
      {
        continue;
      }
    }

    if(ddi_menu_link_save($item)){
      if($item['children']){
        ddi_menu_import_children($item['children'], $item['mlid']);
      }
    }
        
  }
}

function ddi_menu_import_children($children, $plid){
  foreach($children as $item){
    $item['plid'] = $plid;
    if(ddi_menu_link_save($item)){
      if($item['children']){
        ddi_book_import_children($item['children'], $item['mlid'], $bid);
      }
    }
  }
}

function ddi_menu_link_save(&$item){
          
  $item['link_path'] = ddi_menu_alias_origin($item['link_path']);
      
  $existing_item = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE link_path = '%s' and menu_name='%s'", $item['link_path'], $item['menu_name']));
  
  if($existing_item->mlid)
  {
    $item['mlid'] = $existing_item->mlid;
  }
  if(menu_link_save($item))
  {
    drush_log(dt('Menu !link_title  imported', array('!link_title' => $item['link_title'])));
    return TRUE;
  }
  else
  {
    drush_log(dt('Menu !link_title NOT imported', array('!link_title' => $item['link_title'])), 'error');
    return FALSE;
  }
}


function drupal_deploy_export_menu_fetch_data($menu_name = ''){
  $items = array();
  if(empty($menu_name)){
    $result = db_query("SELECT * FROM {menu_links} WHERE plid = %d ORDER BY mlid", 0);
  }else{
    $result = db_query("SELECT * FROM {menu_links} WHERE menu_name='%s' AND plid = %d ORDER BY mlid", $menu_name, 0);
  }
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'menu_name' => $db_item->menu_name,
      'weight' => $db_item->weight,
      'link_title' => $db_item->link_title,
      'link_path' => dde_menu_alias($db_item->link_path),
      'hidden' => $db_item->hidden,
      'has_children' => $db_item->has_children,
      'expanded' => $db_item->expanded,
      'options' => unserialize($db_item->options),
      'module' => $db_item->module,
      'customized' => $db_item->customized,
      'updated' => $db_item->updated,
    );
    if($children = dde_menu_items($db_item->mlid)){
      $item['children'] = $children;
    }    
    
    $items[] = $item;
  }
  return $items;
}

function dde_menu_items($plid){
  $result = db_query("SELECT * FROM {menu_links} WHERE menu_name='%s' and plid = %d ORDER BY mlid", $menu_name, $plid);
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'menu_name' => $db_item->menu_name,
      'weight' => $db_item->weight,
      'link_title' => $db_item->link_title,
      'link_path' => dde_menu_alias($db_item->link_path),
      'hidden' => $db_item->hidden,
      'has_children' => $db_item->has_children,
      'expanded' => $db_item->expanded,
      'options' => unserialize($db_item->options),
      'module' => $db_item->module,
      'customized' => $db_item->customized,
      'updated' => $db_item->updated,
    );
    if($children = dde_menu_items($db_item->mlid)){
      $item['children'] = $children;
    }
    
    $items[] = $item;
  }
  return $items;
}

