<?php

// Time To Live, 2 hours seems reasonable, 1 hour shows some number of
// failed requests (~20 per day).
define("TTL", 2 * 60 * 60);
define("LOG_TIME_FORMAT", "Y-m-d H:i:s");

include("uniqueUrl.php");

function abort_log($client_msg, $msg)
{
  echo $client_msg;

  $log = fopen("/opt/plugin-files/logs/error.log", "a");

  if (!$log) {
    die("Error opening log file!");
  }

  if (!flock($log, LOCK_EX)) {
    die("Error locking log file!");
  }

  fseek($log, 0, SEEK_END);

  fwrite($log, date(LOG_TIME_FORMAT));

  fwrite($log, " {$msg}\n");

  flock($log, LOCK_UN);

  fclose($log);

  exit;
}

ignore_user_abort(TRUE);

$d = decodeUniqueDownloadUrl($_SERVER["QUERY_STRING"]);

if (!$d) {
  abort_log("Bad URI!", "Request for download with invalid query string '" .
            $_SERVER['QUERY_STRING'] . "'");
}

if (time() - $d['ctime'] > TTL) {
  header('HTTP/1.0 401 File not found');

  abort_log("URL expired", "Request for expired URI, created on " .
	    date(LOG_TIME_FORMAT, $d['ctime']));
}

//echo "time = " . strftime("%x %X", $d["ctime"]) . "\n";

$path = "/opt/plugin-files/installers/{$d['file']}";

if (!file_exists($path)) {
  abort_log("File not found!", "Request for missing file '{$d['file']}'");
}

$file_size = filesize($path);

header("Content-Type: application/x-xpinstall");
header("Content-Length: {$file_size}");
header('Pragma: no-cache'); 

$fd = fopen($path, "rb");

$bytes_read = 0;

while (!feof($fd) && !connection_aborted()) {
  $s = fread($fd, 4096);

  $bytes_read += strlen($s);

  print($s);

  flush();
}

fclose($fd);

$pending_log = sprintf("/opt/plugin-files/pending/%s-%s-%u-%u",
		       $d['file'], date("Ymd-H:i:s", $d['ctime']), $d['pid'],
		       $d['r']);

if ($bytes_read != $file_size || connection_aborted()) {
  $file_log = fopen($pending_log, "a");
  fwrite($file_log, date(LOG_TIME_FORMAT) .
	 " sent {$bytes_read} of {$file_size}, status = " .
	 connection_status() . "\n");

  fclose($file_log);

  exit;
}

// This request is no longer pending, remove the 'pending' log.
if (file_exists($pending_log)) {
  unlink($pending_log);
}

$log = fopen("/opt/plugin-files/logs/{$d['file']}.log", "a");

if (!$log) {
  die("Error opening log file!");
}

if (!flock($log, LOCK_EX)) {
  die("Error locking log file!");
}

fseek($log, 0, SEEK_END);

fwrite($log, date(LOG_TIME_FORMAT));

fwrite($log, " finished, stime " . date(LOG_TIME_FORMAT, $d['ctime']) . "\n");

flock($log, LOCK_UN);

fclose($log);

?>