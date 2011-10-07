<?php

/**
 * Commerce checkout pane form builder callback.
 */
function commerce_registration_information_checkout_form($form, &$form_state, $checkout_pane, $order) {
  global $user;
  $show_form = FALSE;
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  if (empty($form)) {
    $form = array();
  }
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_commerce_product:' . $type . '_status', 0);
    if ($can_register == 1) {
      $show_form = TRUE;
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $label = $i+1;
        $form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i] = array(
          '#type' => 'fieldset',
          '#title' => $product->title->value() . ' #' . $label,
          '#collapsible' => TRUE,
        );
        $entity = entity_get_controller('registration')->create(array('type' => variable_get('registration_commerce_product:' . $type . '_bundle', '')));
        $entity->bundle = $entity->type;
        $entity->entity_type = 'commerce_product';
        $entity->entity_bundle = $product->type->value();
        $entity->eid = $product->product_id->value();
        $entity->status = 0;
        $entity->created = REQUEST_TIME;
        $entity->updated = REQUEST_TIME;
        $entity->author_uid = $user->uid;
        field_attach_form('registration', $entity, $form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i], $form_state);
        $fields = &$form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i];
        foreach ($fields as $key => $data) {
          if (is_array($data) && substr($key, 0, 1) != '#') {
            $fields[$key]['#parents'] = array('prod-' . $product->sku->value(), 'prod-' . $product->sku->value() . '-reg-' . $i);
          }
        }
        $form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i]['entity-' . $i] = array(
          '#type' => 'value',
          '#value' => $entity,
        );
      }
    }
  }
  if (!$show_form) {
    // no register enabled products, so go to next step of checkout process
  }
  return $form;
}

/**
 * Commerce checkout pane form validation callback.
 */
function commerce_registration_information_checkout_form_validate($form, &$form_state, $checkout_pane, $order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_commerce_product:' . $type . '_status', 0);
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $form_state['values']['entity-' . $i];
        field_attach_form_validate('registration', $entity, $form['registration_information']['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i], $form_state);
      }
    }
  }
  return TRUE;
}

/**
 * Commerce checkout pane form submit callback.
 */
function commerce_registration_information_checkout_form_submit($form, &$form_state, $checkout_pane, $order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_commerce_product:' . $type . '_status', 0);
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $form_state['values']['entity-' . $i];
        $entity->save();
        field_attach_submit('registration', $entity, $form['registration_information']['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i], $form_state);
      }
    }
  }
}

/**
 * Commerce checkout pane review callback.
 */
function commerce_registration_information_review($form, $form_state, $checkout_pane, $order) {
}