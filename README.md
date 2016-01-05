# suffixC: Simple suffix tree for short reference sequences
[![Build Status](https://travis-ci.org/sherrillmix/suffixc.svg?branch=master)](https://travis-ci.org/sherrillmix/suffixc)

## Introduction
`suffixC` is a simple C program to align a fastq against a shortish reference sequence (around 10kb) using a suffix tree allowing for a small number of mismatches. Partial alignments are preserved in a separate file, useful for finding chimeric reads. It is most useful as a prefilter to pull out reads for alignment using a slower, more accurate program.

## Installation
To install, [download the repository](https://github.com/sherrillmix/suffixr/archive/master.zip) (or git clone) and run make in the resulting directory:

```
wget https://github.com/sherrillmix/suffixr/archive/master.zip
unzip master.zip
cd suffixc-master
make
```

## Usage
```
Usage: ./suffixc ref.fa reads.fastq [-m 2] [-t 4]
  first argument: a reference sequence in a fasta file (if this is much more than 10kb then we could get memory problems)
  second argument: a fastq file containing the reads to search
  -o: (optional) the output prefix for outfiles (default: suffixcOut)
  -l: (optional) specify how long a match has to be considered a partial match. A partial match on both ends counts as a match (default: 20)
  -m: (optional) specify how many mismatches to tolerate (default: 0)
  -h: (optional) display this message and exit
```

