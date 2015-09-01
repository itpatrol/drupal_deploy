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
  drupal_deploy_import_forum_items($data, $vocabulary_name);
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