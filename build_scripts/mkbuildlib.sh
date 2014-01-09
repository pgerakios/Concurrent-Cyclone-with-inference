
bin/cyclex src/buildlib.cyl 
A=`wc -l src/buildlib.cyc | cut -d' ' -f1`
if [ "$A" = "0" ] ;   then
	echo "error";
fi
rm -f src/buildlib.cyc 
#test "1" -eq $A && ls

