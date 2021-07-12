

if ENABLE_PYTHON_BINDINGS

abipythondir = .
abipython_PROGRAMS = abipython
abipython_SOURCES =  libabigail.cpp 
abipython_HEADERS = libabigail.hpp
abipython_SCRIPTS = libabigail.yaml

# Specify the target file and the install directory
OUTPUTFILE  = libabigail

# Default target
.PHONY: all
all: shroud

shroud: $(abipython_HEADERS) $(ABIPYTHON_SOURCES)
	shroud $(abipython_SCRIPTS)

# .PHONY: clean 
# clean:
# rm $(OUTPUTFILE).json $(OUTPUTFILE).log
endif
