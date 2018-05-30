rm -rf eosdacrandom.abi
rm -rf eosdacrandom.wast
eosiocpp -g eosdacrandom.abi eosdacrandom.cpp
eosiocpp -o eosdacrandom.wast eosdacrandom.cpp
