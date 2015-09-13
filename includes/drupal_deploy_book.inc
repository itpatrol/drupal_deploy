<?php
  
function drupal_deploy_export_book($filename = ''){
  
  $nid = drush_get_option('nid', FALSE);
  
  if(empty($nid)){
    $result = db_query("SELECT * FROM {book} WHERE nid = bid ORDER BY nid");
    while ($db_item = db_fetch_object($result)) {
      $nid = $db_item->nid;
      $items = drupal_deploy_export_book_fetch_data($nid);
      $filename = $nid . '.' . 'book.export';
      drupal_deploy_export_save($items, $filename); 
    }
  }else{
    $items = drupal_deploy_export_book_fetch_data($nid);
    $filename = $nid . '.' . 'book.export';
    drupal_deploy_export_save($items, $filename); 
  }
}

function drupal_deploy_import_book($filename = ''){
  if(empty($filename)){
    $filename = 'book.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_book_links($data, $book_name);
}

function drupal_deploy_import_book_links($data, $book_name = ''){
  foreach($data as $item){
    $item['link_path'] = ddi_menu_alias_origin($item['link_path']);
    if(!empty($book_name))
    {
      if($item['book_name'] != $book_name)
      {
        continue;
      }
    }

    if(isset($item['parent_link_path']))
    {
      $item['parent_link_path'] = ddi_menu_alias_origin($item['parent_link_path']);
      
      $parent = db_fetch_object(db_query("SELECT * FROM {book_links} WHERE link_path = '%s'", $item['parent_link_path']));
      
      if($parent->mlid)
      {
        $item['plid'] = $parent->mlid;
      }
    }
    
    $existing_item = db_fetch_object(db_query("SELECT * FROM {book_links} WHERE link_path = '%s' and book_name='%s'", $item['link_path'], $item['book_name']));
    
    if($existing_item->mlid)
    {
      $item['mlid'] = $existing_item->mlid;
    }
    
    if(book_link_save($item))
    {
      drupal_set_message(t('book !link_title  imported', array('!link_title' => $item['link_title'])));
    }
    else
    {
      drupal_set_message(t('book !link_title NOT imported', array('!link_title' => $item['link_title'])), 'error');
    }
  }
}

function drupal_deploy_export_book_fetch_data($nid){
  $items = array();
  
  $menu_name = 'book-toc-' . $nid;
  
  $items = dde_book_menu_items($menu_name, 0);
  
  return $items;
}

function dde_book_menu_items($menu_name, $plid){
  $result = db_query("SELECT * FROM {menu_links} WHERE menu_name='%s' and plid = %d ORDER BY mlid", $menu_name, $plid);
  while ($db_item = db_fetch_object($result)) {
    $item = array(
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
    if($childred = dde_book_menu_items($menu_name, $db_item->mlid)){
      $item['childred'] = $childred;
    }
    
    $items[] = $item;
  }
  return $items;
}

