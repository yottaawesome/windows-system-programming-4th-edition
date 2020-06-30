version >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStage: Mutex, event, SOAW, broadcast, no timeout >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStage 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStage 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStage 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStage 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStage 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStage 32 10000  >> ThreeStageTime.txt
echo 64 threads   >> ThreeStageTime.txt
timep ThreeStage 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStageCS: Critical Section, event, broadcast, 25 ms timeout >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStageCS 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStageCS 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStageCS 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStageCS 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStageCS 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStageCS 32 10000  >> ThreeStageTime.txt
echo 64 threads >> ThreeStageTime.txt
timep ThreeStageCS 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStage_noSOAW: Mutex, event, broadcast, 25 ms timeout >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStage_noSOAW 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 32 10000  >> ThreeStageTime.txt
echo 64 threads   >> ThreeStageTime.txt
timep ThreeStage_noSOAW 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStage_Sig: Mutex, event, SOAW, no timeout, Signal model >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStage_Sig 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 32 10000  >> ThreeStageTime.txt
echo 64 threads   >> ThreeStageTime.txt
timep ThreeStage_Sig 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStageCS_Sig: Critical Section, event, 25 ms timeout, signal model >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStageCS_Sig 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStageCS_Sig 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStageCS_Sig 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStageCS_Sig 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStageCS_Sig 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStageCS_Sig 32 10000  >> ThreeStageTime.txt
echo 64 threads >> ThreeStageTime.txt
timep ThreeStageCS_Sig 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStageCV: NT 6 Condition variable, signal, no TO  >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStageCV 1 10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStageCV 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStageCV 4 10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStageCV 8 10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStageCV 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStageCV 32 10000  >> ThreeStageTime.txt
echo 64 threads   >> ThreeStageTime.txt
timep ThreeStageCV 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

echo ThreeStageSig_noSOAW: Mutex, event, timeout, Signal model >> ThreeStageTime.txt
echo One thread >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 1  10000  >> ThreeStageTime.txt
echo 2 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 2  10000  >> ThreeStageTime.txt
echo 4 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 4  10000  >> ThreeStageTime.txt
echo 8 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 8  10000  >> ThreeStageTime.txt
echo 16 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 16 10000  >> ThreeStageTime.txt
echo 32 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 32 10000  >> ThreeStageTime.txt
echo 64 threads   >> ThreeStageTime.txt
timep ThreeStageSig_noSOAW 64 10000  >> ThreeStageTime.txt
echo ************ >> ThreeStageTime.txt

