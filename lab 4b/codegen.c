/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include "reader.h"
#include "codegen.h"  

#define CODE_SIZE 10000
extern SymTab* symtab;

extern Object* readiFunction;
extern Object* readcFunction;
extern Object* writeiProcedure;
extern Object* writecProcedure;
extern Object* writelnProcedure;

CodeBlock* codeBlock;
int computeNestedLevel(Scope* scope) {
  int level = 0;
  Scope* tmp = symtab->currentScope;
  while (tmp != NULL && tmp != scope) {
    tmp = tmp->outer;
    level++;
  }
  return level;
}
void genVariableAddress(Object* var) {
  int level;
  int offset;

  if (var->kind == OBJ_VARIABLE) {
    level = computeNestedLevel(var->varAttrs->scope);
    offset = var->varAttrs->localOffset;
    genLA(level, offset);
  } else if (var->kind == OBJ_PARAMETER) {
    level = computeNestedLevel(var->paramAttrs->scope);
    offset = var->paramAttrs->localOffset;
    if (var->paramAttrs->kind == PARAM_VALUE) {
      genLA(level, offset);
    } else {
      // Tham chiếu (VAR parameter): giá trị của nó chính là địa chỉ
      genLV(level, offset); 
    }
  } else if (var->kind == OBJ_FUNCTION) {
    // Biến trả về của hàm (nếu gán tên hàm = giá trị)
    genLA(0, 0); 
  }
}

void genVariableValue(Object* var) {
  int level;
  int offset;

  if (var->kind == OBJ_VARIABLE) {
    level = computeNestedLevel(var->varAttrs->scope);
    offset = var->varAttrs->localOffset;
    genLV(level, offset);
  } else if (var->kind == OBJ_CONSTANT) {
    if (var->constAttrs->value->type == TP_INT)
      genLC(var->constAttrs->value->intValue);
    else
      genLC(var->constAttrs->value->charValue);
  } else if (var->kind == OBJ_PARAMETER) {
    level = computeNestedLevel(var->paramAttrs->scope);
    offset = var->paramAttrs->localOffset;
    if (var->paramAttrs->kind == PARAM_VALUE) {
      genLV(level, offset);
    } else {
      // Tham chiếu: load giá trị (địa chỉ), sau đó load indirect
      genLV(level, offset);
      genLI();
    }
  }
}

int isPredefinedFunction(Object* func) {
  return ((func == readiFunction) || (func == readcFunction));
}

int isPredefinedProcedure(Object* proc) {
  return ((proc == writeiProcedure) || (proc == writecProcedure) || (proc == writelnProcedure));
}

void genPredefinedProcedureCall(Object* proc) {
  if (proc == writeiProcedure)
    genWRI();
  else if (proc == writecProcedure)
    genWRC();
  else if (proc == writelnProcedure)
    genWLN();
}

void genPredefinedFunctionCall(Object* func) {
  if (func == readiFunction)
    genRI();
  else if (func == readcFunction)
    genRC();
}

void genLA(int level, int offset) {
  emitLA(codeBlock, level, offset);
}

void genLV(int level, int offset) {
  emitLV(codeBlock, level, offset);
}

void genLC(WORD constant) {
  emitLC(codeBlock, constant);
}

void genLI(void) {
  emitLI(codeBlock);
}

void genINT(int delta) {
  emitINT(codeBlock,delta);
}

void genDCT(int delta) {
  emitDCT(codeBlock,delta);
}

Instruction* genJ(CodeAddress label) {
  Instruction* inst = codeBlock->code + codeBlock->codeSize;
  emitJ(codeBlock,label);
  return inst;
}

Instruction* genFJ(CodeAddress label) {
  Instruction* inst = codeBlock->code + codeBlock->codeSize;
  emitFJ(codeBlock, label);
  return inst;
}

void genHL(void) {
  emitHL(codeBlock);
}

void genST(void) {
  emitST(codeBlock);
}

void genCALL(int level, CodeAddress label) {
  emitCALL(codeBlock, level, label);
}

void genEP(void) {
  emitEP(codeBlock);
}

void genEF(void) {
  emitEF(codeBlock);
}

void genRC(void) {
  emitRC(codeBlock);
}

void genRI(void) {
  emitRI(codeBlock);
}

void genWRC(void) {
  emitWRC(codeBlock);
}

void genWRI(void) {
  emitWRI(codeBlock);
}

void genWLN(void) {
  emitWLN(codeBlock);
}

void genAD(void) {
  emitAD(codeBlock);
}

void genSB(void) {
  emitSB(codeBlock);
}

void genML(void) {
  emitML(codeBlock);
}

void genDV(void) {
  emitDV(codeBlock);
}

void genNEG(void) {
  emitNEG(codeBlock);
}

void genCV(void) {
  emitCV(codeBlock);
}

void genEQ(void) {
  emitEQ(codeBlock);
}

void genNE(void) {
  emitNE(codeBlock);
}

void genGT(void) {
  emitGT(codeBlock);
}

void genGE(void) {
  emitGE(codeBlock);
}

void genLT(void) {
  emitLT(codeBlock);
}

void genLE(void) {
  emitLE(codeBlock);
}

void updateJ(Instruction* jmp, CodeAddress label) {
  jmp->q = label;
}

void updateFJ(Instruction* jmp, CodeAddress label) {
  jmp->q = label;
}

CodeAddress getCurrentCodeAddress(void) {
  return codeBlock->codeSize;
}


void initCodeBuffer(void) {
  codeBlock = createCodeBlock(CODE_SIZE);
}

void printCodeBuffer(void) {
  printCodeBlock(codeBlock);
}

void cleanCodeBuffer(void) {
  freeCodeBlock(codeBlock);
}

int serialize(char* fileName) {
  FILE* f;

  f = fopen(fileName, "wb");
  if (f == NULL) return IO_ERROR;
  saveCode(codeBlock, f);
  fclose(f);
  return IO_SUCCESS;
}
