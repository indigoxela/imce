<?php

/**
 * @file
 * Serves administration pages of IMCE.
 */

/**
 * Admin main page.
 */
function imce_admin() {
  $config = config('imce.settings');
  $profiles = $config->get('imce_profiles');

  $header = array(t('Profile name'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();

  foreach ($profiles as $pid => $profile) {
    $rows[] = array(
      check_plain($profile['name']),
      l(t('Edit'), 'admin/config/media/imce/profile/edit/' . $pid),
      $pid == 1 ? '' : l(t('Delete'), 'admin/config/media/imce/profile/delete/' . $pid),
    );
  }

  $rows[] = array('', array('data' => l(t('Add new profile'), 'admin/config/media/imce/profile'), 'colspan' => 2));

  $output['title'] = array(
    '#markup' => '<h2 class="title">' . t('Configuration profiles') . '</h2>',
  );
  $output['table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array('id' => 'imce-profiles-list'),
  );
  $output['form'] = backdrop_get_form('imce_admin_form');

  // Display security warnings
  if (empty($_POST)) {
    $roles = $config->get('imce_roles_profiles');
    if (!empty($roles[BACKDROP_ANONYMOUS_ROLE]['public_pid']) || !empty($roles[BACKDROP_ANONYMOUS_ROLE]['private_pid'])) {
      backdrop_set_message(t('Anonymous user role has access to IMCE.') . ' ' . t('Make sure this is not a misconfiguration.'), 'warning');
    }
    if (imce_admin_check_wildcard_upload(BACKDROP_AUTHENTICATED_ROLE, $roles)) {
      backdrop_set_message(t('Authenticated user role is assigned a configuration profile with unrestricted file extensions.') . ' ' . t('Make sure this is not a misconfiguration.'), 'warning');
    }
  }

  return $output;
}

/**
 * Admin form.
 */
function imce_admin_form($form, &$form_state) {
  $config = config('imce.settings');

  //roles profiles
  $form['roles'] = array('#tree' => TRUE);
  $roles = imce_sorted_roles();
  $form['#weighted'] = count($roles) > 3;

  foreach ($roles as $rid => $role) {
    $core = $rid == BACKDROP_ANONYMOUS_ROLE || $rid == BACKDROP_AUTHENTICATED_ROLE;
    $form['roles'][$rid] = imce_role_form($role, $form['#weighted'], $core);
  }

  //common settings
  $form['common'] = array(
    '#type' => 'fieldset',
    '#title' => t('Common settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['common']['textarea'] = array(
    '#type' => 'textfield',
    '#title' => t('Enable inline image/file insertion into plain textareas'),
    '#default_value' => $config->get('imce_settings_textarea'),
    '#maxlength' => NULL,
    '#description' => t('If you don\'t use any WYSIWYG editor, this feature will allow you to add your images or files as <strong>html code into any plain textarea</strong>. Enter <strong>comma separated textarea IDs</strong> under which you want to enable a link to IMCE. The * character is a wildcard. Hint: ID of Body fields in most node types starts with edit-body*.'),
  );
  $form['common']['absurls'] = array(
    '#type' => 'checkbox',
    '#title' => t('Absolute URLs'),
    '#default_value' => $config->get('imce_settings_absurls'),
    '#description' => t('Check if you want IMCE to return absolute file URLs.'),
  );
  $form['common']['replace'] = array(
    '#type' => 'radios',
    '#title' => t('Default behaviour for existing files during file uploads'),
    '#default_value' => $config->get('imce_settings_replace'),
    '#options' => array(
      FILE_EXISTS_RENAME => t('Keep the existing file renaming the new one'),
      FILE_EXISTS_ERROR => t('Keep the existing file rejecting the new one'),
      FILE_EXISTS_REPLACE => t('Replace the existing file with the new one')
    ),
  );
  $form['common']['thumb_method'] = array(
    '#type' => 'radios',
    '#title' => t('Default method for creating thumbnails'),
    '#default_value' => $config->get('imce_settings_thumb_method'),
    '#options' => array(
      'scale' => t('Scale the image with respect to the thumbnail dimensions.'),
      'scale_and_crop' => t('First scale then crop the image to fit the thumbnail dimensions.')
    ),
  );
  $form['common']['disable_private'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable serving of private files'),
    '#default_value' => $config->get('imce_settings_disable_private'),
    '#description' => t('IMCE serves all files under private files directory without applying any access restrictions. This allows anonymous access to any file(/system/files/filename) unless there is a module restricting access to the files. Here you can disable this feature.'),
  );

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#theme'] = 'imce_admin';
  $form['#submit'][] = 'imce_admin_submit';
  return $form;
}

/**
 * Admin form themed.
 */
function imce_admin_theme($variables) {
  $form = $variables['form'];
  $profile1 = imce_user1_profile();
  $header = array(t('User role'));
  $rows = array(array(t('Site maintenance account')));
  $keys = array('name');
  //add each stream wrapper as a column
  $swrappers = file_get_stream_wrappers(STREAM_WRAPPERS_VISIBLE);
  foreach ($swrappers as $scheme => $info) {
    $header[] = l($info['name'], 'imce/' . $scheme);
    $rows[0][] = check_plain($profile1['name']);
    $keys[] = $scheme . '_pid';
  }

  //in case we need profile weights.
  $weight_info = '';
  if ($form['#weighted']) {
    $header[] = t('Weight');
    $rows[0][] = t('n/a');
    $keys[] = 'weight';
    $weight_info = t('For users who have <strong>multiple roles</strong>, the <strong>weight</strong> property will determine the assigned profile. Lighter roles that are placed upper will take the precedence. So, an administrator role should be placed over other roles by having a smaller weight, ie. -10.');
  }

  foreach (element_children($form['roles']) as $rid) {
    $cells = array();
    foreach ($keys as $key) {
      $cells[] = backdrop_render($form['roles'][$rid][$key]);
    }
    $rows[] = $cells;
  }

  $output = '<h2 class="title">' . t('Role-profile assignments') . '</h2>';
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= '<div class="form-item"><div class="description">' . t('Assign profiles to user roles for available file systems. Your default file system is %name.', array('%name' => $swrappers[config_get('system.core', 'file_default_scheme')]['name'])) . ' ' . $weight_info . '</div></div>';
  $output .= backdrop_render($form['common']);
  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * Validate admin form.
 */
function imce_admin_form_validate($form, &$form_state) {
  $roles = $form_state['values']['roles'];
  // Check anonymous profile. Do not allow wildcard upload.
  if ($key = imce_admin_check_wildcard_upload(BACKDROP_ANONYMOUS_ROLE, $roles)) {
    form_error($form['roles'][BACKDROP_ANONYMOUS_ROLE][$key], t('Anonymous user role can not have a configuration profile with unrestricted file extensions.'));
  }
}

/**
 * Submit admin form.
 */
function imce_admin_submit($form, &$form_state) {
  $config = config('imce.settings');

  $roles = $form_state['values']['roles'];
  if (count($roles) > 3) {
    uasort($roles, 'imce_rolesort');
  }
  $config->set('imce_roles_profiles', $roles);
  $config->set('imce_settings_textarea', $form_state['values']['textarea']);
  $config->set('imce_settings_absurls', $form_state['values']['absurls']);
  $config->set('imce_settings_replace', $form_state['values']['replace']);
  $config->set('imce_settings_thumb_method', $form_state['values']['thumb_method']);
  $config->set('imce_settings_disable_private', $form_state['values']['disable_private']);
  $config->save();
  backdrop_set_message(t('Changes have been saved.'));
}

/**
 * Add-Edit-Delete profiles.
 */
function imce_profile_operations($op = 'add', $pid = 0) {
  //delete
  if ($op == 'delete') {
    backdrop_set_title(t('Delete configuration profile'));
    return backdrop_get_form('imce_profile_delete_form', $pid);
  }
  //add-edit
  if ($op === 'add' || $op === 'edit') {
    return backdrop_get_form('imce_profile_form', $pid);
  }
  backdrop_access_denied();
}

/**
 * Profile form.
 */
function imce_profile_form($form, &$form_state, $pid = 0) {

  if ($pid && $profile = imce_load_profile($pid)) {
    backdrop_set_title($profile['name']);
  }
  else {
    $pid = 0;
    $profile = imce_sample_profile();
    $profile['name'] = '';
  }
  //import profile
  if (isset($_GET['import']) && $imported = imce_load_profile($_GET['import'])) {
    if (empty($form_state['post'])) {
      backdrop_set_message(t('Settings were imported from the profile %name', array('%name' => $imported['name'])));
    }
    $imported['name'] = $profile['name']; //preserve the original name.
    $profile = $imported;
  }

  $form_state['profile'] = $profile;//store the original profile just in case.

  $form = array('#tree' => TRUE);
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Profile name'),
    '#default_value' => $profile['name'],
    '#description' => t('Give a name to this profile.'),
    '#required' => TRUE,
  );
  $form['import'] = array(
    '#markup' => imce_profile_import_html($pid),
  );
  $form['usertab'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display file browser tab in user profile pages.'),
    '#default_value' => $profile['usertab'],
  );
  $form['filesize'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum file size per upload'),
    '#default_value' => $profile['filesize'],
    '#description' => t('Set to 0 to use the maximum value avaliable.') . ' ' . t('Your PHP settings limit the maximum file size per upload to %size.', array('%size' => format_size(file_upload_max_size()))),
    '#field_suffix' => t('MB'),
  );
  $form['quota'] = array(
    '#type' => 'textfield',
    '#title' => t('Directory quota'),
    '#default_value' => $profile['quota'],
    '#description' => t('Define the upload quota per directory.') . ' ' . t('Set to 0 to use the maximum value avaliable.'),
    '#field_suffix' => t('MB'),
  );
  $form['tuquota'] = array(
    '#type' => 'textfield',
    '#title' => t('Total user quota'),
    '#default_value' => $profile['tuquota'],
    '#description' => t('This quota measures the size of all user uploaded files in the database and does not include FTP files. You can either use both quotations together or safely ignore this by setting the value to 0.'),
    '#field_suffix' => t('MB'),
  );
  $form['extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Permitted file extensions'),
    '#default_value' => $profile['extensions'],
    '#maxlength' => 255,
    '#description' => t('Specify the allowed file extensions for uploaded files. Separate extensions with a space and do not include the leading dot.') . ' ' . t('Set to * to remove the restriction.'),
  );
  $form['dimensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum image dimensions'),
    '#default_value' => $profile['dimensions'],
    '#description' => t('The maximum allowed image size (e.g. 640x480). Set to 0 for no restriction. If an <a href="!image-toolkit-link">image toolkit</a> is installed, files exceeding this value will be scaled down to fit.', array('!image-toolkit-link' => url('admin/config/media/image-toolkit'))),
    '#field_suffix' => '<kbd>' . t('WIDTHxHEIGHT') . '</kbd>',
  );
  $form['filenum'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of files per operation'),
    '#default_value' => $profile['filenum'],
    '#description' => t('You can allow users to select multiple files for operations such as delete, resize, etc. Entire batch file operation is executed in a single drupal load, which may be good. However there will be an increase in script execution time, cpu load and memory consumption possibly exceeding the limits of your server, which is really bad. For unlimited number of file handling, set this to 0.'),
  );

  //Directories
  $form['directories']['#theme'] = 'imce_directories';
  $form['directories']['#weight'] = 1;
  for ($i = 0; $i < count($profile['directories']); $i++) {
    $form['directories'][$i] = imce_directory_form($profile['directories'][$i]);
  }
  $form['directories'][$i] = imce_directory_form();
  $form['directories'][$i+1] = imce_directory_form();

  //Thumbnails
  $form['thumbnails']['#theme'] = 'imce_thumbnails';
  $form['thumbnails']['#weight'] = 2;
  for ($i = 0; $i < count($profile['thumbnails']); $i++) {
    $form['thumbnails'][$i] = imce_thumbnail_form($profile['thumbnails'][$i]);
  }
  $form['thumbnails'][$i] = imce_thumbnail_form();
  $form['thumbnails'][$i+1] = imce_thumbnail_form();

  $form = array('profile' => $form);
  $form['pid'] = array('#type' => 'hidden', '#value' => $pid);
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#validate'][] = 'imce_profile_validate';
  $form['#submit'][] = 'imce_profile_submit';
  return $form;
}

/**
 * Profile form validate.
 */
function imce_profile_validate($form, &$form_state) {
  $profile = &$form_state['values']['profile'];
  $dim_re = '/^\d+x\d+$/';
  $dim_error = t('Dimensions must be specified in <kbd>WIDTHxHEIGHT</kbd> format.');
  // Check max image dimensions
  if ($profile['dimensions'] && !preg_match($dim_re, $profile['dimensions'])) {
    return form_set_error('profile][dimensions', $dim_error);
  }
  // Check thumbnails dimensions
  foreach ($profile['thumbnails'] as $i => $thumb) {
    if (trim($thumb['name']) != '' && !preg_match($dim_re, $thumb['dimensions'])) {
      return form_set_error("profile][thumbnails][$i][dimensions", $dim_error);
    }
  }
}

/**
 * Profile form submit.
 */
function imce_profile_submit($form, &$form_state) {
  $profile = $form_state['values']['profile'];
  $pid = $form_state['values']['pid'];
  $message = $pid > 0 ? t('The changes have been saved.') : t('Profile has been added.');

  //unset empty fields of directories and thumbnails.
  imce_clean_profile_fields($profile);

  //save profile.
  $pid = imce_update_profiles($pid, $profile);

  backdrop_set_message($message);
  $form_state['redirect'] = 'admin/config/media/imce/profile/edit/' . $pid;
}

/**
 * directory settings form
 */
function imce_directory_form($directory = array()) {
  if (empty($directory)) {
    $directory = array('name' => '', 'subnav' => 0, 'browse' => 0, 'upload' => 0, 'thumb' => 0, 'delete' => 0, 'resize' => 0);
  }
  $form['name'] = array(
    '#type' => 'textfield',
    '#default_value' => $directory['name'],
    '#size' => 24,
    '#maxlength' => NULL,
  );
  $form['subnav'] = array(
    '#type' => 'checkbox',
    '#title' => t('Including subdirectories'),
    '#default_value' => $directory['subnav'],
  );
  $form['browse'] = array(
    '#type' => 'checkbox',
    '#title' => t('Browse'),
    '#default_value' => $directory['browse'],
  );
  $form['upload'] = array(
    '#type' => 'checkbox',
    '#title' => t('Upload'),
    '#default_value' => $directory['upload'],
  );
  $form['thumb'] = array(
    '#type' => 'checkbox',
    '#title' => t('Thumbnails'),
    '#default_value' => $directory['thumb'],
  );
  $form['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete'),
    '#default_value' => $directory['delete'],
  );
  $form['resize'] = array(
    '#type' => 'checkbox',
    '#title' => t('Resize'),
    '#default_value' => $directory['resize'],
  );
  return $form;
}

/**
 * Directorys form themed.
 */
function imce_directories_theme($variables) {
  $form = $variables['form'];
  $rows = array();
  $root = t('root');

  foreach (element_children($form) as $key) {
    //directory path
    $row = array('<div class="container-inline">&lt;' . $root . '&gt;' . '/' . backdrop_render($form[$key]['name']) . '</div>' . backdrop_render($form[$key]['subnav']));
    unset($form[$key]['name'], $form[$key]['subnav']);

    //permissions
    $header = array();
    foreach (element_children($form[$key]) as $perm) {
      $header[] = $form[$key][$perm]['#title'];
      unset($form[$key][$perm]['#title']);
      $row[] = backdrop_render($form[$key][$perm]);
    }

    $rows[] = $row;
  }

  array_unshift($header, t('Directory path'));

  $output = '<h3 class="title">' . t('Directories') . '</h3>';
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= '<div class="form-item"><div class="description">' . t('Define directories that users of this profile can access.
<ul>
	<li>Use alphanumeric characters as directory paths.</li>
	<li>To specify file system root, just enter <strong>.</strong>(dot) character.</li>
	<li>Use <strong>%uid</strong> as a placeholder for user ID. Ex: <em>users/user%uid</em> creates directories such as <em>users/user1</em>, <em>users/user42</em>, etc.</li>
  <li>To remove a directory from the list, leave the directory path blank.</li>
  <li>If you want more flexibility in directory paths you can execute php to return a directory path.<br />
  For php execution your directory path must start with <strong>php:</strong> and the rest must be a valid php code that is expected to return the actual directory path. <br />Ex: <strong>php: return \'users/\'.$user->name;</strong> defines <strong>users/USER-NAME</strong> as the directory path.<br />
  A multi-level directory example <strong>php: return date(\'Y\', $user->created).\'/\'.date(\'m\', $user->created).\'/\'.$user->uid;</strong> defines <strong>MEMBERSHIP-YEAR/MONTH/USER-ID</strong> as the directory path, resulting in self-categorized user directories based on membership date.<br />
  Note that you should use the $user variable instead of $GLOBALS[\'user\'] since they are not always the same object.</li>
</ul>
<p>Note that thumbnails permission does not affect thumbnail creation on upload. See thumbnails decription below.</p>
<p>If you need more fields, just fill all and save, and you will get two more on the next page.</p>') . '</div></div>';
  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * thumbnail settings form
 */
function imce_thumbnail_form($thumb = array()) {
  if (empty($thumb)) {
    $thumb = array('name' => '', 'dimensions' => '', 'prefix' => '', 'suffix' => '');
  }
  $form['name'] = array(
    '#type' => 'textfield',
    '#default_value' => $thumb['name'],
    '#size' => 20,
  );
  $form['dimensions'] = array(
    '#type' => 'textfield',
    '#default_value' => $thumb['dimensions'],
    '#size' => 20,
  );
  $form['prefix'] = array(
    '#type' => 'textfield',
    '#default_value' => $thumb['prefix'],
    '#size' => 20,
  );
  $form['suffix'] = array(
    '#type' => 'textfield',
    '#default_value' => $thumb['suffix'],
    '#size' => 20,
  );
  return $form;
}

/**
 * Thumbnails form themed.
 */
function imce_thumbnails_theme($variables) {
  $form = $variables['form'];
  $header = array(t('Name'), t('Dimensions'), t('Prefix'), t('Suffix'));
  $rows = array();

  foreach (element_children($form) as $key) {
    $rows[] = array(
      backdrop_render($form[$key]['name']),
      backdrop_render($form[$key]['dimensions']),
      backdrop_render($form[$key]['prefix']),
      backdrop_render($form[$key]['suffix']),
    );
  }

  $output = '<h3 class="title">' . t('Thumbnails') . '</h3>';
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= '<div class="form-item"><div class="description">' . t('You may create a list of thumbnail options that users can choose from.
<ul>
  <li>Use alphanumeric characters as thumbnail names.</li>
  <li>Specify dimensions as <strong>WidthxHeight</strong>.</li>
  <li>Prefix and suffix are strings that are added to original file name to create the thumbnail name.</li>
  <li>An example thumbnail: Name = <strong>Small</strong>, Dimensions = <strong>80x80</strong>, Prefix = <strong>small_</strong></li>
</ul>
<p>Note that users will always be able to create these thumbnails on file upload no matter what the thumbnail permission is.</p>
<p>If you need more fields, just fill all and save, and you will get two more on the next page.</p>') . '</div></div>';
  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * Role-profile form
 */
function imce_role_form($role, $weight = TRUE, $core = TRUE) {
  $form['name'] = array(
    '#markup' => check_plain($role['name']),
  );
  if ($weight) {
    $form['weight'] = $core ? array(
      '#type' => 'textfield',
      '#value' => $role['weight'],
      '#attributes' => array('readonly' => 'readonly', 'style' => 'border: none; width: 2em; background-color: transparent;'),
    ) : array(
      '#type' => 'weight',
      '#default_value' => $role['weight'],
    );
  }
  foreach (array_keys(file_get_stream_wrappers(STREAM_WRAPPERS_VISIBLE)) as $scheme) {
    $form[$scheme . '_pid'] = array(
      '#type' => 'select',
      '#options' => imce_profile_options(),
      '#default_value' => $role[$scheme . '_pid'],
      '#empty_value' => 0,
    );
  }
  return $form;
}

/**
 * Profile delete form
 */
function imce_profile_delete_form($form, &$form_state, $pid) {
  if ($pid > 1 && $profile = imce_load_profile($pid)) {
    $form['#submit'][] = 'imce_profile_delete_submit';
    $form['pid'] = array('#type' => 'hidden', '#value' => $pid);
    return confirm_form($form,
      t('Are you sure you want to delete the profile %name?',
      array('%name' => $profile['name'])),
      'admin/config/media/imce',
      '',
      t('Delete'),
      t('Cancel')
    );
  }
  backdrop_goto('admin/config/media/imce');
}

/**
 * Profile delete form submit
 */
function imce_profile_delete_submit($form, &$form_state) {
  imce_update_profiles($form_state['values']['pid'], NULL);
  backdrop_set_message(t('Profile has been deleted.'));
  $form_state['redirect'] = 'admin/config/media/imce';
}

/**
 * Profile options.
 */
function imce_profile_options() {
  $options = array();
  foreach (config_get('imce.settings', 'imce_profiles') as $pid => $profile) {
    $options[$pid] = $profile['name'];
  }
  return $options;
}

/**
 * Profile import links.
 */
function imce_profile_import_html($pid = 0) {
  $output = '';
  $links = array();

  foreach (config_get('imce.settings', 'imce_profiles') as $id => $profile) {
    if ($pid != $id) {
      $links[] = l($profile['name'], $_GET['q'], array('query' => array('import' => $id)));
    }
  }

  if (!empty($links)) {
    $output = '<p><strong>' . t('Import settings from other profiles') . '</strong>: ';
    $output .= implode(', ', $links) . '</p>';
  }

  return $output;
}

/**
 * Update role-profile assignments.
 */
function imce_update_roles($pid) {
  $roles = config_get('imce.settings', 'imce_roles_profiles');
  foreach ($roles as $rid => $role) {
    foreach ($role as $key => $value) {
      if (substr($key, -4) == '_pid') {
        if ($value == $pid) {
          $roles[$rid][$key] = 0;
        }
        elseif ($value > $pid) {
          $roles[$rid][$key]--;
        }
      }
    }
  }
  config_set('imce.settings', 'imce_roles_profiles', $roles);
}

/**
 * Add, update or delete a profile.
 */
function imce_update_profiles($pid, $profile = NULL) {
  $profiles = config_get('imce.settings', 'imce_profiles');

  //add or update
  if (isset($profile)) {
    $pid = isset($profiles[$pid]) ? $pid : count($profiles)+1;
    $profiles[$pid] = $profile;
  }

  //delete
  elseif (isset($profiles[$pid]) && $pid > 1) {
    unset($profiles[$pid]);
    for ($i = $pid+1; isset($profiles[$i]); $i++) {
      $profiles[$i-1] = $profiles[$i];
      unset($profiles[$i]);
    }
    imce_update_roles($pid);
  }

  config_set('imce.settings', 'imce_profiles', $profiles);
  return $pid;
}

/**
 * Unset empty fields in thumbnails and directory paths.
 */
function imce_clean_profile_fields(&$profile) {
  $clean = array();
  foreach ($profile['thumbnails'] as $thumb) {
    if (trim($thumb['name']) != '') {
      $clean[] = $thumb;
    }
  }
  $profile['thumbnails'] = $clean;

  $clean = array();
  $names = array();
  foreach ($profile['directories'] as $dir) {
    $dir['name'] = trim($dir['name'], '/ ');
    if ($dir['name'] == '') {
      continue;
    }
    if (isset($names[$dir['name']])) {
      backdrop_set_message(t('Duplicate directory paths are not allowed.'), 'error');
      continue;
    }
    if (!imce_reg_dir($dir['name'])) {
      backdrop_set_message(t('%dirname is not accepted as a proper directory name.', array('%dirname' => $dir['name'])), 'error');
      continue;
    }
    $clean[] = $dir;
    $names[$dir['name']] = 1;
  }
  $profile['directories'] = $clean;
}

/**
 * Profile load.
 */
function imce_load_profile($pid) {
  $profiles = config_get('imce.settings', 'imce_profiles');
  return isset($profiles[$pid]) ? $profiles[$pid] : NULL;
}

/**
 * Sort roles according to their weights.
 */
function imce_sorted_roles() {
  $config = config('imce.settings');

  static $sorted;
  if (!isset($sorted)) {
    $sorted = array();
    $roles = user_roles();
    $profiles = $config->get('imce_profiles');
    $roles_profiles = $config->get('imce_roles_profiles');
    $roles_profiles[BACKDROP_ANONYMOUS_ROLE]['weight'] = 12;
    $roles_profiles[BACKDROP_AUTHENTICATED_ROLE]['weight'] = 11;
    $schemes = array_keys(file_get_stream_wrappers(STREAM_WRAPPERS_VISIBLE));
    foreach ($roles as $rid => $name) {
      $sorted[$rid] = array(
        'name' => $name,
        'weight' => isset($roles_profiles[$rid]['weight']) ? $roles_profiles[$rid]['weight'] : 0
      );
      foreach ($schemes as $scheme) {
        $key = $scheme . '_pid';
        $sorted[$rid][$key] = isset($roles_profiles[$rid][$key]) && isset($profiles[$roles_profiles[$rid][$key]]) ? $roles_profiles[$rid][$key] : 0;
      }
    }
    uasort($sorted, 'imce_rolesort');
  }
  return $sorted;
}

/**
 * Sorting function for roles.
 */
function imce_rolesort($r1, $r2) {
  return $r1['weight']-$r2['weight'];
}

/**
 * Checks if the given role can upload all extensions.
 */
function imce_admin_check_wildcard_upload($rid, $conf = NULL) {
  if (!isset($conf)) {
    $conf = config_get('imce.settings', 'imce_roles_profiles');
  }
  if (!empty($conf[$rid])) {
    foreach ($conf[$rid] as $key => $pid) {
      if ($pid && substr($key, -4) == '_pid') {
        if ($profile = imce_load_profile($pid)) {
          if ($profile['extensions'] === '*' && !empty($profile['directories'])) {
            foreach ($profile['directories'] as $dirconf) {
              if (!empty($dirconf['upload'])) {
                return $key;
              }
            }
          }
        }
      }
    }
  }
  return FALSE;
}

//Include core profile functions.
include_once BACKDROP_ROOT . '/' . backdrop_get_path('module', 'imce') . '/includes/imce.core.profiles.inc';