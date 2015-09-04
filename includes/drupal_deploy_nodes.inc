<?php
  
function drupal_deploy_export_nodes($filename = ''){
  $node_type = drush_get_option('type', FALSE);
  
  $items = drupal_deploy_export_nodes_fetch_data($node_type);
  
  if(empty($filename)){
    $filename = 'nodes.export';
    if($node_type){
      $filename = $node_type . '.nodes.export';  
    }
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


function drupal_deploy_export_nodes_fetch_data($node_type){
  global $user;
  
  $items = array();
  
  if($nids = drush_get_option('nid', FALSE)){
    $result = db_query("SELECT * FROM {node} WHERE nid in (%s) ORDER BY title LIMIT 10", $nids);
  }else{
    if($node_type){
      $result = db_query("SELECT * FROM {node} WHERE type='%s' ORDER BY title LIMIT 10",$node_type);
    }else{
      $result = db_query("SELECT * FROM {node} ORDER BY title LIMIT 10");
    }
  }

  while ($db_item = db_fetch_object($result)) {
    
    $revisions_result = db_query("SELECT * FROM {node_revisions} WHERE nid=%d ORDER BY vid", $db_item->nid);
    while ($db_rev_item = db_fetch_object($revisions_result)) {
      $db_rev_item->type = $db_item->type;
      drupal_deploy_export_node_load($db_rev_item);
      drupal_deploy_export_node_clean($db_rev_item);
      $db_item->revisions[] =  $db_rev_item;
    }
    
    $db_item->user = _get_user_name($db_item->uid);

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

    $items[] = $db_item;
  }
  return $items;
}

function _clean_comment(&$comment){
  unset($comment->nid);
  unset($comment->pid);
  unset($comment->cid); 
  $comment->username = _get_user_name($comment->uid);
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

function _get_user_name($uid){
  $user = db_fetch_object(db_query("SELECT * FROM {users} WHERE uid=%d", $uid));
  return $user->name;
}

function _get_user_id($name){
  if($user = db_fetch_object(db_query("SELECT * FROM {users} WHERE name='%s'", $name))){
    return $user->uid;
  }
  return 0;
}


function _get_format_name($format){
  $format = db_fetch_object(db_query("SELECT * FROM {filter_formats} WHERE format=%d", $format));
  return $format->name;
}

function _get_format_id($format){
  if($format = db_fetch_object(db_query("SELECT * FROM {filter_formats} WHERE name='%s'", $format))){
    return $format->format;
  }else{
    return FILTER_FORMAT_DEFAULT;
  }
}

function drupal_deploy_export_node_clean(&$item){
  $item->user = _get_user_name($item->uid);
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

  if ($extra = node_invoke_nodeapi($node,'load')) {
    foreach ($extra as $key => $value) {
      if(!empty($value)){
        $node->$key = $value;  
      }
    }
  }
}


function drupal_deploy_import_nodes_items($data){
  foreach($data as $item){
    $item = (object) $item;
    $item->uid = _get_user_id($item->user);
    ddi_node_save($item);
    exit();
  }
}

function ddi_node_save($node){
  drupal_write_record('node', $node);
  foreach($node->revisions as $revision){
    $revision = (object) $revision;
    $revision->format = _get_format_id($revision->format);
    $revision->uid = _get_user_id($revision->user);
    $revision->nid = $node->nid;
    
    drupal_write_record('node_revisions', $revision);
    $node->vid = $revision->vid;
    if(!empty($revision->taxonomy)){
      foreach($revision->taxonomy as $vocabulary => $terms){
        $vocab_results = db_query("SELECT * FROM {vocabulary} v LEFT JOIN {vocabulary_node_types} vnt ON vnt.vid = v.vid WHERE v.name='%s' AND vnt.type='%s'", $vocabulary, $node->type);
        $vocabularies = array();
        while($stored_vocabulary = db_fetch_object($vocab_results)){
          $vocabularies[] = $stored_vocabulary->vid;
        }
        if(empty($vocabularies)){
          continue;
        }
        if(count($vocabularies) == 1){
          $vid = $vocabularies[0];
        }
        if(count($vocabularies) > 1){
          $term = key($terms);
          foreach($vocabularies as $vid){
            if($stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} WHERE name='%s' AND vid=%d",$term, $vid))){
              break;
            }
          }
        }
       
        echo "VID" . $vid;
        reset($terms);
        ddi_set_term($terms, '', $vid, $revision);
      }
    }
  }
  drupal_write_record('node', $node, 'nid');
  drupal_write_record('node_comment_statistics',$node);
  if(!empty($node->comments)){
    foreach($node->comments as $comment){
      $comment = (object) $comment;
      $comment->nid = $node->nid;
      $comment->uid = _get_user_id($comment->user);
      $comment->format = _get_format_id($comment->format);
      ddi_comment_save($comment);
    }
  }
  print_r($revision);
}

function ddi_set_term($terms, $parent_term_name, $vid, $revision){
  foreach($terms as $sub_parent_term_name => $sub_terms){
    if(is_array($sub_terms)){
      ddi_set_term($sub_terms, $sub_parent_term_name, $vid, $revision);
    }else{
      echo "STORING " . $sub_terms . "\n";
      $stored_term = FALSE;
      $stored_parent_term_results = db_query("SELECT * FROM {term_data} WHERE name='%s' AND vid=%d",$parent_term_name, $vid);
      $parent_terms = array();
      while($stored_parent_term = db_fetch_object($stored_parent_term_results)){
        $parent_terms[] = $stored_parent_term->tid;
      }
      if(empty($parent_terms)){
        continue;
      }
      if(count($parent_terms) == 1){
        $ptid = $parent_terms[0];
        $stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} td LEFT JOIN {term_hierarchy} th ON th.tid = td.tid WHERE name='%s' AND vid=%d AND parent=%d",$sub_terms, $vid, $ptid));
      }
      if(count($parent_terms) > 1){
        foreach($parent_terms as $ptid){
          if($stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} td LEFT JOIN {term_hierarchy} th ON th.tid = td.tid WHERE name='%s' AND vid=%d AND parent=%d",$sub_terms, $vid, $ptid))){
            break;
          }
        }
      }
      if($stored_term){
        $term_node = array(
          'nid' => $revision->nid,
          'vid' => $revision->vid,
          'tid' => $stored_term->tid,
        );
        drupal_write_record('term_node', $term_node);
        if(isset($revision->forum_term)){
          if($revision->forum_term == $stored_term->name){
            drupal_write_record('forum', $term_node);
          }
        }

      }
        
    }
  }
}

function ddi_comment_save($comment, $pid = 0){
  //pid
  $comment->pid = $pid;
  drupal_write_record('comments', $comment);
  echo "Comment saved" . print_r($comment,true);  
  if(!empty($comment->comments)){
    foreach($comment->comments as $sub_comment){
      ddi_comment_save($sub_comment, $comment->cid);

    }
  }
}

function ddi_node_save_revision(&$node, $uid, $update = NULL) {
  $temp_uid = $node->uid;
  $node->uid = $uid;
  if (isset($update)) {
    drupal_write_record('node_revisions', $node, $update);
  }
  else {
    drupal_write_record('node_revisions', $node);
  }
  $node->uid = $temp_uid;
}
