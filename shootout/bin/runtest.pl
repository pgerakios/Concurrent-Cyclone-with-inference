#!/usr/bin/perl
#eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
#    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

#eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches


# Variables from the user:
#   arg1=test specification file
#   arg2=testing script to use
#   arg3=default program output file

if ($#ARGV != 3) {
    die("usage: $0 specfile iters test-script default-prog-outfile\n");
}

open(IN, $ARGV[0]) or die "Can't open file $ARGV[0]: $!\n";
$iters = $ARGV[1];
$testsh = $ARGV[2];
$out = $ARGV[3];

$FS = "\t";
$numprog = 0;
$numtests = 0;
$numvars = 0;

# First parse the test specification file, populating the relevant arrays

line: while (<IN>) {
    chomp;	# strip record separator
    @Fld = split $FS;
    if (/^TESTNAME.*$/) {
	$numtests = $#Fld;
	for ($i = 1; $i <= $#Fld; $i++) {
	    $testname[$i - 1] = $Fld[$i];
	}
    }
    elsif (/^VAR.*$/) {
	$numvars = $#Fld;
	for ($i = 1; $i <= $#Fld; $i++) {
	    $vars[$i-1] = $Fld[$i];
	}
    }
    elsif (/^PROG.*$/) {
	$n = $#Fld;
	#	printf "fld %s", $Fld;
	if ($n != $numvars) {
	    printf "inconsistent # of variants: %d != %d\n", 
		$n, $numvars;
	    exit 1;
	}
	for ($i = 1; $i <= $#Fld; $i++) {
	    @a = split(' ', $Fld[$i]);
	    if ($numprogs == 0) {
		$numprogs = $#a + 1;
	    }
	    elsif ($numprogs != ($#a + 1)) {
		printf "inconsistent # of programs: %d != %d\n", 
		       $numprogs, $#a + 1;
		for ($j = 0; $j < $numprogs; $j++) {
		    print "$a[$j]\n";
		}
		exit 1;
	    }
	    for ($j = 0; $j < $numprogs; $j++) {
		$prog[$j][$i-1] = $a[$j];
	    }
	}
    }
    elsif (/^ARGS.*$/) {
	if ($#Fld != $numtests) {
	    printf "inconsistent # of tests: %d != %d\n", $#Fld, $numtests;
	    exit 1;
	}
	for ($i = 1; $i <= $#Fld; $i++) {
	    @a = split(' ', $Fld[$i]);
	    if ($numprogs != ($#a + 1)) {
		printf "inconsistent # of progs vs. args: %d != %d\n", 
		    $numprogs, $n;
		exit 1;
	    }
	    for ($j = 0; $j < $numprogs; $j++) {
		if ($a[$j] ne '{}') {
		    $s = '#', $a[$j] =~ s/$s/ /g;
		    $arg[$j][$i-1] = $a[$j];
		}
	    }
	}
    }
    elsif (/^FILES.*$/) {
	if ($#Fld != $numtests) {
	    printf "inconsistent # of tests for files: %d != %d\n", 
	           $#Fld, $numtests;
	    exit 1;
	}
	for ($i = 1; $i <= $#Fld; $i++) {
	    @a = split(' ', $Fld[$i]);
	    if ($numprogs != ($#a + 1)) {
		printf "inconsistent # of progs vs. files: %d != %d\n", 
		    $numprogs, $#a + 1;
		exit 1;
	    }
	    for ($j = 0; $j < $numprogs; $j++) {
		if ($a[$j] ne '{}') {
		    @b = split(/#/, $a[$j]);
		    if ($b[0] =~ /\S/) {
			$files[$j][$i-1][0] = $b[0];
		    }
		    $files[$j][$i-1][1] = $b[1];
	        }
	    }
	}
    }
    else {
	printf "bogus specificier %s\n", $Fld[1];
	exit 1;
    }
}
close(IN);

# Now actually run the tests

$tmp1 = "/tmp/runtest1";
$tmp2 = "/tmp/runtest2";

for ($i = 0; $i < $numtests; $i++) {
    printf "TEST %s\n", $testname[$i];

    # set up the input/output files
    for ($k = 0; $k < $numprogs; $k++) {
	if ($files[$k][$i][0] eq '') {
	    $files[$k][$i][0] = '/dev/null';
	}
	if ($files[$k][$i][1] eq '') {
	    $files[$k][$i][1] = $out;
	    $flag = "";
	}
	else {
	    $flag = "-k";
	}
    }

    # run the test
    for ($j = 0; $j < $numvars; $j++) {
	printf ("VAR %s\n", $vars[$j]);

	for ($k = 0; $k < $numprogs; $k++) {
	    printf "PROG %s in=%s out=%s err=%s arg=%s\n", 
	    $prog[$k][$j], $files[$k][$i][0],
	    $files[$k][$i][1], $out, $arg[$k][$i];

	    $syscmd = sprintf("%s %s %s %s %s %s %s", 
	    $testsh, $flag, $files[$k][$i][0], $files[$k][$i][1], 
	    $out, $prog[$k][$j], $arg[$k][$i]);

	 #print STDERR "$syscmd\n";
	    for ($l = 0; $l < $iters; $l++) {
		system ($syscmd);
	    }
	}
    }
}

exit 0
