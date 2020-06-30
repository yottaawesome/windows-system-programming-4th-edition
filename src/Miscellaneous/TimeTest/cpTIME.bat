version >> cptime.txt
echo Copy 25.6 MB file >> cptime.txt
timep RandFile 400000 big.txt >> cptime.txt
CLEAR
echo cpc  >> cptime.txt
timep cpc big.txt big.u >> cptime.txt
del big.u>> cptime.txt
CLEAR>> cptime.txt
echo cpw>> cptime.txt
timep cpw big.txt big.u>> cptime.txt
del big.u>> cptime.txt
CLEAR>> cptime.txt
echo cpwFA Y-BigBuf Y-SeqScan N-FileSize>> cptime.txt
timep cpwFA big.txt big.u>> cptime.txt
del big.u>> cptime.txt
CLEAR>> cptime.txt
echo cpCF>> cptime.txt
timep cpCF big.txt big.u>> cptime.txt
del big.u>> cptime.txt
CLEAR>> cptime.txt
echo cpUC>> cptime.txt
timep cpUC big.txt big.u>> cptime.txt
del big.u>> cptime.txt
del big.txt>> cptime.txt
echo ======================================== >> cptime.txt



