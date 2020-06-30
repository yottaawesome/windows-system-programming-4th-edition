version >> greptime.txt
RandFile 3650  g1.s
RandFile 16000 g2.s
RandFile 11500  g3.s
RandFile 27350  g4.s
RandFile 2600  g5.s
RandFile 24800  g6.s
RandFile 2360  g7.s
RandFile 2330  g8.s
RandFile 3650  g9.s
RandFile 13600 g10.s
RandFile 14100 g11.s
RandFile 26500 g12.s
RandFile 3150  g13.s
RandFile 2735  g14.s
RandFile 16000 g15.s
RandFile 24800  g16.s
RandFile 13600 g17.s
RandFile 16000 g18.s
RandFile 36500  g19.s
RandFile 16000 g20.s
clear >> greptime.txt
echo grepMP >> greptime.txt
timep grepMP 01234567 g1.s g2.s g3.s g4.s g5.s g6.s g7.s g8.s g9.s g10.s g11.s g12.s g13.s g14.s g15.s g16.s g17.s g18.s g19.s g20.s >> greptime.txt
clear >> greptime.txt
echo grepMT >> greptime.txt
timep grepMT 01234567 g1.s g2.s g3.s g4.s g5.s g6.s g7.s g8.s g9.s g10.s g11.s g12.s g13.s g14.s g15.s g16.s g17.s g18.s g19.s g20.s >> greptime.txt
clear >> greptime.txt
echo grepSQ >> greptime.txt
timep grepSQ.BAT >> greptime.txt
del g*.s >> greptime.txt
del *.tmp >> greptime.txt
echo =========================== >> greptime.txt

