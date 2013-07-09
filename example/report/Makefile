report.pdf:

%.pdf: %.tex
	pdflatex $*.tex && \
	pdflatex $*.tex && \
	pdflatex $*.tex

.PHONY: clean
clean:
	@echo rm *.aux *.log *.toc *.pdf
	@rm *.aux *.log *.toc *.pdf; true
