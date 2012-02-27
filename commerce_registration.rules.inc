<?php
/**
 * @file
 * Commerce Registration rules file.
 */

/**
 * Implements hook_rules_condition_info().
 */
function commerce_registration_rules_condition_info() {
  $conditions = array();

  $conditions['commerce_registration_order_has_registration'] = array(
    'label' => t('Line Item product is register enabled.'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_line_item',
        'label' => t('Commerce Line Item'),
        'description' => t('The Commerce Line Item that should be checked for a register enabled product.'),
      ),
    ),
  );
  $conditions['commerce_registration_item_has_registration_capacity'] = array(
    'label' => t('Product is available for registration'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Commerce Line Item'),
        'description' => t('The Commerce Line Item containing the product to check if registrations are available. Checks open date, closing date, and capacity.'),
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_item_can_register',
    ),
  );

  return $conditions;
}

/**
 * Implements hook_rules_action_info().
 */
function commerce_registration_rules_action_info() {
  $actions = array();

  $actions['commerce_registration_mark_registrations_paid'] = array(
    'label' => t('Mark Commerce Registrations as paid'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order to mark all attached registrations as paid.'),
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_mark_paid',
    ),
  );

  $actions['commerce_registration_cancel_registrations'] = array(
    'label' => t('Cancel all Commerce Registrations'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order to cancel all attached registrations'),
      ),
    ),
  );

  return $actions;
}

/**
 * @defgroup commerce_registration_rules_conditions Rules Conditions
 * @{
 * Condition callbacks for Commerce Registration rules.
 */

/**
 * Condition callback.
 *
 * Checks the commerce line item to see if the product is register enabled.
 *
 * @param CommerceLineItem $line_item
 *   The Commerce Line Item to check for a register enabled product.
 *
 * @return bool
 *   Boolean TRUE if the line item product is register enabled.
 */
function commerce_registration_order_has_registration($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $id = $line_item_wrapper->commerce_product->product_id->value();
  $settings = registration_entity_settings($id, 'commerce_product');
  return !empty($settings);
}

/**
 * Condition callback.
 *
 * Checks if the line item product has registration capacity left. Capacity is
 * only taken if an order is paid for in full.
 *
 * @param CommerceLineItem $line_item
 *   Commerce Line Item to check the product for registration capacity.
 *
 * @return bool
 *   Boolean TRUE if the product has registration capacity.
 */
function commerce_registration_item_can_register($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $id = $line_item_wrapper->commerce_product->product_id->value();
  $settings = registration_entity_settings($id, 'commerce_product');
  if (empty($settings)) {
    return FALSE;
  }
  $room = (int) $settings['capacity'];
  if ($room != 0) {
    $room -= (int) registration_event_count($id, 'commerce_product');
  }
  $room = ($settings['capacity'] == 0 ? TRUE : ($room > 0 ? TRUE : FALSE));
  return ($settings['status'] == 1 && $room);
}

/**
 * @} End of "defgroup commerce_registration_rules_conditions".
 */

/**
 * @defgroup commerce_registration_rules_actions Rules Actions
 * @{
 * Action callbacks for Commerce Registration rules.
 */

/**
 * Action callback.
 *
 * Marks all registrations on a particular order as paid.
 *
 * @param CommerceOrder $order
 *   The Commerce Order object to mark all attached registrations as paid.
 *
 * @todo Custom status definitions.
 */
function commerce_registration_mark_paid($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    if (commerce_registration_item_can_register($line_item_wrapper->value())) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i = 0; $i < $quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        registration_save($entity);
      }
    }
  }
}

/**
 * Action callback.
 *
 * Cancels all registrations on a particular order.
 *
 * @param CommerceOrder $order
 *   The Commerce Order object to cancel all attached registrations.
 */
function commerce_registration_cancel_registrations($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    if (commerce_registration_item_can_register($line_item_wrapper->value())) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i = 0; $i < $quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        // Notify field api that the entity was deleted.
        field_attach_delete('registration', $entity);
        entity_delete('registration', $entity->registration_id);
      }
    }
  }
}

/**
 * @} End of "defgroup commerce_registration_rules_actions".
 */
