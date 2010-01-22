all: xxx.dvi mf logo fsy

xxx.dvi: xxx.tex yyy.tex
	latex xxx
yyy.tex: psn2test parse-psn2.pl Makefile
	./parse-psn2.pl psn2test > yyy.tex

fsy: KpFLK1.eps KpFLK2.eps KpFLK3.eps KpFLK4.eps

KpFLK1.eps: KpFLK1.dvi
	dvips -E KpFLK1

KpFLK1.dvi: KpFLK1.tex
	latex KpFLK1

KpFLK1.tex: KpFLK1.fsy
	./forsyth2tex.pl -l KpFLK1.fsy > KpFLK1.tex

KpFLK2.eps: KpFLK2.dvi
	dvips -E KpFLK2

KpFLK2.dvi: KpFLK2.tex
	latex KpFLK2

KpFLK2.tex: KpFLK2.fsy
	./forsyth2tex.pl -l KpFLK2.fsy > KpFLK2.tex

KpFLK3.eps: KpFLK3.dvi
	dvips -E KpFLK3

KpFLK3.dvi: KpFLK3.tex
	latex KpFLK3

KpFLK3.tex: KpFLK3.fsy
	./forsyth2tex.pl -l KpFLK3.fsy > KpFLK3.tex

KpFLK4.eps: KpFLK4.dvi
	dvips -E KpFLK4

KpFLK4.dvi: KpFLK4.tex
	latex KpFLK4

KpFLK4.tex: KpFLK4.fsy
	./forsyth2tex.pl -l KpFLK4.fsy > KpFLK4.tex

logo: fulogo.dvi
	gftodvi fulogo
fulogo.dvi: fulogo.2602gf
fulogo.2602gf: fulogo.mf
	mf fulogo

mf: shg10.dvi
	gftodvi shg10

shg10.dvi: shg10.mf shogi_koma.mf
	mf shg10

testfont: testfont.dvi
	rm -f shg10.tfm shg10.600pk
	mf "\mode=ljfour; input shg10"
	gftopk shg10.600gf
	tex testfont

test: fullshogitest.tex shogi.sty shg10.600pk 
	latex fullshogitest && xdvi fullshogitest &

shg10.600pk: shg10.600gf
	gftopk shg10.600gf

shg10.600gf: shg10.mf shogi_koma.mf
	mf "\mode=ljfour; input shg10"


