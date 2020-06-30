version >> RandomAccessTime.txt
echo Generate 50K write commands to random records in a 100000 record file >> RandomAccessTime.txt
RecordAccessTestDataGenerate 50000 100000 write_50k.txt w >> RandomAccessTime.txt
echo Generate 100K read commands to random records in a 100000 record file >> RandomAccessTime.txt
RecordAccessTestDataGenerate 100000 100000 read_100k.txt r >> RandomAccessTime.txt
echo Create an empty random access file of 100000 308-byte records >> RandomAccessTime.txt
timep RecordAccessMM mm_100.ra 100000 >> RandomAccessTime.txt
echo Write 50K records into the file with MM. Some records are duplicates >> RandomAccessTime.txt
timep RecordAccessMM mm_100.ra 0 1 < write_50k.txt >> RandomAccessTime.txt
echo Read 100K records from the file with MM >> RandomAccessTime.txt
timep RecordAccessMM mm_100.ra 0 1 < read_100k.txt >> RandomAccessTime.txt
echo Create an empty random access file of 100000 308-byte records >> RandomAccessTime.txt
timep RecordAccess da_100.ra 100000 >> RandomAccessTime.txt
echo Write 50K records into the file with DA. Some records are duplicates >> RandomAccessTime.txt
timep RecordAccess da_100.ra 0 1 < write_50k.txt >> RandomAccessTime.txt
echo Read 100K records from the file with DA >> RandomAccessTime.txt
timep RecordAccess da_100.ra 0 1 < read_100k.txt >> RandomAccessTime.txt
echo Generate 50K write commands to random records in a 1,000,000 record file >> RandomAccessTime.txt
RecordAccessTestDataGenerate 50000 1000000 write_50k.txt w >> RandomAccessTime.txt
echo Generate 100K read commands to random records in a 100000 record file >> RandomAccessTime.txt
RecordAccessTestDataGenerate 100000 1000000 read_100k.txt r >> RandomAccessTime.txt
echo Create an empty random access file of 1000000 308-byte records >> RandomAccessTime.txt
timep RecordAccessMM mm_1000.ra 1000000 >> RandomAccessTime.txt
echo Write 50K records into the file with MM. Some records are duplicates >> RandomAccessTime.txt
timep RecordAccessMM mm_1000.ra 0 1 < write_50k.txt >> RandomAccessTime.txt
echo Read 100K records from the file with MM >> RandomAccessTime.txt
timep RecordAccessMM mm_1000.ra 0 1 < read_100k.txt >> RandomAccessTime.txt
echo Create an empty random access file of 100000 308-byte records >> RandomAccessTime.txt
timep RecordAccess da_1000.ra 1000000 >> RandomAccessTime.txt
echo Write 50K records into the file with DA. Some records are duplicates >> RandomAccessTime.txt
timep RecordAccess da_1000.ra 0 1 < write_50k.txt >> RandomAccessTime.txt
echo Read 100K records from the file with DA >> RandomAccessTime.txt
timep RecordAccess da_1000.ra 0 1 < read_100k.txt >> RandomAccessTime.txt
del *.ra
del read_100k.txt write_50k.txt
