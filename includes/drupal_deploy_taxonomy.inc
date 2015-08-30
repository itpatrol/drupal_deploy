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

function drupal_deploy_import_taxonomy_items($data, $vocabulary_name = ''){
  foreach($data as $item){
    if(!empty($vocabulary_name))
    {
      if($item['name'] != $vocabulary_name)
      {
        continue;
      }
    }
    
    $terms = $item['terms'];
    unset($item['terms']);
    $item['nodes'] = $item['node_types'];
    unset($item['node_types']);
      
    $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE module='%s' AND name='%s'", $item['module'], $item['name']));
    
    if($stored_vocabulary){
      $item['vid'] = $stored_vocabulary->vid;
    }
    
    $status = taxonomy_save_vocabulary($item);
    if($status == SAVED_UPDATED || $status == SAVED_NEW)
    {
      drupal_set_message(t('Vocabulary !name imported',array('!name' => $item['name'])));
      foreach($terms as $term)
      {
        $term['vid'] = $item['vid'];
        $status = drupal_deploy_save_term($term, $item['vid']);
        if($status == SAVED_UPDATED || $status == SAVED_NEW){
          drupal_set_message(t('Term !name imported',array('!name' => $term['name'])));
        }
      }  
    }
    
  }
}

function drupal_deploy_save_term($term, $vid, $parent = 0){
  $term['vid'] = $vid;
  $term['parent'] = $parent;
  if(is_array($term['synonyms'])){
    $term['synonyms'] = implode("\n", $term['synonyms']);
  }
  
  $status = taxonomy_save_term($term);
  if($status == SAVED_UPDATED || $status == SAVED_NEW)
  {
    if(is_array($term['terms']))
    foreach($term['terms'] as $sub_term)
    {
      $status = drupal_deploy_save_term($sub_term, $vid, $term['tid']);
      if($status == SAVED_UPDATED || $status == SAVED_NEW){
        drupal_set_message(t('Term !name imported',array('!name' => $sub_term['name'])));
      }
    }  
  }
  return $status;
}

function drupal_deploy_export_taxonomy_fetch_data($vocabulary_name = ''){
  $items = array();
  if(empty($vocabulary_name)){
    $result = db_query("SELECT * FROM {vocabulary} ORDER BY name");
  }else{
    $result = db_query("SELECT * FROM {vocabulary} WHERE name='%s'",$vocabulary_name);
  }

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
    while ($term = db_fetch_array($terms_result))
    {
      $term_synonym_result = db_query('SELECT * FROM {term_synonym} WHERE tid=%d',$term['tid']);
      while($term_synonym = db_fetch_object($term_synonym_result)){
        $term['synonyms'][] = $term_synonym->name;
      }
      $terms[$term['tid']] = $term;
      
    }
    $tree = array();
    foreach($terms as $term){
      if($term['parent'] == 0){
        $term['terms'] = _get_term_children($term, $terms);
        $store_term = array(
          'name' => $term['name'],
          'description' => $term['description'],
          'weight' => $term['weight'],
        );
        if(!empty($term['synonyms'])){
          $store_term['synonyms'] = $term['synonyms'];
        }
        if(!empty($term['terms'])){
          $store_term['terms'] = $term['terms'];
        }
        $tree[] = $store_term;
      }
    }
    $item['terms'] = $tree;
    $items[] = $item;    
  }
  
  return $items;
}  

function _get_term_children($parent_term, $terms){
  $return = array();
  foreach($terms as $term){
    if($term['parent'] == $parent_term['tid']){
      $term['terms'] = _get_term_children($term, $terms);
      $store_term = array(
        'name' => $term['name'],
        'description' => $term['description'],
        'weight' => $term['weight'],
      );
      if(!empty($term['synonyms'])){
          $store_term['synonyms'] = $term['synonyms'];
        }
      if(!empty($term['terms'])){
        $store_term['terms'] = $term['terms'];
      }
      $return[] = $store_term;
    }
  }
  return $return;
}