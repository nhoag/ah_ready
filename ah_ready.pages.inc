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
function ah_ready_site_form($form, &$form_state) {
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

function theme_ah_ready_domains($vars) {
  $rows = array();
  $output = '';
  $form = $vars['form'];
  $header = array('Production Domains', 'Enable SSL Check', 'Delete Entry');
  foreach (element_children($form) as $d) {
    $rows[] = array(
      drupal_render($form[$d]['domain']),
      drupal_render($form[$d]['ah_ssl']),
      drupal_render($form[$d]['delete']),
    );
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= drupal_render_children($form);

  return $output;
}

function ah_ready_domain_form($form, &$form_state, $no_js_use = FALSE) {
  if (isset($form_state['ah_ready_domains']['confirm'])) {
    $output = "\n<ul>";
    foreach ($form_state['ah_ready_domains']['original_form']['domains'] as $domains) {
      if ($domains['delete'] == 1) {
        $output .= '<li>' . $domains['domain'] . '</li>';
      }
    }
    $output .= '</ul>';
    return confirm_form($form, t("The following domains will be deleted:") . $output , 'admin/config/system/ah-ready/domains');
  } else {
    $form = array();
    $form['ah_ready_domains'] = array(
      '#tree' => false,
      '#prefix' => '<div id="ah-ready-domains-form">',
      '#suffix' => '</div>',
    );
    $form['ah_ready_domains']['domains'] = array(
      '#prefix' => '<div id="ah-ready-domains-wrapper">',
      '#suffix' => '</div>',
      '#theme' => 'ah_ready_domains',
    );
    $defaults = array();
    $result = db_query("SELECT * FROM {ah_ready_domains}");
    if ($result) {
      while ($row = $result->fetchAssoc()) {
        $defaults[] = $row;
      }
    }
    if (isset($form_state['num_domains'])) {
      $domain_count = $form_state['num_domains'];
    } elseif (isset($defaults)) {
      $domain_count = count($defaults);
    } else {
      $domain_count = 1;
    }
    $new = 0;
    for ($i = 0; $i < $domain_count; $i++) {
      if (isset($defaults[$i])) {
        $did = 'did:' . $defaults[$i]['did'];
        $domain = $defaults[$i]['domain'];
        $ssl = $defaults[$i]['ah_ssl'];
      }
      if (!isset($defaults[$i]) && isset($form_state['num_domains'])) {
        $did = 'new:' . $new;
        $domain = $form_state['values']['domains'][$did]['domain'];
        $ssl = $form_state['values']['domains'][$did]['ah_ssl'];
        $new++;
      }
      $form['ah_ready_domains']['domains'][$did] = array(
        '#tree' => true,
        '#weight' => $i,
        '#parents' => array('domains', $did),
      );
      $form['ah_ready_domains']['domains'][$did]['did'] = array(
        '#type' => 'value',
        '#value' => $i,
        '#parents' => array('domains', $did, 'did'),
      );
      $form['ah_ready_domains']['domains'][$did]['domain'] = array(
        '#type' => 'textfield',
        '#weight' => $i,
        '#default_value' => $domain,
        '#title' => 'Production Domains',
        '#title_display' => 'invisible',
        '#parents' => array('domains', $did, 'domain'),
        '#autocomplete_path' => 'admin/ah_ready/domains/autocomplete',
      );
      $form['ah_ready_domains']['domains'][$did]['ah_ssl'] = array(
        '#type' => 'checkbox',
        '#weight' => $i,
        '#default_value' => $ssl,
        '#title_display' => 'invisible',
        '#title' => 'Enable SSL Check',
        '#parents' => array('domains', $did, 'ah_ssl'),
      );
      $form['ah_ready_domains']['domains'][$did]['delete'] = array(
        '#type' => 'checkbox',
        '#weight' => $i,
        '#title_display' => 'invisible',
        '#title' => 'Delete Entry',
        '#parents' => array('domains', $did, 'delete'),
      );
    }

    $form['ah_ready_domains']['add_domain'] = array(
      '#type' => 'submit',
      '#value' => t('Add domain'),
      '#submit' => array('ajax_add_domain'),
      '#ajax' => array(
        'callback' => 'ajax_add_domain_callback',
        'wrapper' => 'ah-ready-domains-wrapper',
        'effect' => 'fade',
      ),
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );

    return $form;
  }
}

function domain_add_schema($domain) {
  if (!preg_match("#^https?://#", $domain)) {
    $domain = "http://" . $domain;
  }

  return $domain;
}

function ah_ready_domain_form_validate($form, &$form_state) {
  $domains = isset($form_state['values']['domains']) ? $form_state['values']['domains'] : $form_state['ah_ready_domains']['original_form']['domains'];
  foreach ($domains as $did => $v) {
    $url = $v['domain'];
    if (!empty($v['domain'])) {
      $url = domain_add_schema($url);
      if (!filter_var($url, FILTER_VALIDATE_URL)) {
        form_set_error('domains][' . $did, t("Invalid domain."));
      }
    }
  }
}

function ah_ready_domain_form_submit($form, &$form_state) {
  $domains = isset($form_state['values']['domains']) ? $form_state['values']['domains'] : $form_state['ah_ready_domains']['original_form']['domains'];
  $delete = array();
  $urls = array();
  foreach ($domains as $domain) {
    $url = parse_url(domain_add_schema($domain['domain']), PHP_URL_HOST);
    if (!empty($url) && $domain['delete'] == 0) {
      $urls[] = $domain;
    } else {
      $delete[] = $domain;
    }
  }
  if (!isset($form_state['ah_ready_domains']['confirm']) && !empty($delete)) {
    $form_state['ah_ready_domains']['confirm'] = true;
    $form_state['ah_ready_domains']['original_form'] = $form_state['values'];
    $form_state['rebuild'] = true;
  } else {
    foreach ($delete as $url) {
      db_delete('ah_ready_domains')
        ->condition('domain', $url['domain'])
        ->execute();
    }
    foreach ($urls as $url) {
      db_merge('ah_ready_domains')
        ->key(array('domain' => $url['domain']))
        ->fields(array(
          'ah_ssl' => $url['ah_ssl'],
        ))
        ->execute();
    }
    drupal_set_message(t("Domains successfully set."), 'status');
    $form_state['redirect'] = array('admin/config/system/ah-ready/domains');
  }
}

function ah_ready_domain_delete($form, &$form_state, $delete) {
  return confirm_form($form, t('Are you sure you want to delete the specified domains?'), 'admin/config/system/ah-ready/domains', '', t('Delete'), t('Cancel'));
}

function ajax_add_domain_callback($form, $form_state) {
  return $form['ah_ready_domains']['domains'];
}

function ajax_add_domain($form, &$form_state) {
  if ($form_state['values']['add_domain']) {
    $n = $_GET['q'] == 'system/ajax' ? 1 : 5;
    $form_state['num_domains'] = count($form_state['values']['domains']) + $n;
  }
  unset($form_state['input']['domains']);
  $form_state['rebuild'] = TRUE;
}

