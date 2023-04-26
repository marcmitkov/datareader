//
// Created by lnarayan on 11/2/20.
//
#ifndef CHANEL_SYMBOLINFO_H
#define CHANEL_SYMBOLINFO_H
#if TWS 
#include "../TwsSocketClient/Contract.h"
#endif
#include <string>
using namespace std;
struct SymbolInfo {
    string m_exchange;
    string m_symbol;
    string m_symbolRoot;
    string m_frequencies;
#ifdef TWS
    Contract m_contract;  //TBD:  do we need just contId?
    ofstream* mp_tickfile;
#else
#endif
    string m_ekey;

};
#endif //CHANEL_SYMBOLINFO_H
