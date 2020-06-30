echo ThreeStage: Mutex, event, with 5 ms timeout >> three.txt
echo One thread >> three.txt
timep ThreeStage 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStage 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStage 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStage 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStage 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStage 32 1000  >> three.txt
echo 64 threads   >> three.txt
timep ThreeStage 64 1000  >> three.txt
echo ************ >> three.txt
echo ThreeStageCS: Critical Section, with 25 ms timeout >> three.txt
echo One thread >> three.txt
timep ThreeStageCS 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStageCS 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStageCS 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStageCS 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStageCS 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStageCS 32 1000  >> three.txt
echo 64 threads >> three.txt
timep ThreeStageCS 64 1000  >> three.txt
echo ************ >> three.txt
echo ThreeStageSOAW: Mutex, event, SignalObjectAndWait, no timeout >> three.txt
echo One thread >> three.txt
timep ThreeStageSOAW 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStageSOAW 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStageSOAW 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStageSOAW 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStageSOAW 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStageSOAW 32 1000  >> three.txt
echo 64 threads   >> three.txt
timep ThreeStageSOAW 64 1000  >> three.txt
echo ************ >> three.txt
echo ************ >> three.txt
echo ThreeStage_Sig: Mutex, event, Signal model >> three.txt
echo One thread >> three.txt
timep ThreeStage_Sig 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStage_Sig 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStage_Sig 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStage_Sig 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStage_Sig 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStage_Sig 32 1000  >> three.txt
echo 64 threads   >> three.txt
timep ThreeStage_Sig 64 1000  >> three.txt
echo ************ >> three.txt
echo ThreeStageCS_Sig: Critical Section, signal model >> three.txt
echo One thread >> three.txt
timep ThreeStageCS_Sig 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStageCS_Sig 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStageCS_Sig 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStageCS_Sig 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStageCS_Sig 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStageCS_Sig 32 1000  >> three.txt
echo 64 threads >> three.txt
timep ThreeStageCS_Sig 64 1000  >> three.txt
echo ************ >> three.txt
echo ThreeStageSOAW_Sig: Mutex, event, SignalObjectAndWait, Signal Model >> three.txt
echo One thread >> three.txt
timep ThreeStageSOAW_Sig 1  1000  >> three.txt
echo 2 threads   >> three.txt
timep ThreeStageSOAW_Sig 2  1000  >> three.txt
echo 4 threads   >> three.txt
timep ThreeStageSOAW_Sig 4  1000  >> three.txt
echo 8 threads   >> three.txt
timep ThreeStageSOAW_Sig 8  1000  >> three.txt
echo 16 threads   >> three.txt
timep ThreeStageSOAW_Sig 16 1000  >> three.txt
echo 32 threads   >> three.txt
timep ThreeStageSOAW_Sig 32 1000  >> three.txt
echo 64 threads   >> three.txt
timep ThreeStageSOAW_Sig 64 1000  >> three.txt
echo ************ >> three.txt


