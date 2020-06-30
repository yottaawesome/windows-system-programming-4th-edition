version >> sorttime.txt
RandFile 100000 r6p4MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortBT >> sorttime.txt
timep sortBT -n r6p4MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortFL >> sorttime.txt
timep sortFL -n r6p4MB.txt >> sorttime.txt
clear  >> sorttime.txt >> sorttime.txt

echo sortBTSR >> sorttime.txt
timep sortBTSR -n r6p4MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortHP >> sorttime.txt
timep sortHP -n r6p4MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortMM >> sorttime.txt
timep sortMM -n r6p4MB.txt >> sorttime.txt
del *.idx >> sorttime.txt
del r6p4MB.txt >> sorttime.txt
echo 25.6 MB file for multithreaded tests >> sorttime.txt
RandFile 400000 r25MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortMT1 >> sorttime.txt
timep sortMT -n 1 r25MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortMT2 >> sorttime.txt
timep sortMT -n 2 r25MB.txt >> sorttime.txt
clear >> sorttime.txt
echo sortMT4 >> sorttime.txt
timep sortMT -n 4 r25MB.txt >> sorttime.txt
echo ================== >> sorttime.txt
del r25MB.txt

