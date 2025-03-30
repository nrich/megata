/******************************************************************************

Copyright (C) 2025 Neil Richardson (nrich@neiltopia.com)

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

******************************************************************************/

#include "CPU.h"

#include <iostream>

static uint8_t Cycles[256] = {
    7,6,2,2,3,3,5,5,3,2,2,2,2,4,6,5,
    2,5,3,2,3,4,6,5,2,4,2,2,4,4,7,5,
    6,6,2,2,3,3,5,5,4,2,2,2,4,4,6,5,
    2,5,3,2,4,4,6,5,2,4,2,2,4,4,7,5,
    6,6,2,2,2,3,5,5,3,2,2,2,3,4,6,5,
    2,5,3,2,2,4,6,5,2,4,3,2,2,4,7,5,
    6,6,2,2,2,3,5,5,4,2,2,2,5,4,6,5,
    2,5,3,2,4,4,6,5,2,4,4,2,2,4,7,5,
    2,6,2,2,3,3,3,5,2,2,2,2,4,4,4,5,
    2,6,4,2,4,4,4,5,2,5,2,2,4,5,5,5,
    2,6,2,2,3,3,3,5,2,2,2,2,4,4,4,5,
    2,5,3,2,4,4,4,5,2,4,2,2,4,4,4,5,
    2,6,2,2,3,3,5,5,2,2,2,2,4,4,6,5,
    2,5,3,2,2,4,6,5,2,4,3,2,2,4,7,5,
    2,6,2,2,3,3,5,5,2,2,2,2,4,4,6,5,
    2,5,3,2,2,4,6,5,2,4,4,2,2,4,7,5,
};

static uint8_t ZNTable[256] = {
    FLAG::Z,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
    FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,FLAG::N,
};

CPU::CPU(std::function<uint8_t(uint16_t)> read, std::function<void(uint16_t, uint8_t)> write, std::function<uint8_t()> loop) : read(read), write(write), loop(loop) {
    reset();
}

inline void CPU::M_FL(uint8_t Rg) {
    P = (P & ~(FLAG::Z|FLAG::N)) | ZNTable[Rg];
}

inline void CPU::M_CMP(WordBytes &K, uint8_t Rg1, uint8_t Rg2) {
    K.W = Rg1 - Rg2;
    P &= ~(FLAG::N|FLAG::Z|FLAG::C);
    P |= ZNTable[K.B.l] | (K.B.h? 0:FLAG::C);
}


inline void CPU::M_ADC(uint8_t &Rg) {
    uint32_t w;

    if ((A ^ Rg) & 0x80) {
        P &= ~FLAG::V;
    } else {
        P |= FLAG::V;
    }

    if (P&FLAG::D) {
        w = (A & 0xf) + (Rg & 0xf) + (P & FLAG::C);

        if (w >= 10) w = 0x10 | ((w+6)&0xf);
            w += (A & 0xf0) + (Rg & 0xf0);
        if (w >= 160) {
            P |= FLAG::C;

            if ((P&FLAG::V) && w >= 0x180)
                P &= ~ FLAG::V;
            w += 0x60;
        } else {
            P &= ~FLAG::C;
            if ((P&FLAG::V) && w < 0x80)
                P &= ~FLAG::V;
        }
    } else {
        w = A + Rg + (P&FLAG::C);
        if (w >= 0x100) {
            P |= FLAG::C;
            if ((P & FLAG::V) && w >= 0x180)
                P &= ~FLAG::V;
        } else {
            P &= ~FLAG::C;
            if ((P&FLAG::V) && w < 0x80)
                P &= ~FLAG::V;
        }
    }

    A = (uint8_t)w;
    P = (P & ~(FLAG::Z | FLAG::N)) | (A >= 0x80 ? FLAG::N : 0) | (A == 0 ? FLAG::Z : 0);
}

inline void CPU::M_SBC(uint8_t val) {
    uint32_t w;

    if ((A ^ val) & 0x80) {
        P |= FLAG::V;
    }
    else {
        P &= ~FLAG::V;
    }

    if (P&FLAG::D) {
        // decimal subtraction
        uint32_t temp = 0xf + (A & 0xf) - (val & 0xf) + (P & FLAG::C);
        if (temp < 0x10) {
            w = 0;
            temp -= 6;
        }
        else {
            w = 0x10;
            temp -= 0x10;
        }
        w += 0xf0 + (A & 0xf0) - (val & 0xf0);
        if (w < 0x100) {
            P &= ~FLAG::C;
            if ((P&FLAG::V) && w < 0x80) P &= ~FLAG::V;
            w -= 0x60;
        } else {
            P |= FLAG::C;
            if ((P&FLAG::V) && w >= 0x180) P &= ~FLAG::V;
        }
        w += temp;
    } else {
        // standard binary subtraction
        w = 0xff + A - val + (P&FLAG::C);
        if (w < 0x100) {
            P &= ~FLAG::C;
            if ((P & FLAG::V) && w < 0x80) P &= ~FLAG::V;
        }
        else {
            P |= FLAG::C;
            if ((P&FLAG::V) && w >= 0x180) P &= ~FLAG::V;
        }
    }
    A = (unsigned char)w;
    P = (P & ~(FLAG::Z | FLAG::N)) | (A >= 0x80 ? FLAG::N : 0) | (A == 0 ? FLAG::Z : 0);
}

void CPU::reset() {
    A = 0x00;
    X = 0x00;
    Y = 0x00;
    P = FLAG::Z | FLAG::R;
    S = 0xFF;
    PC.B.l = read(0xFFFC);
    PC.B.h = read(0xFFFD);
    count = period;
    request = INT::NONE;
    after = 0;
}

void CPU::interupt(INT type) {
    WordBytes J;

    if ((type == INT::NMI) || ((type == INT::IRQ) && !(P&FLAG::I))) {
        count -= 7;
        M_PUSH(PC.B.h);
        M_PUSH(PC.B.l);
        M_PUSH(P&~FLAG::B);
        P &= ~FLAG::D;
        if (type == INT::NMI) {
            J.W = 0xFFFA; 
        } else {
            P |= FLAG::I; 
            J.W = 0xFFFE;
        }
        PC.B.l = read(J.W++);
        PC.B.h = read(J.W);
    }
}

void CPU::run() {
    WordBytes J, K;
    uint8_t I;

    while (true) {
        I = read(PC.W++);
        count -= Cycles[I];
        switch (I) {
            case 0x00: // BRK
                PC.W++;
                M_PUSH(PC.B.h); M_PUSH(PC.B.l);
                M_PUSH(P | FLAG::B);
                P = (P | FLAG::I)&~FLAG::D;
                PC.B.l = read(0xFFFE);
                PC.B.h = read(0xFFFF);
                break;

            case 0x01: // ORA ($ss,x) INDEXINDIR
                MR_Ix(K, K, I);
                M_ORA(I);
                break;

            case 0x04: //
                MM_Zp(I, J, std::bind(&CPU::M_TSB, this, std::placeholders::_1));
                break;

            case 0x05: // ORA $ss ZP
                MR_Zp(J, I);
                M_ORA(I);
                break;

            case 0x06: // ASL $ss ZP
                MM_Zp(I, J, std::bind(&CPU::M_ASL, this, std::placeholders::_1));
                break;

            case 0x08: // PHP
                M_PUSH(P);
                break;

            case 0x09: // ORA #$ss IMM
                MR_Im(I);
                M_ORA(I);
                break;

            case 0x0A: // ASL a ACC
                M_ASL(A);
                break;

            case 0x0C: //
                MM_Ab(I, J,std::bind(&CPU::M_TSB, this, std::placeholders::_1));
                break;

            case 0x0D: // ORA $ssss ABS
                MR_Ab(J, I);
                M_ORA(I);
                break;

            case 0x0E: // ASL $ssss ABS
                MM_Ab(I, J, std::bind(&CPU::M_ASL, this, std::placeholders::_1));
                break;

            case 0x10: // BPL * REL
                if (P & FLAG::N) {
                    PC.W++;
                } else {
                    M_JR();
                }
                break;

            case 0x11: // ORA ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_ORA(I);
                break;

            case 0x12: //
                MR_Izp(J, K, I);
                M_ORA(I);
                break;

            case 0x14: //
                MM_Zp(I, J, std::bind(&CPU::M_TRB, this, std::placeholders::_1));
                break;

            case 0x15: // ORA $ss,x ZP,x
                MR_Zx(J, I);
                M_ORA(I);
                break;

            case 0x16: // ASL $ss,x ZP,x
                MM_Zx(I, J, std::bind(&CPU::M_ASL, this, std::placeholders::_1));
                break;

            case 0x18: // CLC
                P &= ~FLAG::C;
                break;

            case 0x19: // ORA $ssss,y ABS,y
                MR_Ay(J, I);
                M_ORA(I);
                break;

            case 0x1A: //
                M_INC(A);
                break;

            case 0x1C: //
                MM_Ab(I, J, std::bind(&CPU::M_TRB, this, std::placeholders::_1));
                break;

            case 0x1D: // ORA $ssss,x ABS,x
                MR_Ax(J, I);
                M_ORA(I);
                break;

            case 0x1E: // ASL $ssss,x ABS,x
                MM_Ax(I, J, std::bind(&CPU::M_ASL, this, std::placeholders::_1));
                break;

            case 0x20: //
                K.B.l = read(PC.W++);
                K.B.h = read(PC.W);
                M_PUSH(PC.B.h);
                M_PUSH(PC.B.l);
                PC = K;
                break;

            case 0x21: // AND ($ss,x) INDEXINDIR
                MR_Ix(J, K, I);
                M_AND(I);
                break;

            case 0x24: //  BIT $ss ZP
                MR_Zp(J, I);
                M_BIT(I);
                break;

            case 0x25: // AND $ss ZP
                MR_Zp(J, I);
                M_AND(I);
                break;

            case 0x26: // ROL $ss ZP
                MM_Zp(I, J, K, std::bind(&CPU::M_ROL, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x28: // FLAG::B added from new M6502
                M_POP(I);
                if ((request != INT::NONE) && ((I ^ P) & ~I&FLAG::I)) {
                    after = 1;
                    backup = count;
                    count = 1;
                }
                P = I | FLAG::R | FLAG::B;
                break;

            case 0x29: // AND #$ss IMM
                MR_Im(I);
                M_AND(I);
                break;

            case 0x2A: // ROL a ACC
                M_ROL(K, A);
                break;

            case 0x2C: // BIT $ssss ABS
                MR_Ab(J, I);
                M_BIT(I);
                break;

            case 0x2D: // AND $ssss ABS
                MR_Ab(J, I);
                M_AND(I);
                break;

            case 0x2E: // ROL $ssss ABS
                MM_Ab(I, J, K, std::bind(&CPU::M_ROL, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x30: // BMI * REL
                if (P & FLAG::N) {
                    M_JR();
                } else {
                    PC.W++;
                }
                break;

            case 0x31: // AND ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_AND(I);
                break;

            case 0x32: //
                MR_Izp(J, K, I);
                M_AND(I);
                break;

            case 0x34: //
                MR_Zx(J, I);
                M_BIT(I);
                break;

            case 0x35: // AND $ss,x ZP,x
                MR_Zx(J, I);
                M_AND(I);
                break;

            case 0x36: // ROL $ss,x ZP,x
                MM_Zx(I, J, K, std::bind(&CPU::M_ROL, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x38: // SEC
                P |= FLAG::C;
                break;

            case 0x39: // AND $ssss,y ABS,y
                MR_Ay(J, I);
                M_AND(I);
                break;

            case 0x3A: //
                M_DEC(A);
                break;

            case 0x3C: //
                MR_Ax(J, I);
                M_BIT(I);
                break;

            case 0x3D: // AND $ssss,x ABS,x
                MR_Ax(J, I);
                M_AND(I);
                break;

            case 0x3E: // ROL $ssss,x ABS,x
                MM_Ax(I, J, K, std::bind(&CPU::M_ROL, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x40: //
                M_POP(P); P |= FLAG::R;
                M_POP(PC.B.l);
                M_POP(PC.B.h);
                break;

            case 0x41: // EOR ($ss,x) INDEXINDIR
                MR_Ix(J, K, I);
                M_EOR(I);
                break;

            case 0x45: // EOR $ss ZP
                MR_Zp(J, I);
                M_EOR(I);
                break;

            case 0x46: // LSR $ss ZP
                MM_Zp(I, J, std::bind(&CPU::M_LSR, this, std::placeholders::_1));
                break;

            case 0x48: // PHA
                M_PUSH(A);
                break;

            case 0x49: // EOR #$ss IMM
                MR_Im(I);
                M_EOR(I);
                break;

            case 0x4A: // LSR a ACC
                M_LSR(A);
                break;

            case 0x4C: //
                M_LDWORD(K);
                PC = K;
                break;

            case 0x4D: // EOR $ssss ABS
                MR_Ab(J, I);
                M_EOR(I);
                break;

            case 0x4E: // LSR $ssss ABS
                MM_Ab(I, J, std::bind(&CPU::M_LSR, this, std::placeholders::_1));
                break;

            case 0x50: // BVC * REL
                if (P & FLAG::V) {
                    PC.W++;
                } else {
                    M_JR();
                }
                break;

            case 0x51: // EOR ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_EOR(I);
                break;

            case 0x52: //
                MR_Izp(J, K, I);
                M_EOR(I);
                break;

            case 0x55: // EOR $ss,x ZP,x
                MR_Zx(J, I);
                M_EOR(I);
                break;

            case 0x56: // LSR $ss,x ZP,x
                MM_Zx(I, J, std::bind(&CPU::M_LSR, this, std::placeholders::_1));
                break;

            case 0x58: //
                if ((request != INT::NONE) && (P & FLAG::I)) {
                    after = 1;
                    backup = count; 
                    count = 1;
                }
                P &= ~FLAG::I;
                break;

            case 0x59: // EOR $ssss,y ABS,y
                MR_Ay(J, I);
                M_EOR(I);
                break;

            case 0x5A: //
                M_PUSH(Y);
                break;

            case 0x5D: // EOR $ssss,x ABS,x
                MR_Ax(J, I);
                M_EOR(I);
                break;

            case 0x5E: // LSR $ssss,x ABS,x
                MM_Ax(I, J, std::bind(&CPU::M_LSR, this, std::placeholders::_1));
                break;

            case 0x60: //
                M_POP(PC.B.l);
                M_POP(PC.B.h);
                PC.W++;
                break;

            case 0x61: // ADC ($ss,x) INDEXINDIR
                MR_Ix(J, K, I);
                M_ADC(I);
                break;

            case 0x64: //
                MW_Zp(J, 0);
                break;

            case 0x65: // ADC $ss ZP
                MR_Zp(J, I);
                M_ADC(I);
                break;

            case 0x66: // ROR $ss ZP
                MM_Zp(I, J, K, std::bind(&CPU::M_ROR, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x68: // PLA
                M_POP(A);
                M_FL(A);
                break;

            case 0x69: // ADC #$ss IMM
                MR_Im(I);
                M_ADC(I);
                break;

            case 0x6A: // ROR a ACC
                M_ROR(K, A);
                break;

            case 0x6C: // from newer M6502
                M_LDWORD(K);
                PC.B.l = read(K.W);
                K.B.l++;
                PC.B.h = read(K.W);
                break;

            case 0x6D: // ADC $ssss ABS
                MR_Ab(J, I);
                M_ADC(I);
                break;

            case 0x6E: // ROR $ssss ABS
                MM_Ab(I, J, K, std::bind(&CPU::M_ROR, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x70: // BVS * RE
                if (P&FLAG::V) {
                    M_JR();
                } else {
                    PC.W++;
                }
                break;

            case 0x71: // ADC ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_ADC(I);
                break;

            case 0x72: //
                MR_Izp(J, K, I);
                M_ADC(I);
                break;

            case 0x74: //
                MW_Zx(J, 0);
                break;

            case 0x75: // ADC $ss,x ZP,x
                MR_Zx(J, I);
                M_ADC(I);
                break;

            case 0x76: // ROR $ss,x ZP,x
                MM_Zx(I, J, K, std::bind(&CPU::M_ROR, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x78: // SEI
                P |= FLAG::I;
                break;

            case 0x79: // ADC $ssss,y ABS,y
                MR_Ay(J, I);
                M_ADC(I);
                break;

            case 0x7A: //
                M_POP(Y);
                M_FL(Y);
                break;

            case 0x7C: //
                M_LDWORD(K);
                PC.B.l = read(K.W++);
                PC.B.h = read(K.W);
                PC.W += X;
                break;

            case 0x7D: // ADC $ssss,x ABS,x
                MR_Ax(J, I);
                M_ADC(I);
                break;

            case 0x7E: // ROR $ssss,x ABS,x
                MM_Ax(I, J, K, std::bind(&CPU::M_ROR, this, std::placeholders::_1, std::placeholders::_2));
                break;

            case 0x80: //
                M_JR();
                break;

            case 0x81: // STA ($ss,x) INDEXINDIR
                MW_Ix(J, K, A);
                break;

            case 0x84: // STY $ss ZP
                MW_Zp(J, Y);
                break;

            case 0x85: // STA $ss ZP
                MW_Zp(J, A);
                break;

            case 0x86: // STX $ss ZP
                MW_Zp(J, X);
                break;

            case 0x88: // DEY
                Y--;
                M_FL(Y);
                break;

            case 0x89: //
                MR_Im(I);
                M_BIT(I);
                break;

            case 0x8A: // TXA
                A = X;
                M_FL(A);
                break;

            case 0x8C: // STY $ssss ABS
                MW_Ab(J, Y);
                break;

            case 0x8D: // STA $ssss ABS
                MW_Ab(J, A);
                break;

            case 0x8E: // STX $ssss ABS
                MW_Ab(J, X);
                break;

            case 0x90: // BCC * REL
                if (P&FLAG::C) {
                    PC.W++;
                } else {
                    M_JR();
                }
                break;

            case 0x91: // STA ($ss),y INDIRINDEX
                MW_Iy(J, K, A);
                break;

            case 0x92: //
                MW_Izp(J, K, A);
                break;

            case 0x94: // STY $ss,x ZP,x
                MW_Zx(J, Y);
                break;

            case 0x95: // STA $ss,x ZP,x
                MW_Zx(J, A);
                break;

            case 0x96: // STX $ss,y ZP,y
                MW_Zy(J, X);
                break;

            case 0x98: // TYA
                A = Y; M_FL(A);
                break;

            case 0x99: // STA $ssss,y ABS,y
                MW_Ay(J, A);
                break;

            case 0x9A: // TXS
                S = X;
                break;

            case 0x9C: // 
                MW_Ab(J, 0);
                break;

            case 0x9D: // STA $ssss,x ABS,x
                MW_Ax(J, A);
                break;

            case 0x9E: //
                MW_Ax(J, 0);
                break;

            case 0xA0: // LDY #$ss IMM
                MR_Im(Y);
                M_FL(Y);
                break;

            case 0xA1: // LDA ($ss,x) INDEXINDIR
                MR_Ix(J, K, A);
                M_FL(A);
                break;

            case 0xA2: // LDX #$ss IMM
                MR_Im(X);
                M_FL(X);
                break;

            case 0xA4: // LDY $ss ZP
                MR_Zp(J, Y);
                M_FL(Y);
                break;

            case 0xA5: // LDA $ss ZP
                MR_Zp(J, A);
                M_FL(A);
                break;

            case 0xA6: // LDX $ss ZP
                MR_Zp(J, X);
                M_FL(X);
                break;

            case 0xA8:// TAY
                Y = A;
                M_FL(Y);
                break;

            case 0xA9: // LDA #$ss IMM
                MR_Im(A);
                M_FL(A);
                break;

            case 0xAA: // TAX
                X = A;
                M_FL(X);
                break;

            case 0xAC: // LDY $ssss ABS
                MR_Ab(J, Y);
                M_FL(Y);
                break;

            case 0xAD: // LDA $ssss ABS
                MR_Ab(J, A);
                M_FL(A);
                break;

            case 0xAE: // LDX $ssss ABS
                MR_Ab(J, X);
                M_FL(X);
                break;

            case 0xB0: // BCS * REL
                if (P&FLAG::C) {
                    M_JR();
                } else {
                    PC.W++;
                }
                break;

            case 0xB1: // LDA ($ss),y INDIRINDEX
                MR_Iy(J, K, A);
                M_FL(A);
                break;

            case 0xB2: //
                MR_Izp(J, K, A);
                M_FL(A);
                break;

            case 0xB4: // LDY $ss,x ZP,x
                MR_Zx(J, Y);
                M_FL(Y);
                break;

            case 0xB5: // LDA $ss,x ZP,x
                MR_Zx(J, A);
                M_FL(A);
                break;

            case 0xB6: // LDX $ss,y ZP,y
                MR_Zy(J, X);
                M_FL(X);
                break; 

            case 0xB8: // CLV
                P &= ~FLAG::V;
                break;

            case 0xB9: // LDA $ssss,y ABS,y
                MR_Ay(J, A);
                M_FL(A);
                break;

            case 0xBA: // TSX
                X = S;
                M_FL(X);
                break;

            case 0xBC: // LDY $ssss,x ABS,x
                MR_Ax(J, Y);
                M_FL(Y);
                break;

            case 0xBD: // LDA $ssss,x ABS,x
                MR_Ax(J, A);
                M_FL(A);
                break;

            case 0xBE: // LDX $ssss,y ABS,y
                MR_Ay(J, X);
                M_FL(X);
                break;

            case 0xC0: // CPY #$ss IMM
                MR_Im(I);
                M_CMP(K, Y, I);
                break;

            case 0xC1: // CMP ($ss,x) INDEXINDIR
                MR_Ix(J, K, I);
                M_CMP(K, A, I);
                break;

            case 0xC4: // CPY $ss ZP
                MR_Zp(J, I);
                M_CMP(K, Y, I);
                break;

            case 0xC5: // CMP $ss ZP
                MR_Zp(J, I);
                M_CMP(K, A, I);
                break;

            case 0xC6: // DEC $ss ZP
                MM_Zp(I, J, std::bind(&CPU::M_DEC, this, std::placeholders::_1));
                break;

            case 0xC8: // INY
                Y++;
                M_FL(Y);
                break;

            case 0xC9: // CMP #$ss IMM
                MR_Im(I);
                M_CMP(K, A, I);
                break;

            case 0xCA: // DEX
                X--;
                M_FL(X);
                break;

            case 0xCC: // CPY $ssss ABS
                MR_Ab(J, I);
                M_CMP(K, Y, I);
                break;

            case 0xCD: // CMP $ssss ABS
                MR_Ab(J, I);
                M_CMP(K, A, I);
                break;

            case 0xCE: // DEC $ssss ABS
                MM_Ab(I, J, std::bind(&CPU::M_DEC, this, std::placeholders::_1));
                break;

            case 0xD0: // BNE * REL
                if (P & FLAG::Z) {
                    PC.W++;
                } else {
                    M_JR();
                }
                break;

            case 0xD1: // CMP ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_CMP(K, A, I);
                break;

            case 0xD2: //
                MR_Izp(J, K, I);
                M_CMP(K, A, I);
                break;

            case 0xD5: // CMP $ss,x ZP,x
                MR_Zx(J, I);
                M_CMP(K, A, I);
                break;

            case 0xD6: // DEC $ss,x ZP,x
                MM_Zx(I, J, std::bind(&CPU::M_DEC, this, std::placeholders::_1));
                break;

            case 0xD8: // CLD
                P &= ~FLAG::D;
                break;

            case 0xD9: // CMP $ssss,y ABS,y
                MR_Ay(J, I);
                M_CMP(K, A, I);
                break;

            case 0xDA: //
                M_PUSH(X);
                break;

            case 0xDD: // CMP $ssss,x ABS,x
                MR_Ax(J, I);
                M_CMP(K, A, I);
                break;

            case 0xDE: // DEC $ssss,x ABS,x
                MM_Ax(I, J, std::bind(&CPU::M_DEC, this, std::placeholders::_1));
                break;

            case 0xE0: // CPX #$ss IMM
                MR_Im(I); M_CMP(K, X, I);
                break;

            case 0xE1: // SBC ($ss,x) INDEXINDIR
                MR_Ix(J, K, I);
                M_SBC(I);
                break;

            case 0xE4: // CPX $ss ZP
                MR_Zp(J, I);
                M_CMP(K, X, I);
                break;

            case 0xE5: // SBC $ss ZP
                MR_Zp(J, I);
                M_SBC(I);
                break;

            case 0xE6: // INC $ss ZP
                MM_Zp(I, J, std::bind(&CPU::M_INC, this, std::placeholders::_1));
                break;

            case 0xE8: // INX
                X++; M_FL(X);
                break;

            case 0xE9: // SBC #$ss IMM
                MR_Im(I);
                M_SBC(I);
                break;

            case 0xEA: // NOP
                break;

            case 0xEC: // CPX $ssss ABS
                MR_Ab(J, I);
                M_CMP(K, X, I);
                break;

            case 0xED: // SBC $ssss ABS
                MR_Ab(J, I);
                M_SBC(I);
                break;

            case 0xEE: // INC $ssss ABS
                MM_Ab(I, J, std::bind(&CPU::M_INC, this, std::placeholders::_1));
                break;

            case 0xF0: // BEQ * REL
                if (P & FLAG::Z) {
                    M_JR();
                } else {
                    PC.W++;
                }
                break;

            case 0xF1: // SBC ($ss),y INDIRINDEX
                MR_Iy(J, K, I);
                M_SBC(I);
                break;

            case 0xF2: //
                MR_Izp(J, K, I);
                M_SBC(I);
                break;

            case 0xF5: // SBC $ss,x ZP,x
                MR_Zx(J, I);
                M_SBC(I);
                break;

            case 0xF6: // INC $ss,x ZP,x
                MM_Zx(I, J, std::bind(&CPU::M_INC, this, std::placeholders::_1));
                break;

            case 0xF8: // SED
                P |= FLAG::D;
                break;

            case 0xF9: // SBC $ssss,y ABS,y
                MR_Ay(J, I);
                M_SBC(I);
                break;

            case 0xFA: //
                M_POP(X);
                M_FL(X);
                break;

            case 0xFD: // SBC $ssss,x ABS,x
                MR_Ax(J, I);
                M_SBC(I);
                break;

            case 0xFE: // INC $ssss,x ABS,x
                MM_Ax(I, J, std::bind(&CPU::M_INC, this, std::placeholders::_1));
                break;

            default:
                break;
        }

        if (count <= 0) {
            if (after) {
                I = request;
                count += backup - 1;
                after = 0;
            } else {
                I = loop();
                count = period;
            }

            if (I == INT::QUIT)
                return;
            if (I)
                interupt((INT)I);
        }
    }
}

CPU::~CPU() {

}
