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
  $form = array();
  // Determine if we should create a new entity or if the user has gone back in
  // the checkout process and we should load previously stored values from
  // either the checkout pane or from the order.
  $find_values = isset($form_state['values'][$checkout_pane['pane_id']]['register_entities']);
  $order_values = isset($order->data['register_entities']);
  $override_title = variable_get('commerce_registration_product_title', NULL);
  $hide_email = variable_get('commerce_registration_hide_email_logged_in', FALSE);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings('commerce_product', $id);
    if (!empty($settings) && registration_status('commerce_product', $id, TRUE) == 1) {
      $show_form = TRUE;
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      $prodkey = 'prod-' . $product->sku->value();
      for ($i = 0; $i < $quantity; $i++) {
        // We add a single registration for the quantity of the product being
        // purchased/registered for.
        $label = $i + 1;
        $title = check_plain($product->title->value() . ' - Registrant #' . $label);
        if (isset($override_title)) {
          $title = str_replace('[title]', $product->title->value(), $override_title);
          $title = str_replace('[index]', $label, $title);
        }
        $form[$prodkey][$prodkey . '-reg-' . $i] = array(
          '#type' => 'fieldset',
          '#title' => $title,
          '#collapsible' => TRUE,
          '#tree' => TRUE,
          '#description' => variable_get('commerce_registration_registrant_help', ''),
        );
        $entity = NULL;
        $mail = '';
        if ($user->uid != 0) {
          $mail = $user->mail;
        }
        if ($find_values) {
          // The entity exists already, so let's just load it from the
          // form_state.
          $entity = $form_state['values'][$checkout_pane['pane_id']]['register_entities'][$prodkey][$i];
          $mail = $entity->mail;
        }
        elseif ($order_values && isset($order->data['register_entities'][$prodkey][$i])) {
          $entity = $order->data['register_entities'][$prodkey][$i];
          $mail = $entity->mail;
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
          $entity->count = 1;
        }
        $form[$prodkey][$prodkey . '-reg-' . $i]['mail'] = array(
          '#weight' => -50,
          '#type' => ($user->uid != 0 && $hide_email ? 'hidden' : 'textfield'),
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
    $settings = registration_entity_settings('commerce_product', $id);
    if (!empty($settings) && registration_status('commerce_product', $id, TRUE) == 1) {
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
        // Let other modules do any field API validation if necessary.
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
 *
 * @see commerce_registration_entity_update_fields().
 */
function commerce_registration_information_checkout_form_submit($form, &$form_state, $checkout_pane, $order) {
  $order->data['register_entities'] = array();
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    $settings = registration_entity_settings('commerce_product', $id);
    if (!empty($settings) && registration_status('commerce_product', $id, TRUE) == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i = 0; $i < $quantity; $i++) {
        $prodkey = 'prod-' . $product->sku->value();
        $entity = &$form_state['values'][$checkout_pane['pane_id']]['register_entities'][$prodkey][$i];
        // Notify the field API that values were submitted.
        field_attach_submit(
          'registration',
          $entity,
          $form['registration_information'][$prodkey][$prodkey . '-reg-' . $i],
          $form_state
        );
        // We add the field values to our entity so they are stored properly.
        commerce_registration_entity_update_fields(
          $entity,
          $form_state['values']['registration_information'][$prodkey][$prodkey . '-reg-' . $i]
        );
        $order->data['register_entities'][$prodkey][$i] = $entity;
      }
    }
  }
  // Save the order since we added registration entities to the data array.
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
    $settings = registration_entity_settings('commerce_product', $id);
    if (!empty($settings) && registration_status('commerce_product', $id, TRUE) == 1) {
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
        $msg .= drupal_render(field_attach_view('registration', $entity, 'review_pane'));
      }
    }
  }
  return $msg;
}

/**
 * Commerce checkout pane settings form builder.
 */
function commerce_registration_information_settings_form($checkout_pane) {
  $form = array();

  $form['commerce_registration_product_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Product Registration Title'),
    '#description' => t('The text to populate the fieldset label for a registrant\'s information.') . '<br>'
        . t('Available tokens are [title] for the product title, and [index] for a countable index value.'),
    '#default_value' => variable_get('commerce_registration_product_title', ''),
  );

  $form['commerce_registration_registrant_help'] = array(
    '#type' => 'textarea',
    '#title' => t('Product Registration Fieldset Help Text'),
    '#description' => t('The help text to display to the user entering registration information.'),
    '#default_value' => variable_get('commerce_registration_registrant_help', ''),
  );

  $form['commerce_registration_hide_email_logged_in'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hide the Email field for logged in users'),
    '#default_value' => variable_get('commerce_registration_hide_email_logged_in', ''),
  );

  return $form;
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
