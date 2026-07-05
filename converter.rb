#!/usr/bin/env ruby
# converter.rb
# encoding: UTF-8

require 'rmagick'
include Magick
require 'optparse'
require 'fileutils'

options = { quality: 90, recursive: false, size: nil, format: 'jpg', verbose: false }
input = output = nil

OptionParser.new do |opts|
  opts.banner = "Usage: converter.rb <input> [output] [options]"
  opts.on('-q N', Integer, 'Quality') { |v| options[:quality] = v }
  opts.on('-r', 'Recursive') { options[:recursive] = true }
  opts.on('-s WxH', 'Size') { |v| options[:size] = v }
  opts.on('-f EXT', 'Format') { |v| options[:format] = v }
  opts.on('-v', 'Verbose') { options[:verbose] = true }
  opts.on('-h', 'Help') { puts opts; exit }
end.parse!

ARGV.each_with_index do |arg, i|
  input ||= arg
  output ||= arg if i > 0
end

unless input
  puts "Укажите входной файл или папку."
  exit 1
end

if output.nil?
  if File.directory?(input)
    output = input
  else
    output = File.basename(input, '.*') + '.' + options[:format]
  end
end

def get_files(path, recursive)
  exts = %w[.png .jpg .jpeg .bmp .tiff .webp]
  return [path] if File.file?(path) && exts.include?(File.extname(path).downcase)
  pattern = recursive ? '**/*' : '*'
  Dir.glob(File.join(path, pattern)).select { |f| File.file?(f) && exts.include?(File.extname(f).downcase) }
end

files = get_files(input, options[:recursive])
if files.empty?
  puts "Нет изображений."
  exit 1
end

total = files.size
processed = 0
files.each do |f|
  out_file = output
  if File.directory?(output)
    rel = Pathname.new(f).relative_path_from(Pathname.new(input)).to_s
    out_dir = File.join(output, File.dirname(rel))
    FileUtils.mkdir_p(out_dir)
    base = File.basename(f, '.*')
    out_file = File.join(out_dir, base + '.' + options[:format])
  end
  begin
    img = Image.read(f).first
    if options[:size]
      w, h = options[:size].split('x').map(&:to_i)
      img = img.resize(w, h) if w > 0 && h > 0
    end
    # Конвертация в RGB для JPG
    if options[:format].downcase == 'jpg' || options[:format].downcase == 'jpeg'
      img = img.quantize(256, GRAYColorspace) unless img.color_space == SRGBColorspace
      # Если есть альфа, удаляем
      img.alpha(AlphaChannel::Deactivate) if img.alpha?
    end
    img.write(out_file) { self.quality = options[:quality] if options[:format] =~ /jpe?g/ }
    processed += 1
    if options[:verbose]
      puts "✅ #{f} -> #{out_file}"
    else
      pct = processed * 100 / total
      print "\r[#{'█' * (pct/2)}#{'░' * (50 - pct/2)}] #{pct}% #{processed}/#{total}"
    end
  rescue => e
    puts "\n❌ Ошибка #{f}: #{e.message}"
  end
end
puts "\n✅ Обработано #{processed} файлов." unless options[:verbose]
