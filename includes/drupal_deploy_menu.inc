<?php
  
function drupal_deploy_export_menu($menu_name = '', $filename = ''){
  $items = drupal_deploy_export_menu_fetch_data($menu_name);
  
  if(empty($filename)){
    $filename = $menu_name . '.' . 'menu_links.export';
    if(empty($menu_name)){
      $filename = 'menu_links.export';
    }
  }
  drupal_deploy_export_save($items, $filename);
}


function drupal_deploy_export_menu_fetch_data($menu_name = ''){
  $items = array();
  if(empty($menu_name)){
    $result = db_query("SELECT * FROM {menu_links} WHERE customized=%d ORDER BY mlid", 1);
  }else{
    $result = db_query("SELECT * FROM {menu_links} WHERE menu_name='%s' and customized=%d ORDER BY mlid", $menu_name, 1);
  }
  while ($db_item = db_fetch_object($result)) {
    $item = array(
      'menu_name' => $menu_name,
      'weight' => $db_item->weight,
      'link_title' => $db_item->link_title,
      'hidden' => $db_item->hidden,
      'has_children' => $db_item->has_children,
      'expanded' => $db_item->expanded,
      'options' => unserialize($db_item->options),
      'module' => $db_item->module,
      'customized' => 1,
      'updated' => $db_item->updated,
    );
    if($item->plid != 0){
      $parent_item = db_fetch_object(db_query("SELECT * FROM {menu_links} WHERE mlid=%d", $item->plid));
      $item['parent_link'] = $parent_item->link_path;
    }
    
    $items[] = $item;
  }
  return $items;
}

function drupal_deploy_export_save($items, $filename = ''){
  $data = json_encode($items);
  $data = drupal_deploy_export_pretty_json($data);
  if(file_put_contents($filename, $data)){
    drupal_set_message('Exported to file ' . $filename);
  }
}

function drupal_deploy_export_pretty_json($json) {
  $result      = '';
  $pos         = 0;
  $strLen      = strlen($json);
  $indentStr   = '  ';
  $newLine     = "\n";
  $prevChar    = '';
  $outOfQuotes = true;

  for ($i=0; $i<=$strLen; $i++) {
    $char = $json[$i];
    
    if ($char == '"' && $prevChar != '\\') 
    {
      $outOfQuotes = !$outOfQuotes;
    }
    else if(($char == '}' || $char == ']') && $outOfQuotes)
    {
      $result .= $newLine;
      $pos --;
      for ($j = 0; $j < $pos; $j++) {
        $result .= $indentStr;
      }
    }

    $result .= $char;

    if (($char == ',' || $char == '{' || $char == '[') && $outOfQuotes)
    {
      $result .= $newLine;
      if ($char == '{' || $char == '[') {
        $pos ++;
      }

      for ($j = 0; $j < $pos; $j++) {
        $result .= $indentStr;
      }
    }

    $prevChar = $char;
  }

  return $result;
}
