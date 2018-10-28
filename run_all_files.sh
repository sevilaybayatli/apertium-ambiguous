#!/bin/bash


for filename in corpus/*;
do
   ./machine-translation "$filename" "$filename.out";
   wc -l interchunkIn.txt ;
   wc -l interchunkOut.txt ;
   wc -l postchunkOut.txt ;
   wc -l transferOut.txt ;
   wc -l weightOut.txt ;
done
