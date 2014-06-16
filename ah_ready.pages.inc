<?php

/**
 * @file
 *   AH Ready configuration page.
 */

/**
 * Main page function
 */
function ah_ready_settings_page($arg = NULL) {
  $ah_ready_user = variable_get('ah_ready_cloudapi_username', '');
  $ah_ready_password = decrypt(variable_get('ah_ready_cloudapi_password', ''));
  if ($arg == 'domains') {
    return drupal_get_form('ah_ready_domain_form', $ah_ready_user, $ah_ready_password);
  } elseif ($arg == 'site') {
    return drupal_get_form('ah_ready_site_form', $ah_ready_user, $ah_ready_password);
  } else {
    return drupal_get_form('ah_ready_settings_form');
  }
}

/**
 * Settings form builder function.
 */
function ah_ready_settings_form($form, &$form_state) {

  $form = array();

  $form['#prefix'] = t(
    'Unlock additional Insight tests by entering your <a href="!net">CloudAPI username and password</a>.',
    array(
      '!net' => url('https://docs.acquia.com/cloud/api'),
    )
  );
  $form['ah_ready_cloudapi_username'] = array(
    '#type' => 'textfield',
    '#title' => t('CloudAPI Username'),
    '#default_value' => variable_get('ah_ready_cloudapi_username', ''),
    '#required' => TRUE,
  );
  $form['ah_ready_cloudapi_password'] = array(
    '#type' => 'password',
    '#title' => t('CloudAPI Password'),
    '#required' => TRUE,
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function ah_ready_settings_form_submit($form, &$form_state) {
  $ah_ready_user = $form_state['values']['ah_ready_cloudapi_username'];
  $ah_ready_password = $form_state['values']['ah_ready_cloudapi_password'];
  $ah_ready_password = encrypt("$ah_ready_password");

  variable_set('ah_ready_cloudapi_username', $ah_ready_user);
  variable_set('ah_ready_cloudapi_password', $ah_ready_password);
  ah_ready_cloudapi_sites(true);

  drupal_set_message(t('Cloud API credentials added successfully.'), 'status');
  $form_state['redirect'] = array('admin/config/system/ah-ready');
}

/**
 * Select form builder function.
 */
function ah_ready_site_form($form, &$form_state, $ah_ready_user, $ah_ready_password) {
  $form = array();
  $form['#prefix'] = t('Select an Acquia subscription in the dropdown.');
  $form['ah_ready_sites'] = array(
    '#type' => 'select',
    '#title' => t('Choose a site'),
    '#description' => t(
      'If you do not see your site in the dropdown, add or change Cloud API credentials.'
    ),
    '#default_value' => variable_get('ah_ready_cloudapi_site', ''),
    '#options' => ah_ready_cloudapi_sites(),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function ah_ready_site_form_submit($form, &$form_state) {
  $ah_ready_site = $form_state['values']['ah_ready_sites'];
  variable_set('ah_ready_cloudapi_site', $ah_ready_site);

  drupal_set_message(t('Network subscription successfully set.'), 'status');
  $form_state['redirect'] = array('admin/config/system/ah-ready/site');
}

/**
 * Select form builder function.
 */
function ah_ready_domain_form($form, &$form_state, $ah_ready_user, $ah_ready_password) {
  $form = array();

  $cloudapi = ah_ready_cloudapi_connect();
  $site = variable_get('ah_ready_cloudapi_site', '');

  $domains = array();
  if (!empty($cloudapi) && !empty($site)) {
    foreach($cloudapi->domains($site, CLOUD_ENV) as $domain) {
      $domains[$domain['name']] = $domain['name'];
    }
  }

  $form['#prefix'] = t('Enter production URLs to unlock additional Insight tests.');

  $form['ah_ready_domains'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter domains to be checked'),
    '#description' => t(
      'If you do not see your production domains in the autocomplete list, add them to the <a href="https://docs.acquia.com/cloud/configure/domains">Acquia Network Dashboard</a>.'
    ),
    '#autocomplete_path' => 'admin/ah_ready/domains/autocomplete',
    '#default_value' => variable_get('ah_ready_domains', ''),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function ah_ready_domain_form_submit($form, &$form_state) {
  $ah_ready_domains = $form_state['values']['ah_ready_domains'];
  variable_set('ah_ready_domains', $ah_ready_domains);

  drupal_set_message(t('Test domain successfully set.'), 'status');
  $form_state['redirect'] = array('admin/config/system/ah-ready');
}

