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
        print_r($term_synonym);
        $term['synonyms'][] = $term_synonym->name;
      }
      $terms[$term['tid']] = $term;
      
    }
    $tree = array();
    print_r($terms);
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