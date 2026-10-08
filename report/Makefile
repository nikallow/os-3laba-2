MAIN   = report
LATEX ?= pdflatex
FLAGS  = -interaction=nonstopmode -halt-on-error
SOURCES = $(MAIN).tex preamble.tex titlepage.tex $(wildcard inc/*) \
          $(wildcard ../src/*.c) ../src/CMakeLists.txt Dockerfile Makefile

$(MAIN).pdf: $(SOURCES)
	$(LATEX) $(FLAGS) $(MAIN).tex
	$(LATEX) $(FLAGS) $(MAIN).tex

xelatex:
	$(MAKE) -B LATEX=xelatex $(MAIN).pdf

clean:
	rm -f *.aux *.log *.out *.lol *.toc *.fls *.fdb_latexmk *.synctex.gz

.PHONY: xelatex clean
