all: iceboot-ug.pdf

iceboot-ug.dvi: iceboot-ug.tex iceboot-ug.ind
	latex iceboot-ug.tex

iceboot-ug.ind: iceboot-ug.idx
	makeindex iceboot-ug.idx

iceboot-ug.idx: iceboot-ug.tex
	latex iceboot-ug.tex

iceboot-ug.ps: iceboot-ug.dvi
	dvips -o iceboot-ug.ps iceboot-ug.dvi

iceboot-ug.pdf : iceboot-ug.ps
	pdflatex iceboot-ug.tex

install: iceboot-ug.pdf iceboot-ug.ps iceboot-ug.dvi
	cp -v iceboot-ug.pdf iceboot-ug.ps iceboot-ug.dvi ~/public_html/dom

clean:
	rm -f iceboot-ug.dvi iceboot-ug.ind iceboot-ug.log iceboot-ug.aux \
		iceboot-ug.ilg

