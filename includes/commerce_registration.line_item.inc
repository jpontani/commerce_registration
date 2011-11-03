<?php
/**
 * @file
 *   Code for the Commerce Registration wait list line item type.
 */

/**
 * Line item configuration callback.
 */
function commerce_registration_waitlist_configuration() {
  
}

/**
 * Line item title callback.
 */
function commerce_registration_waitlist_title($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  return t('@title (waitlisted)', array('@title' => $line_item_wrapper->waitlist_product->title->value());
}

/**
 * Line item add form callback.
 */
function commerce_registration_waitlist_add_form(&$form_state) {
  
}

/**
 * Line item item add form submit callback.
 */
function commerce_registration_waitlist_add_form_submit(&$line_item, $element, &$form_state, $form) {
  
}