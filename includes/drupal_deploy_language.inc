<?php
  
function drupal_deploy_export_language($filename = ''){
  $items = drupal_deploy_export_language_fetch_data($language);
  
  $language = drush_get_option('name', FALSE);
  
  if(empty($filename)){
    $filename = 'language.export';
    if($language){
      $filename = $language . '.nodes.export';  
    }
    
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_language($filename = ''){
  $language = drush_get_option('name', FALSE);
  
  if(empty($filename)){
    $filename = 'language.export';
    if($language){
      $filename = $language . '.nodes.export';  
    }    
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_language_items($data);
}


function drupal_deploy_export_language_fetch_data($language){
  $groups = module_invoke_all('locale', 'groups');
  print_r($groups);
  
  return $items;
}

function drupal_deploy_import_language_items($data){
  foreach($data as $item){
    variable_set($item['name'],$item['value']);
  }
}
