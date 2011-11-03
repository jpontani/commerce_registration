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
    'label' => t('Order contains a product that is register enabled.'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order whose line items should be checked for register enabled product types.'),
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

  // Waitlist actions
  $actions['commerce_registration_move_to_waitlist'] = array(
    'label' => t('Move product to waitlist'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Commerce Line Item'),
        'description' => t('The Commerce Line Item containing the product to add to the waitlist.'),
      ),
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order containing line items for the products and waitlist.'),
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_move_to_waitlist',
    ),
  );
  $actions['commerce_registration_waitlist_to_cart'] = array(
    'label' => t('Move product from waitlist to cart'),
    'group' => t('Commerce Registration'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Commerce Line Item'),
        'description' => t('The Commerce Line Item containing the waitlist product to move back to the cart.'),
      ),
      'commerce_order' => array(
        'type' => 'commerce_order',
        'label' => t('Commerce Order'),
        'description' => t('The Commerce Order containing line items for the products and waitlist.'),
      ),
    ),
    'callbacks' => array(
      'execute' => 'commerce_registration_waitlist_to_cart',
    ),
  );

  return $actions;
}

/**
 * Condition callback.
 *
 * Checks the commerce order to see if the line items have a product that is
 * register enabled.
 *
 * @param $order
 *   The Commerce Order whose line items will be reviewed.
 * @return
 *   Boolean TRUE if the order has at least one register enabled product in it.
 *   Otherwise FALSE.
 */
function commerce_registration_order_has_registration($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_status:commerce_product:_' . $type, 0);
    if ($can_register == 1) {
      return TRUE;
    }
  }
  return FALSE;
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

/**
 * Action execute callback.
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
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_status:commerce_product:_' . $type, 0);
    if ($can_register == 1) {
      $product = $line_item_wrapper->commerce_product;
      $quantity = (int) $line_item_wrapper->quantity->value();
      for ($i=0; $i<$quantity; $i++) {
        $entity = $order->data['register_entities']['prod-' . $product->sku->value()][$i];
        // we set status to 1 for PAID and save the entity
        $entity->status = 1;
        entity_save('registration', $entity);
      }
    }
  }
}

/**
 * Action execute callback.
 *
 * Cancels all registrations on a particular order.
 *
 * @param $order
 *   The Commerce Order object to cancel all attached registrations.
 */
function commerce_registration_cancel_registrations($order) {
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    $type = $line_item_wrapper->commerce_product->type->value();
    $can_register = (int) variable_get('registration_status:commerce_product:_' . $type, 0);
    if ($can_register == 1) {
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