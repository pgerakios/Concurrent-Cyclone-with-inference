
files="binary-trees_cyc.cyc binary-trees.c chameneos-redux_cyc.cyc \
       chameneos-redux.c thread-ring_cyc.cyc thread-ring.c  \
       fannkuch_cyc.cyc fannkuch.c regex-dna_cyc.cyc regex-dna.c \
       spectral-norm_cyc.cyc spectral-norm.c mandelbrot_cyc.cyc \
       mandelbrot.c"

       echo "Annotations,Constructs,All,Lines,Words"
       for f in $files
       do
          lines=` wc -l -w $f | awk '''{print $1 "," $2 "," $3}' | grep -v total`
          annot=`grep "spawn\|xrgn\|re_entrant\|throws" $f  | grep -v define | wc -l`
          excyc=`grep "xinc\|xdec\|xlinc\|xldec\|cap\|region .*@\|spawn" $f  | grep -v define | wc -l`
          all=`grep "xinc\|xdec\|xlinc\|xldec\|cap\|region.*@\|spawn\|xrgn\|re_entrant\|throws" $f  | grep -v define | wc -l`
          echo "$annot,$excyc,$all,$lines"
       done
