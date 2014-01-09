#!/usr/bin/perl
#
# Prints one line in table comparing C to Cyclone
#
# Inputs: Testname Baseline Comparename
# Output: Testname & Baseline time & Comparename time + var & %ovr & ...
#   (Don't output anything if we don't find the compare value)

if ($#ARGV != 2) {
    die("usage: $0 testname baseline comparetime\n");
}

# parameters: doavg == 1 ->     get the average rather than median
#             dofactor == 1 ->  print diff as factor rather than overhead
#             dovar == 1 ->     print variability measure as well;
#                                 if doavg, this is stddev, else SIQR
$doavg = 0;
$dofactor = 1;
$dovar = 1;

$testname = $ARGV[0];
$baseline = $ARGV[1];
$compare  = $ARGV[2];
$testname =~ s/_/\\_/g;

# print "compare = $compare, baseline = $baseline\n";

$got_vals = 0;

$compare_nobc = sprintf('%s-nobc',$compare);
$n = ($doavg == 1) ? 1 : 3;
$v = ($doavg == 1) ? 2 : 6;

line: while (<STDIN>) {
    chomp;
    @Fld = split " ";
    if (/^#.*/) { # comment; skip
	next line;
    }
    elsif ($Fld[0] eq $baseline) {
	$baselineval = $Fld[$n];
	$baselinevar = $Fld[$v];
	$got_vals |= 1;
    }
    elsif ($Fld[0] eq $compare) {
	$compval = $Fld[$n];
	$compvar = $Fld[$v];
	$got_vals |= 2;
    }
    elsif ($Fld[0] eq $compare_nobc) {
	$compvalnobc = $Fld[$n];
	$compvarnobc = $Fld[$v];
	$got_vals |= 4;
    }
	 #   else {
#	print "non-match: $Fld[0]\n";
#    }
}
$sub = $dofactor ? 0.0 : $baselineval;
$mul = $dofactor ? 1.0 : 100.0;
$per = $dofactor ? '$\times$' : "\\%";

# See if the benchmark was run with the garbage collector;
# if so we will mark it with a dagger footnote
if ($compare eq "Cyclone" &&
    ($testname eq "cfrac" ||
     $testname eq "grobner" ||
     $testname eq "http\\_load" ||
     $testname eq "tile")) {
  $dagger = '$^\dagger$';
}
else {
  $dagger = "";
}

#if ($got_vals == 7) {
    $compvalp = ($compval - $sub) / $baselineval * $mul;
    $compvalnobcp = ($compvalnobc - $sub) / $baselineval * $mul;

    if ($dovar) {
	printf "%s%s & %2.2f \$\\pm\$ %2.2f & %2.2f \$\\pm\$ %2.2f & %2.2f%s & %2.2f \$\\pm\$ %2.2f & %2.2f%s \\\\\n", $testname, $dagger, $baselineval, $baselinevar, $compval, $compvar, $compvalp, $per, $compvalnobc, $compvarnobc, $compvalnobcp, $per;
    } else {
	printf "%s & %2.2f & %2.2f & %2.2f%s & %2.2f & %2.2f%s \\\\\n", $testname, $baselineval, $compval, $compvalp, $per, $compvalnobc, $compvalnobcp, $per;
    }
#}
