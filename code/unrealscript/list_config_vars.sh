grep "^var config" */Classes/*.uc | afterlast "Classes/" | sedreplace "var config " "" | sort -n -k 1 | highlight "[Bb]"ool | highlight "[Ii]"nt | highlight "[Ss]"tring | highlight "[Ff]"loat | higrep "//.*"