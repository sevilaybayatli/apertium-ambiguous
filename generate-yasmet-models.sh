#!/bin/bash

# datasets folder path
datasets=$1
# models folder path
models=$2

mkdir $models

for dataset in $datasets/*;
do
   ./yasmet < $dataset > $models/${dataset:${#datasets}+1}.model;
done
