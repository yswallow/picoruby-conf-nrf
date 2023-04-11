require "fileutils"

MRUBY_CONFIG = "nrf-cortex-m4"

task :default => :debug

desc "build debug (you may need to rake clean before this)"
task :debug => [:libmruby, :cmake_debug, :build]

file "lib/picoruby" do
  #sh "git submodule update --init --recursive"
end

task :libmruby_no_msc => "lib/picoruby" do
  FileUtils.cd "lib/picoruby" do
    #sh "rake test"
    sh "CFLAGS='-DPRK_NO_MSC=1' MRUBY_CONFIG=#{MRUBY_CONFIG} rake -v"
  end
end

task :libmruby => "lib/picoruby" do
  FileUtils.cd "lib/picoruby" do
    #sh "rake test"
    sh "MRUBY_CONFIG=#{MRUBY_CONFIG} rake"
  end
end

task :cmake_debug do
  sh "cmake -DCMAKE_BUILD_TYPE=Debug -B build"
end

desc "build without cmake preparation"
task :build do
  sh "cmake --build build"
end

desc "build PRK Firmware inclusive of keymap.rb (without mass storage)"
task :build_with_keymap, ['keyboard_name'] => [:libmruby_no_msc] do |_t, args|
  unless args.keyboard_name
    raise "Argument `keyboard_name` missing.\nUsage: rake build_with_keymap[prk_meishi2]"
  end
  dir = "keyboards/#{args.keyboard_name}"
  FileUtils.mkdir_p "#{dir}/build"
  #sh "cmake -DPRK_NO_MSC=1 -DCMAKE_BUILD_TYPE=Debug -B #{dir}/build"
  sh "cmake -DPRK_NO_MSC=1 -B #{dir}/build"
  sh "cmake --build #{dir}/build -v"
end

desc "clean built that includes keymap"
task :clean_with_keymap , ['keyboard_name'] do |_t, args|
  unless args.keyboard_name
    raise "Argument `keyboard_name` missing.\nUsage: rake clean_with_keymap[prk_meishi2]"
  end
  FileUtils.rm_r Dir.glob("keyboards/#{args.keyboard_name}/build/*")
end


desc "clean built"
task :clean do
  FileUtils.cd "lib/picoruby" do
    sh "MRUBY_CONFIG=#{MRUBY_CONFIG} rake clean"
  end
  FileUtils.cd "build" do
    FileUtils.rm_rf Dir.glob("prk_firmware-*.*")
  end
  begin
    sh "cmake --build build --target clean"
  rescue => e
    puts "Ignoring an error: #{e.message}"
  end
end

desc "deep clean built"
task :deep_clean do
  FileUtils.cd "lib/picoruby" do
    sh "MRUBY_CONFIG=#{MRUBY_CONFIG} rake deep_clean"
  end
  FileUtils.cd "build" do
    FileUtils.rm_rf Dir.glob("*")
  end
end

desc "create symlinks for develop-convenience"
task :symlinks do
  Dir.glob("lib/picoruby/mrbgems/picoruby-prk-*").each do |src|
    FileUtils.ln_sf File.join("..", src), "symlinks/#{File.basename(src)}"
  end
end

desc "run guard-process"
task :guard do
  sh "bundle exec guard start -i"
end

# Add a new tag then push it
task :release => :test do
  git_status = `git status`
  branch = git_status.split("\n")[0].match(/\AOn branch (.+)\z/)[1]
  if branch != "master"
    puts "You should run `rake release` on master branch!"
    puts "'#{branch}' branch is checked out now."
    exit 1
  end
  unless git_status.include?("up to date")
    puts "Your master branch is not up to date!"
    puts "Might have to git pull or push."
    exit 1
  end
  unless git_status.include?("nothing to commit")
    puts "You still have things to commit!"
    exit 1
  end
  puts "Existing tags:"
  puts tags
  puts
  new_version = Gem::Version.new("#{current_version}.1").bump
  line = ''
  loop do
    loop do
      print "Do you want to add a new tag '#{new_version}'? [y/n/another tag]: "
      line = STDIN.gets.chomp
      break unless line.size.zero?
    end
    if %w(n N).include?(line)
      puts "abort"
      exit 1
    end
    break if %w(y Y).include?(line)
    new_version = line
  end
  sh "git tag #{new_version}"
  sh "git push origin #{new_version}"
  Rake::Task['clean'].invoke
  Rake::Task['skip_test'].invoke
end
