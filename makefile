all: pdf

pdf:
	latex main.tex
	latex main.tex
	latex main.tex
	dvips -R -Poutline -t letter main.dvi -o hw1.ps
	ps2pdf hw1.ps

clean:
	rm -f progReport *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg

.PHONY: all show clean ps pdf showps