<?php
  
function drupal_deploy_export_nodes($filename = ''){
  $items = drupal_deploy_export_nodes_fetch_data($vocabulary_name);
  
  if(empty($filename)){
    $filename = 'nodes.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_nodes($filename = ''){
  
  if(empty($filename)){
    $filename = 'nodes.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_nodes_items($data);
}


function drupal_deploy_export_nodes_fetch_data(){
  global $user;
  
  $items = array();
  if($nids = drush_get_option('nid', FALSE)){
    $result = db_query("SELECT * FROM {node} WHERE nid in (%s) ORDER BY title LIMIT 10", $nids);
  }else{
    $result = db_query("SELECT * FROM {node} ORDER BY title LIMIT 10");
  }

  while ($db_item = db_fetch_object($result)) {
//    drupal_deploy_export_node_load($db_item);
    
    $revisions_result = db_query("SELECT * FROM {node_revisions} WHERE nid=%d ORDER BY vid", $db_item->nid);
    while ($db_rev_item = db_fetch_object($revisions_result)) {
      $db_rev_item->type = $db_item->type;
      drupal_deploy_export_node_load($db_rev_item);
      drupal_deploy_export_node_clean($db_rev_item);
      $db_item->revisions[] =  $db_rev_item;
    }
   // drupal_deploy_export_node_clean($db_item);
    $items[] = $db_item;
  }
  print_r($items);
  return $items;
}

function drupal_deploy_export_node_clean(&$item){
  $user = user_load($item->uid);
  $item->user = $user->name;
  unset($item->nid);
  unset($item->vid);
  unset($item->uid);
  
  $format = db_fetch_object(db_query("SELECT * FROM {filter_formats} WHERE format=%d", $item->format));
  $item->format = $format->name;
  
  $terms = array();
  foreach($item->taxonomy as $term){
    $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE vid=%d", $term->vid));
    $tree = explode("\n",_get_term_tree($term));
    $res=array();
    foreach($tree as $term){
      if(empty($res)){
        $res = array($term);
      }else{
        $res[$term] = $res;
      }
    }
    
    $terms[$stored_vocabulary->name][] = $res;
  }
  $item->taxonomy = $terms;
  
  //tnid
}

function _get_term_tree($term){
  $result = '';
  $stored_term = db_fetch_object(db_query("SELECT t.tid, t.*, parent FROM {term_data} t LEFT JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.tid=%d", $term->tid));
  if($stored_term->parent){
    $parent_term = db_fetch_object(db_query("SELECT t.tid, t.*, parent FROM {term_data} t LEFT JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.tid=%d", $stored_term->parent));
    $result = $stored_term->name . "\n" . _get_term_tree($parent_term);
  }else{
    $result = $stored_term->name;
  }
  return $result;
}

function drupal_deploy_export_node_load(&$node){
  if ($extra = node_invoke($node, 'load')) {
    foreach ($extra as $key => $value) {
      $node->$key = $value;
    }
  }

  if ($extra = node_invoke_nodeapi($node, 'load')) {
    foreach ($extra as $key => $value) {
      $node->$key = $value;
    }
  }
}

function drupal_deploy_import_nodes_items($data){
 
}

