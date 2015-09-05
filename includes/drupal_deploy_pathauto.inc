<?php
  
function drupal_deploy_export_pathauto($filename = ''){
  $items = drupal_deploy_export_pathauto_fetch_data();
  
  if(empty($filename)){
    $filename = 'pathauto.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_pathauto($filename = ''){
  
  if(empty($filename)){
    $filename = 'pathauto.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_pathauto_items($data);
}


function drupal_deploy_export_pathauto_fetch_data(){
  $items = array();
  
  $result = db_query("SELECT * FROM {variable} WHERE name like 'pathauto%' ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $db_item->value = unserialize($db_item->value);
    $items[] = $db_item;
  }
  return $items;
}

function drupal_deploy_import_pathauto_items($data){
  foreach($data as $item){
    variable_set($item['name'],$item['value']);
  }
}
