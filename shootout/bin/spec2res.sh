#!/bin/sh
(bin/runtest.pl  $1.spec $2 bin/test.sh /dev/null |  bin/munge | (tee $1.tmp && bin/hist < $1.tmp > $1.res)) || rm -f $1.res
