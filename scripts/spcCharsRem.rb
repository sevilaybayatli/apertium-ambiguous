if (ARGV.length < 2)
  puts "Usage : ruby2.3 spcCharsRem.rb oldFilePath newFilePath"
  exit
end

File.open(ARGV[0]).each do |line1|
	line1.delete! ('\\\(\)\[\]\{\}\<\>\|\$\/\'\"')
    
  File.open(ARGV[1], "a") do |line2|
    line2.puts line1
  end
end
