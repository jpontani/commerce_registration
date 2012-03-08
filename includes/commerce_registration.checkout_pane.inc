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
  $find_values = isset($form_state['values'][$checkout_pane['pane_id']]['register_entities']);
  $order_values = isset($order->data['register_entities']);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings($id, 'commerce_product');
    if (!empty($settings) && $settings['status'] == 1) {
      $show_form = TRUE;
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      $prodkey = 'prod-' . $product->sku->value();
      for ($i = 0; $i < $quantity; $i++) {
        $label = $i + 1;
        $form[$prodkey][$prodkey . '-reg-' . $i] = array(
          '#type' => 'fieldset',
          '#title' => check_plain($product->title->value() . ' - Registrant #' . $label),
          '#collapsible' => TRUE,
          '#tree' => TRUE,
        );
        $entity = NULL;
        $mail = '';
        if ($user->uid != 0) {
          $mail = $user->mail;
        }
        if (isset($form_state['values'][$checkout_pane['pane_id']]['registration_information'][$prodkey][$prodkey . '-reg-' . $i]['mail'])) {
          $mail = $form_state['values'][$checkout_pane['pane_id']]['registration_information'][$prodkey][$prodkey . '-reg-' . $i]['mail'];
        }
        if ($find_values) {
          // The entity exists already, so let's just load it from the form_state.
          $entity = $form_state['values'][$checkout_pane['pane_id']]['register_entities'][$prodkey][$i];
        }
        elseif ($order_values) {
          $entity = $order->data['register_entities'][$prodkey][$i];
        }
        else {
          // Entity doesn't exist, so we need to create a new registration.
          $bundle = registration_get_entity_registration_type('commerce_product',
                      $line_item_wrapper->commerce_product->value());
          $entity = entity_get_controller('registration')->create(array('type' => $bundle));
          $entity->registration_id = 0;
          $entity->entity_type = 'commerce_product';
          $entity->entity_id = $product->product_id->value();
          $entity->entity = entity_load_single($entity->entity_type, $entity->entity_id);
          $entity->created = REQUEST_TIME;
          $entity->updated = REQUEST_TIME;
          $entity->author_uid = $user->uid;
          $entity->order_id = $order->order_id;
          $entity->mail = $mail;
        }
        $form[$prodkey][$prodkey . '-reg-' . $i]['mail'] = array(
          '#weight' => -50,
          '#type' => 'textfield',
          '#title' => t('Email Address'),
          '#default_value' => $entity->mail,
          '#required' => TRUE,
          '#parents' => array(
            'registration_information',
            $prodkey,
            $prodkey . '-reg-' . $i,
            'email',
          ),
        );
        // Attach the entity's fields to the form.
        field_attach_form(
          'registration',
          $entity,
          $form[$prodkey][$prodkey . '-reg-' . $i],
          $form_state
        );
        $fields = &$form[$prodkey][$prodkey . '-reg-' . $i];
        foreach ($fields as $key => $data) {
          if (is_array($data) && drupal_substr($key, 0, 1) != '#') {
            // We set the parents on just fields so we can find them in
            // form_state later; fields in the form array don't start with a #.
            $fields[$key]['#parents'] = array(
              'registration_information',
              $prodkey,
              $prodkey . '-reg-' . $i,
              $key,
            );
          }
        }
        $form['register_entities'][$prodkey][$i] = array(
          '#type' => 'value',
          '#value' => $entity,
        );
      }
    }
  }
  if (!$show_form) {
    // No register enabled products, so go to next step of checkout process.
    $order_status = commerce_order_status_load($order->status);
    if ($order_status['state'] == 'checkout' && $order_status['checkout_page'] == 'registration') {
      $this_page = commerce_checkout_page_load($order_status['checkout_page']);
      $next_page = $this_page['next_page'];
      if ($order->log == "Customer returned to the previous checkout page via a submit button.") {
        // Not sure how else to check to see if they went back instead of
        // forwards.
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
  $ret = TRUE;
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings($id, 'commerce_product');
    if (!empty($settings) && $settings['status'] == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      $emails = array();
      for ($i = 0; $i < $quantity; $i++) {
        $prodkey = 'prod-' . $product->sku->value();
        $entity = $form_state['values'][$checkout_pane['pane_id']]['register_entities'][$prodkey][$i];
        $mail = $form_state['values']['registration_information'][$prodkey][$prodkey . '-reg-' . $i]['mail'];
        if (registration_is_registered($entity, $mail)) {
          form_set_error(
            "registration_information][" . $prodkey . "]["
                . $prodkey . "-reg-" . $i . "][mail",
              t("The email address entered is already registered for this item.")
          );
          $ret = FALSE;
        }
        else {
          if (in_array($mail, $emails)) {
            form_set_error(
              "registration_information][" . $prodkey . "]["
                . $prodkey . "-reg-" . $i . "][mail",
              t("Email addresses must be unique for each registrant."));
            $ret = FALSE;
          }
          else {
            $emails[] = $mail;
          }
        }
        // Let other modules do any validation necessary.
        field_attach_form_validate(
          'registration',
          $entity,
          $form['registration_information'][$prodkey][$prodkey . '-reg-' . $i],
          $form_state
        );
      }
    }
  }
  return $ret;
}

/**
 * Commerce checkout pane form submit callback.
 */
function commerce_registration_information_checkout_form_submit($form, &$form_state, $checkout_pane, $order) {
  $order->data['register_entities'] = array();
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings($id, 'commerce_product');
    if (!empty($settings) && $settings['status'] == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i = 0; $i < $quantity; $i++) {
        $prodkey = 'prod-' . $product->sku->value();
        $entity = &$form_state['values'][$checkout_pane['pane_id']]['register_entities'][$prodkey][$i];
        field_attach_submit(
          'registration',
          $entity,
          $form['registration_information'][$prodkey][$prodkey . '-reg-' . $i],
          $form_state
        );
        commerce_registration_entity_update_fields(
          $entity,
          $form_state['values']['registration_information'][$prodkey][$prodkey . '-reg-' . $i]
        );
        $order->data['register_entities'][$prodkey][$i] = $entity;
      }
    }
  }
  commerce_order_save($order);
}

/**
 * Commerce checkout pane review callback.
 */
function commerce_registration_information_review($form, $form_state, $checkout_pane, $order) {
  $msg = '';
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings($id, 'commerce_product');
    if (!empty($settings) && $settings['status'] == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      $msg .= '<em>' . $product->title->value() . ' ('
        . format_plural($quantity, '1 registration', '@count registrations')
        . '):</em><br />';
      for ($i = 0; $i < $quantity; $i++) {
        $prodkey = 'prod-' . $product->sku->value();
        $entity = $order->data['register_entities'][$prodkey][$i];
        $l = $i + 1;
        $msg .= "<hr><em><u>" . t("Registrant #@count", array('@count' => $l)) . "</u></em><br />";
        $msg .= "<strong>" . t("Email") . "</strong>:" . check_plain($entity->mail) . "<br />";
        $msg .= drupal_render(field_attach_view('registration', $entity, 'full'));
      }
    }
  }
  return $msg;
}

/**
 * Updates the attached entity with field values.
 *
 * Example $values array:
 * @code
 * $values = array(
 *   'field_passport' => array(
 *     'und' => array(
 *       0 => array('value' => 'MY FIELD VALUE')
 *     )
 *   )
 * );
 * @endcode
 *
 * @param stdClass $entity
 *   The referenced entity to update.
 * @param array $values
 *   Array of values to add to the entity.
 */
function commerce_registration_entity_update_fields(&$entity, $values) {
  if (empty($values)) {
    return;
  }
  foreach ($values as $key => $value) {
    $entity->$key = $value;
  }
}
