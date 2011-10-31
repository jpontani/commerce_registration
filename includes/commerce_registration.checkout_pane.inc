<?php
/**
 * @file
 * Checkout pane callback functions.
 *
 * @todo Don't manually create a registration entity, use the function from
 *   Entity Registrations to create the new registration.
 */

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
  $find_values = !empty($order->data['register_entities']);
  if (!$find_values) {
    $order->data['register_entities'] = array();
  }
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = registration_entity_registration_status('commerce_product', $type, $line_item_wrapper->commerce_product->product_id->value());
    if ($can_register == 1) {
      $show_form = TRUE;
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      if (!$find_values) {
        // order data has not been set, so we initialize an empty array for our
        // entities to be stored
        $order->data['register_entities']['prod-' . $product->sku->value()] = array();
      }
      for ($i=0; $i<$quantity; $i++) {
        $label = $i+1;
        $form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i] = array(
          '#type' => 'fieldset',
          '#title' => check_plain($product->title->value() . ' #' . $label),
          '#collapsible' => TRUE,
          '#tree' => TRUE,
        );
        $entity = NULL;
        if ($find_values && !empty($order->data['register_entities']['prod-' . $product->sku->value()][$i])) {
          // the entity exists already, so let's just load it from the order
          $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        }
        else {
          // entity doesn't exist, so we need to create a new registration
          $bundle = registration_entity_registration_bundle('commerce_product', $type, $product->product_id->value());
          $entity = entity_create('registration', array('type' => $bundle));
          $entity->bundle = $entity->type;
          $entity->entity_type = 'commerce_product';
          $entity->entity_bundle = $product->type->value();
          $entity->eid = $product->product_id->value();
          $entity->created = REQUEST_TIME;
          $entity->updated = REQUEST_TIME;
          $entity->author_uid = $user->uid;
          $entity->status = 0;
          $entity->order_id = $order->order_id;
        }
        // attach the entity's fields to the form
        field_attach_form('registration', $entity, $form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i], $form_state);
        $fields = &$form['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i];
        foreach ($fields as $key => $data) {
          if (is_array($data) && substr($key, 0, 1) != '#') {
            // we set the parents on just fields so we can find them in
            // form_state later; fields in the form array don't start with a #
            $fields[$key]['#parents'] = array('registration_information', 'prod-' . $product->sku->value(), 'prod-' . $product->sku->value() . '-reg-' . $i, $key);
          }
        }
        // store entities in order's data array for use in validation, submission
        // and review callbacks
        $order->data['register_entities']['prod-' . $product->sku->value()][$i] = $entity;
      }
    }
  }
  if (!$show_form) {
    // no register enabled products, so go to next step of checkout process
    $order_status = commerce_order_status_load($order->status);
    if ($order_status['state'] == 'checkout' && $order_status['checkout_page'] == 'registration') {
      watchdog('commerce registration', serialize($form_state));
      $this_page = commerce_checkout_page_load($order_status['checkout_page']);
      $next_page = $this_page['next_page'];
      if ($order->log == "Customer returned to the previous checkout page via a submit button.") {
        // not sure how else to check to see if they went back instead of forwards
        $next_page = $this_page['prev_page'];
      }
      $order = commerce_order_status_update($order, 'checkout_' . $next_page);
      // Inform modules of checkout completion if the next page is Completed.
      if ($next_page == 'complete') {
        commerce_checkout_complete($order);
      }
      drupal_goto('checkout/' . $order->order_id . '/' . $next_page);
    }
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
    $can_register = registration_entity_registration_status('commerce_product', $type, $line_item_wrapper->commerce_product->product_id->value());
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        // let other modules do any validation necessary
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
    $can_register = registration_entity_registration_status('commerce_product', $type, $line_item_wrapper->commerce_product->product_id->value());
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        field_attach_submit('registration', $entity, $form['registration_information']['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i], $form_state);
        commerce_registration_entity_update_fields($entity, $form_state['values']['registration_information']['prod-' . $product->sku->value()]['prod-' . $product->sku->value() . '-reg-' . $i]);
        entity_save('registration', $entity);
      }
    }
  }
}

/**
 * Commerce checkout pane review callback.
 */
function commerce_registration_information_review($form, $form_state, $checkout_pane, $order) {
  $msg = '';
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = registration_entity_registration_status('commerce_product', $type, $line_item_wrapper->commerce_product->product_id->value());
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      $msg .= '<em>' . $product->title->value() . ' (' . format_plural($quantity, '1 registration', '@count registrations') . '):</em><br />';
      $msg .= t('');
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        $l = $i+1;
        $msg .= t("<hr><em><u>Registrant #$l</u></em><br />") . drupal_render(field_attach_view('registration', $entity, 'full'));
      }
    }
  }
  return $msg;
}

/**
 * Updates the attached entity with field values.
 *
 * @param $entity
 *   The referenced entity to update.
 * @param array $values
 *   Array of values to add to the entity. Example value array:
 *     array(
 *       'field_passport' => array(
 *         'und' => array(
 *           0 => array('value' => 'MY FIELD VALUE')
 *         )
 *       )
 *     )
 */
function commerce_registration_entity_update_fields(&$entity, $values) {
  if (empty($values)) {
    return;
  }
  foreach ($values as $key => $value) {
    $entity->$key = $value;
  }
}