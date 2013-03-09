

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


// EVAL.C - Expression analyzer for 4dos batch language
//  Copyright (c) 1991 - 1999 Rex C. Conn  All rights reserved
//  This routine supports the standard arithmetic functions: +, -, *, /,
//    %, and the unary operators + and -
//  EVAL supports BCD numbers with 20 integer and 10 decimal places

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "4all.h"


#define SIZE_INTEGER 20
#define SIZE_DECIMAL 10
#define SIZE_MANTISSA 30

#define AND 0
#define OR 1
#define XOR 2
#define ADD 3
#define SUBTRACT 4
#define MULTIPLY 5
#define EXPONENTIATE 6
#define DIVIDE 7
#define INTEGER_DIVIDE 8
#define MODULO 9
#define OPEN_PAREN 10
#define CLOSE_PAREN 11

#define MAX_DIGITS 31


// BCD structure
typedef struct {
	unsigned char sign;
	unsigned char integer[SIZE_INTEGER];
	unsigned char decimal[SIZE_DECIMAL+2];
} BCD;

static void _near level0(BCD *);
static void _near level1(BCD *);
static void _near level2(BCD *);
static void _near level3(BCD *);
static void _near level4(BCD *);
static void _near level5(BCD *);


extern int _pascal add_bcd(BCD *, BCD *);
extern int _pascal multiply_bcd(BCD *, BCD *);
extern int _pascal divide_bcd(BCD *, BCD *, int);

static char operators[] = "&|^+-**/\\%()";	// valid EVAL operators
static char *line;			// pointer to expression
static char token[MAX_DIGITS+2];	// current token (number or operator)

static char tok_type;			// token type (number or operator)
static char delim_type;			// delimiter type

static jmp_buf env;


#define DELIMITER	1
#define NUMBER		2
#define HEX_NUMBER	3


static char _near is_operator(void);
static void _near get_token(void);

// check for a operator character:  + - * / % ( )
static char _near is_operator(void)
{
	register int i;

	if ( _strnicmp( line, "and", 3 ) == 0 ) {
		delim_type = AND;
		line += 2;
		return operators[AND];
	} else if ( _strnicmp( line, "or", 2 ) == 0 ) {
		delim_type = OR;
		line += 1;
		return operators[OR];
	} else if ( _strnicmp( line, "xor", 3 ) == 0 ) {
		delim_type = XOR;
		line += 2;
		return operators[XOR];
	}

	delim_type = 0;
	for ( i = 0; ( operators[i] != '\0' ); i++ ) {

		if ( *line == operators[i] ) {
			delim_type = (char)i;
			if (( delim_type == MULTIPLY ) && ( line[1] == '*' )) {
				delim_type = EXPONENTIATE;
				line++;
			}
			return ( *line );
		}
	}

	return 0;
}


static void _near get_token( void )
{
	register int i = 0;
	register int decimal_flag = 0;

	tok_type = 0;
	while (( *line == ' ' ) || ( *line == '\t' ))
		line++;

	// get the next token (number or operator)
	for ( ; (( *line != '\0' ) && ( i < MAX_DIGITS )); line++ ) {

		if (( isdigit( *line )) || (( tok_type == HEX_NUMBER ) && ( isxdigit( *line )))) {

			if ( tok_type == 0 )
				tok_type = NUMBER;
			token[i++] = *line;

		} else if ((( *line == 'x' ) || ( *line == 'X' )) && ( _strnicmp( line, "xor", 3 ) != 0 )) {

			tok_type = HEX_NUMBER;
			// skip the 'x'

		} else if ( *line == gaCountryInfo.szDecimal[0] ) {

			// only one '.' allowed!
			if ( decimal_flag++ != 0 ) {
				tok_type = 0;
				return;
			}
			token[i++] = *line;

		} else if (( *line != '.' ) && ( *line != ',' ) && ( *line != gaCountryInfo.szThousandsSeparator[0] ))
			break;
	}

	if (( tok_type == 0 ) && (( token[i++] = is_operator()) != '\0' )) {
		tok_type = DELIMITER;
		line++;
	}

	token[i] = '\0';
}


static void _near arith(int op, register BCD *r, register BCD *h)
{
	int i, rval = 0;
	BCD temp;

	// bitwise operator?
	if ( op < ADD ) {

		long ulVal1, ulVal2;
		char szToken[32], *ptr;

		// remove the decimal portions
		memset( r->decimal, '\0', SIZE_DECIMAL );
		memset( h->decimal, '\0', SIZE_DECIMAL );

		// convert numbers from BCD format back to ASCII format
		for ( i = 0; ( i < SIZE_INTEGER ); i++ )
			h->integer[i] += '0';
		h->integer[i] = '\0';
		ulVal2 = atol(&(h->sign));

		for ( i = 0; ( i < SIZE_INTEGER ); i++ )
			r->integer[i] += '0';
		r->integer[i] = '\0';
		ulVal1 = atol(&(r->sign));

		if ( op == AND )
			ulVal1 &= ulVal2;
		else if ( op == OR )
			ulVal1 |= ulVal2;
		else
			ulVal1 ^= ulVal2;

		sprintf( szToken, FMT_LONG, ulVal1 );

		memset( r, '\0', SIZE_MANTISSA+2 );

		// put result back into *r
		ptr = strend( szToken );

		// stuff the token into the BCD structure
		if (( i = (int)( SIZE_INTEGER - ( ptr - szToken ))) < 0 )
			longjmp( env, ERROR_4DOS_OVERFLOW );

		ptr = szToken;
		if ( *ptr == '-' ) {
			r->sign = '-';
			ptr++;
		} else
			r->sign = '+';

		for ( ; (( *ptr != '\0' ) && ( i < SIZE_INTEGER )); ptr++, i++ )
			r->integer[i] = (char)( *ptr - '0' );

	} else if ( op < MULTIPLY ) {

		// reverse the second argument's sign if subtracting
		if ( op == SUBTRACT )
			h->sign = (char)(( h->sign == '-' ) ? '+' : '-' );

add_values:
		// if first op - & second +, swap & subtract
		if ((r->sign != h->sign) && (r->sign == '-')) {
			memmove(&temp,r,sizeof(BCD));
			memmove(r,h,sizeof(BCD));
			memmove(h,&temp,sizeof(BCD));
		}

		rval = add_bcd(r,h);

	} else if (op == MULTIPLY)
		rval = multiply_bcd(r,h);

	else if (op == DIVIDE)
		rval = divide_bcd(r,h,0);

	else if (op == EXPONENTIATE) {

		// convert number from BCD format back to ASCII format
		for (op = 0; (op < SIZE_INTEGER); op++)
			h->integer[op] += '0';
		h->integer[op] = '\0';
		op = atoi(&(h->sign));

		if (op < 0) {
			// we don't allow negative exponents!
			longjmp(env,ERROR_4DOS_BAD_SYNTAX);
		} else if (op == 0) {
			// zero == 1!
			for (op = 0; (op < SIZE_MANTISSA); op++)
				r->integer[op] = '\0';
			r->decimal[-1] = 1;
		} else {
			memmove( &temp, r, sizeof(BCD) );
			while ((--op > 0) && (rval == 0))
				rval = multiply_bcd( r, &temp );
		}

	} else if (op == INTEGER_DIVIDE) {

		rval = divide_bcd(r,h,0);
		// remove the decimal portion
		memset(r->decimal,'\0',SIZE_DECIMAL);

	} else if (op == MODULO) {

		// remove the decimal portion
		memset( r->decimal, '\0', SIZE_DECIMAL );
		memset( h->decimal, '\0', SIZE_DECIMAL );

		// save the divisor for kludge below
		memmove( &temp, h, sizeof(BCD) );

		rval = divide_bcd( r, h, 1 );

		// remainder was saved in "h"
		memmove( r, h, sizeof(BCD) );

		// kludge to make the mathematicians happy!  if the signs
		//   don't match, modulo = remainder + divisor
		if ( r->sign != temp.sign ) {
			memmove( h, &temp, sizeof(BCD) );
			goto add_values;
		}
	}

	if ( rval == -1 )
		longjmp( env, ERROR_4DOS_OVERFLOW );
}


// do AND, OR, XOR
static void _near level0(register BCD *result)
{
	int op;
	BCD hold;

	level1(result);

	while (( tok_type == DELIMITER ) && ( delim_type < ADD )) {

		op = delim_type;

		// initialize the BCD structure
		memset(&hold,'\0',SIZE_MANTISSA+2);
		get_token();

		level1(&hold);
		arith(op,result,&hold);
	}
}


// do addition & subtraction
static void _near level1(register BCD *result)
{
	int op;
	BCD hold;

	level2(result);

	while ((tok_type == DELIMITER) && ((delim_type == ADD) || (delim_type == SUBTRACT))) {

		op = delim_type;

		// initialize the BCD structure
		memset(&hold,'\0',SIZE_MANTISSA+2);
		get_token();

		level2(&hold);
		arith(op,result,&hold);
	}
}


// do multiplication & division & modulo
static void _near level2(register BCD *result)
{
	int op;
	BCD hold;

	level3(result);

	while ((tok_type == DELIMITER) && ((delim_type == MULTIPLY) || (delim_type == EXPONENTIATE) || (delim_type == DIVIDE) || (delim_type == INTEGER_DIVIDE) || (delim_type == MODULO))) {

		op = delim_type;

		// initialize the BCD structure
		memset(&hold,'\0',SIZE_MANTISSA+2);
		get_token();

		level3(&hold);
		arith(op,result,&hold);
	}
}


// do exponentiation
static void _near level3( register BCD *result )
{
	int op;
	BCD hold;

	level4(result);

	while ((tok_type == DELIMITER) && (delim_type == EXPONENTIATE)) {

		op = delim_type;

		// initialize the BCD structure
		memset( &hold, '\0', SIZE_MANTISSA+2 );
		get_token();

		level4( &hold );
		arith( op, result, &hold );
	}
}


// process unary + & -
static void _near level4( register BCD *result )
{
	register int is_unary = -1;

	if (( tok_type == DELIMITER ) && (( delim_type == ADD ) || ( delim_type == SUBTRACT ))) {
		is_unary = delim_type;
		get_token();
	}

	level5( result );

	if ( is_unary == ADD )
		result->sign = '+';
	else if ( is_unary == SUBTRACT )
		result->sign = (char)((result->sign == '-') ? '+' : '-');
}


// process parentheses
static void _near level5( BCD *result )
{
	register int i;
	register char *ptr;

	// is it a parenthesis?
	if (( tok_type == DELIMITER ) && ( delim_type == OPEN_PAREN )) {

		get_token();
		level0(result);
		if ( delim_type != CLOSE_PAREN )
			longjmp( env, ERROR_4DOS_UNBALANCED_PARENS );

		get_token();

	} else if ( tok_type == NUMBER ) {

		// initialize the BCD structure
		memset( result, '\0', SIZE_MANTISSA+2 );

		result->sign = '+';

		if (( ptr = strchr( token, gaCountryInfo.szDecimal[0] )) != NULL )
			strcpy( ptr, ptr + 1 );
		else
			ptr = strend( token );

		// stuff the token into the BCD structure
		if (( i = (int)( SIZE_INTEGER - ( ptr - token ))) < 0 )
			longjmp(env,ERROR_4DOS_OVERFLOW);

		for ( ptr = token; (( *ptr != '\0' ) && ( i < SIZE_MANTISSA )); ptr++, i++ )
			result->integer[i] = (char)( *ptr - '0' );

		get_token();

	} else if ( tok_type == HEX_NUMBER ) {

		unsigned long ulHex;

		// _fmtin (in inout.asm) doesn't like lower case!
		strupr( token );

		sscanf( token, "%lx", &ulHex );
		sprintf( token, "%lu", ulHex );

		result->sign = '+';

		// stuff the token into the BCD structure
		ptr = strend( token );
		if (( i = (int)( SIZE_INTEGER - ( ptr - token ))) < 0 )
			longjmp( env, ERROR_4DOS_OVERFLOW );

		for ( ptr = token ; (( *ptr != '\0' ) && ( i < SIZE_INTEGER )); ptr++, i++ )
			result->integer[i] = (char)( *ptr - '0' );

		get_token();
	}

	if (( tok_type != DELIMITER ) && ( tok_type != NUMBER ) && ( *line ))
		longjmp( env, ERROR_4DOS_BAD_SYNTAX );
}


// set the default EVAL decimal precision
void SetEvalPrecision( register char *ptr, unsigned int *uMin, unsigned int *uMax )
{
	if ( isdigit( *ptr ))
		sscanf( ptr, "%u%*c%u", uMin, uMax );
	else if (( *ptr == '.' ) || ( *ptr == ',' ))
		*uMax = atoi( ptr+1 );

	if ( *uMax > 10 )
		*uMax = 10;
	if ( *uMin > 10 )
		*uMin = 10;

	if ( *uMax < *uMin )
		*uMax = *uMin;
}


// evaluate the algebraic expression
int _fastcall evaluate( register char *expr )
{
	register int n;
	unsigned int uMax, uMin;
	int rval = 0;
	char *ptr;
	BCD x;

	uMin = gpIniptr->EvalMin;
	uMax = gpIniptr->EvalMax;

	// %@eval[...=n] sets the default minimum decimal precision
	// %@eval[...=.n] sets the default maximum decimal precision
	if (( ptr = strchr( expr, '=' )) != NULL ) {

		*ptr++ = '\0';
		SetEvalPrecision( skipspace( ptr ), &uMin, &uMax );
		if ( *expr == '\0' )
			return 0;
	}

	if (( n = setjmp( env )) >= OFFSET_4DOS_MSG )
		rval = error( n, expr );

	else {

		memset( &x, '\0', SIZE_MANTISSA+2 );
		line = expr;

		get_token();
		if ( token[0] == '\0' )
			longjmp( env, ERROR_4DOS_NO_EXPRESSION );

		level0( &x );

		// round up
		for ( n = SIZE_INTEGER + uMax; ( n > 0 ); n-- ) {

			if ( n >= (int)( SIZE_INTEGER + uMax )) {
				if (x.integer[n] >= 5)
					x.integer[n-1] += 1;
			} else if ( x.integer[n] > 9 ) {
				x.integer[n] = 0;
				x.integer[n-1] += 1;
			}
		}

		// convert number from BCD format back to ASCII format
		for ( n = 0; ( n < SIZE_MANTISSA ); n++ )
			x.integer[n] += '0';
		x.integer[n] = '\0';

		// truncate to maximum precision
		x.decimal[uMax] = '\0';

		// strip trailing 0's
		for ( n = uMax - 1; ( n >= (int)uMin ); n-- ) {
			if ( x.decimal[n] == '0' )
				x.decimal[n] = '\0';
			else
				break;
		}

		if ( x.decimal[0] )
			strins( x.decimal, gaCountryInfo.szDecimal );

		// check for a leading '-' and strip leading 0's
		sscanf( x.integer, "%*19[0]%s", expr );

		if (( x.sign == '-' ) && ( _stricmp( expr, "0" ) != 0 ))
			strins( expr, "-" );
	}

	return rval;
}

