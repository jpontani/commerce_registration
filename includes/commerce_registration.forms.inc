<?php
/**
 * @file
 * Commerce Registration modified validation and submit handlers for
 * registration settings for commerce products.
 */

/**
 * New registration settings validation handler to use with Commerce Products.
 */
function commerce_registration_registration_settings_form_validate($form, $form_state) {
  // Ensure capacity is a positive integer.
  $capacity = $form_state['values']['registration_settings']['capacity'];
  if (!is_numeric($capacity) || ((int)$capacity != $capacity) || ($capacity < 0)) {
    form_set_error('capacity', t('Capacity must be a positive integer.'));
    return FALSE;
  }

  // validate from address
  if (!valid_email_address($form_state['values']['registration_settings']['settings']['from_address'])) {
    form_set_error('registration_settings][settings][from_address', t('From email address is invalid.'));
    return FALSE;
  }
  return TRUE;
}

/**
 * New registration settings submit handler to use with Commerce Products.
 */
function commerce_registration_registration_settings_form_submit($form, &$form_state) {
  list($entity_id) = entity_extract_ids("commerce_product", $form_state['commerce_product']);
  watchdog('commerce registration', serialize($form_state['commerce_product']));
  $fields = array(
    'status' => $form_state['values']['registration_settings']['reg_status'], 
    'capacity' => $form_state['values']['registration_settings']['capacity'], 
    'send_reminder' => $form_state['values']['send_reminder'],
    'settings' => serialize($form_state['values']['registration_settings']['settings']),
  );
  if ($form_state['values']['send_reminder']) {
    $fields['reminder_date'] = $form_state['values']['reminder_date']['year']
      . '-' . $form_state['values']['reminder_date']['month']
      . '-' . $form_state['values']['reminder_date']['day'];
    $fields['reminder_template'] = $form_state['values']['reminder_template'];
  }
  registration_update_entity_settings($entity_id, "commerce_product", $fields);
}
