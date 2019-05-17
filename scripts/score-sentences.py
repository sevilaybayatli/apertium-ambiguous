import sys
import kenlm

if (len(sys.argv) != 4) :
	print('\nUsage: python score-sentences.py arpa_or_binary_LM_file target_lang_file weights_file');
	sys.exit()

targetfile = open(sys.argv[2], 'r')
weightfile = open(sys.argv[3], 'w+')

# Load the language model
model = kenlm.LanguageModel(sys.argv[1])

for sentence in targetfile:
	weightfile.write('%f\n' % (1.0/model.score(sentence)))

targetfile.close()
weightfile.close()
