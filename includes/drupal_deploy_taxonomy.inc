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
    drush_log(dt('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drush_log(dt('Json decode error!'), 'error');
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

    if(is_array($item['node_types'])){
      foreach($item['node_types'] as $type){
        $item['nodes'][$type] = TRUE;
      }
    }
    unset($item['node_types']);
      
    $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE module='%s' AND name='%s'", $item['module'], $item['name']));
    
    if($stored_vocabulary){
      $item['vid'] = $stored_vocabulary->vid;
    }
    $status = taxonomy_save_vocabulary($item);
    if($status == SAVED_UPDATED || $status == SAVED_NEW)
    {
      drush_log(dt('Vocabulary !name imported',array('!name' => $item['name'])), 'ok');
      if(is_array($terms)){
        foreach($terms as $term)
        {
          $term['vid'] = $item['vid'];
          $status = drupal_deploy_save_term($term, $item['vid']);
          if($status == SAVED_UPDATED || $status == SAVED_NEW){
            drush_log(dt('Term !name imported',array('!name' => $term['name'])), 'ok');
          }
        }
      }else{
        drush_log(dt('Vocabulary !name does not have terms',array('!name' => $item['name'])), 'warning');
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
  
  $stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} td LEFT JOIN {term_hierarchy} th ON th.tid = td.tid WHERE name='%s' AND vid=%d  AND parent = %d", $term['name'], $vid, $parent));
  if($stored_term){
    $term['tid'] = $stored_term->tid;
  }
  
  if(isset($term['path_alias'])){
    $term['pathauto_perform_alias'] = FALSE;
  }
  
  $status = taxonomy_save_term($term);
  if($status == SAVED_UPDATED || $status == SAVED_NEW)
  {
    if(is_array($term['terms']))
    foreach($term['terms'] as $sub_term)
    {
      $status = drupal_deploy_save_term($sub_term, $vid, $term['tid']);
      if($status == SAVED_UPDATED || $status == SAVED_NEW){
        drush_log(dt('Term !name imported',array('!name' => $sub_term['name'])), 'ok');
      }
    }
    if(isset($term['path_alias'])){
      path_set_alias('taxonomy/term/' . $term['tid'], $term['path_alias']);
    }
  }
  return $status;
}

function drupal_deploy_export_taxonomy_fetch_data($vocabulary_name = ''){
  $items = array();
  
  $query = db_select('taxonomy_vocabulary', 'tv')
    ->fields('tv');
  
  if(!empty($vocabulary_name)){
    $query->condition('tv.machine_name', $vocabulary_name);
  }

  $query->orderBy('tv.name');
  
  $vocabularies = $query->execute()->fetchAll();
  
  foreach($vocabularies as $db_item){
  
    $vocabulary = taxonomy_vocabulary_machine_name_load($db_item->machine_name);
    
    print_r($vocabulary);
    $tree = taxonomy_get_tree($vocabulary->vid);
    print_r($tree);
    unset($vocabulary['vid']);
    
    exit();
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
        if($alias = db_fetch_object(db_query("SELECT * FROM {url_alias} WHERE src = '%s'", 'taxonomy/term/' . $term['tid']))){
          $store_term['path_alias'] = $alias->dst;
        }
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
      if($alias = db_fetch_object(db_query("SELECT * FROM {url_alias} WHERE src = '%s'", 'taxonomy/term/' . $term['tid']))){
        $store_term['path_alias'] = $alias->dst;
      }
      
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