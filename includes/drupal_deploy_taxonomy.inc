<?php
  
function drupal_deploy_export_taxonomy($filename = ''){
  $vocabulary_name = drush_get_option('name', FALSE);
  $items = drupal_deploy_export_taxonomy_fetch_data($vocabulary_name);
  
   if(empty($filename)){
    $filename = 'taxonomy.export';
    
    if($vocabulary_name){
      $filename = $vocabulary_name . '.taxonomy.export';  
    }
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_taxonomy($filename = ''){
  $vocabulary_name = drush_get_option('name', FALSE);
  
  if(empty($filename)){
    $filename = 'taxonomy.export';
    if($vocabulary_name){
      $filename = $vocabulary_name . '.' . 'taxonomy.export';
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
  drupal_deploy_import_taxonomy_items($data, $vocabulary_name);
}

function drupal_deploy_export_taxonomy_fetch_data($vocabulary_name = ''){
  $items = array();
  if(empty($vocabulary_name)){
    $result = db_query("SELECT * FROM {vocabulary} ORDER BY name");
  }else{
    $result = db_query("SELECT * FROM {vocabulary} WHERE name='%s'",$vocabulary_name);
  }
  /*
            vid: 11
       name: Newsletter
description: NULL
       help:
  relations: 0
  hierarchy: 1
   multiple: 0
   required: 1
     weight: 0
     module: simplenews
       tags: 0
  */
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'name' => $db_item->name,
      'description' => $db_item->description,
      'help' => $db_item->help,
      'relations' => $db_item->relations,
      'hierarchy' => $db_item->hierarchy,
      'multiple' => $db_item->multiple,
      'required' => $db_item->required,
      'weight' => $db_item->weight,
      'module' => $db_item->module,
      'tags' => $db_item->tags,
    );
    $vocabulary_node_types_result = db_query('SELECT * FROM {vocabulary_node_types} WHERE vid=%d',$db_item->vid);
    while($vocabulary_node_type = db_fetch_object($vocabulary_node_types_result)){
      $item['node_types'][] = $vocabulary_node_type->type;
    }
    $terms = array();
    $terms_result = db_query(db_rewrite_sql('SELECT t.tid, t.*, parent FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.vid = %d ORDER BY tid', 't', 'tid'), $db_item->vid);
    while ($term = db_fetch_array($terms_result)) {
      if($term['parent'] == 0){
        $terms[$term['tid']] = $term;
      }else{
        $term[$term['parent']]['terms'][] = $term;
      }
    }
    print_r($terms);
    $items[] = $item;    
  }
  return $items;
}  
