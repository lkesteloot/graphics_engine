.PHONY: insttest

CC =		gcc
CFLAGS =	-g -Wall
SIMOBJS =	sim.o unasm.o display.o functions.o util.o

all:		asm sim disasm mem2ppm mc

sim:		$(SIMOBJS)
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) $(SIMOBJS) -o sim -lcurses

disasm:		disasm.o unasm.o util.o
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) disasm.o unasm.o util.o -o disasm

mem2ppm:	mem2ppm.c
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) mem2ppm.c -o mem2ppm

disasm.o:	disasm.c unasm.h util.h

util.o:		util.c util.h types.h

unasm.o:	unasm.c inst.h unasm.h

sim.o:		sim.h sim.c optable.h display.h functions.h util.h

display.o:	sim.h display.c display.h

functions.o:	sim.h functions.h

inst.h doc/inst.tex optable.h asm.l asm.y: geninst inst.dat asm.template.y asm.template.l
		./geninst inst.dat

asm:		asm.tab.c util.o types.h
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) asm.tab.c util.o -o asm

asm.tab.c:	asm.y asm.h lex.yy.c
		bison asm.y

lex.yy.c:	asm.l asm.h
		flex asm.l

pic:		emem.ppm
		display emem.ppm

emem.ppm:	emem.dump mem2ppm
		./mem2ppm >emem.ppm

emem.dump:	imem sim emem
		./sim -r -d -e emem imem dmem

plot:		freq.plot
		gnuplot freq.plot

freq.plot:	imem sim
		./sim -rf imem

imem:		imem.s asm
		./asm imem.s

insttest:	insttest.s asm
		# Writes to imem.
		./asm insttest.s

rast:		rast.c
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) rast.c -o rast

dltest:		dltest.o dl.o dl.h
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) dltest.o dl.o -o dltest -lm

sphere:		sphere.o dl.o dl.h
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) sphere.o dl.o -o sphere -lm

ztest:		ztest.o dl.o dl.h
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) ztest.o dl.o -o ztest -lm

teapot:		teapot.o dl.o dl.h
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) teapot.o dl.o -o teapot -lm

sqrt:		sqrt.o
		$(RM) $(RMFLAGS) $@
		$(CC) $(CFLAGS) sqrt.o -o sqrt -lm

dl.o:		dl.h

dltest:		dl.h

sphere:		dl.h

mc:		imem emem

tex:		doc/inst.tex doc/ge.tex
		cd doc; ./GO

ps:		tex
		cd doc; dvips -f ge.dvi >ge.ps
		ghostview doc/ge.ps

write:
		cd doc; touch write.ind; makeindex write; latex write.tex
		xdvi doc/write.dvi

clean:
		$(RM) $(RMFLAGS) optable.h inst.h sim asm emem.dump emem.ppm \
		    *.o asm.l asm.y \
		    write.log write.aux write.dvi write.ind write.toc \
		    write.idx write.lof write.lot write.ilg int.tex \
		    rast asm.tab.c lex.yy.c imem disasm mem2ppm
