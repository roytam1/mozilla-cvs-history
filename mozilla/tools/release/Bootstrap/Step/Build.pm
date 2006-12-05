#
# Build step. Calls tinderbox to produce en-US Firefox build.
#
package Bootstrap::Step::Build;
use Bootstrap::Step;
use Bootstrap::Config;
@ISA = ("Bootstrap::Step");

my $config = new Bootstrap::Config;

sub Execute {
    my $this = shift;

    my $buildDir = $config->Get('var' => 'buildDir');
    my $productTag = $config->Get('var' => 'productTag');
    my $rc = $config->Get('var' => 'rc');
    my $buildPlatform = $config->Get('var' => 'buildPlatform');
    my $logDir = $config->Get('var' => 'logDir');
    my $rcTag = $productTag . '_RC' . $rc;

    my $lastBuilt = $buildDir . '/' . $buildPlatform . '/last-built';
    unlink($lastBuilt) 
      or $this->Log('msg' => "Cannot unlink last-built file $lastBuilt: $!");
    $this->Log('msg' => "Unlinked $lastBuilt");

    my $buildLog = $logDir . '/' . $rcTag . '-build.log';
 
    $this->Shell(
      'cmd' => './build-seamonkey.pl --once --mozconfig mozconfig --depend --config-cvsup-dir ' . $buildDir . '/tinderbox-configs',
      'dir' => $buildDir,
      'logFile' => $buildLog,
      'timeout' => 36000
    );
}

sub Verify {
    my $this = shift;

    my $buildDir = $config->Get('var' => 'buildDir');
    my $productTag = $config->Get('var' => 'productTag');
    my $rc = $config->Get('var' => 'rc');
    my $rcTag = $productTag.'_RC'.$rc;
    my $logDir = $config->Get('var' => 'logDir');

    my $buildLog = $logDir . '/' . $rcTag . '-build.log';

    $this->CheckLog(
        'log' => $buildLog,
        'notAllowed' => 'tinderbox: status: failed',
    );

#    $this->CheckLog(
#        'log' => $buildLog,
#        'notAllowed' => '^Error:',
#    );
}

1;
