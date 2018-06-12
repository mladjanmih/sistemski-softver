#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "assembler.h"
#include "utils.h"
#include "string_tokenizer.h"
#include "symbol.h"
#include "ss_exceptions.h"
#include "instruction.h"
#include "operand.h"
#include "section.h"
#include "directive.h"
#include "asm_declarations.h"
#include "relocation.h"
#include "elf.h"

using namespace ss;

void Assembler::writePrettyOutput() {
    bool hasText = false;
    bool hasRoData = false;
    bool hasData = false;

    for(int i = 0; i < SECTION_NUMBER && this->sectionOrder[i] != SectionType::UDF; ++i) {
        if (this->sectionOrder[i] == SectionType::TEXT) 
            hasText = true;

        if (this->sectionOrder[i] == SectionType::RO_DATA)
            hasRoData = true;
        
        if (this->sectionOrder[i] == SectionType::DATA)
            hasData = true;
    }

    if (hasText) {
        *this->objdumpOut << this->textOut << std::endl;
    }
    if (hasRoData) {
        *this->objdumpOut << this->roDataOut << std::endl;
    }
    if (hasData) {
        *this->objdumpOut << this->dataOut << std::endl;
    }

    if (this->symbolTable.size() != 0) {
        *this->objdumpOut << "#symbol table" << std::endl << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "#name" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) 
                                                      << "section" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) 
                                                      << "value" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH)  
                                                      << "no" << std::endl;

        for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); ++it) {
            Symbol *s = it->second;
            
            std::string symbStr = s->toString();
            *this->objdumpOut << symbStr << std::endl;
        }
    }
    
    if (hasText && this->txtRelText.size() != 0) {
        *this->objdumpOut << "#.ret.text" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl; 

        for(auto line:this->txtRelText) {
            *this->objdumpOut << line << std::endl;
        }
    }

    if (hasRoData && this->txtRelROData.size() != 0) {
        *this->objdumpOut << "#.ret.rodata" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl; 
        for(auto line:this->txtRelROData) {
            *this->objdumpOut << line << std::endl;
        }
    }

    if (hasData && this->txtRelData.size() != 0) {
        *this->objdumpOut << "#.ret.data" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl;  
        for(auto line:this->txtRelData) {
            *this->objdumpOut << line << std::endl;
        }
    }

}

void Assembler::writeOutput() {
    ELFHeader header;
    header.entry = this->startAddress;

    ElfWord maxSecSize = MAX_SHORT;

    size_t currentOffset = sizeof(ELFHeader);


    bool hasText = false, 
         hasData = false, 
         hasRoData = false, 
         hasBss = false,
         hasSymTab = false,
         hasRelText = false,
         hasRelData = false,
         hasRelROData = false,
         hasStrTab = false;
    
    SectionHeader sectionHds[EXTENDED_SECTION_NUMBER];
    ElfWord sectionHdNum = 0;

    SectionHeader *textHd = nullptr,
                  *dataHd = nullptr,
                  *roDataHd = nullptr,
                  *bssHd = nullptr;

    for(int i = 0; i < SECTION_NUMBER && this->sectionOrder[i] != SectionType::UDF; ++i) {
        if (this->sectionOrder[i] == SectionType::TEXT) 
            hasText = true;

        if (this->sectionOrder[i] == SectionType::RO_DATA)
            hasRoData = true;
        
        if (this->sectionOrder[i] == SectionType::DATA)
            hasData = true;
        
        if (this->sectionOrder[i] == SectionType::BSS)
            hasBss = true;
    }

    if (hasText) {
        ;
        size_t size = this->textBin.size();
        if (size > maxSecSize) {
            throw AssemblingException("Section text exceeds maximum allowed size.");
        }

        Section *s = (Section*)this->symbolTable[".text"];

        sectionHds[sectionHdNum] = SectionHeader(s->getSectionCode(), s->getAccessRights(), currentOffset, (ElfWord)size, s->getAlign(), 0);

        currentOffset += sectionHds[sectionHdNum].size;

        if (currentOffset > maxSecSize) {
            throw AssemblingException("Output file exceeds maximum allowed size.");
        }
        ++sectionHdNum;
    }

    if (hasRoData) {

        size_t size = this->textBin.size();
        if (size > maxSecSize) {
            throw AssemblingException("Section rodata exceeds maximum allowed size.");
        }

        Section *s = (Section*)this->symbolTable[".rodata"];

        sectionHds[sectionHdNum] = SectionHeader(s->getSectionCode(), s->getAccessRights(), currentOffset, (ElfWord)size, s->getAlign(), 0);

        currentOffset += sectionHds[sectionHdNum].size;

        if (currentOffset > maxSecSize) {
            throw AssemblingException("Output file exceeds maximum allowed size.");
        }
        ++sectionHdNum;
    }

    if (hasData) {

        size_t size = this->textBin.size();
        if (size > maxSecSize) {
            throw AssemblingException("Section data exceeds maximum allowed size.");
        }

        Section *s = (Section*)this->symbolTable[".data"];

        sectionHds[sectionHdNum] = SectionHeader(s->getSectionCode(), s->getAccessRights(), currentOffset, (ElfWord)size, s->getAlign(), 0);
        
        currentOffset += sectionHds[sectionHdNum].size;

        if (currentOffset > maxSecSize) {
            throw AssemblingException("Output file exceeds maximum allowed size.");
        }
        ++sectionHdNum;
    }

    if (hasBss) {
        
        size_t size = this->textBin.size();
        if (size > maxSecSize) {
            throw AssemblingException("Section bss exceeds maximum allowed size.");
        }

        Section *s = (Section*)this->symbolTable[".bss"];

        sectionHds[sectionHdNum] = SectionHeader(s->getSectionCode(), s->getAccessRights(), currentOffset, (ElfWord)size, s->getAlign(), 0);
        
        currentOffset += sectionHds[sectionHdNum].size;

        if (currentOffset > maxSecSize) {
            throw AssemblingException("Output file exceeds maximum allowed size.");
        }
        ++sectionHdNum;
    }

    std::vector<SymTabEntry> symTabEntries;
    std::vector<std::string> symTabNames;
    ElfWord strTabSize;
    ElfWord symCnt = 0;
    hasSymTab = this->symbolTable.size() != 0;

    for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); ++it) {

        Symbol* s = it->second;
        
        ElfWord offset;
        if (s->getSectionPtr() != nullptr) {   
            offset = (ElfWord)(s->getOffset() - s->getSectionPtr()->getOffset());
        }
        else {
            offset = 0;
        }

        symTabEntries.push_back(SymTabEntry(symCnt, offset, s->getSectionCode(), s->getNo()));
        symTabNames.push_back(s->getName());

        strTabSize += (ElfWord)(sizeof(int) + s->getName().length()); 
        symCnt++;
    }

    sectionHds[sectionHdNum] = SectionHeader(SectionType::SYMB_TAB, Access::RD, currentOffset, (ElfWord)(symTabEntries.size() * sizeof(SymTabEntry)), 0, sizeof(SymTabEntry));
    currentOffset += sectionHds[sectionHdNum].size;
    ++sectionHdNum;
    
    SectionHeader *relTextHd = nullptr,
                  *relDataHd = nullptr,
                  *relRODataHd = nullptr;

    if (this->relText.size() != 0) {
        sectionHds[sectionHdNum] = SectionHeader(SectionType::REL_TEXT, Access::RD, currentOffset, (ElfWord)(this->relText.size() * sizeof(Relocation)), 0, sizeof(Relocation));
        currentOffset += sectionHds[sectionHdNum].size;
        hasRelText = true;
        ++sectionHdNum;
    }

    if (this->relROData.size() != 0) {
        sectionHds[sectionHdNum] = SectionHeader(SectionType::REL_RODATA, Access::RD, currentOffset, (ElfWord)(this->relROData.size() * sizeof(Relocation)), 0, sizeof(Relocation));
        currentOffset += sectionHds[sectionHdNum].size;
        hasRelROData = true;
        ++sectionHdNum;
    }

    if (this->relData.size() != 0) {
        sectionHds[sectionHdNum] = SectionHeader(SectionType::REL_DATA, Access::RD, currentOffset, (ElfWord)(this->relData.size() * sizeof(Relocation)), 0, sizeof(Relocation));
        currentOffset += sectionHds[sectionHdNum].size;
        hasRelData = true;
        ++sectionHdNum;
    }

    sectionHds[sectionHdNum] = SectionHeader(SectionType::STR_TAB, Access::RD, currentOffset, strTabSize + (ElfWord)(sizeof(int) * symTabNames.size()), 0, 0);
    currentOffset += sectionHds[sectionHdNum].size;
    hasStrTab = true;
    ++sectionHdNum;

    if (currentOffset > MAX_SHORT) {
        throw AssemblingException("Output file exceeds maximum allowed size.");
    }

    header.shOff = currentOffset;
    header.ehSize = sizeof(ELFHeader);
    header.shEntSize = sizeof(SectionHeader);
    header.shNum = sectionHdNum;

    this->output->write((char*)&header, sizeof(ELFHeader)); 

    if (hasText) {
        this->output->write(&this->textBin[0], this->textBin.size() * sizeof(char)); 
    }

    if (hasRoData) {
        this->output->write(&this->roDataBin[0], this->roDataBin.size() * sizeof(char)); 
    }

    if (hasData) {
        this->output->write(&this->dataBin[0], this->dataBin.size() * sizeof(char)); 
    }

    if (hasSymTab) {
        this->output->write((char*)&symTabEntries[0], sizeof(SymTabEntry) * symTabEntries.size());
    }

    if (hasRelText) {
        this->output->write((char*)&this->relText[0], sizeof(Relocation) * this->relText.size());
    }

    if (hasRelROData) {
        this->output->write((char*)&this->relROData[0], sizeof(Relocation) * this->relROData.size());
    }

    if (hasRelData) {
        this->output->write((char*)&this->relData[0], sizeof(Relocation) * this->relData.size());
    }

    if (hasStrTab) {
        for(int i = 0; i < symTabNames.size(); ++i) {
            unsigned int sz = symTabNames[i].length();

            this->output->write((char*)&sz, sizeof(int));
            this->output->write((char*)&symTabNames[i], sz);
        }
    }


    this->output->write((char*)&sectionHds[0], sizeof(SectionHeader) * sectionHdNum);
    
}