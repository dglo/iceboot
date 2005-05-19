iceboot-ug.dvi: iceboot-ug.tex iceboot-ug.ind
	latex iceboot-ug.tex

iceboot-ug.ind: iceboot-ug.idx
	makeindex iceboot-ug.idx

iceboot-ug.idx: iceboot-ug.tex
	latex iceboot-ug.tex

clean:
	rm -f iceboot-ug.dvi iceboot-ug.ind iceboot-ug.log iceboot-ug.aux \
		iceboot-ug.ilg
