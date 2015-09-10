<?php
  
function drupal_deploy_export_rules($filename = ''){
  $items = drupal_deploy_export_rules_fetch_data();
  
  if(empty($filename)){
    $filename = 'rules.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_rules($filename){
  if(empty($filename)){
      $filename = 'rules.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_rules_items($data);
}

function drupal_deploy_import_rules_items($data)
{
  foreach($data['sets'] as $item){
    if($existing_item = db_fetch_array(db_query("SELECT * FROM {rules_sets} WHERE name='%s'",$item['name'])))
    {
      drupal_write_record('rules_sets', $item, 'name');
      drush_log(dt('Rules set !name updated', array('!name' => $item['name'])), 'ok');
    }
    else
    {
      drupal_write_record('rules_sets', $item);
      drush_log(dt('Rules set !name created', array('!name' => $item['name'])), 'ok');
    }
  }
  foreach($data['rules'] as $item){
    if($existing_item = db_fetch_array(db_query("SELECT * FROM {rules_rules} WHERE name='%s'",$item['name'])))
    {
      drupal_write_record('rules_rules', $item, 'name');
      drush_log(dt('Rules !name updated', array('!name' => $item['name'])), 'ok');
    }
    else
    {
      drupal_write_record('rules_rules', $item);
      drush_log(dt('Rules !name created', array('!name' => $item['name'])), 'ok');
    }
  }
}

function drupal_deploy_export_rules_fetch_data()
{
  $items = array('sets' => array(), 'rules' => array());
  $result = db_query("SELECT * FROM {rules_sets} ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $db_item->data = unserialize($db_item->data);
    $items['sets'][] = $db_item;
  }

  $result = db_query("SELECT * FROM {rules_rules} ORDER BY name");
  while ($db_item = db_fetch_object($result)) {
    $db_item->data = unserialize($db_item->data);
    $items['rules'][] = $db_item;
  }
  return $items;
}
