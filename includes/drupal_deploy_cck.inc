<?php
  
function drupal_deploy_export_cck($filename = ''){
  $items = drupal_deploy_export_cck_fetch_data();
  
  if(empty($filename)){
    $filename = 'cck.export';
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_cck($filename){
  if(empty($filename)){
      $filename = 'cck.export';
  }
  if(!is_file($filename)){
    drupal_set_message(t('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drupal_set_message(t('Json decode error!'), 'error');
  }
  drupal_deploy_import_cck_items($data);
}

function drupal_deploy_import_cck_items($data)
{

  foreach($data['instance'] as $item){
    $existing_item = db_fetch_object(db_query("SELECT * FROM {" . content_instance_tablename() . "} WHERE field_name = '%s'", $item['field_name']));
    if($existing_item)
    {
      if(drupal_write_record(content_instance_tablename(), $item, 'field_name'))
      {
        drupal_set_message(t('Field instance !field_name updated.', array('!field_name' => $item['field_name'])));
      }
    }
    else
    {
      if(drupal_write_record(content_instance_tablename(), $item))
      {
        
        drupal_set_message(t('Field instance !field_name created.', array('!field_name' => $item['field_name'])));
      }
    }
  }

  
  foreach($data['fields'] as $item){  
    $existing_item = db_fetch_array(db_query("SELECT * FROM {" . content_field_tablename() . "} WHERE field_name = '%s'", $item['field_name']));
    if($existing_item)
    {
      $existing_item['db_columns'] = unserialize($existing_item['db_columns']);
      if(drupal_write_record(content_field_tablename(), $item, 'field_name'))
      {
        content_alter_schema((array)$existing_item, (array)$item);
        drupal_set_message(t('Field !field_name updated.', array('!field_name' => $item['field_name'])));
      }
    }
    else
    {
      if(drupal_write_record(content_field_tablename(), $item))
      {
        content_alter_schema(array(), (array)$item);
        drupal_set_message(t('Field !field_name created.', array('!field_name' => $item['field_name'])));
      }
    }
  }
  
  
  content_clear_type_cache(TRUE);
  menu_rebuild();
}

function drupal_deploy_export_cck_fetch_data()
{
  $items = array();
  $result = db_query("SELECT * FROM {" . content_field_tablename() . "} ORDER BY field_name");
  while ($db_item = db_fetch_array($result)) {
    $db_item['global_settings'] = unserialize($db_item['global_settings']);
    $db_item['db_columns'] = unserialize($db_item['db_columns']);
    
    $items['fields'][] = $db_item;
  }

  $result = db_query("SELECT * FROM {" . content_instance_tablename() . "} ORDER BY field_name");
  while ($db_item = db_fetch_array($result)) {
    $db_item['widget_settings'] = unserialize($db_item['widget_settings']);
    $db_item['display_settings'] = unserialize($db_item['display_settings']);
    
    $items['instance'][] = $db_item;
  }

  return $items;
}
