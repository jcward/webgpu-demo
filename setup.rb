#!/usr/bin/env ruby

require 'io/console'

def confirm msg
  puts msg
  STDIN.getch.downcase.match(/^y/)
end

# Ensure that cmake is installed
has_cmake = `which cmake`.include?("cmake")
if (!has_cmake) then
  if (confirm "CMake not found, install with apt now? y/n") then
    system "sudo apt install cmake"
  end
else
  puts "cmake is available"
end

# Ensure that wgpu native library is installed at ./wgpu-native (FYI, is gitignored)
has_wgpu_native = File.directory?("./wgpu-native")
if (!has_wgpu_native) then
  if (confirm "WGPU Native library not found, install it now at ./wgpu-native? y/n") then
    script = <<-EOS
mkdir -p wgpu-native
cd wgpu-native
wget https://github.com/gfx-rs/wgpu-native/releases/download/v0.18.1.3/wgpu-linux-x86_64-release.zip
unzip wgpu-linux-x86_64-release.zip
EOS
    system script
  end
else
  puts "./wgpu-native exists!"
end

# Offer to run the cmake command (with WGPU_NATIVE_ROOT define)
cmd = "cmake . -DWGPU_NATIVE_ROOT=`pwd`/wgpu-native"
puts cmd
system cmd if confirm("Run above cmake command now? y/n")
