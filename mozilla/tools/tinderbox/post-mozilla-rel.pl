#!/usr/bin/perl
#

#
# This script gets called after a full mozilla build & test.
#
# packages and delivers mozilla bits
# Assumptions:
#  mozilla tree
#  produced builds get put in Topsrcdir/installer/sea
#
#
# tinder-config variables that you should set:
#  package_creation_path: directory to run 'make installer' to create an
#                         installer, and 'make' to create a zip/tar build
#  ftp_path: directory to upload nightly builds to
#  url_path: absolute URL to nightly build directory
#  tbox_ftp_path: directory to upload tinderbox builds to
#  tbox_url_path: absolute URL to tinderbox builds directory
#  notify_list: list of email addresses to notify of nightly build completion
#  build_hour: upload the first build completed after this hour as a nightly
#  ssh_server: server to upload build to via ssh
#  ssh_user: user to log in to ssh with
#  ssh_version: if set, force ssh protocol version N
#  milestone: suffix to append to date and latest- directory names
#  stub_installer: (0/1) whether to upload a stub installer
#  sea_installer: (0/1) whether to upload a sea (blob) installer
#  archive: (0/1) whether to upload an archive (tar or zip) build
#
#  windows-specific variables:
#   as_perl_path: cygwin-ized path to Activestate Perl's bin directory

use strict;
use Sys::Hostname;

package PostMozilla;

use Cwd;

# This is set in PreBuild(), and is checked after each build.
my $cachebuild = 0;

sub is_windows { return $Settings::OS =~ /^WIN/; }
sub is_linux { return $Settings::OS eq 'Linux'; }
sub is_os2 { return $Settings::OS eq 'OS2'; }
# XXX Not tested on mac yet.  Probably needs changes.
sub is_mac { return $Settings::OS eq 'Darwin'; }
sub do_installer { return is_windows() || is_linux() || is_os2(); }

sub shorthost {
  my $host = ::hostname();
  $host = $1 if $host =~ /(.*?)\./;
  return $host;
}

sub print_locale_log {
    my ($text) = @_;
    print LOCLOG $text;
    print $text;
}

sub run_locale_shell_command {
    my ($shell_command) = @_;
    local $_;

    my $status = 0;
    chomp($shell_command);
    print_locale_log "$shell_command\n";
    open CMD, "$shell_command $Settings::TieStderr |" or die "open: $!";
    print_locale_log $_ while <CMD>;
    close CMD or $status = 1;
    return $status;
}

sub mail_locale_started_message {
    my ($start_time, $locale) = @_;
    my $msg_log = "build_start_msg.tmp";
    open LOCLOG, ">$msg_log";

    my $platform = $Settings::OS =~ /^WIN/ ? 'windows' : 'unix';

    print_locale_log "\n";
    print_locale_log "tinderbox: tree: $Settings::BuildTree-$locale\n";
    print_locale_log "tinderbox: builddate: $start_time\n";
    print_locale_log "tinderbox: status: building\n";
    print_locale_log "tinderbox: build: $Settings::BuildName $locale\n";
    print_locale_log "tinderbox: errorparser: $platform\n";
    print_locale_log "tinderbox: buildfamily: $platform\n";
    print_locale_log "tinderbox: version: $::Version\n";
    print_locale_log "tinderbox: END\n";
    print_locale_log "\n";

    close LOCLOG;

    if ($Settings::blat ne "" && $Settings::use_blat) {
        system("$Settings::blat $msg_log -t $Settings::Tinderbox_server");
    } else {
        system "$Settings::mail $Settings::Tinderbox_server "
            ." < $msg_log";
    }
    unlink "$msg_log";
}

sub mail_locale_finished_message {
    my ($start_time, $build_status, $logfile, $locale) = @_;

    # Rewrite LOG to OUTLOG, shortening lines.
    open OUTLOG, ">$logfile.last" or die "Unable to open logfile, $logfile: $!";

    my $platform = $Settings::OS =~ /^WIN/ ? 'windows' : 'unix';

    # Put the status at the top of the log, so the server will not
    # have to search through the entire log to find it.
    print OUTLOG "\n";
    print OUTLOG "tinderbox: tree: $Settings::BuildTree-$locale\n";
    print OUTLOG "tinderbox: builddate: $start_time\n";
    print OUTLOG "tinderbox: status: $build_status\n";
    print OUTLOG "tinderbox: build: $Settings::BuildName $locale\n";
    print OUTLOG "tinderbox: errorparser: $platform\n";
    print OUTLOG "tinderbox: buildfamily: $platform\n";
    print OUTLOG "tinderbox: version: $::Version\n";
    print OUTLOG "tinderbox: utilsversion: $::UtilsVersion\n";
    print OUTLOG "tinderbox: logcompression: $Settings::LogCompression\n";
    print OUTLOG "tinderbox: logencoding: $Settings::LogEncoding\n";
    print OUTLOG "tinderbox: END\n";

    if ($Settings::LogCompression eq 'gzip') {
        open GZIPLOG, "gzip -c $logfile |" or die "Couldn't open gzip'd logfile: $!\n";
        TinderUtils::encode_log(\*GZIPLOG, \*OUTLOG);
        close GZIPLOG;
    }
    elsif ($Settings::LogCompression eq 'bzip2') {
        open BZ2LOG, "bzip2 -c $logfile |" or die "Couldn't open bzip2'd logfile: $!\n";
        TinderUtils::encode_log(\*BZ2LOG, \*OUTLOG);
        close BZ2LOG;
    }
    else {
        open LOCLOG, "$logfile" or die "Couldn't open logfile, $logfile: $!";
        TinderUtils::encode_log(\*LOCLOG, \*OUTLOG);
        close LOCLOG;
    }    
    close OUTLOG;
    unlink($logfile);

    # If on Windows, make sure the log mail has unix lineendings, or
    # we'll confuse the log scraper.
    if ($platform eq 'windows') {
        open(IN,"$logfile.last") || die ("$logfile.last: $!\n");
        open(OUT,">$logfile.new") || die ("$logfile.new: $!\n");
        while (<IN>) {
            s/\r\n$/\n/;
	    print OUT "$_";
        } 
        close(IN);
        close(OUT);
        File::Copy::move("$logfile.new", "$logfile.last") or die("move: $!\n");
    }

    if ($Settings::ReportStatus and $Settings::ReportFinalStatus) {
        if ($Settings::blat ne "" && $Settings::use_blat) {
            system("$Settings::blat $logfile.last -t $Settings::Tinderbox_server");
        } else {
            system "$Settings::mail $Settings::Tinderbox_server "
                ." < $logfile.last";
        }
    }
}

sub stagesymbols {
  my $builddir = shift;
  TinderUtils::run_shell_command "make -C $builddir deliver";
}

sub makefullsoft {
  my $builddir = shift;
  if (is_windows()) {
    # need to convert the path in case we're using activestate perl
    $builddir = `cygpath -u $builddir`;
  }
  chomp($builddir);
  # should go in config
  my $moforoot = "cltbld\@cvs.mozilla.org:/mofo"; 
  $ENV{CVS_RSH} = "ssh" unless defined($ENV{CVS_RSH});
  my $fullsofttag = " ";
  $fullsofttag = " -r $Settings::BuildTag"
        unless not defined($Settings::BuildTag) or $Settings::BuildTag eq '';
  TinderUtils::run_shell_command "cd $builddir; cvs -d$moforoot co $fullsofttag -d fullsoft talkback/fullsoft";
  TinderUtils::run_shell_command "cd $builddir/fullsoft; $builddir/build/autoconf/make-makefile -d ..";
  TinderUtils::run_shell_command "make -C $builddir/fullsoft";
  TinderUtils::run_shell_command "make -C $builddir/fullsoft fullcircle-push";
  if (is_mac()) {
    TinderUtils::run_shell_command "make -C $builddir/$Settings::mac_bundle_path";
  }
}

sub processtalkback {
  # first argument is whether to make a new talkback build on server
  #                and upload debug symbols
  # second argument is where we're building our tree
  my $makefullsoft      = shift;
  my $builddir      = shift;   
  # put symbols in builddir/dist/buildid
  stagesymbols($builddir); 
  if ($makefullsoft) {
    $ENV{FC_UPLOADSYMS} = 1;
    makefullsoft($builddir);
  }
  
}

sub packit {
  my ($packaging_dir, $package_location, $url, $stagedir, $builddir) = @_;
  my $status;

  if (is_windows()) {
    # need to convert the path in case we're using activestate perl
    $builddir = `cygpath -u $builddir`;
  }
  chomp($builddir);

  mkdir($stagedir, 0775);

  if (do_installer()) {
    if (is_windows()) {
      $ENV{INSTALLER_URL} = "$url/windows-xpi";
    } elsif (is_linux()) {
      $ENV{INSTALLER_URL} = "$url/linux-xpi/";
    } elsif (is_os2()) {
      $ENV{INSTALLER_URL} = "$url/os2-xpi/";
    } else {
      die "Can't make installer for this platform.\n";
    }
    TinderUtils::print_log "INSTALLER_URL is " . $ENV{INSTALLER_URL} . "\n";

    mkdir($package_location, 0775);

    # the Windows installer scripts currently require Activestate Perl.
    # Put it ahead of cygwin perl in the path.
    my $save_path;
    if (is_windows()) {
      $save_path = $ENV{PATH};
      $ENV{PATH} = $Settings::as_perl_path.":".$ENV{PATH};
    }

    # the one operation we care about saving status of
    if ($Settings::sea_installer || $Settings::stub_installer) {
      $status = TinderUtils::run_shell_command "make -C $packaging_dir installer";
    } else {
      $status = 0;
    }

    if (is_windows()) {
      $ENV{PATH} = $save_path;
      #my $dos_stagedir = `cygpath -w $stagedir`;
      #chomp ($dos_stagedir);
    }

    my $push_raw_xpis;
    if ($Settings::stub_installer) {
      $push_raw_xpis = 1;
    } else {
      $push_raw_xpis = $Settings::push_raw_xpis;
    }

    if (is_windows() || is_os2()) {
      if ($Settings::stub_installer) {
        TinderUtils::run_shell_command "cp $package_location/stub/*.exe $stagedir/";
      }
      if ($Settings::sea_installer) {
        TinderUtils::run_shell_command "cp $package_location/sea/*.exe $stagedir/";
      }

      # If mozilla/dist/install/*.msi exists, copy it to the staging
      # directory.
      my @msi = grep { -f $_ } <${package_location}/*.msi>;
      if ( scalar(@msi) gt 0 ) {
	my $msi_files = join(' ', @msi);
        TinderUtils::run_shell_command "cp $msi_files $stagedir/";
      }

      if ($push_raw_xpis) {
        # We need to recreate the xpis with compression on, for update.
        # Since we've already copied over the 7zip-compressed installer, just
        # re-run the installer creation with 7zip disabled to get compressed
        # xpi's, then copy them to stagedir.
        #
        # Also set MOZ_PACKAGE_MSI to null to avoid repackaging it
        # unnecessarily.
        my $save_msi = $ENV{MOZ_PACKAGE_MSI};
        my $save_7zip = $ENV{MOZ_INSTALLER_USE_7ZIP};
        $ENV{MOZ_PACKAGE_MSI} = "";
        $ENV{MOZ_INSTALLER_USE_7ZIP} = "";
        TinderUtils::run_shell_command "make -C $packaging_dir installer";
        $ENV{MOZ_PACKAGE_MSI} = $save_msi;
        $ENV{MOZ_INSTALLER_USE_7ZIP} = $save_7zip;
        TinderUtils::run_shell_command "cp -r $package_location/xpi $stagedir/windows-xpi";
      }
    } elsif (is_linux()) {
      TinderUtils::run_shell_command "cp -r $package_location/raw/xpi $stagedir/linux-xpi";
      if ($Settings::stub_installer) {
        TinderUtils::run_shell_command "cp $package_location/stub/*.tar.gz $stagedir/";
      }
      if ($Settings::sea_installer) {
        TinderUtils::run_shell_command "cp $package_location/sea/*.tar.gz $stagedir/";
      }
      if ($push_raw_xpis) {
        my $xpi_loc = $package_location;
        if ($Settings::package_creation_path eq "/xpinstall/packager") {
          $xpi_loc = "$xpi_loc/raw";
        }
        TinderUtils::run_shell_command "cp -r $xpi_loc/xpi $stagedir/linux-xpi";
      }
    }
  } # do_installer

  if ($Settings::archive) {
    TinderUtils::run_shell_command "make -C $packaging_dir";

    my(@xforms_xpi);
    if ($Settings::BuildXForms) {
      TinderUtils::run_shell_command "cd $builddir/extensions/schema-validation; $builddir/build/autoconf/make-makefile";
      TinderUtils::run_shell_command "make -C $builddir/extensions/schema-validation";

      TinderUtils::run_shell_command "cd $builddir/extensions/xforms; $builddir/build/autoconf/make-makefile";
      TinderUtils::run_shell_command "make -C $builddir/extensions/xforms";

      TinderUtils::run_shell_command "cd $builddir/extensions/xforms/package; $builddir/build/autoconf/make-makefile";
      TinderUtils::run_shell_command "make -C $builddir/extensions/xforms/package xpi";

      @xforms_xpi = grep { -f $_ } <${builddir}/extensions/xforms/package/stage/xforms/xforms.xpi>;
    }

    if (is_windows()) {
      TinderUtils::run_shell_command "cp $package_location/../*.zip $stagedir/";
      if ( scalar(@xforms_xpi) gt 0 ) {
        my $xforms_xpi_files = join(' ', @xforms_xpi);
        TinderUtils::run_shell_command "mkdir -p $stagedir/windows-xpi/" if ( ! -e "$stagedir/windows-xpi/" );
        TinderUtils::run_shell_command "cp $xforms_xpi_files $stagedir/windows-xpi/";
      }
    } elsif (is_mac()) {
      system("mkdir -p $package_location");
      system("mkdir -p $stagedir");

      # If .../*.dmg.gz exists, copy it to the staging directory.  Otherwise, copy
      # .../*.dmg if it exists.
      my @dmg;
      @dmg = grep { -f $_ } <${package_location}/../*.dmg.gz>;
      if ( scalar(@dmg) eq 0 ) {
	@dmg = grep { -f $_ } <${package_location}/../*.dmg>;
      }

      if ( scalar(@dmg) gt 0 ) {
	my $dmg_files = join(' ', @dmg);
	TinderUtils::print_log "Copying $dmg_files to $stagedir/\n";
	TinderUtils::run_shell_command "cp $dmg_files $stagedir/";
      } else {
	TinderUtils::print_log "No files to copy\n";
      }

      if ( scalar(@xforms_xpi) gt 0 ) {
        my $xforms_xpi_files = join(' ', @xforms_xpi);
        TinderUtils::run_shell_command "mkdir -p $stagedir/mac-xpi/" if ( ! -e "$stagedir/mac-xpi/" );
        TinderUtils::run_shell_command "cp $xforms_xpi_files $stagedir/mac-xpi/";
      }
    } elsif (is_os2()) {
      TinderUtils::run_shell_command "cp $package_location/../*.zip $stagedir/";
      if ( scalar(@xforms_xpi) gt 0 ) {
        my $xforms_xpi_files = join(' ', @xforms_xpi);
        TinderUtils::run_shell_command "mkdir -p $stagedir/os2-xpi/" if ( ! -e "$stagedir/os2-xpi/" );
        TinderUtils::run_shell_command "cp $xforms_xpi_files $stagedir/os2-xpi/";
      }
    } else {
      my $archive_loc = "$package_location/..";
      if ($Settings::package_creation_path eq "/xpinstall/packager") {
        $archive_loc = "$archive_loc/dist";
      }
      TinderUtils::run_shell_command "cp $archive_loc/*.tar.gz $stagedir/";
      if ( scalar(@xforms_xpi) gt 0 ) {
        my $xforms_xpi_files = join(' ', @xforms_xpi);
        TinderUtils::run_shell_command "mkdir -p $stagedir/linux-xpi/" if ( ! -e "$stagedir/linux-xpi/" );
        TinderUtils::run_shell_command "cp $xforms_xpi_files $stagedir/linux-xpi/";
      }
    }
  }

  # need to reverse status, since it's a "unix" truth value, where 0 means 
  # success
  return ($status)?0:1;
}

sub packit_l10n {
  my ($srcdir, $objdir, $packaging_dir, $package_location, $url, $stagedir) = @_;
  my $status;

  TinderUtils::print_log "Starting l10n builds\n";

  unless (open(ALL_LOCALES, "<$srcdir/browser/locales/all-locales")) {
      TinderUtils::print_log "Error: Couldn't read $srcdir/browser/locales/all-locales.\n";
      return (("testfailed"));
  }

  my @locales = <ALL_LOCALES>;
  chomp @locales;
  close ALL_LOCALES;

  TinderUtils::print_log "Building following locales: @locales\n";

  my $start_time = TinderUtils::adjust_start_time(time());
  foreach my $locale (@locales) {
      mail_locale_started_message($start_time, $locale);
      TinderUtils::print_log "$locale...";

      my $logfile = "$Settings::DirName-$locale.log";
      my $tinderstatus = 'success';
      open LOCLOG, ">$logfile";

      # Make the log file flush on every write.
      my $oldfh = select(LOCLOG);
      $| = 1;
      select($oldfh);

    if (do_installer()) {
      if (is_windows()) {
        $ENV{INSTALLER_URL} = "$url/windows-xpi";
      } elsif (is_linux()) {
        $ENV{INSTALLER_URL} = "$url/linux-xpi/";
      } else {
        die "Can't make installer for this platform.\n";
      }
  
      mkdir($package_location, 0775);
  
      # the Windows installer scripts currently require Activestate Perl.
      # Put it ahead of cygwin perl in the path.
      my $save_path;
      if (is_windows()) {
        $save_path = $ENV{PATH};
        $ENV{PATH} = $Settings::as_perl_path.":".$ENV{PATH};
      }
  
      # the one operation we care about saving status of
      if ($Settings::sea_installer || $Settings::stub_installer) {
        $status = run_locale_shell_command "$Settings::Make -C $objdir/browser/locales installers-$locale";
        if ($status != 0) {
          $tinderstatus = 'busted';
        }
      } else {
        $status = 0;
      }
  
      if ($tinderstatus eq 'success') {
        if (is_windows()) {
          run_locale_shell_command "mkdir -p $stagedir/windows-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/windows-xpi/$locale.xpi";
	} elsif (is_mac()) {
	  # Instead of adding something here (which will never be called),
	  # please add your code to the is_mac() section below.
        } elsif (is_linux()) {
          run_locale_shell_command "mkdir -p $stagedir/linux-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/linux-xpi/$locale.xpi";
        }
      }

      if (is_windows()) {
        $ENV{PATH} = $save_path;
        #my $dos_stagedir = `cygpath -w $stagedir`;
        #chomp ($dos_stagedir);
      }
      mkdir($stagedir, 0775);
  
      if (is_windows()) {
        if ($Settings::stub_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/stub/*.exe $stagedir/";
        }
        if ($Settings::sea_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/sea/*.exe $stagedir/";
        }
      } elsif (is_linux()) {
        if ($Settings::stub_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/stub/*.tar.gz $stagedir/";
        }
        if ($Settings::sea_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/sea/*.tar.gz $stagedir/";
        }
      }
    } # do_installer
  
    if ($Settings::archive && $tinderstatus ne 'busted' ) {
      if (is_windows()) {
        run_locale_shell_command "cp $package_location/../*$locale*.zip $stagedir/";
      } elsif (is_mac()) {
        $status = run_locale_shell_command "$Settings::Make -C $objdir/browser/locales installers-$locale";
        if ($status != 0) {
          $tinderstatus = 'busted';
        }
        system("mkdir -p $package_location");
        system("mkdir -p $stagedir");

        # If .../*.dmg.gz exists, copy it to the staging directory.  Otherwise, copy
        # .../*.dmg if it exists.
        my @dmg;
        @dmg = grep { -f $_ } <${package_location}/../*$locale*.dmg.gz>;
        if ( scalar(@dmg) eq 0 ) {
          @dmg = grep { -f $_ } <${package_location}/../*$locale*.dmg>;
        }

        if ( scalar(@dmg) gt 0 ) {
          my $dmg_files = join(' ', @dmg);
          TinderUtils::print_log "Copying $dmg_files to $stagedir/\n";
          TinderUtils::run_shell_command "cp $dmg_files $stagedir/";
        } else {
	  TinderUtils::print_log "No files to copy\n";
        }

        if ($tinderstatus eq 'success') {
          run_locale_shell_command "mkdir -p $stagedir/mac-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/mac-xpi/$locale.xpi";
        }
      } else {
        my $archive_loc = "$package_location/..";
        if ($Settings::package_creation_path eq "/xpinstall/packager") {
          $archive_loc = "$archive_loc/dist";
        }
        run_locale_shell_command "cp $archive_loc/*$locale*.tar.gz $stagedir/";
      }
    }

    $status = run_locale_shell_command "$^X $srcdir/toolkit/locales/compare-locales.pl $srcdir/toolkit/locales/en-US $srcdir/toolkit/locales/$locale";
    if ($tinderstatus eq 'success' && $status != 0) {
      $tinderstatus = 'testfailed';
    }

    $status = run_locale_shell_command "$^X $srcdir/toolkit/locales/compare-locales.pl $srcdir/browser/locales/en-US $srcdir/browser/locales/$locale";
    if ($tinderstatus eq 'success' && $status != 0) {
      $tinderstatus = 'testfailed';
    }
    close LOCLOG;

    mail_locale_finished_message($start_time, $tinderstatus, $logfile, $locale);
    TinderUtils::print_log "$tinderstatus.\n";

  } # foreach

  # remove en-US files since we're building that on a different system
  run_locale_shell_command "rm -f $stagedir/*en-US* $stagedir/*-xpi";

  TinderUtils::print_log "locales completed.\n";

    # need to reverse status, since it's a "unix" truth value, where 0 means 
    # success
  return ($status)?0:1;

} # packit_l10n
        

sub pad_digit {
  my ($digit) = @_;
  if ($digit < 10) { return "0" . $digit };
  return $digit;
}

sub pushit {
  my ($ssh_server,$upload_path,$upload_directory,$cachebuild) = @_;

  unless ( -d $upload_directory) {
    TinderUtils::print_log "No $upload_directory to upload\n";
    return 0;
  }

  if (is_windows()) {
    # need to convert the path in case we're using activestate perl
    $upload_directory = `cygpath -u $upload_directory`;
  }
  chomp($upload_directory);
  my $short_ud = `basename $upload_directory`;
  chomp ($short_ud);

  my $ssh_opts = "";
  my $scp_opts = "";
  if (defined($Settings::ssh_version) && $Settings::ssh_version ne '') {
    $ssh_opts = "-".$Settings::ssh_version;
    $scp_opts = "-oProtocol=".$Settings::ssh_version;
  }

  # The ReleaseToDated and ReleaseToLatest configuration settings give us the
  # ability to fine-tune where release files are stored.  ReleaseToDated
  # will store the release files in a directory of the form
  #
  #   nightly/YYYY-MM-DD-HH-<milestone>
  #
  # while ReleaseToLatest stores the release files in a directory of the form
  #
  #   nightly/latest-<milestone>
  #
  # Before we allowed the fine-tuning, we either published to both dated and
  # latest, or to neither.  In case some installations don't have these
  # variables defined yet, we want to set them to default values here.
  # Hopefully, though, we'll have also updated tinder-defaults.pl if we've
  # updated post-mozilla-rel.pl from CVS.

  $Settings::ReleaseToDated = 1 if !defined($Settings::ReleaseToDated);
  $Settings::ReleaseToLatest = 1 if !defined($Settings::ReleaseToLatest);

  if ( $Settings::ReleaseToDated ) {
    TinderUtils::run_shell_command "ssh $ssh_opts -l $Settings::ssh_user $ssh_server mkdir -p $upload_path";
    TinderUtils::run_shell_command "scp $scp_opts -r $upload_directory $Settings::ssh_user\@$ssh_server:$upload_path";
    TinderUtils::run_shell_command "ssh $ssh_opts -l $Settings::ssh_user $ssh_server chmod -R 775 $upload_path/$short_ud";

    if ( $cachebuild and $Settings::ReleaseToLatest ) {
      TinderUtils::run_shell_command "ssh $ssh_opts -l $Settings::ssh_user $ssh_server cp -dpf $upload_path/$short_ud/* $upload_path/latest-$Settings::milestone/";
    }
  } elsif ( $Settings::ReleaseToLatest ) {
    TinderUtils::run_shell_command "ssh $ssh_opts -l $Settings::ssh_user $ssh_server mkdir -p $upload_path";
    TinderUtils::run_shell_command "scp $scp_opts -r $upload_directory/* $Settings::ssh_user\@$ssh_server:$upload_path/latest-$Settings::milestone/";
    TinderUtils::run_shell_command "ssh $ssh_opts -l $Settings::ssh_user $ssh_server chmod -R 775 $upload_path/latest-$Settings::milestone/";
  }

  return 1;
}


# Cache builds in a dated directory if:
#  * The hour of the day is $Settings::build_hour or higher 
#    (set in tinder-config.pl)
#    -and-
#  * the "last-built" file indicates a day before today's date
#
sub cacheit {
  my ($c_hour, $c_yday, $target_hour, $last_build_day) = @_;
  TinderUtils::print_log "c_hour = $c_hour\n";
  TinderUtils::print_log "c_yday = $c_yday\n";
  TinderUtils::print_log "last_build_day = $last_build_day\n";
  if (($c_hour > $target_hour) && ($last_build_day != $c_yday)) {
    return 1;
  } else {
    return 0;
  }
}

sub reportRelease {
  my ($url, $datestamp)       = @_;

  if ($Settings::notify_list ne "") {
    my $donemessage =   "\n" .
                        "$Settings::OS $Settings::ProductName Build available at: \n" .
                        "$url \n";
    open(TMPMAIL, ">tmpmail.txt");
    print TMPMAIL "$donemessage \n";
    close(TMPMAIL);

    TinderUtils::print_log ("$donemessage \n");
    my $subject = "[build] $datestamp $Settings::ProductName $Settings::OS Build Complete";
    if ($Settings::blat ne "" && $Settings::use_blat) {
      system("$Settings::blat tmpmail.txt -to $Settings::notify_list -s \"$subject\"");
    } else {
      system "$Settings::mail -s \"$subject\" $Settings::notify_list < tmpmail.txt";
    }
    unlink "tmpmail.txt";
  }
  return (("success", "$url"));
}

sub returnStatus{
  # build-seamonkey-util.pl expects an array, if no package is uploaded,
  # a single-element array with 'busted', 'testfailed', or 'success' works.
  my ($logtext, @status) = @_;
  TinderUtils::print_log "$logtext\n";
  return @status;
}

sub PreBuild {
    my $last_build_day;
    my ($c_hour,$c_yday) = (localtime(time))[2,7];

    if ( -e "last-built") {
	($last_build_day) = (localtime((stat "last-built")[9]))[7];
    } else {
	$last_build_day = -1;
    }

    if (cacheit($c_hour,$c_yday,$Settings::build_hour,$last_build_day)) {
	TinderUtils::print_log "starting nightly release build\n";
	# clobber the tree
	TinderUtils::run_shell_command "rm -rf mozilla";
	$cachebuild = 1;
      } else {
	TinderUtils::print_log "starting non-release build\n";
	$cachebuild = 0;
      }
}


sub main {
  # Get build directory from caller.
  my ($mozilla_build_dir) = @_;
  TinderUtils::print_log "Post-Build packaging/uploading commencing.\n";

  chdir $mozilla_build_dir;

  if (is_windows()) {
    #$mozilla_build_dir = `cygpath $mozilla_build_dir`; # cygnusify the path
    #chop $mozilla_build_dir; # remove whitespace
  
    #unless ( -e $mozilla_build_dir) {
      #TinderUtils::print_log "No cygnified $mozilla_build_dir to make packages in.\n";
      #return (("testfailed")) ;
    #}
  }

  my $srcdir = "$mozilla_build_dir/${Settings::Topsrcdir}";
  my $objdir = $srcdir;
  if ($Settings::ObjDir ne '') {
    $objdir .= "/${Settings::ObjDir}";
  }
  unless ( -e $objdir) {
    TinderUtils::print_log "No $objdir to make packages in.\n";
    return (("testfailed")) ;
  }

  system("rm -f $objdir/dist/bin/codesighs");

  # set up variables with default values
  # need to modify the settings from tinder-config.pl
  my $package_creation_path = $objdir . $Settings::package_creation_path;
  my $package_location;
  if (is_windows() || is_mac() || is_os2() || $Settings::package_creation_path ne "/xpinstall/packager") {
    $package_location = $objdir . "/dist/install";
  } else {
    $package_location = $objdir . "/installer";
  }
  my $ftp_path = $Settings::ftp_path;
  my $url_path = $Settings::url_path;

  my ($c_hour,$c_day,$c_month,$c_year,$c_yday) = (localtime(time))[2,3,4,5,7];
  $c_year       = $c_year + 1900; # ftso perl
  $c_month      = $c_month + 1; # ftso perl
  $c_hour       = pad_digit($c_hour);
  $c_day        = pad_digit($c_day);
  $c_month      = pad_digit($c_month);
  my $datestamp = "$c_year-$c_month-$c_day-$c_hour-$Settings::milestone";

  if (is_windows()) {
    # hack for cygwin installs with "unix" filetypes
    TinderUtils::run_shell_command "unix2dos $mozilla_build_dir/mozilla/LICENSE";
    TinderUtils::run_shell_command "unix2dos $mozilla_build_dir/mozilla/mail/LICENSE.txt";
    TinderUtils::run_shell_command "unix2dos $mozilla_build_dir/mozilla/README.txt";
    TinderUtils::run_shell_command "unix2dos $mozilla_build_dir/mozilla/browser/EULA";
  }

  my $upload_directory;

  if ($cachebuild) {
    TinderUtils::print_log "uploading nightly release build\n";
    $upload_directory = "$datestamp";
    $url_path         = $url_path . "/" . $upload_directory;
  } else {
    $ftp_path   = $Settings::tbox_ftp_path;
    $upload_directory = shorthost() . "-" . "$Settings::milestone";
    $url_path   = $Settings::tbox_url_path . "/" . $upload_directory;
  }

  if (!is_os2()) {
    processtalkback($cachebuild && $Settings::shiptalkback, $objdir, "$mozilla_build_dir/${Settings::Topsrcdir}");
  }

  $upload_directory = $package_location . "/" . $upload_directory;

  unless (packit($package_creation_path,$package_location,$url_path,$upload_directory,$objdir)) {
    return returnStatus("Packaging failed", ("testfailed"));
  }

  if ($Settings::BuildLocales) {
    packit_l10n($srcdir,$objdir,$package_creation_path,$package_location,$url_path,$upload_directory);
  }

  unless (pushit($Settings::ssh_server,
		 $ftp_path,$upload_directory, $cachebuild)) {
    return returnStatus("Pushing package $upload_directory failed", ("testfailed"));
  }

  if ($cachebuild) { 
    # remove saved builds older than a week and save current build
    TinderUtils::run_shell_command "find $mozilla_build_dir -type d -name \"200*-*\" -prune -mtime +7 -print | xargs rm -rf";
    TinderUtils::run_shell_command "cp -rpf $upload_directory $mozilla_build_dir/$datestamp";

    unlink "last-built";
    open BLAH, ">last-built"; 
    close BLAH;
    return reportRelease ("$url_path\/", "$datestamp");
  } else {
    return (("success"));
  }
} # end main

# Need to end with a true value, (since we're using "require").
1;
