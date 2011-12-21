<?php
/**
 * @file
 * Commerce Registration attached entity views field handler.
 */

class commerce_registration_handler_field_registration_entity extends views_handler_field {
  function construct() {
    parent::construct();
    $this->additional_fields['eid'] = 'eid';
  }

  function render($values) {
    $product = commerce_product_load($values->registration_eid);
    $uri = entity_uri('commerce_product', $product);
    $text = t('@product (@sku)', array('@product' => $product->title, '@sku' => $product->sku));
    return l($text, $uri['path']);
  }
}