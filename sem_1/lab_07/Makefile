MAIN = report
CC = pdflatex
FLAGS = 
TEMP = *.aux *.log *.out *.toc *.bbl *.blg *.nav *.snm
PARTS = parts

all: report

report: $(MAIN).tex $(PARTS)/*.tex
	$(CC) $(FLAGS) $< # > /dev/null
	-bibtex $(MAIN).aux
	$(CC) $(FLAGS) $< # > /dev/null
	$(CC) $(FLAGS) $< # > /dev/null
	# -mv $(MAIN).pdf $(TITLE)
	make hide

hide:
	mkdir -p out && \
	for ext in $(TEMP); do \
	  files=$$(ls $$ext 2>/dev/null) && mv -f $$files out/ 2>/dev/null || true; \
	done

	mkdir -p out/$(PARTS) && \
	for ext in $(TEMP); do \
	  files=$$(ls $(PARTS)/$$ext 2>/dev/null) && mv -f $$files out/$(PARTS) 2>/dev/null || true; \
	done

clean:
	@rm -f $(TEMP)
	@cd $(PARTS) && rm -f $(TEMP)
	@rm -rf out

.PHONY: all, clean, rpz, hide, restore