require 'pragmatic_segmenter'

File.open(ARGV[1]).each do |line1|
	line1.delete! ('\\\(\)\[\]\{\}\<\>\|\$\/\'\"')
    ps = PragmaticSegmenter::Segmenter.new(text: line1, language: ARGV[0], doc_type: 'txt')
    sentences = ps.segment
    
    File.open(ARGV[2], "a") do |line2|
        sentences.each { |sentence| line2.puts sentence }
    end
end

