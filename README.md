# suffiC: Simple suffix tree for short reference sequences

## Introduction
`suffiC` is a simple C program to align a fastq against a shortish reference sequence (around 10kb) using a suffix tree allowing for a small number of mismatches. Partial alignments are preserved in a separate file, useful for finding chimeric reads. 

## Installation
To install, [download the repository](https://github.com/sherrillmix/suffixr/archive/master.zip) (or git clone) and run make in the resulting directory:

```
wget https://github.com/sherrillmix/suffixr/archive/master.zip
unzip master.zip
cd suffic-master
make
```

