#!/bin/sh
#
# usage: arg1 run number

RESULTSDIR=.
MAKETABLINE=bin/maketabline.pl

if [ $# != 1 ]; then
  echo usage $0 RUN
  exit 1
fi

RUN=$1

# Do the header
#
echo '\begin{tabular}{|l|c|cc|cc|} \hline'
echo 'Test & C time(s) & \multicolumn{4}{c|}{Cyclone time} \\'
echo '     &           & checked(s) & factor & unchecked(s) & factor \\ \hline'

# First get normal C/Cyclone comparisons
#
for file in $RUN/*.res
do
  $MAKETABLINE `basename ${file%.res}` C Cyclone < $file  
done

# Now get the regionized numbers
#
echo '\hline'
echo '\multicolumn{6}{l}{$^\dagger$Compiled with the garbage collector} \\'
echo '\multicolumn{6}{c}{} \\'
#echo '\multicolumn{6}{c}{\emph{regionized benchmarks}} \\ \hline'
#for file in $RUN/*.res
#do
#  cat $file | $MAKETABLINE `basename ${file%.res}` C Cyclone-region
#done

# Do the footer
#
echo '\hline'
echo '\end{tabular}'
