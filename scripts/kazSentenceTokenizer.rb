require 'pragmatic_segmenter'

File.open(ARGV[0]).each do |line1|
    ps = PragmaticSegmenter::Segmenter.new(text: line1, language: 'kk', doc_type: 'txt')
    sentences = ps.segment
    
    File.open(ARGV[1], "a") do |line2|
        sentences.each { |sentence| line2.puts sentence }
    end
end

