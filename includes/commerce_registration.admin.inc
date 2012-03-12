<?php
/**
 * @file
 * Commerce Registration admin page functions.
 */

/**
 * Admin registration page.
 */
function commerce_registration_admin_overview() {
  // Set breadcrumbs since they don't always show.
  $crumb[] = l(t('Home'), '');
  $crumb[] = l(t('Administration'), 'admin');
  $crumb[] = l(t('Store'), 'admin/commerce');
  drupal_set_breadcrumb($crumb);
  $view = views_get_view('commerce_registration_admin_regs', TRUE);
  $view->set_display('block');
  $out = "<h2>" . $view->get_title() . "</h2>" . views_embed_view('commerce_registration_admin_regs');
  return $out;
}
