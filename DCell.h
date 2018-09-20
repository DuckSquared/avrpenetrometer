/*
 * DCell.h
 *
 * Created: 19/06/2018 12:34:56
 *  Author: Jake Bird
 */ 

#ifndef DCELL_H_
#define DCELL_H_

#define FLAG_TEMPUR 4
#define FLAG_TEMPOR 8
#define FLAG_ECOMUR 16
#define FLAG_ECOMOR 32
#define FLAG_CRAWUR 64
#define FLAG_CRAWOR 128
#define FLAG_SYSUR 256
#define FLAG_SYSOR 512
#define FLAG_LCINTEG 2048
#define FLAG_WDRST 4096
#define FLAG_BRWNOUT 16384
#define FLAG_REBOOT 32768

#define FLAG_TEMPUR_BIT 2
#define FLAG_TEMPOR_BIT 3
#define FLAG_ECOMUR_BIT 4
#define FLAG_ECOMOR_BIT 5
#define FLAG_CRAWUR_BIT 6
#define FLAG_CRAWOR_BIT 7
#define FLAG_SYSUR_BIT 8
#define FLAG_SYSOR_BIT 9
#define FLAG_LCINTEG_BIT 11
#define FLAG_WDRST_BIT 12
#define FLAG_BRWNOUT_BIT 14
#define FLAG_REBOOT_BIT 15

#define STAT_SPSTAT 1
#define STAT_IPSTAT 2
#define STAT_TEMPUR 4
#define STAT_TEMPOR 8
#define STAT_ECOMUR 16
#define STAT_ECOMOR 32
#define STAT_CRAWUR 64
#define STAT_CRAWOR 128
#define STAT_SYSUR 256
#define STAT_SYSOR 512
#define STAT_LCINTEG 2048
#define STAT_SCALON 4096
#define STAT_OLDVAL 8192
#define STAT_BRWNOUT 16384
#define STAT_REBOOT 32768

#define STAT_SPSTAT_BIT 0
#define STAT_IPSTAT_BIT 1
#define STAT_TEMPUR_BIT 2
#define STAT_TEMPOR_BIT 3
#define STAT_ECOMUR_BIT 4
#define STAT_ECOMOR_BIT 5
#define STAT_CRAWUR_BIT 6
#define STAT_CRAWOR_BIT 7
#define STAT_SYSUR_BIT 8
#define STAT_SYSOR_BIT 9
#define STAT_LCINTEG_BIT 11
#define STAT_SCALON_BIT 12
#define STAT_OLDVAL_BIT 13
#define STAT_BRWNOUT_BIT 14
#define STAT_REBOOT_BIT 15

#define MD_CMVV 10
#define MD_STAT 12
#define MD_MVV 16
#define MD_SOUT 18
#define MD_SYS 20
#define MD_TEMP 22
#define MD_SRAW 24
#define MD_CELL 26
#define MD_FLAG 28
#define MD_CRAW 30
#define MD_ELEC 32
#define MD_SZ 44
#define MD_SYSN 46
#define MD_PEAK 48
#define MD_TROF 50
#define MD_CFCT 52
#define MD_VER 60
#define MD_SERL 62
#define MD_SERH 64
#define MD_STN 66
#define MD_BAUD 68
#define MD_OPCL 70
#define MD_RATE 72
#define MD_DP 74
#define MD_DPB 76
#define MD_NMVV 78
#define MD_CGAI 80
#define MD_COFS 82
#define MD_CMIN 88
#define MD_CMAX 90
#define MD_CLN 100
#define MD_CLX1 102
#define MD_CLX2 104
#define MD_CLX3 106
#define MD_CLX4 108
#define MD_CLX5 110
#define MD_CLX6 112
#define MD_CLX7 114
#define MD_CLK1 122
#define MD_CLK2 124
#define MD_CLK3 126
#define MD_CLK4 128
#define MD_CLK5 130
#define MD_CLK6 132
#define MD_CLK7 134
#define MD_SGAI 140
#define MD_SOFS 142
#define MD_SMIN 148
#define MD_SMAX 150
#define MD_USR1 162
#define MD_USR2 164
#define MD_USR3 166
#define MD_USR4 168
#define MD_USR5 170
#define MD_USR6 172
#define MD_USR7 174
#define MD_USR8 176
#define MD_USR9 178
#define MD_FFLV 184
#define MD_FFST 186
#define MD_RST 200
#define MD_SNAP 206
#define MD_RSPT 208
#define MD_SCON 210
#define MD_SCOF 212
#define MD_OPON 214
#define MD_OPOF 216
#define MD_CTN 220
#define MD_CT1 222
#define MD_CT2 224
#define MD_CT3 226
#define MD_CT4 228
#define MD_CT5 230
#define MD_CTG1 232
#define MD_CTG2 234
#define MD_CTG3 236
#define MD_CTG4 238
#define MD_CTG5 240
#define MD_CTO1 242
#define MD_CTO2 244
#define MD_CTO3 246
#define MD_CTO4 248
#define MD_CTO5 250
#endif /* DCELL_H_ */