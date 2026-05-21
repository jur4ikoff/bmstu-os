LATEX := pdflatex
LATEXFLAGS := -interaction=nonstopmode -halt-on-error

MAIN := report
TEX_SOURCES := $(MAIN).tex preambula.tex title.tex main.tex
IMAGES := $(wildcard images/*)

.PHONY: all report code clean clean-code distclean

all: report

report: $(MAIN).pdf

$(MAIN).pdf: $(TEX_SOURCES) $(IMAGES)
	$(LATEX) $(LATEXFLAGS) $(MAIN).tex
	$(LATEX) $(LATEXFLAGS) $(MAIN).tex

code:
	$(MAKE) -C code

clean:
	$(RM) *.aux *.log *.out *.toc *.listing *.synctex.gz *.fls *.fdb_latexmk
	$(RM) -r _minted-$(MAIN)

clean-code:
	$(MAKE) -C code clean

distclean: clean clean-code
	$(RM) $(MAIN).pdf
