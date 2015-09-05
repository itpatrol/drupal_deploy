<?php
  
function drupal_deploy_export_language($filename = ''){
  $language = drush_get_option('name', 'en');
  $group = drush_get_option('group', 'default');

  $data = drupal_deploy_export_language_fetch_data($language, $group);
  
  if(empty($filename)){
    $filename = $group . '.' .$language . '.language.po';  
  }
  if(file_put_contents($filename, $data)){
    drupal_set_message(t('Exported to file !filename', array('!filename' => $filename)));
  }
}

function drupal_deploy_import_language($filename = ''){
  $language = drush_get_option('name', 'en');
  $group = drush_get_option('group', 'default');
  $mode = drush_get_option('mode', 'add');
  
  if($mode == 'add'){
    $mode = LOCALE_IMPORT_KEEP;
  }else{
    $mode = LOCALE_IMPORT_OVERWRITE;
  }
  
  if(empty($filename)){
    $filename = $group . '.' .$language . '.language.po';  
  }

  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  
  $languages = language_list('language', TRUE);
  if (!isset($languages[$language])) {
    $predefined = _locale_get_predefined_list();
    locale_add_language($language);
    drupal_set_message(t('The language %language has been created.', array('%language' => t($predefined[$language][0]))));
  }

  if ($ret = _locale_import_po($filename, $language, $mode, $group) == FALSE) {
    $variables = array('%filename' => $filename);
    drupal_set_message(t('The translation import of %filename failed.', $variables), 'error');
  }else{
    drupal_set_message(t('Translation imported !lancode.', array('!lancode', $language)), 'success');
  }
}


function drupal_deploy_export_language_fetch_data($language, $group){
  
  $languages = language_list();
  $language = $languages[$language];
  return _locale_export_po_generate($language, _locale_export_get_strings($language, $group));
}
