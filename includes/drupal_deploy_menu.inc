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
    $item['link_path'] = ddi_menu_alias_origin($item['link_path']);
    if(!empty($menu_name))
    {
      if($item['menu_name'] != $menu_name)
      {
        continue;
      }
    }

    if(isset($item['parent_link_path']))
    {
      $item['parent_link_path'] = ddi_menu_alias_origin($item['parent_link_path']);
      
      $parent = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE link_path = '%s'", $item['parent_link_path']));
      
      if($parent->mlid)
      {
        $item['plid'] = $parent->mlid;
      }
    }
    
    $existing_item = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE link_path = '%s' and menu_name='%s'", $item['link_path'], $item['menu_name']));
    
    if($existing_item->mlid)
    {
      $item['mlid'] = $existing_item->mlid;
    }
    
    if(menu_link_save($item))
    {
      drupal_set_message(t('Menu !link_title  imported', array('!link_title' => $item['link_title'])));
    }
    else
    {
      drupal_set_message(t('Menu !link_title NOT imported', array('!link_title' => $item['link_title'])), 'error');
    }
  }
}
function dde_menu_alias($path){
  if($alias = db_fetch_object(db_query("SELECT * FROM {url_alias} WHERE src = '%s'", $path))){
    return $alias->dst;
  }
  return $path;
}

function ddi_menu_alias_origin($path){
  if($alias = db_fetch_object(db_query("SELECT * FROM {url_alias} WHERE dst = '%s'", $path))){
    return $alias->src;
  }
  return $path;   
}

function drupal_deploy_export_menu_fetch_data($menu_name = ''){
  $items = array();
  if(empty($menu_name)){
    $result = db_query("SELECT * FROM {menu_links} WHERE customized=%d ORDER BY mlid", 1);
  }else{
    $result = db_query("SELECT * FROM {menu_links} WHERE menu_name='%s' and customized=%d ORDER BY mlid", $menu_name, 1);
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
      'customized' => 1,
      'updated' => $db_item->updated,
    );
    if($db_item->plid != 0){
      $parent_item = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE mlid=%d", $db_item->plid));
      $item['parent_link_path'] = dde_menu_alias($parent_item->link_path);
    }
    
    $items[] = $item;
  }
  return $items;
}

