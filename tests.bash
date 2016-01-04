#!/bin/bash

set -e


tmpDir=$(mktemp -d)
fastqFile=$tmpDir/test.fastq
cat<< END >$fastqFile
@seq1
GATATATATA
+
((((((((((
@seq2
CTCTCTCTCT
+
((((((((((
@seq3
GGGGGGGGGG
+
((((((((((
@seq4
AAACTGTCTAGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGAAACTCTCTCTGTA
+
((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((
@seq5
AAACTGTCTAGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
+
(((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((
@nonACTG1
AAAAAAAAAAAAAAAAAAAAAAAAAAAAZAAAAAAAAAAAAAAAAAAAAAAAAAA
+
(((((((((((((((((((((((((((((((((((((((((((((((((((((((
END

refFile=$tmpDir/ref.fa
cat<< END >$refFile
>ref
AAAAAAAAAAAAAGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACTCTCTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
GATATATATTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
CTCTCTCTCTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACTCTCTCTCTAAAAA


END
brokenFastqFile=$tmpDir/testBroken.fastq
head -5 $fastqFile>$brokenFastqFile
nonActgRefFile=$tmpDir/refNonActg.fa
cp $refFile $nonActgRefFile
echo "AAAAAAAAAAAAAAAXAAAAAAAAAAAAAAAAAAAAAAAAAA" >> $nonActgRefFile


./suffixc 2>/dev/null && { echo "Missing files did not fail"; exit 1; }
./suffixc $refFile 2>/dev/null && { echo "Missing file did not fail"; exit 1; }
./suffixc -h $refFile $fastqFile 2>/dev/null | grep Usage: >/dev/null|| { echo "-h did not display usage"; exit 1; }
./suffixc -z $refFile $fastqFile 2>/dev/null && { echo "Weird argument did not fail"; exit 1; }
./suffixc -s 10 $refFile $fastqFile 2>/dev/null && { echo "Weird argument did not fail"; exit 1; }
./suffixc -$'\05' $refFile $fastqFile 2>/dev/null && { echo "Weird argument did not fail"; exit 1; }
./suffixc $refFile $fastqFile 2>/dev/null || { echo "Running with 2 args failed"; exit 1; }
./suffixc $refFile $fastqFile -t4 2>/dev/null && { echo "Running with threads worked although implementation removed"; exit 1; }
./suffixc $refFile $fastqFile -m2 2>/dev/null || { echo "Running with 2 mismatch failed"; exit 1; }
./suffixc $refFile $brokenFastqFile -m2 2>/dev/null && { echo "Running with broken fastq did not fail"; exit 1; }
./suffixc $nonActgRefFile $fastqFile -m2 2>/dev/null && { echo "Running with non-ACTG ref file  did not fail"; exit 1; }

cmd="./suffixc $refFile $fastqFile -o $tmpDir/test -m 1 -l10" 
echo $cmd
$cmd 2>/dev/null || { echo "Running with output file set to /tmp failed"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq1 >/dev/null || { echo "seq1 not found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq2 >/dev/null || { echo "seq2 not found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq3 >/dev/null && { echo "seq3 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq3 >/dev/null && { echo "seq3 erroneously found in partial"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq4 >/dev/null || { echo "seq4 two sided not found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq4 >/dev/null && { echo "seq4 erroneously found in partial"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq5 >/dev/null && { echo "seq5 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq5 >/dev/null || { echo "seq5 not found in partial"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep nonACTG1 >/dev/null && { echo "nonACTG found in partial"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep nonACTG1 >/dev/null && { echo "nonACTG found in match"; exit 1; }

cmd="./suffixc $refFile $fastqFile -o $tmpDir/test -m 0"
echo $cmd
$cmd 2>/dev/null || { echo "Running with mismatch 0 failed"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq1 >/dev/null && { echo "seq1 erroneously found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq2 >/dev/null || { echo "seq2 not found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq3 >/dev/null && { echo "seq3 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq3 >/dev/null && { echo "seq3 erroneously found in partial"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq4 >/dev/null && { echo "seq3 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq4 >/dev/null && { echo "seq3 erroneously found in partial"; exit 1; }

cmd="./suffixc $refFile $fastqFile -o $tmpDir/test -m 0 -l 11" 
echo $cmd
$cmd 2>/dev/null || { echo "Running with output file set to /tmp failed"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq1 >/dev/null && { echo "seq1 erroneously found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq2 >/dev/null || { echo "seq2 not found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq3 >/dev/null && { echo "seq3 erroneously found in match"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq4 >/dev/null && { echo "seq4 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq4 >/dev/null && { echo "seq4 erroneously found in partial"; exit 1; }
zcat $tmpDir/test_match.fastq.gz |grep seq5 >/dev/null && { echo "seq5 erroneously found in match"; exit 1; }
zcat $tmpDir/test_partial.fastq.gz |grep seq5 >/dev/null && { echo "seq5 erroneously found in partial"; exit 1; }

longFileName=$tmpDir/$(printf '0123456789%.0s' {1..101})
#probably can't copy because filename too long but that means safe
(cp $refFile $longFileName 2>/dev/null || ./suffixc $longFileName $fastqFile 2>/dev/null) && { echo "Long file name did not fail"; exit 1; }
exit 0
