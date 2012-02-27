<?php
/**
 * @file
 * Default rules configuration.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_registration_default_rules_configuration() {
  $rule = rules_reaction_rule();
  $rule->label = "Mark registrations as confirmed upon full payment";
  $rule->active = TRUE;
  $rule->event('commerce_payment_order_paid_in_full')
    ->action('commerce_registration_mark_registrations_paid', array('commerce_order:select' => "commerce_order"));

  return array('commerce_registration_paid_in_full' => $rule);
}
