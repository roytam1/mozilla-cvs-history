#!/usr/bin/perl -w

$OLDFILE = $ARGV[0];
$NEWFILE = $ARGV[1];

sub processFile {
    my ($filename, $map, $prevMap) = @_;
    open(FH, $filename);
    while (<FH>) {
        if (m{
              ^\s*(\d+)\s          # Line number
              ([\w:]+)\s+          # Name
              (-?\d+)\s+           # Size
              (-?\d+)\s+           # Leaked
              (-?\d+)\s+           # Objects Total
              (-?\d+)\s+           # Objects Rem
              \(\s*(-?[\d.]+)\s+   # Objects Mean
                 \+/-\s+
              ([\w.]+)\)\s+        # Objects StdDev
              (-?\d+)\s+           # Reference Total
              (-?\d+)\s+           # Reference Rem
              \(\s*(-?[\d.]+)\s+   # Reference Mean
                 \+/-\s+
              ([\w\.]+)\)          # Reference StdDev
             }x) {
          $$map{$2} = { name => $2,
                        size => $3,
                        leaked => $4,
                        objTotal => $5,
                        objRem => $6,
                        objMean => $7,
                        objStdDev => $8,
                        refTotal => $9,
                        refRem => $10,
                        refMean => $11,
                        refStdDev => $12,
                        bloat => $3 * 5 # size * objTotal
                      };
        } else {
#            print "failed to parse: $_\n";
        }
    }
    close(FH);
}

%oldMap = ();
processFile($OLDFILE, \%oldMap);

%newMap = ();
processFile($NEWFILE, \%newMap);

################################################################################

$inf = 9999999.99;

sub getLeaksDelta {
    my ($key) = @_;
    my $oldLeaks = $oldMap{$key}{leaked} || 0;
    my $newLeaks = $newMap{$key}{leaked};
    my $percentLeaks = 0;
    if ($oldLeaks == 0) {
        if ($newLeaks != 0) {
            # there weren't any leaks before, but now there are!
            $percentLeaks = $inf;
        }
    }
    else {
        $percentLeaks = ($newLeaks - $oldLeaks) / $oldLeaks * 100;
    }
    # else we had no record of this class before
    return ($newLeaks - $oldLeaks, $percentLeaks);
}
    
################################################################################

sub getBloatDelta {
    my ($key) = @_;
    my $newBloat = $newMap{$key}{bloat};
    my $percentBloat = 0;
    my $oldSize = $oldMap{$key}{size} || 0;
    my $oldTotal = $oldMap{$key}{objTotal} || 0;
    my $oldBloat = $oldTotal * $oldSize;
    if ($oldBloat == 0) {
        if ($newBloat != 0) {
            # this class wasn't used before, but now it is
            $percentBloat = $inf;
        }
    }
    else {
        $percentBloat = ($newBloat - $oldBloat) / $oldBloat * 100;
    }
    # else we had no record of this class before
    return ($newBloat - $oldBloat, $percentBloat);
}

################################################################################

foreach $key (keys %newMap) {
    my ($newLeaks, $percentLeaks) = getLeaksDelta($key);
    my ($newBloat, $percentBloat) = getBloatDelta($key);
    $newMap{$key}{leakDelta} = $newLeaks;
    $newMap{$key}{leakPercent} = $percentLeaks;
    $newMap{$key}{bloatDelta} = $newBloat;
    $newMap{$key}{bloatPercent} = $percentBloat;
}

################################################################################

print "Bloat/Leak Delta Report\n";
print "-------------------------------------------\n";
print "Current file:  $NEWFILE\n";
print "Previous file: $OLDFILE\n";
printf "%-20s %10s %10.2f%% %10s %10.2f%%\n",
       ("TOTAL",
        $newMap{"TOTAL"}{leaked}, $newMap{"TOTAL"}{leakPercent},
        $newMap{"TOTAL"}{bloat}, $newMap{"TOTAL"}{bloatPercent});

################################################################################

sub percentStr {
    my ($p) = @_;
    if ($p == $inf) {
        return "-";
    }
    else {
        return sprintf "%10.2f%%", $p;
    }
}

# NEW LEAKS
@keys = sort { $newMap{$b}{leakPercent} <=> $newMap{$a}{leakPercent} } keys %newMap;
my $needsHeading = 1;
my $total = 0;
foreach $key (@keys) {
    my $percentLeaks = $newMap{$key}{leakPercent};
    my $leaks = $newMap{$key}{leaked};
    if ($percentLeaks > 0 && $key !~ /TOTAL/) {
        if ($needsHeading) {
            printf "--NEW-LEAKS--------------------------------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $leaks, percentStr($percentLeaks));
        $total += $leaks;
    }
}
if (!$needsHeading) {
    printf "%-20s %10s\n", ("TOTAL", $total);
}

# FIXED LEAKS
@keys = sort { $newMap{$b}{leakPercent} <=> $newMap{$a}{leakPercent} } keys %newMap;
$needsHeading = 1;
$total = 0;
foreach $key (@keys) {
    my $percentLeaks = $newMap{$key}{leakPercent};
    my $leaks = $newMap{$key}{leaked};
    if ($percentLeaks < 0 && $key !~ /TOTAL/) {
        if ($needsHeading) {
            printf "--FIXED-LEAKS------------------------------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $leaks, percentStr($percentLeaks));
        $total += $leaks;
    }
}
if (!$needsHeading) {
    printf "%-20s %10s\n", ("TOTAL", $total);
}

# NEW BLOAT
@keys = sort { $newMap{$b}{bloatPercent} <=> $newMap{$a}{bloatPercent} } keys %newMap;
$needsHeading = 1;
$total = 0;
foreach $key (@keys) {
    my $percentBloat = $newMap{$key}{bloatPercent};
    my $bloat = $newMap{$key}{bloat};
    if ($percentBloat > 0  && $key !~ /TOTAL/) {
        if ($needsHeading) {
            printf "--NEW-BLOAT--------------------------------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $bloat, percentStr($percentBloat));
        $total += $bloat;
    }
}
if (!$needsHeading) {
    printf "%-20s %10s\n", ("TOTAL", $total);
}

# ALL LEAKS
@keys = sort { $newMap{$b}{leaked} <=> $newMap{$a}{leaked} } keys %newMap;
$needsHeading = 1;
$total = 0;
foreach $key (@keys) {
    my $leaks = $newMap{$key}{leaked};
    my $percentLeaks = $newMap{$key}{leakPercent};
    if ($leaks > 0) {
        if ($needsHeading) {
            printf "--ALL-LEAKS--------------------------------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $leaks, percentStr($percentLeaks));
        if ($key !~ /TOTAL/) {
            $total += $leaks;
        }
    }
}
if (!$needsHeading) {
#    printf "%-20s %10s\n", ("TOTAL", $total);
}

# ALL BLOAT
@keys = sort { $newMap{$b}{bloat} <=> $newMap{$a}{bloat} } keys %newMap;
$needsHeading = 1;
$total = 0;
foreach $key (@keys) {
    my $bloat = $newMap{$key}{bloat};
    my $percentBloat = $newMap{$key}{bloatPercent};
    if ($bloat > 0) {
        if ($needsHeading) {
            printf "--ALL-BLOAT--------------------------------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $bloat, percentStr($percentBloat));
        if ($key !~ /TOTAL/) {
            $total += $bloat;
        }
    }
}
if (!$needsHeading) {
#    printf "%-20s %10s\n", ("TOTAL", $total);
}

# NEW CLASSES
@keys = sort { $newMap{$b}{bloatDelta} <=> $newMap{$a}{bloatDelta} } keys %newMap;
$needsHeading = 1;
my $ltotal = 0;
my $btotal = 0;
foreach $key (@keys) {
    my $leaks = $newMap{$key}{leaked};
    my $bloat = $newMap{$key}{bloat};
    my $percentBloat = $newMap{$key}{bloatPercent};
    if ($percentBloat == $inf && $key !~ /TOTAL/) {
        if ($needsHeading) {
            printf "--CLASSES-NOT-REPORTED-LAST-TIME-----------\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $leaks, $bloat);
        if ($key !~ /TOTAL/) {
            $ltotal += $leaks;
            $btotal += $bloat;
        }
    }
}
if (!$needsHeading) {
    printf "%-20s %10s %10s\n", ("TOTAL", $ltotal, $btotal);
}

# OLD CLASSES
@keys = sort { ($oldMap{$b}{bloat} || 0) <=> ($oldMap{$a}{bloat} || 0) } keys %oldMap;
$needsHeading = 1;
$ltotal = 0;
$btotal = 0;
foreach $key (@keys) {
    if (!defined($newMap{$key})) {
        my $leaks = $oldMap{$key}{leaked};
        my $bloat = $oldMap{$key}{bloat};
        if ($needsHeading) {
            printf "--CLASSES-THAT-WENT-AWAY--leaks------bloat-\n";
            $needsHeading = 0;
        }
        printf "%-20s %10s %10s\n", ($key, $leaks, $bloat);
        if ($key !~ /TOTAL/) {
            $ltotal += $leaks;
            $btotal += $bloat;
        }
    }
}
if (!$needsHeading) {
    printf "%-20s %10s %10s\n", ("TOTAL", $ltotal, $btotal);
}

print "-------------------------------------------\n";
