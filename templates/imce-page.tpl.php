<!DOCTYPE html>
<html lang="<?php print $GLOBALS['language']->langcode; ?>" class="imce">

<head>
  <title><?php print t('File Browser'); ?></title>
  <meta name="robots" content="noindex,nofollow" />
  <?php if (isset($_GET['app'])): backdrop_add_js(backdrop_get_path('module', 'imce') .'/js/imce_set_app.js'); endif;?>
  <?php print backdrop_get_html_head(); ?>
  <?php print backdrop_get_css(); ?>
  <?php print backdrop_get_js('header'); ?>
  <style media="all" type="text/css">/*Quick-override*/</style>
</head>

<body class="imce">
<div id="imce-messages"><?php print theme('status_messages'); ?></div>
<?php print $content; ?>
<?php print backdrop_get_js('footer'); ?>
</body>

</html>
