<?php
/**
 * @file
 *   Commerce Registration rules file.
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
    'callbacks' => array(
      'execute' => 'commerce_registration_order_has_registration',
    ),
  );
  $conditions['commerce_registration_multiple_product_types_allowed'] = array(
    'label' => t('Order contains only specific product types.'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order whose line items should be checked for allowed product types.'),
      ),
      'product_types' => array(
        'type' => 'list<text>',
        'label' => t('Product Types'),
        'description' => t('The product types to allow in the cart.'),
        'options list' => 'commerce_registration_product_types',
        'restriction' => 'input',
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_order_product_types_allowed',
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
    'callbacks' => array(
      'execute' => 'commerce_registration_cancel_registrations',
    ),
  );

  $actions['commerce_registration_delete_line_item'] = array(
    'label' => t('Delete the line item'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Commerce Line Item'),
        'description' => t('The Commerce Line Item to delete.'),
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_delete_line_item',
    ),
  );

  return $actions;
}

/**
 * @defgroup commerce_registration_rules_conditions Commerce Registration Rules Conditions
 * @{
 * Condition callbacks for Commerce Registration rules.
 */

/**
 * Condition callback.
 *
 * Checks the commerce line item to see if the product is register enabled.
 *
 * @param $line_item
 *   The Commerce Line Item to check for a register enabled product.
 * @return
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
 * @param $line_item
 *   Commerce Line Item to check the product for registration capacity.
 *
 * @return
 *   Boolean TRUE if the product has registration capacity.
 */
function commerce_registration_item_can_register($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $id = $line_item_wrapper->commerce_product->product_id->value();
  return registration_has_room($id, 'commerce_product');
}

/**
 * Condition callback.
 *
 * Checks the commerce order to see if the line items have products that are in
 * the allowed products list.
 *
 * @param $order
 *   The Commerce Order whose line items will be reviewed.
 * @param $types
 *   Array of machine names of allowed product types.
 * @return
 *   Boolean FALSE if any product type is not in the allowed products list.
 */
function commerce_registration_order_product_types_allowed($order, $types) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    if (!in_array($type, $types)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * @} End of "defgroup commerce_registration_rules_conditions".
 */

/**
 * @defgroup commerce_registration_rules_actions Commerce Registration Rules Actions
 * @{
 * Action callbacks for Commerce Registration rules.
 */

/**
 * Action callback.
 *
 * Marks all registrations on a particular order as paid.
 *
 * @param $order
 *   The Commerce Order object to mark all attached registrations as paid.
 * @todo Custom status definitions.
 */
function commerce_registration_mark_paid($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    if (registration_has_room($id, 'commerce_product')) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        entity_save('registration', $entity);
      }
    }
  }
}

/**
 * Action callback.
 *
 * Cancels all registrations on a particular order.
 *
 * @param $order
 *   The Commerce Order object to cancel all attached registrations.
 */
function commerce_registration_cancel_registrations($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $id = $line_item_wrapper->commerce_product->product_id->value();
    if (registration_has_room($id, 'commerce_product')) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        // notify field api that the entity was deleted
        field_attach_delete('registration', $entity);
        entity_delete('registration', $entity->registration_id);
      }
    }
  }
}

/**
 * Action callback.
 *
 * Removes the line item from the order.
 */
function commerce_registration_delete_line_item($line_item) {
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $order = commerce_order_load($line_item_wrapper->order_id->value());
  // Delete the old line_item.
  $order = commerce_cart_order_product_line_item_delete($order, $line_item->line_item_id);
  commerce_order_save($order);
}

/**
 * @} End of "defgroup commerce_registration_rules_actions".
 */

/**
 * Returns array of product types keyed by machine name.
 */
function commerce_registration_product_types() {
  $product_types = commerce_product_types();
  $types = array();
  foreach ($product_types as $key => $value) {
    $types[$key] = $value['name'];
  }
  return $types;
}