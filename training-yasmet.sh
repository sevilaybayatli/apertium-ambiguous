#!/bin/bash

# Arguments passed to the bash file are:
# 1) 2-character ISO 639-1 code for the source language. For Kazakh => kk
langCode=$1;

# 2) corpus file path.
inputFile=$2;

# 3) apertium pair code. for Kazakh Turkish pair => kaz-tur
pairCode=$3;

# 4) apertium pair parent directory path. If it's in your home directory then we expect $HOME. 
pairPar=$4;

# 5) model weight program path. (e.g. score-sentences.py)
modelWeight=$5;

# 6) ICU locale ID for the source language. For Kazakh => kk-KZ
localeId=$6;

# 7) Analysis output file name.
outputFile=$7;

# 8) Folder name that will contain the yasmet datasets.
datasets=$8;

# 9) Language model
LM=$9

# Break corpus into sentences using the ruby program sentenceTokenizer.rb built on the pragmatic segmenter
ruby2.3 sentenceTokenizer.rb $langCode $inputFile sentences.txt;

# Apply the apertium tool biltrans on the sentences file
apertium -d $pairPar/apertium-$pairCode $pairCode-biltrans sentences.txt biltrans.txt;

# Apply the apertium tool lextor on the biltrans file
lrx-proc -m $pairPar/apertium-$pairCode/$pairCode.autolex.bin biltrans.txt > lextor.txt;

# Run rules-applier program
./rules-applier sentences.txt lextor.txt rulesOut.txt;

# Apply the apertium tool interchunk on the rulesOut file
apertium-interchunk $pairPar/apertium-$pairCode/apertium-$pairCode.$pairCode.t2x $pairPar/apertium-$pairCode/$pairCode.t2x.bin rulesOut.txt interchunk.txt;

# Apply the apertium tool postchunk on the interchunck file
apertium-postchunk $pairPar/apertium-$pairCode/apertium-$pairCode.$pairCode.t3x $pairPar/apertium-$pairCode/$pairCode.t3x.bin interchunk.txt postchunk.txt;

# Apply the apertium tool transfer on the postchunck file
apertium-transfer -n $pairPar/apertium-$pairCode/apertium-$pairCode.$pairCode.t4x $pairPar/apertium-$pairCode/$pairCode.t4x.bin postchunk.txt | lt-proc -g $pairPar/apertium-$pairCode/$pairCode.autogen.bin | lt-proc -p $pairPar/apertium-$pairCode/$pairCode.autopgen.bin > transfer.txt;

# Run model weight program on the transfer file
python3 $modelWeight $LM < transfer.txt > weights.txt;

# Run yasmet formatter to prepare yasmet datasets. Also this will generate the analysis output file , beside the best model weighting translations -in file 'modelWeight.txt'- and random translations -in file 'randomWeight.txt'-
./yasmet-formatter $localeId sentences.txt lextor.txt transfer.txt weights.txt $outputFile $datasets;
