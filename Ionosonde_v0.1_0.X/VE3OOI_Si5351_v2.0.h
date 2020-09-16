#ifndef _Si5351_H_
#define _Si5351_H_



// Flags

#define SI5351_ADDRESS (0x60) 


void setupSi5351 (int correction);
void DisableSi5351Clock (unsigned char clk);
void EnableSi5351Clock (unsigned char clk);
void PowerDownSi5351Clock (unsigned char clk);

void SetFrequency (unsigned char clk, char pll, unsigned long freq);
void ProgramSi5351PLL (char pll, unsigned long pllfreq);
void ProgramSi5351MSN (char clk, char pll, unsigned long pllfreq, unsigned long freq);
unsigned long validateLowFrequency (unsigned long freq);
void InvertClk (unsigned char clk, unsigned char invert);
void UpdateClkControlRegister (unsigned char clk);

void Si5351WriteRegister (unsigned char reg, unsigned char value);
void Si5351RepeatedWriteRegister(unsigned char  addr, unsigned char  bytes, unsigned char *data);
unsigned char Si5351ReadRegister (unsigned char reg);



#define SI_CLK0  0
#define SI_CLK1  1
#define SI_CLK2  2

#define SIREG_0_DEVICE_STAT        0
#define SIREG_1_INT_STAT_STICKY    1
#define SIREG_2_INT_STAT_MASK      2
#define SIREG_3_OUTPUT_ENABLE_CTL  3
#define SIREG_9_OEB_PIN_ENABLE_CTL 9
#define SIREG_15_PLL_INPUT_SRC     15
#define SIREG_16_CLK0_CTL          16
#define SIREG_17_CLK1_CTL          17
#define SIREG_18_CLK2_CTL          18

#define SI_MSREGS                  8
#define SIREG_26_MSNA_1            26             // Base register address for PLL A
#define SIREG_27_MSNA_2            27
#define SIREG_28_MSNA_3            28
#define SIREG_29_MSNA_4            29
#define SIREG_30_MSNA_5            30
#define SIREG_31_MSNA_6            31
#define SIREG_32_MSNA_7            32
#define SIREG_33_MSNA_8            33

#define SIREG_34_MSNB_1            34               // Base register address for PLL b
#define SIREG_35_MSNB_2            35
#define SIREG_36_MSNB_3            36
#define SIREG_37_MSNB_4            37
#define SIREG_38_MSNB_5            38
#define SIREG_39_MSNB_6            39
#define SIREG_40_MSNB_7            40
#define SIREG_41_MSNB_8            41

#define SIREG_42_MSYN0_1           42
#define SIREG_43_MSYN0_2           43
#define SIREG_44_MSYN0_3           44
#define SIREG_45_MSYN0_4           45
#define SIREG_46_MSYN0_5           46
#define SIREG_47_MSYN0_6           47
#define SIREG_48_MSYN0_7           48
#define SIREG_49_MSYN0_8           49

#define SIREG_50_MSYN1_1           50
#define SIREG_51_MSYN1_2           51
#define SIREG_52_MSYN1_3           52
#define SIREG_53_MSYN1_4           53
#define SIREG_54_MSYN1_5           54
#define SIREG_55_MSYN1_6           55
#define SIREG_56_MSYN1_7           56
#define SIREG_57_MSYN1_8           57

#define SIREG_58_MSYN2_1           58
#define SIREG_59_MSYN2_2           59
#define SIREG_60_MSYN2_3           60
#define SIREG_61_MSYN2_4           61
#define SIREG_62_MSYN2_5           62
#define SIREG_63_MSYN2_6           63
#define SIREG_64_MSYN2_7           64
#define SIREG_65_MSYN2_8           65

#define SIREG_092_CLOCK_6_7_OUTPUT_DIVIDER  92
#define SIREG_165_CLK0_PHASE_OFFSET         165
#define SIREG_166_CLK1_PHASE_OFFSET         166
#define SIREG_167_CLK2_PHASE_OFFSET         167
#define SIREG_177_PLL_RESET                 177
#define SIREG_183_CRY_LOAD_CAP         183

#define SI_ENABLE_CLK0     0b00000001
#define SI_ENABLE_CLK1     0b00000010
#define SI_ENABLE_CLK2     0b00000100

#define SI_PLL_A        'A'
#define SI_PLL_B        'B'
#define SI_XTAL         'X'
#define SI_MSCLK0           '0'
#define SI_MSCLK1           '1'
#define SI_MSCLK2           '2'

#define SI_CRY_LOAD_6PF     0x52
#define SI_CRY_LOAD_8PF     0x92
#define SI_CRY_LOAD_10PF    0x52

#define SI_CRY_FREQ_25MHZ     25000000
#define SI_CRY_FREQ_27MHZ     27000000

#define SI_MAX_PLL_FREQ     900000000
#define SI_MIN_PLL_FREQ     600000000

#define SI_MIN_MSRATIO4_FREQ 150000000
#define SI_MAX_MSRATIO4_FREQ 200000000

#define SI_MIN_MSRATIO6_FREQ 110000000
#define SI_MAX_MSRATIO6_FREQ 150000000

#define SI_MIN_MSRATIO8_FREQ  75000000
#define SI_MAX_MSRATIO8_FREQ 110000000



#define SI_MAX_OUT_FREQ     200000000
#define SI_MIN_OUT_FREQ     2500

#define SI_MAX_MS_FREQ      150000000
#define SI_MSYN_DIV_4     4
#define SI_MSYN_DIV_6     6
#define SI_MSYN_DIV_8     8

#define SI_MAX_DIVIDER          1048575   // 20 bits of denomintor and 2^20 = 1048576, which is 0 to 1048575
#define SI_MAXIMUM_DENOMINATOR  1048575   // 20 bits of denomintor and 2^20 = 1048576, which is 0 to 1048575


#define SI_PHASE_CONSTANT     11430      // This is 127 x 360 / 4 (127 is the max value allowed in phase register)

#define SI_R_DIV_1              0
#define SI_R_DIV_2              1
#define SI_R_DIV_4              2
#define SI_R_DIV_8              3
#define SI_R_DIV_16             4
#define SI_R_DIV_32             5
#define SI_R_DIV_64             6
#define SI_R_DIV_128            7

#define SI_CLK_OFF    0b10000000
#define SI_CLK_MS_INT   0b01000000
#define SI_CLK_SRC_PLLB   0b00100000
#define SI_CLK_SRC_MS   0b00001100 
#define SI_CLK_2MA    0b00000000
#define SI_CLK_4MA    0b00000001
#define SI_CLK_6MA    0b00000010
#define SI_CLK_8MA    0b00000011

#define SI_CLK_INVERT   0b00010000

#define SI_CLK_CLR_DRIVE        0b11111100
  
#define SI_PLLA_RESET   0b10000000
#define SI_PLLB_RESET   0b00100000

#endif // _Si5351_H_

