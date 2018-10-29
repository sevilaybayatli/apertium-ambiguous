#!/bin/bash


for filename in corpus/*;
do
   ./src/apertium-ambiguous "$filename" "results/$filename.out";
   wc -l results/interchunkIn.txt ;
   wc -l results/interchunkOut.txt ;
   wc -l results/postchunkOut.txt ;
   wc -l results/transferOut.txt ;
   wc -l results/weightOut.txt ;
done
