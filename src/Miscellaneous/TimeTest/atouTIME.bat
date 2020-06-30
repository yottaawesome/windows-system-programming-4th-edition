version > ccitime.txt
RandFile 1000000 big.txt >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cci >> ccitime.txt
timep cci big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cciLBSS Y-LargeBuf Y-SeqScan N-FileSize >> ccitime.txt
timep cciLBSS big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cciMM >> ccitime.txt
timep cciMM big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cciMT >> ccitime.txt
timep cciMT big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cciOV >> ccitime.txt
timep cciOV big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo *************** >> ccitime.txt

echo cciEX >> ccitime.txt
timep cciEX big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
del big.txt >> ccitime.txt
echo *************** >> ccitime.txt
echo ===================================================== >> ccitime.txt
