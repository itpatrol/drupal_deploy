<?php
  
function drupal_deploy_export_nodes($filename = ''){
  $node_type = drush_get_option('type', FALSE);

  if(empty($filename)){
    $filename = 'nodes.export';
    if($node_type){
      $filename = $node_type . '.nodes.export';  
    }
  }
  
  $items = drupal_deploy_export_nodes_fetch_data($node_type, $filename);

  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_nodes($filename = ''){
  
  if(empty($filename)){
    $filename = 'nodes.export';
  }
  if(!is_file($filename)){
    drush_log(dt('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  
  $data = file_get_contents($filename);
  
  ini_set('memory_limit', '1G');
  
  if(!$data = json_decode($data, TRUE)){
    drush_log(dt('Json decode error!'), 'error');
  }
  if(!empty($data['files'])){
    ddi_node_files($data['files'], $filename);
  }
  drupal_deploy_import_nodes_items($data['nodes']);
  
  drupal_deploy_import_book_items($data['book_outlines'], $data['books']);
}

function drupal_deploy_import_book_items($outlines, $books){

  foreach($outlines as $outline){

    $origin = ddi_menu_alias_origin($outline);
    if($origin == $outline){
      drush_log(dt('Path !outline is not created yet.',array('!outline' => $outline )),'error');
      continue;
    }

    $origin = explode('/', $origin);
    if($origin[0] == 'node' && is_numeric($origin[1])){
      $menu_name = 'book-toc-'.$origin[1];
      
      $stored_outline = db_fetch_object(db_query("SELECT * FROM {book} WHERE bid = %d AND nid = %d", $origin[1], $origin[1]));
      if(!$stored_outline){
        
        $menu_item = $books[$outline];
        $menu_item['menu_name'] = $menu_name;
        unset($books[$outline]);

        if(ddi_book_menu_link($menu_item))
        {
          $outline = array(
            'nid' => $origin[1],
            'bid' => $origin[1],
            'mlid' => $menu_item['mlid'],
          );
          drupal_write_record('book', $outline);
        }
        
        drush_log(dt('Create outline !bid', array('!bid' => $origin[1])));
      }
    }
  }

  foreach($books as $link => $book){
    
    if($outline_nid = ddi_get_outline($book['book_link'])){
      
      $book['menu_name'] = 'book-toc-'.$outline_nid;
      
      if(ddi_book_menu_link($book))
      {
        if(!$origin_book_item = db_fetch_object(db_query("SELECT * FROM {book} WHERE nid = %d AND bid = %d and mlid = %d", $book['nid'], $outline_nid, $book['mlid']))){
          $outline = array(
            'nid' => $book['nid'],
            'bid' => $outline_nid,
            'mlid' => $book['mlid'],
          );
          drupal_write_record('book', $outline);
          drush_log(dt('Create outline page !mlid', array('!mlid' => $outline['mlid'])));
        }
      }
    }
  }
}

function ddi_get_outline($outline_link_origin){
  
  $outline_link = ddi_menu_alias_origin($outline_link_origin);
  if($outline_link == $outline_link_origin){
    drush_log(dt('Path !outline is not created yet.',array('!outline' => $outline_link_origin )),'error');
    return FALSE;
  }
  
  $outline_link = explode('/', $book_link);
  $nid = $outline_link[1];
  if(is_numeric($nid)){
    return $nid;
  }
  return FALSE;
}

function ddi_book_menu_link($item){
  
  print_r($item);
          
  $item['link_path'] = ddi_menu_alias_origin($item['link_path']);
  
  $node_link = explode('/', $book['link_path']);
  if($node_link[0] == 'node' && is_numeric($node_link[1])){
    $item['nid'] = $node_link[1];
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

  print_r($item);
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

function drupal_deploy_import_nodes_items($data){
  global $user;
  $user = user_load(1);
  
  foreach($data as $item){
    $item = (object) $item;
    $item->uid = ddi_get_user_id($item->user);
    if($node_saved = ddi_check_node($item)){
      drush_log(dt('Node !title imported already. Update it.', array('!title' => $item->title)),'notice');
      foreach($item as $key => $val){
        if(is_array($val)){
          $item->$key = ddi_nodes_prepare_vars($val);
        }
      }
      ddi_node_update($item, $node_saved);
      drush_log(dt('Node !title imported.', array('!title' => $item->title)),'notice');
    }else{
      foreach($item as $key => $val){
        if(is_array($val)){
          $item->$key = ddi_nodes_prepare_vars($val);
        }
      }
      ddi_node_save($item);
      drush_log(dt('Node !title imported.', array('!title' => $item->title)),'notice');
    }
  }
  if(module_exists('content')) {
    db_query("TRUNCATE {".content_cache_tablename()."}");
  }
}

function drupal_deploy_export_nodes_fetch_data($node_type, $filename){
  global $user;
  
  $items = array();
  
  if($nids = drush_get_option('nid', FALSE)){
    $result = db_query("SELECT * FROM {node} WHERE nid in (%s) ORDER BY nid", $nids);
  }else{
    if($node_type){
      $result = db_query("SELECT * FROM {node} WHERE type='%s' ORDER BY nid",$node_type);
    }else{
      $result = db_query("SELECT * FROM {node} ORDER BY nid");
    }
  }
  $all_books = FALSE;
  
  while ($db_item = db_fetch_object($result)) {
    
    $book = FALSE;
    if(module_exists('book')){
      $book = TRUE;
    }
    
    $revisions_result = db_query("SELECT * FROM {node_revisions} WHERE nid=%d ORDER BY vid", $db_item->nid);
    while ($db_rev_item = db_fetch_object($revisions_result)) {
      $db_rev_item->type = $db_item->type;
      dde_node_load($db_rev_item);
      if($book){
        if(isset($db_rev_item->book)){
          $book = (object) $db_rev_item->book;
          unset($db_rev_item->book);
        }
      }
      dde_node_clean($db_rev_item);
      $db_item->revisions[] =  $db_rev_item;
    }
    
    if(is_object($book)){
      $all_books[$book->bid][] = $book;
    }
    
    $db_item->user = dde_get_user_name($db_item->uid);

    $db_item->comments = NULL;
    
    $comments_result = db_query("SELECT * FROM {comments} WHERE nid=%d AND pid=0 ORDER BY cid", $db_item->nid);
    while ($db_comm_item = db_fetch_object($comments_result)) {
      if($comments = dde_get_sub_comments($db_comm_item)){
        $db_comm_item->comments = $comments;
      }
      dde_clean_comment($db_comm_item);
      
      $db_item->comments[] = $db_comm_item;
    }
    
    unset($db_item->nid);
    unset($db_item->vid);
    unset($db_item->uid);
    unset($db_item->translate);
    unset($db_item->tnid);
    $items['nodes'][] = $db_item;
    drush_log(dt('Node !title exported', array('!title' => $db_item->title)),'notice');
  }
  if($all_books){
    $result_books = array();
    $bids_links = array();
    foreach($all_books as $bid => $books){
      if(!isset($bids_links[$bid])){
        $bids_links[$bid] = dde_menu_alias('node/' . $bid);
      }
      foreach($books as $book){
        $link_path = dde_menu_alias($book->link_path);
        $result_books[$link_path] = array(
          'link_title' => $book->link_title,
          'weight' =>  $book->weight,
          'link_path' => $link_path,
          'hidden' =>  $book->hidden,
          'has_children' =>  $book->has_children,
          'expanded' =>  $book->expanded,
          'options' =>  $book->options,
          'module' =>  $book->module,
          'customized' =>  $book->customized,
          'updated' =>  $book->updated,
          'book_link' => $bids_links[$bid],
        );
        if($book->plid != 0){
          $parent_item = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE mlid=%d", $book->plid));
          $result_books[$link_path]['parent_link_path'] = dde_menu_alias($parent_item->link_path);
        }
      }
    }
    $items['books'] = $result_books;
    foreach($bids_links as $b_link){
      $items['book_outlines'][] = $b_link;
    }
  }
  
  $files = dde_nodes_files_to_export('get');
  if(!empty($files)){
    $export_folder = $filename . '.files';
    if(!is_dir($export_folder)){
      mkdir($export_folder);
    }
    
    $file_directory_path = variable_get('file_directory_path', conf_path() . '/files');
    $file_directory_path_len = strlen($file_directory_path);
    
    foreach($files as $file){
      if(is_file($file->filepath)){
        
        if(0 == strcmp($file_directory_path, substr($file->filepath, 0,$file_directory_path_len))){
          $store_filepath = $export_folder . '/' . substr($file->filepath, $file_directory_path_len + 1);
        }else{
          $store_filepath = $export_folder . '/' . basename($file->filepath);
        }
        
        $info = pathinfo($store_filepath);
        if(!is_dir($info['dirname'])){
          mkdir($info['dirname'], 0750, TRUE); 
        }
        
        copy($file->filepath, $store_filepath);
        $file->stored_filepath = $store_filepath;
        $items['files'][] = $file;
        drush_log(dt('!filename exported', array('!filename' => $file->filename)),'notice');
      }else{
        drush_log(dt('!filename does not exists', array('!filename' => $file->filename)), 'warning');
      }
    }
  }
  return $items;
}

function dde_clean_comment(&$comment){
  unset($comment->nid);
  unset($comment->pid);
  unset($comment->cid); 
  $comment->username = dde_get_user_name($comment->uid);
  unset($comment->uid);
  
  $comment->format = dde_get_format_name($comment->format);
  $comment->status = dde_get_comment_status($comment->status);
}

function dde_get_comment_status($status){
  if($status){
    return  'Not Published';
  }
  return 'Published';
}

function ddi_get_comment_status_id($title){
  if($title == 'Not Published'){
    return 1;
  }
  return 0;
}

function dde_get_sub_comments($comment){
  $result = FALSE;
  $comments_result = db_query("SELECT * FROM {comments} WHERE nid=%d AND pid=%d ORDER BY cid", $comment->nid, $comment->cid);
  while ($db_comm_item = db_fetch_object($comments_result)) {
    if($comments = dde_get_sub_comments($db_comm_item)){
      $db_comm_item->comments = $comments;
    }

    dde_clean_comment($db_comm_item);
    $result[] = $db_comm_item;
  }
  return $result;
}

function dde_node_clean(&$item){
  $item->user = dde_get_user_name($item->uid);
  unset($item->nid);
  unset($item->vid);
  unset($item->tid);
  unset($item->uid);
  
  $item->format = dde_get_format_name($item->format);
  
  $terms = array();
  if(is_array($item->taxonomy)){
    foreach($item->taxonomy as $term){
      $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE vid=%d", $term->vid));
      $tree = explode("\n",dde_get_term_tree($term));
      if(count($tree) == 1){
        $terms[$stored_vocabulary->name][] = $tree[0];
      }else{
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
    }
    $item->taxonomy = $terms;
  }
  
  if(isset($item->forum_tid)){
    $term = db_fetch_object(db_query("SELECT * FROM {term_data} WHERE tid=%d", ($item->forum_tid)));
    $item->forum_term = $term->name;
    unset($item->forum_tid);
  }
  foreach($item as $key => $val){
    if(is_array($val) or is_object($val)){
      $item->$key = dde_nodes_replace_and_clean_vars($val);
    }
  }
  
  $module = $item->type;
  if(module_exists($module)) {
    $function = $module . '_node_deploy';
    if(function_exists($function)){
      $function('export', $item);
    }
  }
  unset($item->tags);
  unset($item->type);
  unset($item->last_comment_timestamp);
  unset($item->comment_count);  
  //tnid
}

function dde_nodes_replace_and_clean_vars($object){
  $is_object = TRUE;
  if(is_array($object)){
    $is_object = FALSE;  
  }
  foreach($object as $key => $val){
    if(is_array($val) or is_object($val)){
      if($is_object){
        $object->$key = dde_nodes_replace_and_clean_vars($val);
      }else{
        $object[$key] = dde_nodes_replace_and_clean_vars($val);
      }
    }else{
      if($key === 0){
        
        continue;
      }
      switch($key){
        case 'fid':
          if(!empty($val)){
            dde_nodes_files_to_export('export', $val);
          }
        case 'nid':
        case 'vid':
          if($is_object){
            unset($object->$key);
          }else{
            unset($object[$key]);
          }
          break;
        case 'uid':
          if($is_object){
            $object->username = dde_get_user_name($val);
            unset($object->$key);
          }else{
            $object['username'] = dde_get_user_name($val);
            unset($object[$key]);
          }
          break;          
      }
    }
  }
  return $object;
}

function dde_get_term_tree($term){
  $result = '';
  $stored_term = db_fetch_object(db_query("SELECT t.tid, t.*, parent FROM {term_data} t LEFT JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.tid=%d", $term->tid));
  if($stored_term->parent){
    $parent_term = db_fetch_object(db_query("SELECT t.tid, t.*, parent FROM {term_data} t LEFT JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.tid=%d", $stored_term->parent));
    $result = $stored_term->name . "\n" . dde_get_term_tree($parent_term);
  }else{
    $result = $stored_term->name;
  }
  return $result;
}

function dde_node_load(&$node){

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



function ddi_nodes_prepare_vars($object){
  $is_object = TRUE;
  if(is_array($object)){
    $is_object = FALSE;  
  }
  foreach($object as $key => $val){
    if(is_array($val) or is_object($val)){
      if($is_object){
        $object->$key = ddi_nodes_prepare_vars($val);
      }else{
        $object[$key] = ddi_nodes_prepare_vars($val);
      }
    }else{
      if($key === 0){
        continue;
      }
      switch($key){
        case 'filepath':
          $file = dde_nodes_files_imported('get', $object);
          if($is_object){
            $object->fid = $file['fid'];
          }else{
            $object['fid'] = $file['fid'];
          }
          break;
        case 'username':
          if($is_object){
            $object->uid = ddi_get_user_id($val);
            unset($object->$key);
          }else{
            $object['uid'] = ddi_get_user_id($val);
            unset($object[$key]);
          }
          break;          
      }
    }
  }
  return $object;  
}

function ddi_check_node($node){
  if($node_saved = db_fetch_object(db_query("SELECT * FROM {node} WHERE title='%s' AND type='%s' AND created='%s' AND uid=%d",$node->title, $node->type, $node->created, $node->uid))){  
    return $node_saved;
  }
  return FALSE;
}

function ddi_node_save($node){
  drupal_write_record('node', $node);
  foreach($node->revisions as $revision){
    $revision = (object) $revision;
    $revision->format = ddi_get_format_id($revision->format);
    $revision->uid = ddi_get_user_id($revision->user);
    $revision->nid = $node->nid;
    
    drupal_write_record('node_revisions', $revision);
    $node->vid = $revision->vid;
    if(!empty($revision->taxonomy)){
      ddi_save_taxonomy($revision, $node->type);
    }
  }
  
  drupal_write_record('node', $node, 'nid');  
  ddi_prepare_node_save($node);
  node_invoke($node, 'insert');
  node_invoke_nodeapi($node, 'insert');
  if(!empty($node->comments)){
    foreach($node->comments as $comment){
      $comment = (object) $comment;
      ddi_comment_save($comment, $node->nid);
    }
  }
  drupal_write_record('node', $node, 'nid');

  _comment_update_node_statistics($node->nid);
}

function ddi_node_update($node, $node_saved){
  $node->nid = $node_saved->nid;
  $node->vid = $node_saved->vid;
  
  
  //Save new revisions if available
  $rev_count = count($node->revisions);
  $revision = (object)$node->revisions[$rev_count - 1];
  $revision->nid = $node->nid;
  $revision->vid = $node->vid;
  $revision->uid = ddi_get_user_id($revision->user);
  
  $revision->format = ddi_get_format_id($revision->format);
  
  drupal_write_record('node_revisions', $revision, 'vid');
  
  
  db_query('DELETE FROM {term_node} WHERE nid=%d AND vid=%d', $node->nid, $node->vid);
  if(module_exists('forum')) {
    db_query('DELETE FROM {forum} WHERE nid=%d AND vid=%d', $node->nid, $node->vid);
  }
  
  if(!empty($revision->taxonomy)){
    ddi_save_taxonomy($revision, $node->type);
  }
  
  unset($node->revisions);
  unset($node->user);
  

  drupal_write_record('node', $node, 'nid');
  
  unset($revision->log);
  unset($revision->timestamp);
  unset($revision->user);
  unset($revision->forum_term);
  unset($revision->taxonomy);

  foreach($revision as $field => $value){
    if($field != 'uid'){
      $node->$field = $value;
    }
  }
  
  if($node->path){
    $node->pathauto_perform_alias = FALSE;
  }
  
  $module = $node->type;
  if(module_exists($module)) {
    $function = $module . '_node_deploy';
    if(function_exists($function)){
      $function('import', $node);
    }
  }

  node_invoke($node, 'update');
  node_invoke_nodeapi($node, 'update');
  
  if(!empty($node->comments)){
    foreach($node->comments as $comment){
      $comment = (object) $comment;
      ddi_comment_save($comment, $node->nid);
    }
  }

  drupal_write_record('node', $node, 'nid');
  
  _comment_update_node_statistics($node->nid);  
  
}

function ddi_save_taxonomy($revision, $nodetype){
    
  foreach($revision->taxonomy as $vocabulary => $terms){
    $vocab_results = db_query("SELECT * FROM {vocabulary} v LEFT JOIN {vocabulary_node_types} vnt ON vnt.vid = v.vid WHERE v.name='%s' AND vnt.type='%s'", $vocabulary, $nodetype);
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
    reset($terms);
    ddi_set_term($terms, '', $vid, $revision);
  }
}

function ddi_prepare_node_save(&$node){
  $revisions = count($node->revisions);
  $revision = (array)$node->revisions[$revisions-1];
  unset($node->revisions);
  unset($node->user);
  
  unset($revision['log']);
  unset($revision['timestamp']);
  unset($revision['user']);
  unset($revision['forum_term']);
  unset($revision['taxonomy']);
  $revision['format'] = ddi_get_format_id($revision['format']);
  
  foreach($revision as $field => $value){
    $node->$field = $value;
  }
  
  if($node->path){
    $node->pathauto_perform_alias = FALSE;
  }
  
  $module = $node->type;
  if(module_exists($module)) {
    $function = $module . '_node_deploy';
    if(function_exists($function)){
      $function('import', $node);
    }
  }
  
}

function ddi_set_term($terms, $parent_term_name, $vid, $revision){
  foreach($terms as $sub_parent_term_name => $sub_terms){
    if(is_array($sub_terms)){
      ddi_set_term($sub_terms, $sub_parent_term_name, $vid, $revision);
    }else{
      $stored_term = FALSE;
      if(empty($parent_term_name)){
        if($stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} WHERE name='%s' AND vid=%d",$sub_terms, $vid))){
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
      }else{
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
}

function ddi_comment_save($comment, $nid, $pid = 0){
  $comment->nid = $nid;
  if(!isset($comment->uid)){
    $comment->uid = ddi_get_user_id($comment->username);
  }
  if(is_string($comment->format)){
    $comment->format = ddi_get_format_id($comment->format);
  }
  $comment->pid = $pid;
  if(is_string($comment->status)){
    $comment->status = ddi_get_comment_status_id($comment->status);
  }
  
  //check if comment already exists.
  if($comment_saved = db_fetch_object(db_query("SELECT * FROM {comments} WHERE nid=%d AND uid=%d AND timestamp=%d AND comment='%s'",$comment->nid, $comment->uid, $comment->timestamp, $comment->comment))){
    drush_log(dt('Comment !comment already saved. Skip it.', array('!comment' => $comment->comment)));
  }else{
    drupal_write_record('comments', $comment);
    if(!empty($comment->comments)){
      foreach($comment->comments as $sub_comment){
        ddi_comment_save((object)$sub_comment, $nid, $comment->cid);
  
      }
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


function dde_nodes_files_to_export($op, $fid = NULL){
  static $files;
  if($op == 'export' && !empty($fid)){
    if(!isset($files[$fid])){
      $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid=%d", $fid));
      $file->username = dde_get_user_name($file->uid);
      unset($file->uid);
      unset($file->fid);      
      $files[$fid] = $file; 
    }
  }
  if($op == 'get'){
    return $files;
  }
}

function dde_nodes_files_imported($op, $file){
  static $files;
  if($op == 'set'){
    if(!isset($files[$file['filepath']])){
      $files[$file['filepath']] = $file;
    }
  }
  if($op == 'get'){
    return $files[$file['filepath']];
  }
}


function ddi_node_files($files, $filename){
  $info = pathinfo($filename);
  $dir = $info['dirname'];
  if(!is_dir($dir)){
    drush_log(dt('File folder !folder does not exists', array('!folder' => $dir)),'error');
    return FALSE;
  }
  foreach($files as $file){
    $file['uid'] = ddi_get_user_id($file['username']);
    if($stored_file = db_fetch_array(db_query("SELECT * FROM {files} WHERE filepath='%s'", $file['filepath']))){
      drush_log(dt('!filename already stored. No changes', array('!filename' => $stored_file['filename'])));
      dde_nodes_files_imported('set', $stored_file);
    }else{
      $info = pathinfo($file['filepath']);
      if(!is_dir($info['dirname'])){
        mkdir($info['dirname'], 0751, TRUE); 
      }
      copy($dir . '/' .$file['stored_filepath'], $file['filepath']);
      
      drupal_write_record('files', $file);
      dde_nodes_files_imported('set', $file);
      
      drush_log(dt('!filename imported.', array('!filename' => $file['filename'])), 'notice');
    }
  }
}
