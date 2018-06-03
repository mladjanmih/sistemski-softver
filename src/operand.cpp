#include "operand.h"
#include <regex>

using namespace ss;

std::regex decimalRegex("(^(-)?[0-9]+$)");
std::regex regIndDecRegex("^r[0-7]\\[[0-9]+\\]$");
std::regex regIndLabRegex("^r[0-7]\\[[a-zA-Z_]\\w+\\]$");
std::regex regDirRegex("^r[0-7]$");
std::regex labelRegex("[^\\[][a-zA-Z_]\\w+[^\\]]$");
std::regex locationRegex("^[0-9]+$");

Operand::Operand(std::string op) : valid(true) {
    if (std::regex_match(op, labelRegex)) {
        this->extraBytes = true;

        if (op[0] == '&') {
            this->type = OperandType::LABEL_VAL;
        }
        
        else if (op[0] == '$') {
            this->type = OperandType::PCREL_VAL;
        }

        else {
            this->type = OperandType::MEMDIR_VAL;
        }
    }

    else if (std::regex_match(op, regDirRegex)) {
        this->extraBytes = false;
        this->type = OperandType::REGDIR_VAL;
    }

    else if (std::regex_match(op, regIndDecRegex)) {
        this->extraBytes = true;
        this->type = OperandType::REGIND_DEC_VAL;
    }

    else if (std::regex_match(op, regIndLabRegex)) {
        this->extraBytes = true;
        this->type = OperandType::REGIND_LAB_VAL;
    }

    else if (std::regex_match(op, locationRegex)) {
        this->extraBytes = true;
        this->type = OperandType::DECIMAL_LOCATION_VAL;
    }

    else if (std::regex_match(op, decimalRegex)) {
        this->extraBytes = true;
        this->type = OperandType::IMMED_VAL;
    }

    else {
        this->valid = false;
    }
}