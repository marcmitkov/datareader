//
// Created by lnarayan on 12/8/20.
//
#ifndef CHANEL_POSITION_H
#define CHANEL_POSITION_H
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../Base/decimal.h"
#include <string>
using namespace dec;
using namespace std;


/*  start enum related */
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

// This is the type that will hold all the strings.
// Each enumeration type will declare its own specialization.
// Any enum that does not have a specialization will generate a compiler error
// indicating that there is no definition of this variable (as there should be
// be no definition of a generic version).
template<typename T>
struct enumStrings
{
    static char const* data[];
};

// This is a utility type.
// Created automatically. Should not be used directly.
template<typename T>
struct enumRefHolder
{
    T& enumVal;
    enumRefHolder(T& enumVal1): enumVal(enumVal1) {}
};
template<typename T>
struct enumConstRefHolder
{
    T const& enumVal;
    enumConstRefHolder(T const& enumVal1): enumVal(enumVal1) {}
};

// The next two functions do the actual work of reading/writing an
// enum as a string.
template<typename T>
std::ostream& operator<<(std::ostream& str, enumConstRefHolder<T> const& data)
{
    return str << enumStrings<T>::data[data.enumVal];
}

template<typename T>
std::istream& operator>>(std::istream& str, enumRefHolder<T> const& data)
{
    std::string value;
    str >> value;

    // These two can be made easier to read in C++11
    // using std::begin() and std::end()
    //
    static auto begin  = std::begin(enumStrings<T>::data);
    static auto end    = std::end(enumStrings<T>::data);

    auto find   = std::find(begin, end, value);
    if (find != end)
    {
        data.enumVal = static_cast<T>(std::distance(begin, find));
    }
    return str;
}


// This is the public interface:
// use the ability of function to deduce their template type without
// being explicitly told to create the correct type of enumRefHolder<T>
template<typename T>
enumConstRefHolder<T>  enumToString(T const& e) {return enumConstRefHolder<T>(e);}

template<typename T>
enumRefHolder<T>       enumFromString(T& e)     {return enumRefHolder<T>(e);}

/*end enum related */



using namespace boost::posix_time;
class EntryCriteria {

public:
    enum EntryType {
        A,
        HL,
        LH,
        HH,
        LL,
        RHL,
        RLH,
        RHH,
        RLL,
        KR,
        RKR,
        CR0,
        OFFSET
    };
    enum StrategyName{
        STRATA,
    };
    static const char* getEntryType(int);
    static const char* getStrategyName(int);
    EntryCriteria() { _strategyName = STRATA;}
    StrategyName _strategyName;
    EntryType _type;
    Decimal _value;
    string _description;
    string ToString() const;
    static string Headers();
};
extern const char * entryTypes[];

class ExitCriteria {
public:
    ExitCriteria();
    ExitCriteria(ExitCriteria *ec);
    Decimal _stoplevel=Decimal(0.0);
    Decimal _targetlevel=Decimal(0.0);
    ptime _expiry;
    string ToString() const;
    static string Headers();
};



struct Fill {
    ptime TimeStamp;
    int FillSize;
    double Price;
};


class Candle;
class Position {
public:
    enum ExitCode { NA, STOP, TARGET, EXPIRE, EXIT, HARDSTOP, MISS, MISSPARTIAL, MISSSIGNAL, MISSEXPOSURE, EXITEXPOSURE, MISSCONSECUTIVE, CONTRA, SUPRESSEDSIGNAL, THROTTLED, AUTOFILL };
    static string getExitCodeString(ExitCode e);
    enum State { OpenPending, Open, ClosedPending, Closed, DeferredClose };


    bool  IsSignalExpired(vector<Candle* >*, int mins, Decimal mid);
    static string getStateString(State e);
    Position(int id, ptime ts, Decimal avgpx, int size);
    int Id;
    ptime TimeStamp;
    Decimal AvgPx;
    int Size;
    Decimal ClosePx;
    boost::posix_time::ptime CloseTimeStamp;
    Decimal PnlRealized;
    Decimal PnlUnrealized;
    State mState=OpenPending;

    // position  fill related
    int Remaining=INT_MAX;
    Decimal FillPx;
    boost::posix_time::ptime FillTimeStamp;

    ExitCode mExitCode=NA;

    //position close fill related
    int CloseRemaining=INT_MAX;
    Decimal CloseFillPx;
    boost::posix_time::ptime CloseFillTimeStamp;

    // entry/exit criteria
    EntryCriteria mEntryCriteria;
    ExitCriteria mExitCriteria;

    uint64_t OrderIdEntry=0;
    uint64_t OrderIdExit=0;

    //stats
    Decimal Slippage;

    string ToString() const;
    static int nextId;
    vector<Fill>  Fills;
    vector<Fill>  CloseFills;
    static string GetHeader();
    int Offset(Decimal closepx);
    void Pnl(bool first, Decimal nextpx  /* next position */, Decimal closepx /* offset */, int positionsize);
    int ComputeRemaining(); // not fully opened
    int ComputeNet();
    void ComputeAvgPx(bool first, Decimal closepx);
    Decimal PnL();
    Decimal PnLU(Decimal markpx);
    Decimal Target();
    Decimal Stop();
    void Close(ExitCode ec, Decimal);
    void Read(std::string iss);
};
#endif //CHANEL_POSITION_H
