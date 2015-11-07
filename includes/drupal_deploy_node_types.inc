<?php
  
function drupal_deploy_export_node_types($filename = ''){
  $items = drupal_deploy_export_node_types_fetch_data();
  
  if(empty($filename)){
    $filename = 'node_types.export';
    $type = drush_get_option('type', FALSE);
    if($type){
      $filename = $type . '.node_types.export';  
    }
  }
  drupal_deploy_export_save($items, $filename);
}

function drupal_deploy_import_node_types($filename){
  if(empty($filename)){
      $filename = 'node_types.export';
  }
  if(!is_file($filename)){
    drush_log(dt('File !filename does not exists', array('!filename' => $filename)), 'error');
    return FALSE;
  }
  $data = file_get_contents($filename);
  if(!$data = json_decode($data, TRUE)){
    drush_log(dt('Json decode error!'), 'error');
  }
  drupal_deploy_import_node_types_items($data);
}

function drupal_deploy_import_node_types_items($data)
{
  $not_save_variable = array(
    'type',
    'name',
    'module',
    'has_title',
    'title_label',
    'has_body',
    'body_label',
    'description',
    'help',
    'min_word_count',
    'custom',
    'modified',
    'locked',
  );
  $type = drush_get_option('type', FALSE);
  foreach($data as $item){
    
    if(!empty($type)){
      if($type != $item['type']){
        continue;
      }
    }
    
    $fields = $item['fields'];
    unset($item['fields']);

    print_r($item);    
    $values_options = drupal_deploy_import_get_variables($item);
    print_r($values_options);

    $item = _replace_values($item, $values_options);
    print_r($item);
    exit();
    
    $item = _prepare_item_values_to_save($item, $values_options);
            
    $node_type = (object) $item;
    $status = node_type_save($node_type);
    if($status == SAVED_UPDATED || $status == SAVED_NEW)
    {
      foreach ($item as $key => $value) {
        if(in_array($key, $not_save_variable)){
          continue;
        }
        $variable = $key . '_' . $item['type'];
        if (is_array($value)) {
          $value = array_keys(array_filter($value));
        }
        variable_set($variable, $value);
      }
      drush_log(dt('Node type !title imported.', array('!title' => $item['name'])));
    }
    else{
      drush_log(dt('Node type !title NOT imported.', array('!title' => $item['name'])));
    }
  }
  node_types_rebuild();
  menu_rebuild();
}

function _prepare_item_values_to_save($item, $values_options){
  $result = array_diff($item, $values_options);
  foreach($values_options as $key => $val){
    if(is_array($val)){
      if(array_key_exists('#tree',$val)){
        if($val['#tree']){
          $result[$key] = _prepare_item_values_to_save($item[$key],$values_options[$key]);
        }
        else
        {
          $tmp_arr = _prepare_item_values_to_save($item[$key],$values_options[$key]);
          foreach($tmp_arr as $tkey => $tval){
            $result[$tkey] = $tval;
          }
        }
      }
      else{
        $result[$key] = $item[$key];
      }
    } else{
      $result[$key] = $item[$key];
    }
  }
  if(array_key_exists('#tree',$result)){
    unset($result['#tree']);
  }
  return $result;
}

function _replace_values($item, $values_options){
  print_r($item);
  print_r($values_options);
  if(is_array($values_options)){
    $values_array = FALSE;
    foreach($values_options as $key => $val){
      if(is_array($val)){
        $values_array = TRUE;
        $item[$key] = _replace_values($item[$key],$values_options[$key]);
      }
    }
    if($values_array == FALSE){
      $result = NULL;
      foreach($values_options as $key => $val){
        if(is_array($item)){
          echo "array\n";
          echo " item" .print_r($item);
          echo " value " .$val;
          echo "\n";
          if(is_numeric($val) && $val == 1){
            echo "TRUE?";
            $result = FALSE;
            break;
          }
          foreach($item as $item_val){
            if($item_val == $val){
              $result[] = $key;
            }
          }
        }else{
          echo " item" .$item;
          echo " value " .$val;
          echo " key " .$key;          
          echo "\n";
          if($item == $val){
            return $key;
          }
        }
      }
      if(!$result){
        return $item;
      }
      print_r($result);     
      return $result;
    }
  }
  return $item;
}

function drupal_deploy_export_node_types_fetch_data()
{
  $items = array();
  
  $query = db_select('node_type', 'nt')
    ->fields('nt');
  
  $type = drush_get_option('type', FALSE);
  if(!empty($type)){
    $query->condition('nt.type', $type);
  }
  
  $query->orderBy('nt.type');
  
  $node_type_items = $query->execute()->fetchAll();
  
  foreach($node_type_items as $db_item){

    $item = (array) $db_item;
    $item = array_merge($item, drupal_deploy_export_get_variables($db_item));
    $item['fields'] = drupal_deploy_export_get_fields($db_item->type);
    $items[] = $item;
  }
  return $items;
}

function drupal_deploy_export_get_fields($type){
  $fields = field_info_instances('node', $type);
  foreach($fields as $name => $settings){
    unset($fields[$name]['id']);
    unset($fields[$name]['field_id']);    
    unset($fields[$name]['entity_type']);    
    unset($fields[$name]['bundle']);    
  }
  return $fields;
}

function drupal_deploy_export_get_variables($type){
  global $language;
  
  $language->language = 'en';
  $form_id = 'node_type_form';
  $form_state = array();
  $form = array();
  $form = node_type_form($form, $form_state, $type);
  drupal_prepare_form($form_id, $form, $form_state);
  

  $return = array();

  unset($form['name']);
  unset($form['type']);
  unset($form['description']);
  unset($form['additional_settings']);
  
  unset($form['submission']['title_label']);
  unset($form['submission']['help']);
  unset($form['old_type']);
  unset($form['orig_type']);
  unset($form['base']);
  unset($form['actions']);
  unset($form['module']);
  unset($form['custom']);
  unset($form['modified']);
  unset($form['locked']);
  unset($form['form_id']);
  unset($form['form_build_id']);
  

  $tmp_array = array();
  foreach($form as $key => $val){
    if($key[0] == '#')
    {
      continue;
    }else{
      $return[$key] = _process_form_variable($val);  
    }
  }
  
  return $return;
}

function _process_form_variable($array)
{
  if(isset($array['#type'])){
    switch($array['#type']){
      case 'fieldset':
      case 'item':
        $return = array();
        foreach($array as $key => $val){
          if($key[0] == '#')
          {
            continue;
          }else{
            $return[$key] = _process_form_variable($val);
          }
        }
        return $return;
        break;
      case 'checkbox':
      case 'textarea':
      case 'textfield':
        return $array['#default_value'];
        break;
      case 'radios':
      case 'select':
        return $array['#options'][$array['#default_value']];
        break;
      case 'checkboxes':
        $return = array();
        foreach($array['#default_value'] as $val){
           $return[] =  $array['#options'][$val];
        }
        return $return;
    }
  }
}

function drupal_deploy_import_get_variables($type){
  
  global $language;
  
  $language->language = 'en';
  $form_id = 'node_type_form';
  $form_state = array();
  $form = array();
  $form = node_type_form($form, $form_state, $type);
  drupal_prepare_form($form_id, $form, $form_state);

  $return = array();
  
  unset($form['name']);
  unset($form['type']);
  unset($form['description']);
  unset($form['additional_settings']);
  
  unset($form['submission']['title_label']);
  unset($form['submission']['help']);
  unset($form['old_type']);
  unset($form['orig_type']);
  unset($form['base']);
  unset($form['actions']);
  unset($form['module']);
  unset($form['custom']);
  unset($form['modified']);
  unset($form['locked']);
  unset($form['form_id']);
  unset($form['form_build_id']);

  foreach($form as $key => $val){
    if($key[0] == '#')
    {
      continue;
    }else{
      $return[$key] = _process_form_variable_import($val);  
    }
  }
  
  return $return;
}

function _process_form_variable_import($array)
{
  if(isset($array['#type'])){
    switch($array['#type']){
      case 'fieldset':
      case 'item':
        $return = array();
        if($array['#tree']){
          $return['#tree'] = $array['#tree'];
        }else{
          $return['#tree'] = FALSE;
        }
        foreach($array as $key => $val){
          if($key[0] == '#')
          {
            continue;
          }else{
            $return[$key] = _process_form_variable_import($val);
          }
        }
        return $return;
        break;
      case 'checkbox':
      case 'textarea':
      case 'textfield':
        return TRUE;
        break;
      case 'radios':
      case 'select':
      case 'checkboxes':
        return $array['#options'];
        break;
    }
  }
}
