#pragma once
//#include <direct.h>
#include <string>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "../Base/decimal.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <sys/stat.h>
#include <boost/filesystem/operations.hpp>
#include <thread>
#include <mutex>
#if TWS
#define GetCurrentDir GetCurrentDirectory
#else
#define GetCurrentDir getcwd
#endif
using namespace std;


using namespace boost::posix_time;
using namespace boost::gregorian;
date OffsetDate(date d, int offset);
string GetCurrentWorkingDir(void);
void CreateDirectoryNew(string dir);
string FormatTime(ptime);
string PrintThreadID(string);
void ReadConfig(string configPath, string configFile);
string PosixTimeToStringFormat(ptime time, const char* timeformat);

ptime AdvanceTime(date base, int hour,  int min, int sec);
string GetDateDir(bool today, bool input);
string CurrentThreadId(const char* f);
std::string int64_to_string( int64_t value );

// global variable 
extern ptime gAsofDate;
extern ptime gTomorrow;
extern ptime gRoll;
extern int gRollHours;
extern int gRollMins;
extern ptime gEOD;
extern int gEODHours;
extern int gEODMins;
extern map<string, string> gConfigMap;

//  0:	livesim/live
//  1:	backtest reading candles
//  2:	backtest reading ticks
//  3:  generate candles from live date
extern int gMode;

extern bool eof(vector<ifstream*> vec);
extern vector<string> Split(const string &s, char delim);
extern bool To_bool(std::string str);
vector<string> split(const string &s, char delim);
void ConvertFloat(float timedecimal, float *hours, float *mins);
extern string formatDouble(double d,int prec);
extern ofstream ofglobal;

//set to true to write ticks to file
extern bool gWriteTicks;
extern string gOutputDirectory;
extern bool gDebug;
extern bool gSendOrder;
extern bool gBacktest;
extern bool gNewSignal;
extern bool gEnableMultipleBooks;

bool IsDST(ptime now);
template<typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}
template <class T>
void PrintVector(vector<T> a, int index, ostream& ofs) {
    /*
    for (auto it = a.begin(); it != a.end(); ++it)
    {
        ofs << *it << ",";
    }
    */
    for(size_t i=0;i < a.size(); i ++)
    {
        ofs << ((size_t)index==i?"*":"")<<  a[i] << ((size_t)index==i?"*":"") << ",";
    }
    ofs << endl;

}
class CrossStats{
        public:
            int numCrossed=0;
            double maxSpread=0;
            double meanSpread=0;
            ptime first;
            ptime last;
            int numTooWide=0;
            ptime firstWide;
            ptime lastWide;
        };

extern map<date, CrossStats> crossCountMap;
extern int gMarketType;
extern std::mutex gOrderMapTradeMutex;
extern std::mutex gOrderMapStrategyMutex;
extern std::mutex gPositionsMutex;
extern bool gWriteCandles;
extern bool gCancelKludge; //  cancelled orders
extern int gParamNumber;
extern bool gMarketHalted;
