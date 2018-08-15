.code

top:
        ; LI      r2, 0x1122
        ; LI      r3, 0x3344
        ; ADD     r1, r2, r3
        ; JMP     top

        LI      r2, 0x123
        LI      r3, 0x124
        SUB     r1, r2, r3
        JEQ     top

