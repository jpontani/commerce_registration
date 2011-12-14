<?php
/**
 * @file
 * Commerce Registration admin page functions.
 */

/**
 * Admin registration page.
 */
function commerce_registration_admin_overview() {
  $view = views_get_view('commerce_registration_admin_registrations', TRUE);
  $view->set_display('block');
  $out = t("<h2>@title</h2>", array('@title' => $view->get_title())) . views_embed_view('commerce_registration_admin_registrations');
  unset($view);
  return $out;
}