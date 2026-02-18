#ifndef MASKED_PCTX_INL_H
#define MASKED_PCTX_INL_H




#endif // MASKED_PCTX_INL_H

/*
 * USAGE:
 * Process Context Register:
 *
 ASN[46:39] // Address Space Number
 ASTRR[12:9] // AST Request Register bits:U[12],S[11],E[10],K[9]
 ASTER[8:5] // AST Enable REgister bits: U[8],S[7],E[6],K[5]
 FPE[2]     // Floating Point Enable - clear FP instruction generate FEN exceptions. Reset by Hardware Reset.
 PPCE[1]    // Process performaance counting enable
(use PCTR0, and PCTR1

*/
