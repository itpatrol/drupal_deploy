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
    
    $user = user_load($db_item->uid);
    $db_item->user = $user->name;

    $db_item->comments = NULL;
    
    $comments_result = db_query("SELECT * FROM {comments} WHERE nid=%d AND pid=0 ORDER BY cid", $db_item->nid);
    while ($db_comm_item = db_fetch_object($comments_result)) {
      if($comments = _get_sub_comments($db_comm_item)){
        $db_comm_item->comments = $comments;
      }
      _clean_comment($db_comm_item);
      
      $db_item->comments[] = $db_comm_item;
    }
    
    unset($db_item->nid);
    unset($db_item->vid);
    unset($db_item->uid);
    unset($db_item->translate);
    unset($db_item->tnid);

  
   // drupal_deploy_export_node_clean($db_item);
    $items[] = $db_item;
  }
  print_r($items);
  return $items;
}

function _clean_comment(&$comment){
  unset($comment->nid);
  unset($comment->pid);
  unset($comment->cid); 
  $user = user_load($comment->uid);
  $comment->username = $user->name;       
  unset($comment->uid);
  
  $comment->format = _get_format_name($comment->format);
  
  if($comment->status){
    $comment->status = 'Not Published';
  }else{
    $comment->status = 'Published';
  }  
}

function _get_sub_comments($comment){
  $result = FALSE;
  $comments_result = db_query("SELECT * FROM {comments} WHERE nid=%d AND pid=%d ORDER BY cid", $comment->nid, $comment->cid);
  while ($db_comm_item = db_fetch_object($comments_result)) {
    if($comments = _get_sub_comments($db_comm_item)){
      $db_comm_item->comments = $comments;
    }

    _clean_comment($db_comm_item);
    $result[] = $db_comm_item;
  }
  return $result;
}

function _get_format_name($format){
  $format = db_fetch_object(db_query("SELECT * FROM {filter_formats} WHERE format=%d", $format));
  return $format->name;
}

function drupal_deploy_export_node_clean(&$item){
  $user = user_load($item->uid);
  $item->user = $user->name;
  unset($item->nid);
  unset($item->vid);
  unset($item->tid);
  unset($item->uid);
  
  $item->format = _get_format_name($item->format);
  
  $terms = array();
  foreach($item->taxonomy as $term){
    $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE vid=%d", $term->vid));
    $tree = explode("\n",_get_term_tree($term));
    $res=array();
    foreach($tree as $term){
      if(empty($res)){
        $res = array($term);
      }else{
        $res = array( $term => $res);
      }
    }
    
    list($key,$value) = each($res);
    if(isset($terms[$stored_vocabulary->name][$key])){
      $terms[$stored_vocabulary->name][$key] = array_merge_recursive($terms[$stored_vocabulary->name][$key],$value);
    }
    else {
      $terms[$stored_vocabulary->name][$key] = $value;
    }
  }
  $item->taxonomy = $terms;
  
  if(isset($item->forum_tid)){
    $term = db_fetch_object(db_query("SELECT * FROM {term_data} WHERE tid=%d", ($item->forum_tid)));
    $item->forum_term = $term->name;
    unset($item->forum_tid);
  }
  
  unset($item->tags);
  unset($item->type);
  unset($item->last_comment_timestamp);
  unset($item->comment_count);  
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
      if(!empty($value)){
        $node->$key = $value;
      }
    }
  }

  if ($extra = dde_node_invoke_nodeapi($node)) {
    foreach ($extra as $key => $value) {
      if(!empty($value)){
        $node->$key = $value;  
      }
    }
  }
}

function dde_node_invoke_nodeapi($node){
  $result = array();
  foreach (module_implements('nodeapi') as $name) {
    $function = $name . '_nodeapi';
    $result[$name] = $function($node, 'load');
  }
  return $result;
}

function drupal_deploy_import_nodes_items($data){
 
}

