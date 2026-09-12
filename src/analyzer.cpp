/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 First Person
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <fstream>
#include <iomanip>
#include <map>
#include <LIEF/LIEF.hpp>
#include <algorithm>
#include <threading.h>
#include <unistd.h>
#include <cstdint>

#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/TargetParser/Host.h>

#include <dynvar.h>
#include <runtime.h>
#include <analyzer.h>
#include <definations.h>
#include <insset/cominsset.h>

struct AST {
    virtual ~AST() {}
};

struct InstructionNode : AST {
    CommonOperator ioperator;
    vector* operand;

    InstructionNode(CommonOperator op, vector* opr) : ioperator(std::move(op)),
                                                      operand(std::move(opr)) { }
};

struct ValueNode : AST {
    dynvar value;
    ValueNode(dynvar vl) : value(std::move(vl)) { }
};

typedef struct {
    ProcessorResult res;
    AST* operation;
} OperationResult;

typedef struct {
    InstructionNode* node;
    AST* targvar;
} InstructionQueue;

static vector instructionQueues;
static vector executionTasks;
std::map<AST*, std::pair<bool, int>> cachedSteps;
static int lockout = -1;
static LLVMArch carch;
static vector processorResults;

dynvar evalResult(AST* node) {
    dynvar result;
    result.address = 0;
    result.length = 0;

    if (auto instr = dynamic_cast<InstructionNode*>(node)) {
        if (instr->ioperator == ADD) {
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    lgr leftOpBuffer;
                    vectorGetValue(instr->operand, 0, &leftOpBuffer);
                    lgr leftOp;
                    getValue(evalResult((AST*)leftOpBuffer), &leftOp);
                    lgr rightOpBuffer;
                    vectorGetValue(instr->operand, 1, &rightOpBuffer);
                    lgr rightOp;
                    getValue(evalResult((AST*)rightOpBuffer), &rightOp);

                    if (((int32_t)((uintptr_t)leftOp) > maxCalcValue || (int32_t)((uintptr_t)rightOp) > maxCalcValue) ||
                        ((int32_t)((uintptr_t)leftOp) > (maxCalcValue * -1) || (int32_t)((uintptr_t)rightOp) > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    const ProcessorResult pres = add((uintptr_t)leftOp, sizeof(leftOp),
                                                 (uintptr_t)rightOp, sizeof(rightOp),
                                            emvec, emvec);
                    if ((int64_t)leftOp > maxCalcValue) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    OperationResult res;
                    res.operation = node;
                    res.res = pres;
                    lgr bufr;
                    memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                    vectorAppend(&processorResults, bufr);

                    if (pres != PROC_SUCCESS)
                        return result;
                    setValue(&result, leftOp);
                    return result;

                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
        } else if (instr->ioperator == SUB) {
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    lgr leftOpBuffer;
                    vectorGetValue(instr->operand, 0, &leftOpBuffer);
                    lgr leftOp;
                    getValue(evalResult((AST*)leftOpBuffer), &leftOp);
                    lgr rightOpBuffer;
                    vectorGetValue(instr->operand, 1, &rightOpBuffer);
                    lgr rightOp;
                    getValue(evalResult((AST*)rightOpBuffer), &rightOp);

                    if (((int32_t)((uintptr_t)leftOp) > maxCalcValue || (int32_t)((uintptr_t)rightOp) > maxCalcValue) ||
                        ((int32_t)((uintptr_t)leftOp) > (maxCalcValue * -1) || (int32_t)((uintptr_t)rightOp) > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    const ProcessorResult pres = subtract((uintptr_t)leftOp,
                        sizeof(leftOp), (uintptr_t)rightOp, sizeof(rightOp),
                                     emvec, emvec);
                    if (((int64_t)leftOp > maxCalcValue) ||
                        ((int64_t)leftOp > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    OperationResult res;
                    res.operation = node;
                    res.res = pres;
                    lgr bufr;
                    memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                    vectorAppend(&processorResults, bufr);

                    if (pres != PROC_SUCCESS)
                        return result;
                    setValue(&result, leftOp);
                    return result;

                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
        } else if (instr->ioperator == MUL) {
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    lgr leftOpBuffer;
                    vectorGetValue(instr->operand, 0, &leftOpBuffer);
                    lgr leftOp;
                    getValue(evalResult((AST*)leftOpBuffer), &leftOp);
                    lgr rightOpBuffer;
                    vectorGetValue(instr->operand, 1, &rightOpBuffer);
                    lgr rightOp;
                    getValue(evalResult((AST*)rightOpBuffer), &rightOp);

                    if (((int32_t)((uintptr_t)leftOp) > maxCalcValue || (int32_t)((uintptr_t)rightOp) > maxCalcValue) ||
                        ((int32_t)((uintptr_t)leftOp) > (maxCalcValue * -1) || (int32_t)((uintptr_t)rightOp) > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    const ProcessorResult pres = multiply((uintptr_t)leftOp, sizeof(leftOp),
                                                 (uintptr_t)rightOp, sizeof(rightOp),
                                            emvec, emvec);
                    if ((int64_t)leftOp > maxCalcValue) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    OperationResult res;
                    res.operation = node;
                    res.res = pres;
                    lgr bufr;
                    memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                    vectorAppend(&processorResults, bufr);

                    if (pres != PROC_SUCCESS)
                        return result;
                    setValue(&result, leftOp);
                    return result;

                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
        } else if (instr->ioperator == DIV) {
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    lgr leftOpBuffer;
                    vectorGetValue(instr->operand, 0, &leftOpBuffer);
                    lgr leftOp;
                    getValue(evalResult((AST*)leftOpBuffer), &leftOp);
                    lgr rightOpBuffer;
                    vectorGetValue(instr->operand, 1, &rightOpBuffer);
                    lgr rightOp;
                    getValue(evalResult((AST*)rightOpBuffer), &rightOp);

                    if (((int32_t)((uintptr_t)leftOp) > maxCalcValue || (int32_t)((uintptr_t)rightOp) > maxCalcValue) ||
                        ((int32_t)((uintptr_t)leftOp) > (maxCalcValue * -1) || (int32_t)((uintptr_t)rightOp) > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    } else if ((int32_t)((uintptr_t)leftOp) == 0 || (int32_t)((uintptr_t)rightOp) == 0) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_DIVISION_BY_ZERO;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    const ProcessorResult pres = divide((uintptr_t)leftOp,
                        sizeof(leftOp), (uintptr_t)rightOp, sizeof(rightOp),
                                     emvec, emvec);
                    if (((int64_t)leftOp > maxCalcValue) ||
                        ((int64_t)leftOp > (maxCalcValue * -1))) {
                        OperationResult res;
                        res.operation = node;
                        res.res = PROC_BUFFER_OVERFLOW;
                        lgr bufr;
                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                        vectorAppend(&processorResults, bufr);
                        return result;
                    }

                    OperationResult res;
                    res.operation = node;
                    res.res = pres;
                    lgr bufr;
                    memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                    vectorAppend(&processorResults, bufr);

                    if (pres != PROC_SUCCESS)
                        return result;
                    setValue(&result, leftOp);
                    return result;

                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
        }
    } else if (auto valNode = dynamic_cast<ValueNode*>(node))
        return valNode->value;

    return result;
}

void handleRealtimeOperation(void* arg) {
    ThreadCallbackArgs* callbackArgs = (ThreadCallbackArgs*)arg;
    InstructionQueue* queue = (InstructionQueue*)callbackArgs->originalArg;
    const int threadId = callbackArgs->threadId;
    int idx;
    while (true) {
        while (lockout != threadId) usleep(2000);
        lockout = threadId;
        if (cachedSteps.contains((AST*)queue))
            cachedSteps.find((AST*)queue)->second.second = threadId;
        std::map<AST*, std::pair<bool, int>> currentSteps = cachedSteps;
        lockout = -1;
        idx = 0;
        for (auto [cachedStep, stepInfo] : currentSteps) {
            auto [locked, stepThreadId] = stepInfo;
            if (auto instr = dynamic_cast<InstructionQueue*>(cachedStep))
                if (instr->targvar == queue->targvar) {
                    while (locked) {
                        locked = std::next(cachedSteps.begin(), idx)->second.first;
                        usleep(2000);
                    }
                    goto finalStep;
                }
            idx++;
        }
        usleep(2000);
    }

finalStep:
    lockout = -1;
}

CommonOperator mapLLVMOpcodeToOperator(unsigned opcode, const llvm::MCInstrInfo* MII) {
    // Get instruction name from MCInstrInfo
    const llvm::StringRef instName = MII->getName(opcode);
    
    // Map instruction names to CommonOperator
    std::string name(instName.str());
    
    // Convert to uppercase for easier comparison
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    if (upperName.size() >= 3) {
        if (upperName.substr(0, 3) == "MOV")
            return MOV;
        else if (upperName.substr(0, 3) == "ADD")
            return ADD;
        else if (upperName.substr(0, 3) == "SUB")
            return SUB;
        else if (upperName.substr(0, 3) == "MUL")
            return MUL;
        else if (upperName.substr(0, 3) == "DIV")
            return DIV;
        else if (upperName.substr(0, 3) == "INC")
            return INC;
        else if (upperName.substr(0, 3) == "DEC")
            return DEC;
        else if (upperName.substr(0, 3) == "AND")
            return AND;
        else if (upperName.substr(0, 3) == "XOR")
            return XOR;
        else if (upperName.substr(0, 3) == "NOT")
            return NOT;
        else if (upperName.substr(0, 3) == "SHL")
            return SHL;
        else if (upperName.substr(0, 3) == "SHR")
            return SHR;
        else if (upperName.substr(0, 3) == "SAR")
            return SAR;
        else if (upperName.substr(0, 3) == "CMP")
            return CMP;
        else if (upperName.substr(0, 3) == "JMP")
            return JMP;
        else if (upperName.substr(0, 3) == "JNE")
            return JNE;
        else if (upperName.substr(0, 3) == "JGE")
            return JGE;
        else if (upperName.substr(0, 3) == "JLE")
            return JLE;
        else if (upperName.substr(0, 3) == "JAE")
            return JAE;
        else if (upperName.substr(0, 3) == "JBE")
            return JBE;
        else if (upperName.substr(0, 3) == "JNO")
            return JNO;
        else if (upperName.substr(0, 3) == "RET")
            return RET;
        else if (upperName.substr(0, 3) == "POP")
            return POP;
        else if (upperName.substr(0, 3) == "LEA")
            return LEA;
        else if (upperName.substr(0, 3) == "NEG")
            return NEG;
        else if (upperName.substr(0, 3) == "ROL")
            return ROL;
        else if (upperName.substr(0, 3) == "ROR")
            return ROR;
        else if (upperName.substr(0, 3) == "INT")
            return INT;
        else if (upperName.substr(0, 3) == "CLD")
            return CLD;
        else if (upperName.substr(0, 3) == "STD")
            return STD;
        else if (upperName.substr(0, 3) == "CDQ" || upperName.substr(0, 3) == "CQO")
            return CLTD;
        else if (upperName.substr(0, 3) == "CBW")
            return CBW;
        else if (upperName.substr(0, 3) == "NOP")
            return NOP;
    } else if (upperName.size() >= 4) {
        if (upperName.substr(0, 4) == "IDIV")
            return DIV;
        else if (upperName.substr(0, 4) == "TEST")
            return TEST;
        else if (upperName.substr(0, 4) == "CALL")
            return CALL;
        else if (upperName.substr(0, 4) == "PUSH")
            return PUSH;
        else if (upperName.substr(0, 4) == "XCHG")
            return XCHG;
        else if (upperName.substr(0, 4) == "CWDE" || upperName.substr(0, 4) == "CDQE")
            return CBW;
        else if (upperName.substr(0, 4) == "LOOP")
            return LOOP;
    } else if (upperName.size() >= 2) {
        if (upperName.substr(0, 2) == "OR")
            return OR;
        else if (upperName.substr(0, 2) == "JE")
            return JE;
        else if (upperName.substr(0, 2) == "JG")
            return JG;
        else if (upperName.substr(0, 2) == "JL")
            return JL;
        else if (upperName.substr(0, 2) == "JA")
            return JA;
        else if (upperName.substr(0, 2) == "JB")
            return JB;
        else if (upperName.substr(0, 2) == "JO")
            return JO;
        else if (upperName.substr(0, 2) == "JS")
            return JS;
    } else if (upperName.size() >= 7) {
        if (upperName.substr(0, 7) == "SYSCALL")
            return SYSCALL;
        else if (upperName.substr(0, 7) == "SYSEXIT")
            return SYSEXIT;
    } else if (upperName.size() >= 8) {
        if (upperName.substr(0, 8) == "SYSENTER")
            return SYSENTER;
    } else if (upperName.size() >= 5) {
        if (upperName.substr(0, 5) == "LEAVE")
            return LEAVE;
        else if (upperName.substr(0, 5) == "ENTER")
            return ENTER;
    }
    
    return UNKNOWN_OP;
}

extern "C" {
    void init(LLVMArch arch) {
        vectorInit(&instructionQueues, sizeof(int));
        vectorInit(&executionTasks, sizeof(int));
        vectorInit(&processorResults, sizeof(OperationResult));
    }

    analyzedResult analyzeFunction(dynvar functionName, dynvar source) {
        analyzedResult result;
        result.riskyValue = NULL;
        result.valueBehavor = NULL;
        vector pushedFunction;
        uintptr_t currentFunction;

        vectorInit(&pushedFunction, sizeof(currentFunction));
        static lgr bufr = {0};
        getValue(source, &bufr);
        std::unique_ptr<LIEF::ELF::Binary> bin = LIEF::ELF::Parser::parse(bufr);
        if (!bin) {
            std::cerr << "Cannot Analyze The Library From " << bufr << std::endl;
            exit(1);
        }

        const auto* text = bin->get_section(".text");
        if (!text) {
            std::cerr << bufr << " is Useless." << std::endl;
            return result;
        }

        getValue(functionName, &bufr);
        auto* sym = bin->get_symbol(bufr);
        if (!sym) {
            lgr libpath = {0};
            getValue(source, &libpath);
            std::cerr << bufr << " Is Not Found From " << libpath
                      << " But Required." << std::endl;
            exit(1);
        }

        std::vector<AST*> nodes;
        llvm::Triple targetTriple;
        std::unique_ptr<llvm::MCDisassembler> dis;
        std::unique_ptr<llvm::MCInstrInfo> MII;
        std::unique_ptr<llvm::MCRegisterInfo> MRI;
        try {
            llvm::InitializeAllTargetInfos();
            llvm::InitializeAllTargetMCs();
            llvm::InitializeAllDisassemblers();

            targetTriple = llvm::Triple("x86_64-pc-linux-gnu");
            std::string err;
            const llvm::Target *target =
                        llvm::TargetRegistry::lookupTarget(targetTriple.getTriple(), err);

            if (!target) {
                char* errCopy = strdup(err.c_str());
                bugDetected(errCopy);
                free(errCopy);
            }

            MRI = std::unique_ptr<llvm::MCRegisterInfo>(
                target->createMCRegInfo(targetTriple.getTriple()));

            llvm::MCTargetOptions mcOpts;
            auto MAI = std::unique_ptr<llvm::MCAsmInfo>(
                target->createMCAsmInfo(*MRI, targetTriple.getTriple(), mcOpts));

            auto STI = std::unique_ptr<llvm::MCSubtargetInfo>(
                target->createMCSubtargetInfo(
                    targetTriple.getTriple(), "x86-64", ""));

            MII = std::unique_ptr<llvm::MCInstrInfo>(
                target->createMCInstrInfo());

            llvm::MCContext ctx(targetTriple, MAI.get(), MRI.get(), STI.get());

            dis = std::unique_ptr<llvm::MCDisassembler>(
                target->createMCDisassembler(*STI, ctx));
        } catch (...) {
            char* msg = strdup("Disassembler Is Failed To Initialize.");
            bugDetected(msg);
            free(msg);
        }
        std::string sbufr;
        {
            lgr bufr;
            getValue(source, &bufr);
            sbufr = bufr;
        }

        std::ifstream file(sbufr);
        if (!file.is_open()) {
            lgr libpath = {0};
            getValue(source, &libpath);
            std::cerr << "Cannot Open " << sbufr << " But Required." << std::endl;
            exit(1);
        }
        sbufr = "";
        std::vector<uint8_t> pbufr;
        while (std::getline(file, sbufr))
            for (char ch : sbufr)
                pbufr.push_back(static_cast<uint8_t>(ch));

        currentFunction = (uintptr_t)sym->value();
        const uintptr_t textStart = text->virtual_address();
        const uintptr_t textEnd = textStart + text->size();
        llvm::ArrayRef<uint8_t> bytes(pbufr);
        uint64_t size = 0;
        
        for (uintptr_t addr = currentFunction; addr < textEnd; addr += size) {
            llvm::MCInst inst;
            size = 0;
            auto status = dis->getInstruction(
                        inst,
                        size,
                        bytes.slice(addr - textStart),
                        addr,
                        llvm::nulls());

            if (status == llvm::MCDisassembler::Success) {
                CommonOperator op = mapLLVMOpcodeToOperator(inst.getOpcode(), MII.get());
                
                vector* operands = new vector();
                vectorInit(operands, sizeof(uintptr_t));
                
                for (unsigned opIdx = 0; opIdx < inst.getNumOperands(); ++opIdx) {
                    const llvm::MCOperand &operand = inst.getOperand(opIdx);
                    uintptr_t operandValue = 0;

                    if (operand.isReg()) {
                        operandValue = operand.getReg();
                        llvm::outs() << "Reg: " << MRI->getName(operand.getReg()) << "\n";
                    } else if (operand.isImm()) {
                        operandValue = operand.getImm();
                        llvm::outs() << "Imm: " << operand.getImm() << "\n";
                    } else if (operand.isExpr())
                        llvm::outs() << "Expr\n";
                    
                    lgr operandBuf;
                    memcpy(operandBuf, &operandValue, sizeof(uintptr_t));
                    vectorAppend(operands, operandBuf);
                }
                
                InstructionNode* node = new InstructionNode(op, operands);
                nodes.push_back(node);
            } else
                size = 1; // Skip one byte if disassembly fails
        }

        for (AST* node : nodes) {
            if (auto instrNode = dynamic_cast<InstructionNode*>(node)) {
                if ((instrNode->ioperator == ADD || instrNode->ioperator == SUB ||
                     instrNode->ioperator == MUL || instrNode->ioperator == DIV ||
                     instrNode->ioperator == INC || instrNode->ioperator == DEC) &&
                    instrNode->operand->count > 0) {
                    InstructionQueue inqueue;
                    inqueue.node = instrNode;
                    lgr tvarBuffer;
                    vectorGetValue(instrNode->operand, 0, &tvarBuffer);
                    AST* tvar = (AST*)tvarBuffer;
                    inqueue.targvar = tvar;
                    int threadID = threadNew(handleRealtimeOperation, &inqueue);
                    if (threadID < 0)
                        handleRealtimeOperation(&inqueue);
                    else {
                        lgr threadIDBuffer;
                        memcpy(threadIDBuffer, &threadID, sizeof(int));
                        vectorAppend(&instructionQueues, threadIDBuffer);
                        threadDetach(threadID);
                    }
                }
            }
        }

        // For now, clean up AST nodes after analysis
        // In a complete implementation, these would be stored in the result
        for (AST* node : nodes) {
            InstructionNode* instrNode = static_cast<InstructionNode*>(node);
            if (instrNode->operand) {
                vectorDeleteAll(instrNode->operand);
                delete instrNode->operand;
            }
            delete instrNode;
        }

        return result;
    }
}