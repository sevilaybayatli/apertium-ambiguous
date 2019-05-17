require 'pragmatic_segmenter'

if (ARGV.length < 3)
  puts "\nUsage : ruby2.3 sentenceTokenizer.rb 639-1ISOlangCode textFilePath sentencesFilePath"
  exit
end

File.open(ARGV[1]).each do |line1|
	line1.delete! ('\\\(\)\[\]\{\}\<\>\|\$\/\'\"')
  ps = PragmaticSegmenter::Segmenter.new(text: line1, language: ARGV[0], doc_type: 'txt')
  sentences = ps.segment
    
  File.open(ARGV[2], "a") do |line2|
      sentences.each { |sentence| line2.puts sentence }  
  end
end

