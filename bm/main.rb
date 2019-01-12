require 'yaml'
require './Test'

def mkdir(*args)
  args.each do |arg|
    Dir.exist?(arg) || Dir.mkdir(arg)
  end
end

def main
  if ARGV.size != 3
    puts "Use: #{$0} <cmd> <repetitions> <output_name>"
    exit false
  else
    cmd = ARGV[0]
    repetitions = ARGV[1]
    output_name = ARGV[2]
  end

  output_dir = "output/#{output_name}"

  mkdir *%W(output
            #{output_dir}
            #{output_dir}/series
            #{output_dir}/valgrind
            #{output_dir}/map)

  bm = { "notes" => "Add notes here...", "results" => [] }

  # create benchmarks for each test
  Dir['tours/*.tsp'].each do |filename|
    test = Test.new(filename)

    mkdir "#{output_dir}/series/#{test.name}"

    (1..30).each do |i|
      %x(#{cmd} #{test.filename} -o #{output_dir}/series/#{test.name}/#{i}.tour -r #{repetitions})
    end

    test.from_series("#{output_dir}/series")

    %x(valgrind --log-file="#{output_dir}/valgrind/#{test.name}.log" #{cmd} #{test.filename} -r #{repetitions} -f)
    %x(#{cmd} #{test.filename} -r #{repetitions} -f -p | polygonfy #{output_dir}/map/#{test.name}.svg)

    bm['results'] << test.to_h
  end

  File.open("#{output_dir}/results.yml", 'w') do |f|
    f.write(bm.to_yaml)
  end
end

main
