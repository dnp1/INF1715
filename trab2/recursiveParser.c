#include <stdlib.h>
#include <stdio.h>
#include "../trab1/token.h"
#include "../trab1/tokenList.h"
#include "../trab3/AbstractSyntaxTree.h"
#include "recursiveParser.h"

//global_variable is more intuitive;
static error_flag = 0;

static callbackOnDerivation actinOnRules = NULL;

static int callOnConsume(int nodeType, int line, Token t) {
	if( actinOnRules != NULL && error_flag == 0) {
		return actinOnRules(nodeType, line, t);
	} else
		return -1;
}

static TokenList expression(TokenList tl);
static TokenList expressionList(TokenList tl);
static TokenList commandAttrOrCall( TokenList tl );
static TokenList block( TokenList tl );
static TokenList command( TokenList tl );
static TokenList arrayAccess( TokenList tl );
static TokenList call( TokenList tl );
static TokenList U(TokenList tl);



// -- Auxiliar Functions:
static  int verifyCurrentToken(TokenList tl, TokenKind tk) {
	if (tl == NULL)
		return 0;
	Token t = tokenListGetCurrentToken( tl );
	return tokenGetKind( t ) == tk;
}

static void printError(int line, char *expected, char *got){
	error_flag++;
	printf("Error at Line %d: The expected was %s but got %s", line, expected, got);	

}

static  TokenList processTerminal( TokenList tl, TokenKind tk ) {
	Token t = tokenListGetCurrentToken(tl);
	if(!error_flag && tl == NULL) {
		error_flag++;
		printf("Error: Unexpected End of File. The expected was <%s>\n", tokenKindToString(tk) );
		
	}
	if(tl == NULL) {
		error_flag++;
		return NULL;
	}	
	if( t!= NULL && tokenGetKind(t) == tk ) {
		//printf("Token: %s, Line: %d\n", tokenToString(t), tokenGetLine(t));
		return tokenListNext( tl );
	}
	else {
		error_flag++;
		printError( tokenGetLine(t), tokenKindToString(tk),  tokenToString(t));
		return NULL;
	}
}


// end_auxiliar.


/*
type ->  'CHAR'
type -> 'STRING'
type -> 'INT'
type -> 'BOOL'
type -> '[' ']' type
*/
static TokenList type( TokenList tl ) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Type, tokenGetLine(t), NULL);
	switch( tokenGetKind(t) ) {
		case INT: case STRING: case CHAR: case BOOL:
			tl = processTerminal(tl, tokenGetKind(t));
			break;
		case OP_BRACKET:
			while( verifyCurrentToken( tl, OP_BRACKET ) ) {
				tl = processTerminal( tl, OP_BRACKET );
				tl = processTerminal( tl, CL_BRACKET );	
			}
			tl = type( tl ) ;
			break;
		default:
			printError(tokenGetLine(t), "a type", tokenToString(t));
			tl = NULL;
			break;
	}
	return tl;
}

/*
declGlobalVar -> 'ID' ':' type 'NL'
*/
static TokenList declGlobalVar(TokenList tl) {	
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_DeclGlobalVar, tokenGetLine(t), NULL);
	tl = processTerminal( tl, IDENTIFIER );
	tl = processTerminal( tl, COLON );
	tl = type(tl);
	return processTerminal( tl, NL );
}
/*
param -> 'ID' ':' type
*/
static TokenList param(TokenList tl) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Param, tokenGetLine(t), NULL);
	tl = processTerminal(tl, IDENTIFIER);
	tl = processTerminal(tl, COLON);
	return type(tl);
}
/*
params -> param { ','' param }
*/
static TokenList params(TokenList tl) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Params, tokenGetLine(t), NULL);
	tl = param(tl);
	//pay attention, loop
	while( verifyCurrentToken(tl, COMMA) ) {
		tl = processTerminal(tl, COMMA);
		tl = param(tl);
	}
	return tl;
}

/*
declOrCommand -> ':' type 'NL'
declOrCommand -> ':' type 'NL' 'ID' declOrCommand
declOrCommand ->  commandAttrOrCall 'NL'
*/
static  TokenList declOrCommand( TokenList tl ) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_DeclOrCommand, tokenGetLine(t), NULL);
	if( verifyCurrentToken( tl, COLON ) )	{
		tl = processTerminal( tl, COLON );
		tl = type( tl );
		tl = processTerminal( tl, NL );
		if( verifyCurrentToken( tl, IDENTIFIER ) ) {		
			tl = processTerminal( tl, IDENTIFIER );
			tl = declOrCommand( tl );
		}	
		return tl;
	}
	else {
		tl = commandAttrOrCall( tl );	
		return processTerminal( tl, NL );
	}
	return tl;
}

/*
new -> 'new' '[' expression ']' type
*/
static TokenList new(TokenList tl) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_New, tokenGetLine(t), NULL);
	tl = processTerminal(tl, NEW);
	tl = processTerminal(tl, OP_BRACKET);
	tl = expression(tl);
	tl = processTerminal(tl, CL_BRACKET);
	tl = type(tl);
	return tl;
}

/*
F -> '(' expression ')' -- it isn't very clear at code, because I didn't use recursion.
F -> - F
F -> NOT F
F -> 'BOOL_VAL'
F -> 'INT_VAL'
F -> 'STRING_VAL'
F -> varOrCall
F-> new
*/
static  TokenList F(TokenList tl) {
	Token t;
	t = tokenListGetCurrentToken( tl );	
	switch( tokenGetKind(t) ) {
		case OP_PARENTHESIS:
			tl = processTerminal( tl, OP_PARENTHESIS );
			tl = expression(tl);
			tl = processTerminal(tl, CL_PARENTHESIS);
			break;
		case BOOL_VAL:
			tl = processTerminal(tl, BOOL_VAL);
			break;
		case INT_VAL:
			tl = processTerminal(tl, INT_VAL);
			break;
		case STRING_VAL:
			tl = processTerminal(tl, STRING_VAL);
			break;
		case IDENTIFIER:
			tl = processTerminal(tl, IDENTIFIER);
			if ( verifyCurrentToken( tl, OP_PARENTHESIS) ) {
				tl = call(tl);
			}
			else {
				tl = arrayAccess(tl);
			}
			break;
		case NEW:
			tl = new(tl);
			break;
		default:
			error_flag++;
			printf( "Line %d.\nAn expression was expected but got %s.\n", tokenGetLine(t),  tokenToString(t) );
			tl = NULL;
			break;
	}
		
	return tl;
}


/*
U-> '-' F
U-> 'not' F
U-> F 
*/
static TokenList U( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	switch ( tokenGetKind(t) ) {
		case MINUS: case NOT:
			callOnConsume(AST_DeclOrCommand, 1, NULL);
			tl = processTerminal( tl, tokenGetKind( t ) );
			tl = U( tl );
			break;
		default:
			tl = F(tl);
			break;
	}
	return tl;
}


/*
T -> U
T -> U '*' E
T -> U '/' E
First(U) u First(F)
*/
static TokenList T(TokenList tl) {
	Token t;
	tl = U(tl);
	t = tokenListGetCurrentToken(tl);
	switch( tokenGetKind(t) ) {
		case MUL:case DIV:
			tl = processTerminal( tl, tokenGetKind( t ) );
			tl = T( tl );	
			break;
		default:
			 break;
 	}
	return tl;
}


/*
E -> T
E -> T '+' E
E -> T '-' E
-- First(E) == First(U) u First(F)
*/
static TokenList E( TokenList tl) {
	Token t;
	tl = T( tl );
	t = tokenListGetCurrentToken(tl);	
	switch( tokenGetKind(t) ) {
		case PLUS: case MINUS:
			tl = processTerminal( tl, tokenGetKind( t ) );
			tl = E( tl );
		default: 
			break;
	}
	return tl;
}

/*
C -> E 
C -> E '>' C
C -> E '>=' C
C -> E '<' C
C -> E '<=' C
C -> E '=' C
C -> E '<>' C
*/

static TokenList C( TokenList tl) {
	Token t;
	tl = E( tl );
	t = tokenListGetCurrentToken(tl);
	switch( tokenGetKind(t) ) {
		case GREATER:
		case GREATER_EQUAL:
		case LESS:
		case LESS_EQUAL:
		case EQUAL:
		case DIFFERENT:
			tl = processTerminal( tl, tokenGetKind(t) );
			tl = C( tl );
			break;
		default:
			break;
	}
	return tl;
}
	
/*
expression -> C 'AND' expression
expression -> C 'OR'  expression
expression -> C*/

static TokenList expression( TokenList tl ) {
	Token t;
	tl = C( tl );
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Expression, tokenGetLine(t), NULL);
	switch( tokenGetKind(t) ) {
		case AND:
		case OR:
			tl = processTerminal( tl, tokenGetKind(t) );
			tl = expression( tl );
			break;
		default:
			break;
	}
	return tl;
}

/*
expressionList -> expression { ',' expression }
*/
static TokenList expressionList( TokenList tl ) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_ExpressionList, tokenGetLine(t), NULL);
	tl = expression( tl );
	while(verifyCurrentToken(tl, COMMA)) {
		tl = processTerminal(tl, COMMA);
		tl = expression(tl);
	}
	return tl;
}

/*
call -> 	'(' [expressionList] ')'
*/
static TokenList call( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Call, tokenGetLine(t), NULL);
	tl = processTerminal(tl, OP_PARENTHESIS );
	t = tokenListGetCurrentToken( tl );
	switch ( tokenGetKind( t )) {
		case IDENTIFIER:
		case INT_VAL:
		case STRING_VAL:
		case BOOL_VAL:
		case NEW:
		case OP_PARENTHESIS:
		case NOT:
		case MINUS:
			tl = expressionList( tl );
			break;
		default: 
			break;
	}
	
	tl = processTerminal( tl, CL_PARENTHESIS );
	return tl;
}

/*
attr -> 	arrayAccess '=' expression
*/
static TokenList attr( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Attr, tokenGetLine(t), NULL);
	tl = arrayAccess(tl);
	tl = processTerminal( tl, EQUAL );
	tl = expression( tl );
	return tl;
}

/*
arrayAccess -> 	{ '['  expression']' }
*/
static TokenList arrayAccess( TokenList tl) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_ArrayAccess, tokenGetLine(t), NULL);
	while( verifyCurrentToken( tl, OP_BRACKET ) ) {
		tl = processTerminal( tl, OP_BRACKET );
		tl = expression( tl );
		tl = processTerminal( tl, CL_BRACKET );
	}
	return tl;
}

/*
commandAttrOrCall -> attr | call
*/
static TokenList commandAttrOrCall( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_CommandAttrOrCall, tokenGetLine(t), NULL);
	if( verifyCurrentToken(tl, EQUAL) || verifyCurrentToken(tl, OP_BRACKET) ) {
		tl = attr( tl );	
	}
	else if ( verifyCurrentToken(tl, OP_PARENTHESIS) ) {
		tl = call(tl);
	}
	else {
		error_flag++;
		t = tokenListGetCurrentToken(tl);
		printf("Error at Line %d. Expected a function call or attribution, but got %s\n", tokenGetLine( t ), tokenToString( t ));
		tl = NULL;
	}
	return tl;
}
/*
commandWhile -> 'WHILE' expression'NL'
					block
				'LOOP'
*/
static TokenList commandWhile( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_CommandWhile, tokenGetLine(t), NULL);
	tl = processTerminal(tl, WHILE);
	tl = expression(tl);
	tl = processTerminal(tl, NL);
	tl = block(tl);
	tl = processTerminal(tl, LOOP);
	return tl;
}

/*
commandReturn -> 'RETURN' [ 'expression' ]
*/
static  TokenList commandReturn( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_CommandReturn, tokenGetLine(t), NULL);
	tl = processTerminal( tl, RETURN );
	t = tokenListGetCurrentToken(tl);		
	switch( tokenGetKind( t ) ) {
		case IDENTIFIER:
		case INT_VAL:
		case STRING_VAL:
		case BOOL_VAL:
		case NEW:
		case OP_PARENTHESIS:
		case NOT:
		case MINUS:
			tl = expression( tl );
			break;
		default:
			break;
	}
	return tl;
}


/*
commandIf->'IF' expression 'NL'
				block	
			{ 
			'ELSE' 'IF' expression
				block }
			[
			'ELSE'
				block
			]
			'END'
*/
static TokenList commandIf( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_CommandIf, tokenGetLine(t), NULL);
	tl = processTerminal( tl, IF );
	tl = expression( tl );
	tl =  processTerminal( tl, NL );
	tl = block( tl );

	while( verifyCurrentToken( tl, ELSE ) ) {
		tl = processTerminal( tl, ELSE);
		if(verifyCurrentToken (tl, IF)) {
			tl = processTerminal( tl, IF );
			tl = expression( tl );
			tl = processTerminal( tl, NL);
			tl = block(tl);
		}
		else{
			tl = processTerminal( tl, NL);
			tl = block(tl);
			break;
		}
		
	}
	
	tl = processTerminal(tl, END);
	return tl;

}

/*

command -> 'ID' commandAttrOrCall 'NL'
command -> commandWhile 'NL'
command -> commandIf 'NL'
command -> commandReturn 'NL'

*/
static  TokenList command( TokenList tl ) {
	Token t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Command, tokenGetLine(t), NULL);
	switch( tokenGetKind( t ) ) {
		case WHILE:
			tl = commandWhile( tl );
			break;
		case RETURN:
			tl = commandReturn( tl );
			break;
		case IF:
			tl = commandIf( tl );
			break;
		case IDENTIFIER:
			tl = processTerminal( tl, IDENTIFIER );
			tl = commandAttrOrCall( tl );
			break;
		default:
			error_flag++;
			Token t = tokenListGetCurrentToken( tl );
			printf( "\nToken: %s ", tokenToString(t));
			printf( "Line %d.\nExpected identifier, if, return or while but got %s.\n", tokenGetLine(t),  tokenToString(t) );
			return NULL;
			break;
	}
	return processTerminal( tl, NL );
};


/*block -> [ 'ID' declOrCommand ]
		 { command }*/
static TokenList block( TokenList tl ) {
	TokenList tlf;
	Token t;
	t = tokenListGetCurrentToken(tl);
	callOnConsume(AST_Block, tokenGetLine(t), NULL);
	if ( verifyCurrentToken( tl, IDENTIFIER ) ) {
		tl = processTerminal( tl, IDENTIFIER );
		tl = declOrCommand( tl );//Recursividade
	}
	tlf = NULL;
	while( tlf!=tl && tl ) {
		t = tokenListGetCurrentToken( tl );	
		tlf = tl;
		switch( tokenGetKind(t) ) {
			case ELSE: case END: case LOOP:
				break;
			default:
				tl = command( tl );
				break;
		}	
	}
	return tl;
}
/*
declFunction -> 'FUN' 'ID' '(' params ')' [ ':' type ] 'NL'
					block
				'END' 'NL'

*/
static TokenList declFunction( TokenList tl ) {
	Token t = tokenListGetCurrentToken(tl);
	callOnConsume(AST_DeclFunction, tokenGetLine(t), NULL);
	tl = processTerminal( tl, FUN );
	tl = processTerminal( tl, IDENTIFIER );
	tl = processTerminal( tl, OP_PARENTHESIS );
	if(verifyCurrentToken(tl, IDENTIFIER)) {
		tl = params( tl );
	}
	tl = processTerminal( tl, CL_PARENTHESIS );
	if( verifyCurrentToken( tl, COLON ) ) {
		tl = processTerminal( tl, COLON );
		tl = type( tl );
	}
	tl = processTerminal(tl, NL);
	tl = block( tl );
	tl = processTerminal(tl, END);
	tl = processTerminal(tl, NL);
	//Temporary, we need make decision about that
	//tl = processTerminalIfCurrent(tl, NL);
	
	return tl;
}



/*
decl -> declGlobalVar start 
decl -> declFunction start
*/
static TokenList decl( TokenList tl ) {
	Token t;
	t = tokenListGetCurrentToken( tl );
	callOnConsume(AST_Decl, tokenGetLine(t), NULL);
	switch ( tokenGetKind(t) ) {
		case IDENTIFIER:
			tl = declGlobalVar( tl );
			break;
		case FUN: 
			tl = declFunction( tl );
			break;
		default:
			break;
			
	}
	return tl;
}


/*
program -> {NL} decl {decl};

*/
TokenList program( TokenList tl ) {
	Token t;
	callOnConsume(AST_Program, 1, NULL);

	if(tl == NULL) {
		printf("Error Empty Program.\n");
		error_flag++;
		return tl;
	}

	// "{NL}"
	while( verifyCurrentToken(tl, NL) ) {
			tl = processTerminal( tl, NL );
	}

	do{
		
		t = tokenListGetCurrentToken( tl );
		switch ( tokenGetKind( t ) ) {
			case IDENTIFIER: case FUN:
				tl = decl( tl );
				break;
			default:
				error_flag++;
				printf("Error at line %d. Expected 'fun' or identifier but got %s\n", tokenGetLine(t), tokenToString(t));
				tl=NULL; // loop protection
				break;
			}
	} while(tl);
	
	return tl;
}



int parser( TokenList tl, callbackOnDerivation f )
{	
	actinOnRules = f;
	program(tl);
	return error_flag;
}
