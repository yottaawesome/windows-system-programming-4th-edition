RandFile 200000 big.txt >> ccitime.txt
CLEAR >> ccitime.txt
echo cci >> ccitime.txt
timep cci big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciSS N-LargeBuf Y-SeqScan N-FileSize >> ccitime.txt
timep cciSS big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciLB Y-LargeBuf N-SeqScan N-FileSize >> ccitime.txt
timep cciLB big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciLBSS Y-LargeBuf Y-SeqScan N-FileSize >> ccitime.txt
timep cciLBSS big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciLSFP Y-LargeBuf Y-SeqScan Y-FileSize >> ccitime.txt
timep cciLSFP big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciMM >> ccitime.txt
timep cciMM big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
CLEAR >> ccitime.txt
echo cciMT >> ccitime.txt
timep cciMT big.txt big.u >> ccitime.txt
del big.u >> ccitime.txt
del big.txt >> ccitime.txt
echo ===================================================== >> ccitime.txt
