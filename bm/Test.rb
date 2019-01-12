class Test < Struct.new(:filename, :name, :distances, :times)
  def initialize(*args)
    super(*args)

    File.open(self.filename, 'r') do |f|
      f.each_line do |line|
        self.name = $1 if line.match(/NAME[ ]?:[ ]?(\S+)/)
      end
    end

    self.distances = []
    self.times = []
  end

  def from_series(path)
    Dir["#{path}/#{name}/*.tour"].each do |result|
      File.open(result, 'r') do |f|
        f.each_line do |line|
          self.distances << $1.to_i if line.match(/DISTANCE:[ ]?(\S+)/)
          self.times << $1.to_f if line.match(/TIME:[ ]?(\S+)/)
        end
      end
    end
  end

  def to_h
    {
      'name' => name,
      'distance' => {
        'average' => average_of(distances),
        'std_dev' => standard_deviation_of(distances),
      },
      'Time' => {
        'average' => average_of(times),
        'std_dev' => standard_deviation_of(times),
      }
    }
  end

  private

  def average_of(list)
    list.reduce(:+).to_f / list.size
  end

  def standard_deviation_of(list)
    mean = list.inject(:+) / list.length.to_f
    var_sum = list.map{|n| (n-mean)**2}.inject(:+).to_f
    sample_variance = var_sum / (list.length - 1)
    Math.sqrt(sample_variance)
  end
end