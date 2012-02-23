<?php
/**
 * @file
 * Commerce Registration admin page functions.
 */

/**
 * Admin registration page.
 */
function commerce_registration_admin_overview() {
  $crumb[] = l(t('Home'), '');
  $crumb[] = l(t('Administration'), 'admin');
  $crumb[] = l(t('Store'), 'admin/commerce');
  drupal_set_breadcrumb($crumb);
  $view = views_get_view('commerce_registration_admin_regs', TRUE);
  $view->set_display('block');
  $out = t("<h2>@title</h2>", array('@title' => $view->get_title())) . views_embed_view('commerce_registration_admin_regs');
  unset($view);
  return $out;
}