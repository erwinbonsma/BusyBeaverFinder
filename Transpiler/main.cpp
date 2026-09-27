//
//  Transpiler.cpp
//  Transpiler
//
//  Created by Erwin on 27/09/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//


#include <string>
#include <iostream>
#include <fstream>

#include "cxxopts.hpp"

#include "Program.h"
#include "InterpretedProgramBuilder.h"
#include "InterpretedProgramCanonizer.h"

class Transpiler {
    std::shared_ptr<InterpretedProgram> _program;
    std::size_t _dataSize = 1000;
    std::size_t _maxSteps = 1000;
    std::size_t _numUnrolls = 1;
    std::size_t _padSizeMin = 0;
    std::size_t _padSizeMax = 0;

    void gotoExit(std::ostream &os, const char* indent, int errorCode);
    void gotoBlock(std::ostream &os, const char* indent, int nextIter, const ProgramBlock* block);
    void transpileBlock(std::ostream &os, int unrollIndex, int stepIndex);

public:
    Transpiler(std::shared_ptr<InterpretedProgram> program) : _program(program) {}

    void setDataSize(std::size_t dataSize) { _dataSize = dataSize; }
    void setMaxSteps(std::size_t maxSteps) { _maxSteps = maxSteps; }
    void setNumUnrolls(std::size_t numUnrolls) { _numUnrolls = numUnrolls; }

    void transpile(std::ostream &os);
};

void Transpiler::gotoExit(std::ostream &os, const char* indent, int errorCode) {
    os << indent << "errorCode = " << errorCode << ";" << std::endl;
    os << indent << "goto done;" << std::endl;
}

void Transpiler::gotoBlock(std::ostream &os,
                           const char* indent,
                           int nextIter,
                           const ProgramBlock* block) {
    if (block) {
        os << indent << "goto step_"
           << nextIter << "_" << _program->indexOf(block) << ";" << std::endl;
    } else {
        gotoExit(os, indent, 1);
    }
}

void Transpiler::transpileBlock(std::ostream &os, int unrollIndex, int stepIndex) {
    os << "step_" << unrollIndex << "_" << stepIndex << ":" << std::endl;

    auto block = _program->programBlockAt(stepIndex);

    if (block->isExit()) {
        gotoExit(os, "\t", 0);
        return;
    }

    if (!block->isFinalized()) {
        gotoExit(os, "\t", 1);
        return;
    }

    if (block->isHang()) {
        gotoExit(os, "\t", 2);
        return;
    }

    if (unrollIndex == 0) {
        os << "\tnumSteps += deltaSteps;" << std::endl;
        os << "\tdeltaSteps = 0;" << std::endl;
        os << "\tif (numSteps > maxSteps) {" << std::endl;
        gotoExit(os, "\t\t", 3);
        os << "\t}" << std::endl;

        os << "\tif (dataP < dataMinP || dataP >= dataMaxP) {" << std::endl;
        gotoExit(os, "\t\t", 4);
        os << "\t}" << std::endl;
    }

    os << "\t";
    if (block->isDelta()) {
        os << "*";
    }
    os << "dataP " << (block->getInstructionAmount() > 0 ? "+" : "-");
    os << "= " << abs(block->getInstructionAmount()) << ";" << std::endl;

    os << "\tdeltaSteps += " << block->getNumSteps() << ";" << std::endl;

    int nextIter = (unrollIndex + 1) % _numUnrolls;
    os << "\tif (*dataP) {" << std::endl;
    gotoBlock(os, "\t\t", nextIter, block->nonZeroBlock());
    os << "\t} else {" << std::endl;
    gotoBlock(os, "\t\t", nextIter, block->zeroBlock());
    os << "\t}" << std::endl;
}


void Transpiler::transpile(std::ostream &os) {
    os << "#import <stdio.h>" << std::endl;
    os << "#import <stdlib.h>" << std::endl;
    os << "#import <string.h>" << std::endl;
    os << std::endl;

    os << "// Error codes:" << std::endl;
    os << "// 0 = Program terminated" << std::endl;
    os << "// 1 = Unset instruction (late escape)" << std::endl;
    os << "// 2 = No-instruction hang" << std::endl;
    os << "// 3 = Assumed hang" << std::endl;
    os << "// 4 = Exceeded data bounds" << std::endl;
    os << std::endl;

    int minShift = 0;
    int maxShift = 0;
    for (int i = 0; i < _program->numProgramBlocks(); ++i) {
        auto block = _program->programBlockAt(i);
        if (!block->isDelta()) {
            minShift = std::max(minShift, -block->getInstructionAmount());
            maxShift = std::max(maxShift, block->getInstructionAmount());
        }
    }

    std::size_t dataSize = (maxShift + minShift) * _numUnrolls + _dataSize;
    os << "// minShift = " << minShift << std::endl;
    os << "// maxShift = " << maxShift << std::endl;

    os << "int main(int argc, char * argv[]) {" << std::endl;
    os << "\tunsigned long numSteps = 0;" << std::endl;
    os << "\tunsigned long maxSteps = " << _maxSteps << ";" << std::endl;
    os << "\tunsigned int deltaSteps = 0;" << std::endl;
    os << "\tint errorCode = -1;" << std::endl;
    os << "\tint data[" << dataSize << "];" << std::endl;
    os << "\tint* dataP = &data[" << minShift * _numUnrolls + _dataSize / 2 << "];" << std::endl;
    os << "\tint* dataMinP = &data[" << minShift * _numUnrolls << "];" << std::endl;
    os << "\tint* dataMaxP = &data[" << minShift * _numUnrolls + _dataSize << "];" << std::endl;
    os << std::endl;

    os << "\tif (argc == 2) {" << std::endl;
    os << "\t\tchar* endp;" << std::endl;
    os << "\t\tmaxSteps = strtoul(argv[1], &endp, 10);" << std::endl;
    os << "\t}" << std::endl;
    os << std::endl;

    os << "\tmemset(data, 0, " << dataSize << " * sizeof(int));" << std::endl;
    os << std::endl;

    for (int i = 0; i < _numUnrolls; ++i) {
        for (int j = 0; j < _program->numProgramBlocks(); ++j) {
            transpileBlock(os, i, j);
        }
    }

    os << "done:" << std::endl;
    os << "\tprintf(\"errorCode=%d\\tsteps=%lu\\n\", errorCode, numSteps);" << std::endl;
    os << "\texit(errorCode);" << std::endl;
    os << "}" << std::endl;
}


int main(int argc, char * argv[]) {
    cxxopts::Options options("BB-Transpiler", "Transpiler for 2L-BB Programs");
    options.add_options()
        ("l,loopunrolls", "Loop unroll size", cxxopts::value<std::size_t>()->default_value("1"))
        ("d,datasize", "Data size", cxxopts::value<std::size_t>()->default_value("100000"))
        ("max-steps", "Maximum program execution steps",
         cxxopts::value<std::size_t>()->default_value("1000000"))
        ("program", "Program specification", cxxopts::value<std::string>())
        ("block-sizes", "Size specification of each program block", cxxopts::value<std::string>())
        ("outfile", "Path of output file", cxxopts::value<std::string>())
        ("help", "Show help");
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    if (!args.count("program")) {
        std::cerr << "Must specify a program" << std::endl;
        exit(-1);
    }
    auto programSpec = args["program"].as<std::string>();

    std::shared_ptr<InterpretedProgram>  program;
    if (args.count("block-sizes")) {
        // When block sizes are specified, assume the program spec is of an interpreted program
        auto sizeSpec = args["block-sizes"].as<std::string>();

        program = std::make_shared<InterpretedProgramFromString>(programSpec, sizeSpec);
    } else {
        // When there are no block sizes, assume the program spec is of a 2L-BB program
        Program program2D = Program::fromString(programSpec);

        auto builder = std::make_shared<InterpretedProgramBuilder>();
        builder->buildFromProgram(program2D);

        program = builder;
    }

    Transpiler transpiler {program};
    transpiler.setDataSize(args["datasize"].as<std::size_t>());
    transpiler.setNumUnrolls(args["loopunrolls"].as<std::size_t>());
    transpiler.setMaxSteps(args["max-steps"].as<std::size_t>());

    {
        std::ostream* fp = &std::cout;
        std::ofstream fout;
        if (args.count("outfile")) {
            fout.open(args["outfile"].as<std::string>());
            fp = &fout;
        }

        *fp << "// Program spec: " << programSpec << std::endl;
        *fp << std::endl;
        transpiler.transpile(*fp);
    }

    return 0;
}
