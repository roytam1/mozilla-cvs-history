<?php
class MailComponent extends Object {
  var $from;
  var $to;
  var $reply;
  var $subject;
  var $message;
  var $envelope;
  var $head = "<strong>Firefox Party!</strong><br/>";
  var $foot;


  function mail($params) {
    if (array_key_exists('from', $params))
      $this->from = $params['from'];

    if (array_key_exists('to', $params))
      $this->to = $params['to'];

    if (array_key_exists('reply', $params))
      $this->reply = $params['reply'];

    if (array_key_exists('subject', $params))
      $this->subject = $params['subject'];

    if (array_key_exists('message', $params))
      $this->message = $params['message'];

    if (array_key_exists('envelope', $params))
      $this->envelope = $params['envelope'];

    if (array_key_exists('type', $params)) {
      switch($params['type']) {
        case 'act':
          $this->message = $this->head."<br/>\nThank you for registering! To activate your account, <a href=\"".$params['link']."\">click here</a> or paste the link below into your browser:<br/> ".$params['link'].$this->foot;
          break;

        case 'prec':
          $this->message = $this->head."<br/>\nTo reset your password, <a href=\"".$params['link']."\">click here</a> or paste the link below into your browser:<br/> ".$params['link'].$this->foot;
          break;

        case 'invite':
          $this->message = $this->head."<br/>\nYou've been invited by a friend to join them in celebrating the release of Firefox 2. Simply <a href=\"".$params['link']."\">click here</a> to confirm or cancel this invitation. If you don't already have an account, you'll need to create one.\n
                           If you're unable to use the link above, simply paste the following URL into your browser: ".$params['link'].$this->foot;
          break;

        case 'cancel':
          $this->message = $this->head."<br/>\nThe party you were attending has been cancelled. For more information, please <a href=\"".$params['link']."\">click here</a>, or see the link below.\n ".$params['link'].$this->foot;
          break;
      }
    }
  }

  function make_headers($type='html') {
    $headers = '';

    switch($type) {
      case 'html':
        $headers .= 'MIME-Version: 1.0' . "\r\n";
        $headers .= 'Content-type: text/html; charset=iso-8859-1' . "\r\n";
        break;
    }

    if (!empty($this->from)) {
      $headers .= "From: {$this->from}\r\n";

      if (!empty($this->reply))
        $headers .= "Reply-To: {$this->reply}\r\n";
    }
    return $headers;
  }

  function make_additional_parameters() {
    if (!empty($this->envelope)) {
      return '-f'.$this->envelope;
    }
  }

  function send() {
    mail($this->to, $this->subject, $this->message, $this->make_headers(), $this->make_additional_parameters());
  }
}
?>
