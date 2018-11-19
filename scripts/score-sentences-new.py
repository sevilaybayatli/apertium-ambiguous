import sys
import kenlm

# Load the language model
#LM = os.path.join(os.path.dirname('/home/sevilay/apertium-kaz-tur-mt/scripts/training.klm'), '..', 'scripts', 'training.klm')

if len(sys.argv) != 2:
	print('Usage: score-sentences.py <language model>');
	sys.exit(-1)

LM = sys.argv[1]

model = kenlm.LanguageModel(sys.argv[1])

#Score each sentence in stdin
for line in sys.stdin:
	line = line.strip('\n')
	print ('%.4f\t%s' % (model.score(line), line))
