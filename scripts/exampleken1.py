import os
import sys
import re
import kenlm
import multiprocessing as mp

# Read sentences from file into text
text = sys.stdin.read()

#print("Loding model started")

# Load the language model
LM = os.path.join(os.path.dirname('/home/sevilay/Normalisek/training.klm'), '..', 'Normalisek', 'training.klm')
model = kenlm.LanguageModel(LM)

#print("Loding model finished")

# Split the text into sentences
regex = re.compile('\n')
sentences = regex.split(text)

#print("Splitting finished")

for i in range (len(sentences)-1):
	print ('%.4f' % model.score(sentences[i]))
