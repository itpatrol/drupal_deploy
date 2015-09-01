<?php
  
function drupal_deploy_export_forum($filename = ''){
  $items = drupal_deploy_export_forum_fetch_data($vocabulary_name);
  
  if(empty($filename)){
    $filename = 'forum.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_forum($filename = ''){
  
  if(empty($filename)){
    $filename = 'forum.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_forum_items($data);
}


function drupal_deploy_export_forum_fetch_data(){
  $items = array();
  
  $terms = variable_get('forum_containers', FALSE);
  
  $result = db_query("SELECT td.name, v.name as v_name FROM {term_data} td LEFT JOIN {vocabulary} v ON v.vid = td.vid WHERE td.tid IN (%s)", implode(",", $terms));

  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'name' => $db_item->name,
      'vocabulary' => $db_item->v_name,
    );

    $items[] = $item;    
  }
  
  return $items;
}

function drupal_deploy_import_forum_items($data){
  $forum_containers = array();
  foreach($data as $item){

    $stored_vocabulary = db_fetch_object(db_query("SELECT * FROM {vocabulary} WHERE  name='%s'", $item['vocabulary']));
    if($stored_vocabulary){
      $stored_term = db_fetch_object(db_query("SELECT * FROM {term_data} WHERE name='%s' AND vid=%d", $item['name'], $stored_vocabulary->vid));
      
      if($stored_term){
        $forum_containers[] = $stored_term->tid;
        drupal_set_message(t('Container !name imported',array('!name' => $stored_term->name)));
      }
    }
  }
  variable_set('forum_containers', $forum_containers);
}