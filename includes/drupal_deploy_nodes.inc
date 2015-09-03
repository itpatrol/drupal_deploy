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
  if ($user->uid != 1) {
    drush_log('Switching to uid=1 now', 'notice');
    $user = user_load(1);
  }
  
  $items = array();
  $result = db_query("SELECT * FROM {node} ORDER BY title LIMIT 10");
  while ($db_item = db_fetch_object($result)) {
    $data = drupal_deploy_export_get_node_forms($db_item->nid);
  }
   
  return $items;
}

function drupal_deploy_import_nodes_items($data){
 
}

function drupal_deploy_export_get_node_forms($nid){
  global $language;
  $language->language = 'en';
    
  $node = node_load($nid);
  print_r($node);
  $form_state = array();
  $function = $node->type . '_form'; 
  $form_id = $node->type . '_node_form';
  $form = $function($form_state, $node);
  drupal_prepare_form($form_id, $form, $form_state);
  $result = drupal_deploy_export_process_form($form);
  print_r($result);
}
