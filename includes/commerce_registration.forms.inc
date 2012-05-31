<?php
/**
 * @file
 * Commerce Registration modified validation and submit handlers for
 * registration settings for commerce products.
 */

/**
 * Registration settings form for product display nodes.
 */
function commerce_registration_node_registrations_settings_form($form, &$form_state, $type, $entity) {
  // Loop through all products on the entity and add the settings form to a
  // fieldset.
  form_load_include($form_state, 'inc', 'registration', 'includes/registration.forms');
  $form = array();
  $fields = field_read_fields(array('type' => 'commerce_product_reference'));
  foreach ($fields as $field) {
    if (isset($entity->{$field['field_name']})) {
      $form['settings'] = array(
        '#type' => 'container',
        '#tree' => TRUE,
      );
      foreach ($entity->{$field['field_name']}['und'] as $product) {
        if (commerce_registration_product_has_registration_field($product['product_id'])) {
          $product = commerce_product_load($product['product_id']);
          $settings = registration_entity_settings('commerce_product', $product->product_id);
          $prod = 'prod-' . check_plain($product->product_id);
          $form['settings'][$prod] = array(
            '#type' => 'fieldset',
            '#title' => t('Registration Settings for !product (@sku)',
              array(
                '!product' => '<em>' . $product->title . '</em>',
                '@sku' => $product->sku,
              )
            ),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
          );
          $addform = registration_entity_settings_form($form, $form_state, $settings, 'commerce_product', $product);
          unset($addform['save']);
          $addform['status']['#weight'] = -10;
          $addform['capacity']['#weight'] = -9;
          $addform['reminder']['reminder_settings']['#states']['visible'] = array(
            ':input[name="settings[' . $prod . '][reminder][send_reminder]"]' => array(
              'checked' => TRUE,
            ),
          );
          $form['settings'][$prod] += $addform;
          $form['settings'][$prod]['reminder']['#parents'] = array('settings', $prod, 'reminder');
        }
      }
    }
  }
  if (empty($form)) {
    list($entity_id) = entity_extract_ids($type, $entity);
    $settings = registration_entity_settings($type, $entity_id);
    $form = registration_entity_settings_form($form, $form_state, $settings, $type, $entity);
    $form['#submit'] = array('registration_entity_settings_form_submit');
    $form['#validate'] = array('registration_entity_settings_form_validate');
  }
  else {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save Settings'),
      '#validate' => array('commerce_registration_node_registrations_settings_form_validate'),
      '#submit' => array('commerce_registration_node_registrations_settings_form_submit'),
    );
  }
  return $form;
}

/**
 * Registration settings form validation for product display nodes.
 */
function commerce_registration_node_registrations_settings_form_validate($form, &$form_state) {
  foreach ($form_state['values']['settings'] as $id => $settings) {
    $id = substr($id, 5);
    $temp_form_state = array(
      'entity' => array(
        'entity_id' => $id,
        'entity_type' => 'commerce_product',
      ),
      'values' => $settings,
    );
    registration_entity_settings_form_validate($form, $temp_form_state);
  }
}

/**
 * Registration settings form submission for product display nodes.
 */
function commerce_registration_node_registrations_settings_form_submit($form, &$form_state) {
  foreach ($form_state['values']['settings'] as $id => $settings) {
    $id = substr($id, 5);
    $temp_form_state = array(
      'entity' => array(
        'entity_id' => $id,
        'entity_type' => 'commerce_product',
      ),
      'values' => $settings,
    );
    registration_entity_settings_form_submit(array(), $temp_form_state);
  }
}

/**
 * Registration broadcast form for product display nodes.
 */
function commerce_registration_registrations_broadcast_form($form, &$form_state, $type, $entity) {
  // Loop through all products on the entity and add the broadcast form to a
  // fieldset.
  form_load_include($form_state, 'inc', 'registration', 'includes/registration.forms');
  $form = array();
  $all = array(0 => 'All Product Registrants');
  $products = array();
  $fields = field_read_fields(array('type' => 'commerce_product_reference'));
  foreach ($fields as $field) {
    if (isset($entity->{$field['field_name']})) {
      foreach ($entity->{$field['field_name']}[LANGUAGE_NONE] as $product) {
        if (commerce_registration_product_has_registration_field($product['product_id'])) {
          $product = commerce_product_load($product['product_id']);
          $products[$product->product_id] = check_plain($product->title);
        }
      }
    }
  }
  if (empty($products)) {
    $form = registration_registrations_broadcast_form($form, $form_state, $type, $entity);
    $form['#submit'] = array('registration_registrations_broadcast_form_submit');
  }
  else {
    $form['product'] = array(
      '#type' => 'select',
      '#options' => $all + $products,
      '#title' => t('Select a Product'),
      '#description' => t('The email message will be sent to all registrants for this product.'),
      '#required' => TRUE,
    );
    $form += registration_registrations_broadcast_form($form, $form_state, $type, $entity);
    unset($form['send']);
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Send'),
    );
    $form['#submit'] = array('commerce_registration_registrations_broadcast_form_submit');
  }
  return $form;
}

/**
 * Broadcast email form submit handler.
 */
function commerce_registration_registrations_broadcast_form_submit($form, &$form_state) {
  $id = $form_state['values']['product'];
  $subject = $form_state['values']['subject'];
  $message = $form_state['values']['message'];
  if ($id == 0) {
    foreach ($form['product']['#options'] as $key => $value) {
      if (is_int($key) && $key != 0) {
        registration_send_broadcast(
          'commerce_product',
          $key,
          $subject,
          $message
        );
      }
    }
  }
  else {
    registration_send_broadcast(
      'commerce_product',
      $id,
      $subject,
      $message
    );
  }
}

/**
 * New registration settings validation handler to use with Commerce Products.
 */
function commerce_registration_registration_settings_form_validate($form, $form_state) {
  if (!function_exists('registration_entity_settings_form_validate')) {
    form_load_include($form_state, 'inc', 'registration', 'includes/registration.forms');
  }
  $temp_form_state = array('values' => $form_state['values']['registration_settings']);
  $temp_form_state['values']['status'] = $temp_form_state['values']['reg_status'];
  return registration_entity_settings_form_validate($form, $temp_form_state);
}

/**
 * New registration settings submit handler to use with Commerce Products.
 */
function commerce_registration_registration_settings_form_submit($form, &$form_state) {
  list($entity_id) = entity_extract_ids("commerce_product", $form_state['commerce_product']);
  $temp_form_state = array(
    'entity' => array(
      'entity_type' => 'commerce_product',
      'entity_id' => $entity_id,
    ),
    'values' => $form_state['values']['registration_settings'],
  );
  $temp_form_state['values']['status'] = $temp_form_state['values']['reg_status'];
  registration_entity_settings_form_submit($form, $temp_form_state);
}
