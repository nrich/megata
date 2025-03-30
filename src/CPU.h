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

#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <functional>

enum INT : uint8_t {
    NONE    = 0,
    IRQ     = 1,
    NMI     = 2,
    QUIT    = 3,
};

typedef union {
    struct {
        uint8_t l;
        uint8_t h;
    } B;
    uint16_t W;
} WordBytes;

enum FLAG : uint8_t {
    C       = 0x01,
    Z       = 0x02,
    I       = 0x04,
    D       = 0x08,
    B       = 0x10,
    R       = 0x20,
    V       = 0x40,
    N       = 0x80,
};

class CPU {
    WordBytes PC;

    int32_t period;
    int32_t count;
    uint8_t request;

    uint8_t after;
    int32_t backup;

    uint8_t A;
    uint8_t P;
    uint8_t X;
    uint8_t Y;
    uint8_t S;

    std::function<uint8_t(uint16_t)> read;
    std::function<void(uint16_t, uint8_t)> write;
    std::function<uint8_t()> loop;

    void M_ADC(uint8_t &Rg);
    void M_FL(uint8_t Rg);

    inline void M_LDWORD(WordBytes &Rg) {
        Rg.B.l = read(PC.W++);
        Rg.B.h = read(PC.W++);
    }

    inline void MC_Ab(WordBytes &Rg) {
        M_LDWORD(Rg);
    }

    inline void MC_Zp(WordBytes &Rg) {
        Rg.W = read(PC.W++);
    }

    inline void MC_Zx(WordBytes &Rg) {
        Rg.W = (uint8_t)(read(PC.W++)+X);
    }

    inline void MC_Zy(WordBytes &Rg) {
        Rg.W = (uint8_t)(read(PC.W++)+Y);
    }

    inline void MC_Ax(WordBytes &Rg) {
        M_LDWORD(Rg);
        Rg.W += X;
    }

    inline void MC_Ay(WordBytes &Rg) {
        M_LDWORD(Rg);
        Rg.W += Y;
    }

    inline void MC_Ix(WordBytes &K, WordBytes &Rg) {
        K.W = (uint8_t)(read(PC.W++)+X);
        Rg.B.l = read(K.W++);
        Rg.B.h = read(K.W);
    }

    inline void MC_Iy(WordBytes &K, WordBytes &Rg) {
        K.W = read(PC.W++);
        Rg.B.l = read(K.W++);
        Rg.B.h = read(K.W);
        Rg.W += Y;
    }

    inline void MC_Izp(WordBytes &K, WordBytes &Rg) {
        K.W = read(PC.W++);
        Rg.B.l = read(K.W++);
        Rg.B.h = read(K.W);
    }

    inline void MR_Ab(WordBytes &J, uint8_t &Rg) {
        MC_Ab(J);
        Rg = read(J.W);
    }

    inline void MR_Im(uint8_t &Rg) {
        Rg = read(PC.W++);
    }

    inline void MR_Zp(WordBytes &J, uint8_t &Rg) {
        MC_Zp(J);
        Rg = read(J.W);
    }

    inline void MR_Zx(WordBytes &J, uint8_t &Rg) {
        MC_Zx(J);
        Rg = read(J.W);
    }

    inline void MR_Zy(WordBytes &J, uint8_t &Rg) {
        MC_Zy(J);
        Rg = read(J.W);
    }

    inline void MR_Ax(WordBytes &J, uint8_t &Rg) {
        MC_Ax(J);
        Rg = read(J.W);
    }

    inline void MR_Ay(WordBytes &J, uint8_t &Rg) {
        MC_Ay(J);
        Rg = read(J.W);
    }

    inline void MR_Ix(WordBytes &J, WordBytes &K, uint8_t &Rg) {
        MC_Ix(K, J);
        Rg = read(J.W);
    }

    inline void MR_Iy(WordBytes &J, WordBytes &K, uint8_t &Rg) {
        MC_Iy(K, J);
        Rg = read(J.W);
    }

    inline void MR_Izp(WordBytes &J, WordBytes &K, uint8_t &Rg) {
        MC_Izp(K, J);
        Rg = read(J.W);
    }

    inline void MW_Ab(WordBytes &J, uint8_t Rg) {
        MC_Ab(J);
        write(J.W, Rg);
    }

    inline void MW_Zp(WordBytes &J, uint8_t Rg) {
        MC_Zp(J);
        write(J.W, Rg);
    }

    inline void MW_Zx(WordBytes &J, uint8_t Rg) {
        MC_Zx(J);
        write(J.W, Rg);
    }

    inline void MW_Zy(WordBytes &J, uint8_t Rg) {
        MC_Zy(J);
        write(J.W, Rg);
    }

    inline void MW_Ax(WordBytes &J, uint8_t Rg) {
        MC_Ax(J);
        write(J.W, Rg);
    }

    inline void MW_Ay(WordBytes &J, uint8_t Rg) {
        MC_Ay(J);
        write(J.W, Rg);
    }

    inline void MW_Ix(WordBytes &J, WordBytes &K, uint8_t Rg) {
        MC_Ix(K, J);
        write(J.W, Rg);
    }

    inline void MW_Iy(WordBytes &J, WordBytes &K, uint8_t Rg) {
        MC_Iy(K, J);
        write(J.W, Rg);
    }

    inline void MW_Izp(WordBytes &J, WordBytes &K, uint8_t Rg) {
        MC_Izp(K, J);
        write(J.W, Rg);
    }

    void M_SBC(uint8_t Rg);

    inline void M_PUSH(uint8_t Rg) {
        write(0x0100|S, Rg);
        S--;
    }

    inline void M_POP(uint8_t &Rg) {
        S++;
        Rg = read(0x0100|S);
    }

    inline void M_JR() {
        PC.W += (int8_t)read(PC.W) + 1;
        count--;
    }

    inline void M_AND(uint8_t Rg) {
        A &= Rg;
        M_FL(A);
    }

    inline void M_ORA(uint8_t Rg) {
        A |= Rg;
        M_FL(A);
    }

    inline void M_EOR(uint8_t Rg) {
        A ^= Rg;
        M_FL(A);
    }

    inline void M_INC(uint8_t &Rg) {
        Rg++;
        M_FL(Rg);
    }

    inline void M_DEC(uint8_t &Rg) {
        Rg--;
        M_FL(Rg);
    }

    inline void M_ASL(uint8_t &Rg) {
        P &= ~FLAG::C;
        P |= Rg >> 7;
        Rg <<= 1;
        M_FL(Rg);
    }

    inline void M_LSR(uint8_t &Rg) {
        P &= ~FLAG::C;
        P |= Rg & FLAG::C;
        Rg >>= 1;
        M_FL(Rg);
    }

    inline void M_ROL(WordBytes &K, uint8_t &Rg) {
        K.B.l = (Rg<<1)|(P&FLAG::C);
        P &= ~FLAG::C;
        P |= Rg>>7;
        Rg = K.B.l;
        M_FL(Rg);
    }

    inline void M_ROR(WordBytes &K, uint8_t &Rg) {
        K.B.l = (Rg>>1)|(P<<7);
        P &=~ FLAG::C;
        P |= Rg&FLAG::C;
        Rg = K.B.l;
        M_FL(Rg);
    }

    inline void M_BIT(uint8_t Rg) {
        P &= ~(FLAG::N|FLAG::V|FLAG::Z);
        P |= (Rg & (FLAG::N|FLAG::V)) | (Rg & A? 0 : FLAG::Z);
    }

    void M_CMP(WordBytes &K, uint8_t Rg1, uint8_t Rg2);

    inline void M_TSB(uint8_t &Data) {
        P = (P & ~FLAG::Z) | ((Data & A) == 0 ? FLAG::Z : 0);
        Data |= A;
    }

    inline void M_TRB(uint8_t &Data) {
        P = (P & ~FLAG::Z) | ((Data & A) == 0 ? FLAG::Z : 0);
        Data &= ~A;
    }

    inline void MM_Ab(uint8_t &I, WordBytes &J, std::function<void(uint8_t &)> cmd) {
        MC_Ab(J);
        I = read(J.W);
        cmd(I);
        write(J.W, I);
    }

    inline void MM_Ab(uint8_t &I, WordBytes &J, WordBytes &K, std::function<void(WordBytes &, uint8_t &)> cmd) {
        MC_Ab(J);
        I = read(J.W);
        cmd(K, I);
        write(J.W, I);
    }

    inline void MM_Zp(uint8_t &I, WordBytes &J, std::function<void(uint8_t &)> cmd) {
        MC_Zp(J);
        I = read(J.W);
        cmd(I);
        write(J.W, I);
    }

    inline void MM_Zp(uint8_t &I, WordBytes &J, WordBytes &K, std::function<void(WordBytes &, uint8_t &)> cmd) {
        MC_Zp(J);
        I = read(J.W);
        cmd(K, I);
        write(J.W, I);
    }

    inline void MM_Zx(uint8_t &I, WordBytes &J, std::function<void(uint8_t &)> cmd) {
        MC_Zx(J);
        I = read(J.W);
        cmd(I);
        write(J.W, I);
    }

    inline void MM_Zx(uint8_t &I, WordBytes &J, WordBytes &K, std::function<void(WordBytes &, uint8_t &)> cmd) {
        MC_Zx(J);
        I = read(J.W);
        cmd(K, I);
        write(J.W, I);
    }

    inline void MM_Ax(uint8_t &I, WordBytes &J, std::function<void(uint8_t &)> cmd) {
        MC_Ax(J);
        I = read(J.W);
        cmd(I);
        write(J.W, I);
    }

    inline void MM_Ax(uint8_t &I, WordBytes &J, WordBytes &K, std::function<void(WordBytes &, uint8_t &)> cmd) {
        MC_Ax(J);
        I = read(J.W);
        cmd(K, I);
        write(J.W, I);
    }

    void SBCInstruction(uint8_t val);

public:
    CPU(std::function<uint8_t(uint16_t)> read, std::function<void(uint16_t, uint8_t)> write, std::function<uint8_t()> loop);


    void setPeriod(int32_t new_period) {
        period = new_period;
    }

    void reset();
    void run();
    void interupt(INT type);

    ~CPU();
};

#endif //CPU_H
