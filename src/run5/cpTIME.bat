version >> cptime.txt
echo ******* >> cptime.txt
echo Create 320 MB file >> cptime.txt
timep RandFile 5000000 big.txt >> cptime.txt
clear
echo ******* >> cptime.txt
echo cpc  >> cptime.txt
timep cpc big.txt big_copy.txt >> cptime.txt
del big_copy.txt>> cptime.txt
clear>> cptime.txt
echo ******* >> cptime.txt
echo cpw>> cptime.txt
timep cpw big.txt big_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
clear>> cptime.txt
echo ******* >> cptime.txt
echo cpwFA Y-BigBuf Y-SeqScan N-FileSize>> cptime.txt
timep cpwFA big.txt big_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
clear>> cptime.txt
echo ******* >> cptime.txt
echo cpCF>> cptime.txt
timep cpCF big.txt big_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
clear>> cptime.txt
echo ******* >> cptime.txt
echo cpUC>> cptime.txt
timep cpUC big.txt big_copy.txt>> cptime.txt
del big_copy.txt>> cptime.txt
del big.txt>> cptime.txt
echo ******* >> cptime.txt
echo ======================================== >> cptime.txt



