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

/**
 * Creates a new waitlist line item from an existing line item.
 *
 * @param $line_item
 *   Commerce Line Item that we are moving to a waitlist.
 * @param $order
 *   Commerce Order for the new line item to be added to.
 */
function commerce_registration_move_to_waitlist($line_item, $order) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $waitlist_line_item = entity_create('commerce_line_item', array(
    'type' => 'waitlist',
    'order_id' => $order->order_id,
    'quantity' => $line_item_wrapper->quantity->value(),
  ));
  commerce_line_item_save($waitlist_line_item);
  
  if (!($waitlist_line_item instanceof EntityMetadataWrapper)) {
    $waitlist_wrapper = entity_metadata_wrapper('commerce_line_item', $waitlist_line_item);
  }

  $waitlist_wrapper->commerce_product = $line_item_wrapper->commerce_product->value();

  // Delete the existing line item so we don't have it as a normal line item
  // and a waitlist line item.
  $order = commerce_cart_order_product_line_item_delete($order, $line_item_wrapper->line_item_id->value());

  commerce_order_save($order);
  
  // Invoke product removal event
  rules_invoke_event(
    'commerce_cart_product_remove',
    $order,
    $line_item_wrapper->commerce_product->value(),
    $line_item_wrapper->quantity->value(),
    $line_item,
    $line_item
  );

  return $waitlist_wrapper->value();
}

/**
 * Moves the product on the waitlist line item to a normal product line item.
 *
 * @param $line_item
 *   Commerce Line Item that we are moving to a normal product line item.
 * @param $order
 *   Commerce Order for the new line item to be added to.
 */
function commerce_registration_waitlist_to_cart($line_item, $order) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $product_id = $line_item_wrapper->commerce_product->product_id->value();
  $quantity = $line_item_wrapper->quantity->value();
  // we delete the waitlist line item
  $order = commerce_cart_order_product_line_item_delete($order, $line_item_wrapper->line_item_id->value());
  commerce_order_save($order);
  // add product from the waitlist item to the cart as a normal product line item
  commerce_cart_product_add($order->uid, $product_id, $quantity);
}