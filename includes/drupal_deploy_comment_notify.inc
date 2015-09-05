<?php
  
function drupal_deploy_export_comment_notify($filename = ''){
  $items = drupal_deploy_export_comment_notify_fetch_data();
  
  if(empty($filename)){
    $filename = 'comment_notify.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_comment_notify($filename = ''){
  
  if(empty($filename)){
    $filename = 'comment_notify.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_comment_notify_items($data);
}


function drupal_deploy_export_comment_notify_fetch_data(){
  $items = array();
  
  $result = db_query("SELECT * FROM {variable} WHERE name like 'comment_notify_%' OR name like 'node_notify_%' ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $db_item->value = unserialize($db_item->value);
    $items[] = $db_item;
  }
  return $items;
}

function drupal_deploy_import_comment_notify_items($data){
  foreach($data as $item){
    variable_set($item['name'],$item['value']);
  }
}
