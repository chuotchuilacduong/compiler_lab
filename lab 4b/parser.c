/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"
#include "codegen.h"

Token *currentToken;
Token *lookAhead;

extern Type* intType;
extern Type* charType;
extern SymTab* symtab;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    //    printToken(lookAhead);
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  program->progAttrs->codeAddress = getCurrentCodeAddress();
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  // Halt the program
  genHL();

  exitBlock();
}

void compileConstDecls(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);
    do {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);
      declareObject(constObj);
      
      eat(SB_EQ);
      constValue = compileConstant();
      constObj->constAttrs->value = constValue;
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
}

void compileTypeDecls(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);
    do {
      eat(TK_IDENT);
      
      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);
      declareObject(typeObj);
      
      eat(SB_EQ);
      actualType = compileType();
      typeObj->typeAttrs->actualType = actualType;
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  } 
}

void compileVarDecls(void) {
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    do {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);
      eat(SB_COLON);
      varType = compileType();
      varObj->varAttrs->type = varType;
      declareObject(varObj);      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  } 
}

void compileBlock(void) {
  Instruction* jmp;
  // Jump to the body of the block
  jmp = genJ(DC_VALUE);

  compileConstDecls();
  compileTypeDecls();
  compileVarDecls();
  compileSubDecls();

  // Update the jmp label
  updateJ(jmp,getCurrentCodeAddress());
  // Skip the stack frame
  genINT(symtab->currentScope->frameSize);

  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else compileProcDecl();
  }
}

void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  funcObj->funcAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);
  
  compileParams();

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);

  compileBlock();

  eat(SB_SEMICOLON);

  exitBlock();
}

void compileProcDecl(void) {
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  procObj->procAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();

  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue* compileConstant(void) {
  ConstantValue* constValue;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = - constValue->intValue;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_INT_CONSTANT,currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type* compileType(void) {
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER);
    type =  makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind = PARAM_VALUE;

  if (lookAhead->tokenType == KW_VAR) {
    paramKind = PARAM_REFERENCE;
    eat(KW_VAR);
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // EmptySt needs to check FOLLOW tokens
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
    // Error occurs
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type* compileLValue(void) {
  Object* var;
  Type* varType;

  eat(TK_IDENT);
  
  var = checkDeclaredLValueIdent(currentToken->string);

  switch (var->kind) {
  case OBJ_VARIABLE:
    genVariableAddress(var); // Đã hiện thực
    if (var->varAttrs->type->typeClass == TP_ARRAY) {
      varType = compileIndexes(var->varAttrs->type);
    }
    else
      varType = var->varAttrs->type;
    break;
  case OBJ_PARAMETER:
    genVariableAddress(var); // Đã hiện thực
    varType = var->paramAttrs->type;
    break;
  case OBJ_FUNCTION:
    genVariableAddress(var); // Đã hiện thực
    varType = var->funcAttrs->returnType;
    break;
  default: 
    error(ERR_INVALID_LVALUE,currentToken->lineNo, currentToken->colNo);
  }

  return varType;
}
void compileAssignSt(void) {
  Type* varType;
  Type* expType;

  varType = compileLValue(); // Đẩy địa chỉ lên stack
  
  eat(SB_ASSIGN);
  expType = compileExpression(); // Đẩy giá trị lên stack
  checkTypeEquality(varType, expType);
  
  genST(); // Store: lưu giá trị vào địa chỉ
}

void compileCallSt(void) {
  // Generate code for call-statement
  // TEMPORARY. halt
  Object* proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  if (isPredefinedProcedure(proc)) {
    compileArguments(proc->procAttrs->paramList);
    genPredefinedProcedureCall(proc);
  } else {
    compileArguments(proc->procAttrs->paramList);
    genHL();
  }
}

void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  
  Instruction* fj = genFJ(DC_VALUE); // Nhảy nếu điều kiện sai
  compileStatement();
  
  if (lookAhead->tokenType == KW_ELSE) {
    Instruction* j = genJ(DC_VALUE); // Nhảy qua phần Else sau khi xong phần Then
    updateFJ(fj, getCurrentCodeAddress()); // Cập nhật địa chỉ nhảy sai
    eat(KW_ELSE);
    compileStatement();
    updateJ(j, getCurrentCodeAddress()); // Cập nhật địa chỉ nhảy xong Then
  } else {
    updateFJ(fj, getCurrentCodeAddress());
  }
}

void compileWhileSt(void) {
  CodeAddress label1 = getCurrentCodeAddress(); // Đánh dấu đầu vòng lặp
  eat(KW_WHILE);
  compileCondition();
  
  Instruction* fj = genFJ(DC_VALUE); // Nhảy ra ngoài nếu sai
  eat(KW_DO);
  compileStatement();
  
  genJ(label1); // Quay lại đầu vòng lặp
  updateFJ(fj, getCurrentCodeAddress()); // Cập nhật đích nhảy ra ngoài
}

void compileForSt(void) {
  // Cấu trúc: FOR <var> := <exp1> TO <exp2> DO <stmt>
  
  eat(KW_FOR);
  
  eat(TK_IDENT);
  Object* var = checkDeclaredVariable(currentToken->string);
  
  genVariableAddress(var); // Stack: [addr]
  genCV();                 // [1] Duplicate address. Stack: [addr, addr]
  
  eat(SB_ASSIGN);
  Type* type1 = compileExpression();
  checkTypeEquality(var->varAttrs->type, type1);
  genST();                 // [2] Store init value. Stack: [addr]
  
  CodeAddress labelLoop = getCurrentCodeAddress();
  
  // So sánh: var <= exp2
  genCV();                 // [3] Duplicate addr. Stack: [addr, addr]
  genLI();                 // [4] Load Value. Stack: [addr, val]
  
  // [FIX] Capture the address after loading the value to jump here later
  CodeAddress labelCheck = getCurrentCodeAddress();

  eat(KW_TO);
  Type* type2 = compileExpression();
  checkTypeEquality(var->varAttrs->type, type2);
  
  genLE();                 // Stack: [addr, bool]
  Instruction* fj = genFJ(DC_VALUE); // Stack: [addr] (nếu False nhảy Exit)

  eat(KW_DO);
  compileStatement();      // Thực hiện thân vòng lặp

  // Tăng biến đếm: var := var + 1
  genCV();                 // Stack: [addr, addr]
  genCV();                 // Stack: [addr, addr, addr]
  genLI();                 // Stack: [addr, addr, val]
  genLC(1);                // Stack: [addr, addr, val, 1]
  genAD();                 // Stack: [addr, addr, val+1]
  genST();                 // Store. Stack: [addr]

  // [FIX] Update to match Reference Binary structure
  // The reference loads the value here (CV, LI) and jumps directly to the check
  genCV();                 
  genLI();                 // Replaces genDCT(1) - opcode 0x03 matches reference

  genJ(labelCheck);        // [FIX] Jump to labelCheck instead of labelLoop

  updateFJ(fj, getCurrentCodeAddress()); // Cập nhật nhãn thoát
  genDCT(1);               // Dọn dẹp stack: Xóa địa chỉ var còn sót lại
}

void compileArgument(Object* param) {
  Type* type;

  if (param->paramAttrs->kind == PARAM_VALUE) {
    type = compileExpression();
    checkTypeEquality(type, param->paramAttrs->type);
  } else {
    type = compileLValue();
    checkTypeEquality(type, param->paramAttrs->type);
  }
}

void compileArguments(ObjectNode* paramList) {
  ObjectNode* node = paramList;

  switch (lookAhead->tokenType) {
  case SB_LPAR:
    eat(SB_LPAR);
    if (node == NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    compileArgument(node->object);
    node = node->next;

    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      if (node == NULL)
	error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
      compileArgument(node->object);
      node = node->next;
    }

    if (node != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    
    eat(SB_RPAR);
    break;
    // Check FOLLOW set 
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void) {
  Type* type1;
  Type* type2;
  TokenType op;

  type1 = compileExpression();
  checkBasicType(type1);

  op = lookAhead->tokenType;
  switch (op) {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  type2 = compileExpression();
  checkTypeEquality(type1,type2);
  switch (op) {
  case SB_EQ: genEQ(); break;
  case SB_NEQ: genNE(); break;
  case SB_LE: genLE(); break;
  case SB_LT: genLT(); break;
  case SB_GE: genGE(); break;
  case SB_GT: genGT(); break;
  default: break;
  }
}

Type* compileExpression(void) {
  Type* type;
  
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    checkIntType(type);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    checkIntType(type);
    genNEG(); // Sinh mã đảo dấu
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

Type* compileExpression2(void) {
  Type* type;

  type = compileTerm();
  type = compileExpression3(type);

  return type;
}


Type* compileExpression3(Type* argType1) {
  Type* argType2;
  Type* resultType;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    checkIntType(argType2);
    genAD();
    resultType = compileExpression3(argType1);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    checkIntType(argType2);
    genSB();
    resultType = compileExpression3(argType1);
    break;
    // check the FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}

Type* compileTerm(void) {
  Type* type;
  type = compileFactor();
  type = compileTerm2(type);

  return type;
}

Type* compileTerm2(Type* argType1) {
  Type* argType2;
  Type* resultType;

  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genML();
    resultType = compileTerm2(argType1);
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genDV();
    resultType = compileTerm2(argType1);
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}
Type* compileFactor(void) {
  Type* type;
  Object* obj;
  int level;
  Scope* scope;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    type = intType;
    genLC(currentToken->value); // [1] Nạp hằng số nguyên
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    type = charType;
    genLC(currentToken->string[0]); // [1] Nạp hằng số ký tự
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind) {
    case OBJ_CONSTANT:
      switch (obj->constAttrs->value->type) {
      case TP_INT:
        type = intType;
        genLC(obj->constAttrs->value->intValue); // [1] Nạp giá trị hằng số định danh
        break;
      case TP_CHAR:
        type = charType;
        genLC(obj->constAttrs->value->charValue); // [1] Nạp giá trị hằng số định danh
        break;
      default:
        break;
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY) {
        // Tính địa chỉ phần tử mảng, kết quả là địa chỉ nằm trên đỉnh stack
        type = compileIndexes(obj->varAttrs->type);
        genLI(); // [3] Load Indirect: Lấy giá trị từ địa chỉ đó
      } else {
        type = obj->varAttrs->type;
        genVariableValue(obj); // [2] Nạp giá trị biến đơn
      }
      break;
    case OBJ_PARAMETER:
      type = obj->paramAttrs->type;
      genVariableValue(obj); // [2] Nạp giá trị tham số (xử lý cả tham biến/tham trị bên trong)
      break;
    case OBJ_FUNCTION:
      if (isPredefinedFunction(obj)) {
        compileArguments(obj->funcAttrs->paramList);
        genPredefinedFunctionCall(obj);
      } else {
        compileArguments(obj->funcAttrs->paramList);
        
        // [4] Tính toán level difference (Static Link)
        level = 0;
        scope = symtab->currentScope;
        // Duyệt ngược scope hiện tại cho đến khi gặp scope bao ngoài hàm được gọi
        while (scope != obj->funcAttrs->scope->outer) {
            level++;
            scope = scope->outer;
        }
        genCALL(level, obj->funcAttrs->codeAddress);
      }
      type = obj->funcAttrs->returnType;
      break;
    default: 
      error(ERR_INVALID_FACTOR,currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  case SB_LPAR:
    eat(SB_LPAR);
    type = compileExpression();
    eat(SB_RPAR);
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
  
  return type;
}

Type* compileIndexes(Type* arrayType) {
  // TEMPORARY: halt
  Type* type;

  
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    type = compileExpression();
    checkIntType(type);
    checkArrayType(arrayType);

    genHL();

    arrayType = arrayType->elementType;
    eat(SB_RSEL);
  }
  checkBasicType(arrayType);
  return arrayType;
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  cleanSymTab();
  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}
