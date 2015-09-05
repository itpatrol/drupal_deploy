<?php
  
function drupal_deploy_export_language($filename = ''){
  $language = drush_get_option('name', 'en');
  $group = drush_get_option('group', 'default');

  $data = drupal_deploy_export_language_fetch_data($language, $group);
  
  if(empty($filename)){
    $filename = $group . '.' .$language . '.language.export';  
  }
  if(file_put_contents($filename, $data)){
    drupal_set_message(t('Exported to file !filename', array('!filename' => $filename)));
  }
}

function drupal_deploy_import_language($filename = ''){
  $language = drush_get_option('name', FALSE);
  
  if(empty($filename)){
    $filename = 'language.export';
    if($language){
      $filename = $language . '.language.export';  
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


function drupal_deploy_export_language_fetch_data($language, $group){
  
  $languages = language_list();
  $language = $languages[$language];
  return _locale_export_po_generate($language, _locale_export_get_strings($language, $group));
}

function drupal_deploy_import_language_items($data){
  foreach($data as $item){
    variable_set($item['name'],$item['value']);
  }
}
