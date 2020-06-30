version >> cptime.txt
echo ******* >> cptime.txt
echo Create 320 MB file >> cptime.txt
timep RandFile 5000000 big1.txt >> cptime.txt
timep RandFile 5000000 big2.txt >> cptime.txt
timep RandFile 5000000 big3.txt >> cptime.txt
timep RandFile 5000000 big4.txt >> cptime.txt
timep RandFile 5000000 big5.txt >> cptime.txt

#clear
echo ******* >> cptime.txt
echo cpc  >> cptime.txt
timep cpc big1.txt big1_copy.txt >> cptime.txt
del big_copy.txt>> cptime.txt
#clear>> cptime.txt
echo ******* >> cptime.txt
echo cpw>> cptime.txt
timep cpw big2.txt big2_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
#clear>> cptime.txt
echo ******* >> cptime.txt
echo cpwFA Y-BigBuf Y-SeqScan N-FileSize>> cptime.txt
timep cpwFA big3.txt big3_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
#clear>> cptime.txt
echo ******* >> cptime.txt
echo cpCF>> cptime.txt
timep cpCF big4.txt big4_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
#clear>> cptime.txt
echo ******* >> cptime.txt
echo cpUC>> cptime.txt
timep cpUC big5.txt big5_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
del big.txt>> cptime.txt
echo ******* >> cptime.txt
echo ======================================== >> cptime.txt



