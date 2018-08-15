	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Data memory
	;

.data

jumptable:
	.word	instVERTEX
	.word	instCALL
	.word	instRETURN
	.word	instPUSH
	.word	instPOP
	.word	instLOAD
	.word	instMULT
	.word	instLIGHT
	.word	instSET
	.word	instRESET
	.word	instCOLOR
	.word	instNORMAL

dmabuf:					; DMA buffer for instructions
	.space	16			; 16 words

framebuf:
	.word	0x01000358		; Address of framebuffer in
					; external memory (high=color, low=z)

modeword:				; 32-bit mode word
	.word	0

surfcolor:				; Current surface color
	.space	3			; (r, g, b)

normal:					; Current surface normal
	.space	4			; (nx, ny, nz)

lightdir:				; Current light vector in s15.16
	.space	4			; towards light (lx, ly, lz, 0)
lightcolor:				; Current light color
	.space	3			; (lr, lg, lb)

callstack:				; Stack for display lists (8 deep)
	.space	8
stackptr:
	.word	callstack		; Pointer to top of stack

savereturn:				; Area to save your return address
	.word	0

saveinst:	.word	0
savedma:	.word	0

projection:				; Projection matrix
	.space	16			; Row-major order

modelview:				; Model-view matrix
	.space	16			; Row-major order

tmat1:		.space	16		; Temp matrices
tmat2:		.space	16		; for matrix multiply

scratch:	.space	4		; temp x,y,z,w for 
					; transforming vertices
; Space used by the rasterizer:
minx:	.word	0			; Minimum X of bounding box

x:	.space 	3
y:	.space 	3
z:	.space 	3

r:	.space 	3
g:	.space 	3
b:	.space	3

planes:	.space 	15

.code
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Okay, this is the real source.  We want to load opcodes from
	; emem and execute them.  Assume that the instructions start
	; at address zero.
	;

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; First some constants

	; Mode word:

.const	M_LIGHTING,1
.const	M_NORMALIZE,2
.const	M_SMOOTH,4
.const	M_DEPTH_READ,8
.const	M_DEPTH_WRITE,16
.const	M_CULL,32


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Define a few global registers
	;

.sym	INSTADDR,r27			; Address of instruction in emem
.sym	DMAADDR,r28			; What 16 words we have in memory
.sym	INST,r29			; The instruction itself
.sym	TMP,r30				; Always the temporary register
.sym	RETURN,r31			; Where CALL puts the return address

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Initialize some registers
	;

	LI	DMAADDR,0xFFFF		; Set DMAADDR to invalid

	LI	INSTADDR,0		; Instruction address to start of emem

.sym	WIDTH,r1
.sym	HEIGHT,r2
	LI	WIDTH,640		; Number of pixels horizontally
	LI	HEIGHT,480		; Number of pixels vertically
	SCRSET	WIDTH,HEIGHT,framebuf	; Set width and location of screen
.unsym	WIDTH
.unsym	HEIGHT

	LI	TMP,0x7800 | 0x03E0 | 0x000F
	CLS	TMP, DMAADDR		; Clear frame and z buffers

	LI	TMP, 1			; Set modelview and projection
	LSLI	TMP, TMP, 16		; matrices to identity
	ST	modelview+0, TMP
	ST	modelview+5, TMP
	ST	modelview+10, TMP
	ST	modelview+15, TMP
	ST	projection+0, TMP
	ST	projection+5, TMP
	ST	projection+10, TMP
	ST	projection+15, TMP
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Main loop
	;

instloop:				; Instruction loop
	CALL	getinst			; Get the next instruction (into INST)
	LSRI	TMP,INST,24		; Get the opcode from upper byte
	; XXX Maybe we should check that TMP is in the right range here
	LSLI	INST,INST,8		; Lose opcode
	LSRI	INST,INST,8		; Lose opcode
	LD	TMP,TMP(jumptable) 	; Address of routine to handle opcode
	CALL	TMP			; Jump to it

	JMP	instloop		; Get next instruction

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Get instruction (or any word) from external memory
	;

getinst:
	LI	TMP,0x0F		; Mask for DMA buffer (16 entries)
	NOT	TMP,TMP			; Mask for start of DMA buffer address
	AND	TMP,TMP,INSTADDR	; Find what we need
	SUB	R0,TMP,DMAADDR		; See if we have it
	JEQ	inmemory		; Yes, get the instruction
	OR	DMAADDR,TMP,TMP		; What we need into DMAADDR
	LI	TMP,dmabuf		; Where to put the buffer
	READ	TMP,DMAADDR		; Get it from external memory
inmemory:
	LI	TMP,0x0F		; Mask for DMA buffer (16 entries)
	AND	TMP,TMP,INSTADDR	; Offset into buffer
	LD	INST,TMP(dmabuf)	; Get instruction
	LI	INSTADDR,INSTADDR(1)	; Increment instruction counter
	JMP	RETURN			; Return

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Vertex: 00 VV XX XX  YY YY ZZ ZZ
	;

instVERTEX:
    ; Basic info
.sym	VNUM,R1
.sym	TMP2,R2
.sym	V1,R3
.sym	V2,R4
.sym	V3,R5
.sym	W, R6
    ; Bounding box
.sym	Y,R25
.sym	MAXX,R26
.sym	MAXY,R27
.sym	X,R28

	ST	savereturn,RETURN	; Save the return address
	LSRI	VNUM,INST,16		; Get the vertex number

	; if VNUM < 2
	;    extract vertex data
	;    return
	; if VNUM == 2
	;    extract vertex data
	;    draw triangle
	;    return
	; if VNUM > 2
	;    shift previous data 
	;    (to save first vertex and last vertex)
	;    then proceed as above
	;    return
	LI	TMP, 2
	SUB	R0,VNUM, TMP		; if VNUM <= 2
	JLEL	extract			;     goto extract
	OR	VNUM, TMP, TMP		; otherwise shift data
	LI	TMP, 17			
	LI	V1, 0
shift:	LD	V2, V1(x+2)		; data[x] = data[x+1]
	ST	V1(x+1), V2
	LI	V1, V1(3)		; Increment X
	SUB	R0,V1,TMP		; if X < 17 
	JLTL	shift			;     shift next element

extract:LSLI	V1,INST,16		; Save vertex V1

	CALL	getinst			; Contains V2 and V3

	OR	V2,INST,INST		; Save V2
	LSRI	V2,V2,16		; Clear out lower half-word
	LSLI	V2,V2,16		; 

	OR	V3,INST,INST		; Save V3
	LSLI	V3,V3,16		; Put into upper half-word

	; Transform the vertex

trans:	ST	scratch, V1		; Store x, y, z, 1
	ST	scratch+1, V2		; for matrix transform
	ST	scratch+2, V3
	LI	TMP, 1
	LSLI	TMP, TMP, 16
	ST	scratch+3, TMP

	LI	TMP, modelview		; Transform vertex by 
	LI	TMP2, scratch		; model view
	DOT	V1, TMP, TMP2
	LI	TMP, TMP(4)
	DOT	V2, TMP, TMP2
	LI	TMP, TMP(4)
	DOT	V3, TMP, TMP2
	LI	TMP, TMP(4)
	DOT	W, TMP, TMP2

	RECP	W, W		; Perform W divide
	MULF	V1, V1, W
	MULF	V2, V2, W
	MULF	V3, V3, W

.resym	TMP3,W

	LSRI	V1, V1, 16	; Shift results
	LSRI	V2, V2, 16
	LSRI	V3, V3, 16
	
	; We now have V1, V2, and V3 all set.

	ST	VNUM(x),V1		; Store X
	LI	TMP,479			; Flip Y in screen
	SUB	V2,TMP,V2
	ST	VNUM(y),V2		; Store Y
	ST	VNUM(z),V3		; Store Z

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Light each vertex if lighting is on

	LI	TMP,M_LIGHTING		; Lighting bit in mode word
	LD	TMP2,modeword		; Load the mode word
	AND	R0,TMP,TMP2		; See if the bit is on
	JEQ	nolighting		; Skip lighting if bit is off

	LI	TMP, lightdir		; Dot normal with light vector
	LI	TMP2, normal		; (Gets diffuse)
	DOT	TMP, TMP, TMP2		; Result in TMP

	JGE	notnegative		; Check if facing away from light
	LI	TMP,0			; If so, truncate to zero
notnegative:
	LSRI	R0,TMP,16		; See if it's >= 1
	JEQ	nottoobig		; If so, clamp
	LI	TMP,1			; Load a one
	LSLI	TMP,TMP,16		; Put in correct place (s15.16)
nottoobig:

	; diffuse is now in s15.16 format, 0 <= dot <= 1

	; XXX HACK: Add ambient as "dot = dot * 0.75 + 0.25"

	LSRI	TMP3,TMP,2		; Divide by four
	LSRI	TMP,TMP,1		; Divide by two
	ADD	TMP,TMP,TMP3		; TMP = 3/4 DOT
	LI	TMP2,1			; 1/2
	LSLI	TMP2,TMP2,14		; Slide into .25 position
	ADD	TMP,TMP,TMP2		; Add .25

	LD	TMP2,lightcolor		; Load light red
	MULF	TMP2,TMP2,TMP		; Multiply by intensity
	LD	TMP3,surfcolor		; Load surface red
	MULF	TMP2,TMP2,TMP3		; Multiply by it
	LSRI	TMP2,TMP2,11		; Enough bits for red (5)
	ST	VNUM(r),TMP2		; Store in red

	LD	TMP2,lightcolor+1	; Load light green
	MULF	TMP2,TMP2,TMP		; Multiply by intensity
	LD	TMP3,surfcolor+1	; Load surface green
	MULF	TMP2,TMP2,TMP3		; Multiply by it
	LSRI	TMP2,TMP2,10		; Enough bits for green (6)
	ST	VNUM(g),TMP2		; Store in green

	LD	TMP2,lightcolor+2	; Load light blue
	MULF	TMP2,TMP2,TMP		; Multiply by intensity
	LD	TMP3,surfcolor+2	; Load surface blue
	MULF	TMP2,TMP2,TMP3		; Multiply by it
	LSRI	TMP2,TMP2,11		; Enough bits for blue (5)
	ST	VNUM(b),TMP2		; Store in blue

        JMP     donewithcolor           ; Finished dealing with color

nolighting:
        LD      TMP,surfcolor           ; Copy surface color to output
	LSRI	TMP,TMP,11		; Enough bits for red (5)
        ST      VNUM(r),TMP
        LD      TMP,surfcolor+1
	LSRI	TMP,TMP,10		; Enough bits for green (6)
        ST      VNUM(g),TMP
        LD      TMP,surfcolor+2
	LSRI	TMP,TMP,11		; Enough bits for blue (5)
        ST      VNUM(b),TMP

donewithcolor:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Rasterize a triangle
	ST	saveinst,INSTADDR	; Save the instruction address
	ST	savedma,DMAADDR 	; Save the DMA  address

	; Skip those that have vertex number < 2

	LI	TMP,1
	SUB	R0,VNUM,TMP		; if VNUM <= 1
	JLEL	rastend			;    return

	; Compute bounding box
	LD	X, x
	LD	TMP, x+1
	SUB	R0, X, TMP		; if x0 < x1
	JLT	xmin1			;     xmin1
	OR	X, TMP, TMP		; else X = x1
xmin1:	LD	TMP, x+2
	SUB	R0, X, TMP		; if X < x2
	JLT	xmin2			;     xmin2
	OR	X, TMP, TMP		; else X = x2
xmin2:	
	
	LD	Y, y
	LD	TMP, y+1
	SUB	R0, Y, TMP		; if y0 < y1
	JLT	ymin1			;     ymin1
	OR	Y, TMP, TMP		; else Y = y1
ymin1:	LD	TMP, y+2
	SUB	R0, Y, TMP		; if Y < y2
	JLT	ymin2			;     ymin2
	OR	Y, TMP, TMP		; else Y = y2
ymin2:	

	LD	MAXX, x
	LD	TMP, x+1
	SUB	R0, MAXX, TMP		; if x0 > x1
	JGT	xmax1			;     xmax1
	OR	MAXX, TMP, TMP		; else MAXX = x1
xmax1:	LD	TMP, x+2
	SUB	R0, MAXX, TMP		; if MAXX > x2
	JGT	xmax2			;     xmax2
	OR	MAXX, TMP, TMP		; else MAXX = x2
xmax2:	

	LD	MAXY, y
	LD	TMP, y+1
	SUB	R0, MAXY, TMP		; if y0 > y1
	JGT	ymax1			;     ymax1
	OR	MAXY, TMP, TMP		; else MAXY = y1
ymax1:	LD	TMP, y+2
	SUB	R0, MAXY, TMP		; if MAXY > y2
	JGT	ymax2			;     ymax2
	OR	MAXY, TMP, TMP		; else MAXY = y2
ymax2:	


	; Figure out edge equations
.sym	A1,R1
.sym	C1,R2
.sym	A2,R3
.sym	C2,R4
.sym	A3,R5
.sym	C3,R6
.sym	CC1,R7
.sym	CC2,R8
.sym	CC3,R9
.sym	B,R10

	; Edge 1
	LD	A1, y
	LD	TMP, y+1
	SUB	A1, A1, TMP		; A1 = y0 - y1

	LD	B, x+1
	LD	TMP, x
	SUB	B, B, TMP		; B = x1 - x0
	ST	planes+12, B		; Store B1

	LD	INST, y+1
	LD	TMP, x
	MUL	C1, INST, TMP
	LD	INST, y
	LD	TMP, x+1
	MUL	TMP, INST, TMP
	SUB	C1, C1, TMP		; CC1 = y1*x0 - y0*x1
	MUL	TMP, A1, X		; TMP = A1*minx
	ADD	CC1, C1, TMP		
	MUL	TMP, B, Y		; TMP = B*miny
	ADD	CC1, CC1, TMP		; CC = Ax + By + C

	; Edge 2
	LD	A2, y+1
	LD	TMP, y+2
	SUB	A2, A2, TMP		; A2 = y1 - y2

	LD	B, x+2
	LD	TMP, x+1
	SUB	B, B, TMP		; B = x2 - x1
	ST	planes+13, B		; Store B2

	LD	INST, y+2
	LD	TMP, x+1
	MUL	C2, INST, TMP
	LD	INST, y+1
	LD	TMP, x+2
	MUL	TMP, INST, TMP
	SUB	C2, C2, TMP		; C2 = y2*x1 - y1*x2
	MUL	TMP, A2, X		; TMP = A2*minx
	ADD	CC2, C2, TMP		
	MUL	TMP, B, Y		; TMP = B*miny
	ADD	CC2, CC2, TMP		; CC = Ax + By + C

	; Edge 3
	LD	A3, y+2
	LD	TMP, y
	SUB	A3, A3, TMP		; A3 = y2 - y0

	LD	B, x
	LD	TMP, x+2
	SUB	B, B, TMP		; B = x0 - x2
	ST	planes+14, B		; Store B3

	LD	INST, y
	LD	TMP, x+2
	MUL	C3, INST, TMP
	LD	INST, y+2
	LD	TMP, x
	MUL	TMP, INST, TMP
	SUB	C3, C3, TMP		; C3 = y0*x2 - y2*x0
	MUL	TMP, A3, X		; TMP = A3*minx
	ADD	CC3, C3, TMP		
	MUL	TMP, B, Y		; TMP = B*miny
	ADD	CC3, CC3, TMP		; CC = Ax + By + C

	; Figure out plane equations for colors

.sym 	DET, R14
.sym	D1, R15
.sym	D2, R16
.sym	D3, R17
.sym 	I, R11
.sym	A, R12
.sym	C, R13

	; Det
det:	LD	INST, x+1
	LD	TMP, y+2
	MUL	D1, TMP, INST		;d1 = x1y2
	LD	INST, x+2
	LD	TMP, y+1
	MUL	TMP, TMP, INST
	SUB	D1, D1, TMP		;d1 = x1y2-x2y1

	LD	INST, x+0
	LD	TMP, y+2
	MUL	TMP, TMP, INST
	SUB	D2, R0, TMP		;d2 = -x0y2
	LD	INST, x+2
	LD	TMP, y+0
	MUL	TMP, TMP, INST
	ADD	D2, D2, TMP		;d2 = -x0y2+x2y0
	ADD	DET, D1, D2		;det = x1y2-x2y1-x0y2+x2y0

	LD	INST, x+0
	LD	TMP, y+1
	MUL	D3, TMP, INST		;d3 = x0y1
	LD	INST, x+1
	LD	TMP, y+0
	MUL	TMP, TMP, INST
	SUB	D3, D3, TMP		;d3 = x0y1-x1y0
	ADD	DET, D3, DET		;det = x1y2-x2y1-x0y2+x2y0+x0y1-x1y0
	JEQ	rastend			;Don't divide by zero
	RECP	DET, DET

	LI	I, 0
	
	; Compute A
calcA:	LD	INST, I(z+1)
	LD	TMP, y+2
	MUL	A, TMP, INST		;a = r1y2
	LD	INST, I(z+2)
	LD	TMP, y+1
	MUL	TMP, TMP, INST
	SUB	A, A, TMP		;a = r1y2-r2y1
	LD	INST, I(z+0)
	LD	TMP, y+2
	MUL	TMP, TMP, INST
	SUB	A, A, TMP		;a = r1y2-r2y1-r0y2
	LD	INST, I(z+2)
	LD	TMP, y+0
	MUL	TMP, TMP, INST
	ADD	A, A, TMP		;a = r1y2-r2y1-r0y2+r2y0
	LD	INST, I(z+0)
	LD	TMP, y+1
	MUL	TMP, TMP, INST
	ADD	A, A, TMP		;a = r1y2-r2y1-r0y2+r2y0+r0y1
	LD	INST, I(z+1)
	LD	TMP, y+0
	MUL	TMP, TMP, INST
	SUB	A, A, TMP		;a = r1y2-r2y1-r0y2+r2y0+r0y1-r1y0
	MULF	A, A, DET

	; B
	LD	INST, x+1
	LD	TMP, I(z+2)
	MUL	B, TMP, INST		;b = x1r2
	LD	INST, x+2
	LD	TMP, I(z+1)
	MUL	TMP, TMP, INST
	SUB	B, B, TMP		;b = x1r2-x2r1
	LD	INST, x+0
	LD	TMP, I(z+2)
	MUL	TMP, TMP, INST
	SUB	B, B, TMP		;b = x1r2-x2r1-x0r2
	LD	INST, x+2
	LD	TMP, I(z+0)
	MUL	TMP, TMP, INST
	ADD	B, B, TMP		;b = x1r2-x2r1-x0r2+x2r0
	LD	INST, x+0
	LD	TMP, I(z+1)
	MUL	TMP, TMP, INST
	ADD	B, B, TMP		;b = x1r2-x2r1-x0r2+x2r0+x0r1
	LD	INST, x+1
	LD	TMP, I(z+0)
	MUL	TMP, TMP, INST
	SUB	B, B, TMP		;b = x1r2-x2r1-x0r2+x2r0+x0r1-x1r0
	MULF	B, B, DET

	;C
	
	LD	TMP, I(z+0)
	MUL	C, D1, TMP
	LD	TMP, I(z+1)
	MUL	TMP, D2, TMP
	ADD	C, C, TMP
	LD	TMP, I(z+2)
	MUL	TMP, D3, TMP
	ADD	C, C, TMP
	MULF	C, C, DET
	MUL	TMP, A, X
	ADD	C, C, TMP
	MUL	TMP, B, Y
	ADD	C, C, TMP

	ST	I(planes), A
	ST	I(planes+1), B
	ST	I(planes+2), C
	LI	I, I(3)
	LI	TMP, 12
	SUB	R0, I, TMP
	JNE	calcA

    ; Plane equations
.sym	ZA,R10
.sym	ZC,R11
.sym	RA,R12
.sym	RC,R13
.sym	GA,R14
.sym	GC,R15
.sym	BA,R16
.sym	BC,R17

.sym	ZCC,R18
.sym	RCC,R19
.sym	GCC,R20
.sym	BCC,R21

	LD	ZA, planes+0
	LD	ZCC, planes+2
	LD	RA, planes+3
	LD	RCC, planes+5
	LD	GA, planes+6
	LD	GCC, planes+8
	LD	BA, planes+9
	LD	BCC, planes+11

	; Clip bounding box to screen

	; Setup tree

	; Loop over bounding box
	ST	minx, X
yloop:
	LD	X,minx			; Start of X loop

	OR	ZC,ZCC,ZCC		; Load beginning of line values
	OR	RC,RCC,RCC
	OR	GC,GCC,GCC
	OR	BC,BCC,BCC
	OR	C1,CC1,CC1
	OR	C2,CC2,CC2
	OR	C3,CC3,CC3

xloop:
	TREESET	X,Y,R0			; Setup tree and clear enable bits
	TREEIN	C1,A1			; First edge
	TREEIN	C2,A2			; Second edge
	TREEIN	C3,A3			; Third edge
	TREEVAL	ZC,ZA,0			; Interpolate Z
	TREEVAL	RC,RA,1			; Interpolate red
	TREEVAL	GC,GA,2			; Interpolate green
	TREEVAL	BC,BA,3			; Interpolate blue
	TREEPUT				; Stick it into memory

	LI	X,X(16)			; Increment X
	SUB	R0, X,MAXX		; Stop at right end of bounding box
	JLEL	xloop

	LD	TMP, planes+1
	ADD	ZCC,ZCC,TMP
	LD	TMP, planes+4
	ADD	RCC,RCC,TMP
	LD	TMP, planes+7
	ADD	GCC,GCC,TMP
	LD	TMP, planes+10
	ADD	BCC,BCC,TMP
	LD	TMP, planes+12
	ADD	CC1,CC1,TMP
	LD	TMP, planes+13
	ADD	CC2,CC2,TMP
	LD	TMP, planes+14
	ADD	CC3,CC3,TMP
	
	LI	Y,Y(1)			; Increment Y
	SUB	R0, Y,MAXY		; Stop at top of bounding box
	JLEL	yloop


rastend:
	LD	INSTADDR,saveinst	; Restore the instruction address
	LD	DMAADDR,savedma		; Restore the DMA address
	LD	RETURN,savereturn	; Restore the return address
	JMP	RETURN

.unsym	VNUM
.unsym	V1
.unsym	V2
.unsym	V3
.unsym	Y
.unsym	MAXX
.unsym	MAXY
.unsym	X

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Call: 01 AA AA AA
	;

instCALL:
	LD	TMP,stackptr
	ST	TMP,INSTADDR		; Store return address
	LI	TMP,TMP(1)		; Increment stack pointer
	ST	stackptr,TMP		; Store stack pointer
	OR	INSTADDR,INST,INST	; Jump to new address
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Return: 02 00 00 00
	;

instRETURN:
.sym	STACK,R1
	LD	STACK,stackptr		; Get stack pointer
	LI	STACK,STACK(-1)		; Decrement by one
	LD	INSTADDR,STACK		; Jump to old address
	ST	stackptr,STACK		; Store stack pointer
	LI	TMP,callstack		; Get address of stack
	SUB	R0, STACK,TMP		; Compare with stack pointer
	JLT	halt			; If popped everything off, halt
	JMP	RETURN
.unsym	STACK

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Push: 03 00 00 00
	;

instPUSH:
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Pop: 04 00 00 00
	;

instPOP:
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Load: 05 00 00 WW  MM MM MM MM  ...  MM MM MM MM  (16 words)
	;

instLOAD:
.sym	DEST,R1
.sym	COUNT,R2
	ST	savereturn,RETURN	; Save the return address
	LI	COUNT,16		; Number of words to copy
	LI	DEST,projection		; Default to projection
	SUB	R0, INST,R0		; See what matrix we're loading
	JNE	loadloop		; If !0, then projection was right
	LI	DEST,modelview		; Else load the modelview
loadloop:
	CALL	getinst			; Get next matrix element
	ST	DEST,INST		; Store the element
	LI	DEST,DEST(1)		; Increment destination
	LI	COUNT,COUNT(-1)		; Decrement count
	JNEL	loadloop		; If not done, get next element
	; Should probably do something with the matrices here
	LD	RETURN,savereturn	; Restore the return address
	JMP	RETURN
.unsym	DEST
.unsym	COUNT

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Multiply: 06 00 00 WW  MM MM MM MM  ...  MM MM MM MM  (16 words)
	;

instMULT:
.sym	DEST,R1
.sym	TMPDEST,R2
.sym	COUNT,R3
.sym	PROD1,R4
.sym	PROD2,R5
.sym	PROD3,R6
.sym	PROD4,R7
.sym	SRC,R8
	ST	savereturn,RETURN	; Save the return address
	LI	COUNT,4			; Number of columns to copy
	LI	TMPDEST,tmat1		; Load transpose into tmp matrix
	LI	DEST,projection		; Default to projection
	SUB	R0, INST,R0		; See what matrix we're loading
	JNE	multloop		; If !0, then projection was right
	LI	DEST,modelview		; Else load the modelview
multloop:
	CALL	getinst			; Get next matrix element
	ST	TMPDEST,INST		; Store the element
	CALL	getinst			; Get next matrix element
	ST	TMPDEST(4),INST		; Store the element
	CALL	getinst			; Get next matrix element
	ST	TMPDEST(8),INST		; Store the element
	CALL	getinst			; Get next matrix element
	ST	TMPDEST(12),INST	; Store the element
	LI	TMPDEST,TMPDEST(1)	; Increment destination
	LI	COUNT,COUNT(-1)		; Decrement count
	JNEL	multloop		; If not done, get next element

	; Now the transpose of the new matrix is in tmat1.
	; We take four dot products at a time and store them afterwards
	; so we can avoid having to copy the source matrix.

	LI	COUNT,4			; Number of rows to multiply
multmatrix:
	LI	SRC,tmat1		; First column of new matrix
	DOT	PROD1,DEST,SRC		; Row * column1
	LI	SRC,tmat1+4		; Next column
	DOT	PROD2,DEST,SRC		; Row * column1
	LI	SRC,tmat1+8		; Next column
	DOT	PROD3,DEST,SRC		; Row * column1
	LI	SRC,tmat1+12		; Next column
	DOT	PROD4,DEST,SRC		; Row * column1
	ST	DEST,PROD1		; Store first element
	ST	DEST(1),PROD2		; Store second element
	ST	DEST(2),PROD3		; Store third element
	ST	DEST(3),PROD4		; Store fourth element
	LI	DEST,DEST(4)		; Next row of source
	LI	COUNT,COUNT(-1)		; Decrement count
	JNEL	multmatrix		; If not done, do next row

	; Should probably do something with the matrices here
	LD	RETURN,savereturn	; Restore the return address
	JMP	RETURN
.unsym	DEST
.unsym	TMPDEST
.unsym	COUNT
.unsym	PROD1
.unsym	PROD2
.unsym	PROD3
.unsym	PROD4
.unsym	SRC


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Light: 07 NX NY NZ  00 RR GG BB
	;

instLIGHT:
	ST	savereturn,RETURN	; Save the return address

	LSRI	TMP,INST,16		; Lose lower bits for Nx (s24.7)
	LSLI	TMP,TMP,24		; Shift all the way left
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	lightdir,TMP		; Store in light vector

	LSRI	TMP,INST,8		; Lose lower bits for Ny (s24.7)
	LSLI	TMP,TMP,24		; Shift all the way left
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	lightdir+1,TMP		; Store in light vector

	LSLI	TMP,INST,24		; Shift all the way left (Nz) (s24.7)
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	lightdir+2,TMP		; Store in light vector

	LI	TMP,0			; Store 0 in fourth element
	ST	lightdir+3,TMP		; (So dot product isn't affected)

	CALL	getinst			; Get light color

	LSRI	TMP,INST,16		; Lose lower bits of R
	LSLI	TMP,TMP,24		; Lose upper bits of R
	LSRI	TMP,TMP,16		; Put in correct place
	ST	lightcolor,TMP		; Store in light color

	LSRI	TMP,INST,8		; Lose lower bits of G
	LSLI	TMP,TMP,24		; Lose upper bits of G
	LSRI	TMP,TMP,16		; Put in correct place
	ST	lightcolor+1,TMP	; Store in light color

	LSLI	TMP,INST,24		; Lose upper bits of B
	LSRI	TMP,TMP,16		; Put in correct place
	ST	lightcolor+2,TMP	; Store in light color

	LD	RETURN,savereturn	; Restore the return address
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Set: 08 XX XX XX
	;

instSET:
	LD	TMP,modeword
	OR	TMP,TMP,INST		; OR in the bits we want to set
	ST	modeword,TMP
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Reset: 09 XX XX XX
	;

instRESET:
	NOT	INST,INST		; Make mask
	LD	TMP,modeword
	AND	TMP,TMP,INST		; AND out the bits we want to clear
	ST	modeword,TMP
	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Color: 0A RR GG BB
	;

instCOLOR:
	LSRI	TMP,INST,16		; Lose lower bits of R
	LSLI	TMP,TMP,24		; Lose upper bits of R
	LSRI	TMP,TMP,16		; Put in correct place
	ST	surfcolor,TMP		; Store in surface color

	LSRI	TMP,INST,8		; Lose lower bits of G
	LSLI	TMP,TMP,24		; Lose upper bits of G
	LSRI	TMP,TMP,16		; Put in correct place
	ST	surfcolor+1,TMP		; Store in surface color

	LSLI	TMP,INST,24		; Lose upper bits of B
	LSRI	TMP,TMP,16		; Put in correct place
	ST	surfcolor+2,TMP		; Store in surface color

	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Normal: 0A NX NY NZ
	;

instNORMAL:
	LSRI	TMP,INST,16		; Lose lower bits for Nx (s24.7)
	LSLI	TMP,TMP,24		; Shift all the way left
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	normal,TMP		; Store in normal vector

	LSRI	TMP,INST,8		; Lose lower bits for Ny (s24.7)
	LSLI	TMP,TMP,24		; Shift all the way left
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	normal+1,TMP		; Store in normal vector

	LSLI	TMP,INST,24		; Shift all the way left (Nz) (s24.7)
	ASRI	TMP,TMP,15		; Sign-extend value
	ST	normal+2,TMP		; Store in normal vector

	LI	TMP,0			; Store 0 in fourth element
	ST	normal+3,TMP		; (So dot product isn't affected)

	JMP	RETURN

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;
	; Halt
	;

halt:
	; No instructions.  Jumping here goes until the end of memory.

