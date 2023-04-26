#include "StrategyA.h"
#include "Instrument.h"
#include <string>
#include "../Base/IAdapterIn.h"
#include "../Base/Utilities.h"
#include <commonv2/Enum.h>
#include<boost/algorithm/string.hpp>
#include <algorithm>
#include "../Base/Utilities.h"


extern IAdapterOut *pTrader;

extern bool continuous;
extern map<string, map<string, string>> stratAparamMap;

//determine if trading halt applies
bool StrategyA::initTradingHalt(string n) {
    if (n.rfind("ES", 0) == 0 || n.rfind("NQ", 0) == 0 || n.rfind("YM", 0) == 0) {
        return true;
    }
    return false;
}

ptime Strategy::sHalt1;
ptime Strategy::sResume1;

void RunOnceStrategy() {
    Strategy::sHalt1 = AdvanceTime(gAsofDate.date(), 21, 14,
                                   59);  // non DST CME halt1  (applies only to equity indices)
    Strategy::sResume1 = AdvanceTime(gAsofDate.date(), 21, 30, 01);
    if (IsDST(gAsofDate)) {
        cout << "we are in DST " << to_simple_string(gAsofDate) << endl;
        Strategy::sHalt1 = AdvanceTime(gAsofDate.date(), 20, 14, 59);
        Strategy::sResume1 = AdvanceTime(gAsofDate.date(), 20, 30, 01);
    }
}

string StrategyA::GetParam(string param) {
    string retval;
    std::string n = _pMarketable->Name() + "_" + to_string(_moniker);
    if (stratAparamMap.count(n) > 0) {
        if (stratAparamMap[n].count(param) > 0)
            retval = stratAparamMap[n][param];
    }
    return retval;
}


StrategyA::StrategyA(Marketable *m, string name, string group, int moniker) : Strategy(m, name, group, moniker) {
    std::string n = m->Name() + "_" + to_string(moniker); //"DX  FMZ0020!"  AUD/USD  etc
    _tradingHalt = initTradingHalt(n);
    if (stratAparamMap.count(n) > 0) {
        if (stratAparamMap[n].count("ticksize") > 0)
            _ticksize = std::stod(stratAparamMap[n]["ticksize"]);
        else
            throw "specify ticksize";
        if (stratAparamMap[n].count("minlen") > 0)
            _minlen = std::stoi(stratAparamMap[n]["minlen"]);
        if (stratAparamMap[n].count("graphSpline") > 0)
            _graphSpline = std::stoi(stratAparamMap[n]["graphSpline"]);
        if (stratAparamMap[n].count("optimizeFit") > 0) {
            _optimizeFit = std::stoi(stratAparamMap[n]["optimizeFit"]);
            if (_optimizeFit) {
                cout << "will optmize fit using 3.17 " << endl;
            }
        }
        if (stratAparamMap[n].count("rho") > 0)
            _rho = std::stof(stratAparamMap[n]["rho"]);
        if (stratAparamMap[n].count("cushion") > 0)
            _cushion = Decimal(std::stof(stratAparamMap[n]["cushion"]));
        if (stratAparamMap[n].count("mode") > 0)
            _mode = std::stoi(stratAparamMap[n]["mode"]);
        if (stratAparamMap[n].count("riskfactor") > 0)
            _riskFactor = Decimal(std::stof(stratAparamMap[n]["riskfactor"]));
        if (stratAparamMap[n].count("contracts") > 0)
            _contracts = std::stoi(stratAparamMap[n]["contracts"]);
        // pilot testing related
        if (stratAparamMap[n].count("test") > 0)
            _test = std::stoi(stratAparamMap[n]["test"]);
        if (stratAparamMap[n].count("testBS") > 0)
            _testBS = std::stoi(stratAparamMap[n]["testBS"]);
        if (stratAparamMap[n].count("holdingPeriod") > 0)
            _holdingPeriod = std::stoi(stratAparamMap[n]["holdingPeriod"]);
        if (stratAparamMap[n].count("signalExpiry") > 0)
            _signalExpiry = std::stoi(stratAparamMap[n]["signalExpiry"]);
        if (stratAparamMap[n].count("aggressorin") > 0)
            _aggressorin = std::stoi(stratAparamMap[n]["aggressorin"]);
        if (stratAparamMap[n].count("aggressorout") > 0)
            _aggressorout = std::stoi(stratAparamMap[n]["aggressorout"]);
        if (stratAparamMap[n].count("mr") > 0)
            _mr = std::stoi(stratAparamMap[n]["mr"]);
        if (stratAparamMap[n].count("maxExposure") > 0)
            _maxExposure = std::stoi(stratAparamMap[n]["maxExposure"]);
        if (stratAparamMap[n].count("scalefactor") > 0)
            _scalefactor = std::stoi(stratAparamMap[n]["scalefactor"]);
        if (stratAparamMap[n].count("clearscaling") > 0)
            _clearscaling = std::stoi(stratAparamMap[n]["clearscaling"]);
        if (stratAparamMap[n].count("sessionStart") > 0) {
            //MR NewCmake
            float h, min;
            ConvertFloat(std::stof(stratAparamMap[n]["sessionStart"]), &h, &min);
            _sessionStart = ptime(gAsofDate.date(), hours((long) h) + minutes((long) min) + seconds(0));
            cout << "session will start at " << to_simple_string(_sessionStart) << endl;
        }
        if (stratAparamMap[n].count("payup") > 0)
            _payup = std::stod(stratAparamMap[n]["payup"]);
        else // naive payup
        {
            if (strstr(name.c_str(), "JPY") != NULL) {
                _payup = 0;
            } else {
                _payup = 0;
            }
        }
        if (stratAparamMap[n].count("spreadtheshold") > 0)
            _spreadthreshold = std::stod(stratAparamMap[n]["spreadtheshold"]);
        else // naive payup
        {
            if (strstr(name.c_str(), "JPY") != NULL) {
                _spreadthreshold = 20;
            } else {
                _spreadthreshold = 20;
            }
        }
        if (stratAparamMap[n].count("usemid") > 0)
            _usemid = std::stoi(stratAparamMap[n]["usemid"]);
        else
            _usemid = 0;

        if (stratAparamMap[n].count("sessionEnd") > 0) {
            float h, mi;
            ConvertFloat(std::stof(stratAparamMap[n]["sessionEnd"]), &h, &mi);
            _sessionEnd = ptime(gAsofDate.date(), hours((long) h) + minutes((long) mi) + seconds(0));
            cout << "session will end at " << to_simple_string(_sessionEnd) << endl;
        }
        if (stratAparamMap[n].count("toowide") > 0) // should be specified in ticks
            _toowide = Decimal(std::stod(stratAparamMap[n]["toowide"])) * _ticksize;
        if (stratAparamMap[n].count("stop") > 0)
            _stopsize = Decimal(std::stod(stratAparamMap[n]["stop"])) * _ticksize;
        if (stratAparamMap[n].count("target") > 0)
            _targetsize = Decimal(std::stod(stratAparamMap[n]["target"])) * _ticksize;

        if (stratAparamMap[n].count("h2l2stopx") > 0)
            _h2l2stopx = std::stof(stratAparamMap[n]["h2l2stopx"]);
        else
            _h2l2stopx = 2;

        if (stratAparamMap[n].count("skipmissedsignal") > 0)
            _skipmissedsignal = std::stoi(stratAparamMap[n]["skipmissedsignal"]);

        if (stratAparamMap[n].count("continuous") > 0)
            _continuous = std::stoi(stratAparamMap[n]["continuous"]);

        if (stratAparamMap[n].count("tradeable") > 0)
            _tradeable = std::stoi(stratAparamMap[n]["tradeable"]);

        if (stratAparamMap[n].count("aggressoradd") > 0)
            _aggressoradd = std::stoi(stratAparamMap[n]["aggressoradd"]);

        if (stratAparamMap[n].count("aggressorfade") > 0)
            _aggressorfade = std::stoi(stratAparamMap[n]["aggressorfade"]);

        if (stratAparamMap[n].count("scalePositions") > 0)
            _scalePositions = std::stoi(stratAparamMap[n]["scalePositions"]);

        if (stratAparamMap[n].count("rank1") > 0)  //HL adjustment
        {
            _useRank1 = std::stoi(stratAparamMap[n]["rank1"]);
            cout << "rank1 is enabled for " << n << endl;
        }

        if (stratAparamMap[n].count("rank2") > 0)  //HL adjustment
        {
            _useRank2 = std::stoi(stratAparamMap[n]["rank2"]);
            cout << "rank2 is enabled for " << n << endl;
        }

        if (stratAparamMap[n].count("sr") > 0)  //HL adjustment
        {
            _useSR = std::stoi(stratAparamMap[n]["sr"]);
            cout << "sr is enabled for " << n << endl;
        }

        if (stratAparamMap[n].count("scale") > 0)
            _scale = std::stoi(stratAparamMap[n]["scale"]);

        if (stratAparamMap[n].count("scaleMultiplier") > 0)
            _scaleMultiplier = std::stoi(stratAparamMap[n]["scaleMultiplier"]);

        Instrument *ins = static_cast<Instrument *>(_pMarketable);
        if (stratAparamMap[n].count("candleQuantum") > 0) {
            _candleQuantum = std::stoi(stratAparamMap[n]["candleQuantum"]);
        }
        if (stratAparamMap[n].count("candleFrequency") > 0) {
            _freq = std::stod(stratAparamMap[n]["candleFrequency"]);
            cout << "candle frequency overridden for " << n << " to " << to_string(_freq) << endl;
        } else {
            _freq = moniker;
            cout << "candle frequency set to moniker for " << n << " to " << to_string(_freq) << endl;
        }
        if (_candleQuantum != 0) {
            ins->AddCandleMaker(_candleQuantum, true);
            _logA << "Created quantum candlemaker with value " << _candleQuantum << endl;
        } else {
            ins->AddCandleMaker(_freq);
            _logA << "Created regular candlemaker with value " << to_string(_freq) << endl;
        }

        if (stratAparamMap[n].count("a") > 0) {
            if (std::stoi(stratAparamMap[n]["a"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::A] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::A)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("rhl") > 0) {
            if (std::stoi(stratAparamMap[n]["rhl"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::RHL] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::RHL)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("rlh") > 0) {
            if (std::stoi(stratAparamMap[n]["rlh"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::RLH] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::RLH)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("hl") > 0) {
            if (std::stoi(stratAparamMap[n]["hl"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::HL] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::HL)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("lh") > 0) {
            if (std::stoi(stratAparamMap[n]["lh"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::LH] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::LH)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("hh") > 0) {
            if (std::stoi(stratAparamMap[n]["hh"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::HH] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::HH)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("ll") > 0) {
            if (std::stoi(stratAparamMap[n]["ll"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::LL] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::LL)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("rhh") > 0) {
            if (std::stoi(stratAparamMap[n]["rhh"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::RHH] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::RHH)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("rll") > 0) {
            if (std::stoi(stratAparamMap[n]["rll"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::RLL] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::RLL)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("kr") > 0) {
            if (std::stoi(stratAparamMap[n]["kr"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::KR] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::KR)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("rkr") > 0) {
            if (std::stoi(stratAparamMap[n]["rkr"]) == 0) {
                _supressedSignalsMap[EntryCriteria::EntryType::RKR] = true;
                cout << "signal is supressed at init " << EntryCriteria::getEntryType(EntryCriteria::EntryType::RKR)
                     << endl;
            }
        }
        if (stratAparamMap[n].count("signals") > 0) {
            // suppress all
            for (int i = 0; i < EntryCriteria::OFFSET + 1; i++) {
                _supressedSignalsMap[(EntryCriteria::EntryType) i] = true;
            }
            string s = stratAparamMap[n]["signals"].c_str();
            if (s == "CR0") {
                _circadianRun = true;
            }
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) -> unsigned char { return std::toupper(c); });
            boost::tokenizer<> tok(s);
            for (boost::tokenizer<>::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
                cout << *beg << "\n";
                for (int i = 0; i < EntryCriteria::OFFSET + 1; i++) {
                    if (*beg == entryTypes[i])
                        _supressedSignalsMap.erase((EntryCriteria::EntryType) i);
                }
            }
        }
        if (stratAparamMap[n].count("intv") > 0) {
            _intv = std::stoi(stratAparamMap[n]["intv"]);
            cout << "current average interval volume is " << _intv << endl;
        } else {
            if (_candleQuantum > 0) {
                _intv = _candleQuantum;
                cout << "Interval value is defauled to candleQuantum" << _intv;
            }
        }
        if (stratAparamMap[n].count("intvMultiplier") > 0) {
            _intvMultiplier = std::stof(stratAparamMap[n]["intvMultiplier"]);
            /*
            if (_pMarketable->IsFX() && !gBacktest) {
                _intvMultiplier = _intvMultiplier * 2;
            }
            */
            cout << "current average interval volume multiplier is " << _intvMultiplier << endl;
        }
        if (stratAparamMap[n].count("modeKR") > 0) {
            _modeKR = std::stoi(stratAparamMap[n]["modeKR"]);
            cout << "modeKR is " << _modeKR << endl;
        }
        if (stratAparamMap[n].count("stopKR") > 0) {
            _stopKR = std::stoi(stratAparamMap[n]["stopKR"]);
            cout << "stopKR is " << _stopKR << endl;
        }
        if (stratAparamMap[n].count("multiplierKR") > 0) {  // covers the case when both are same
            _multiplierKRstop = std::stoi(stratAparamMap[n]["multiplierKR"]);
            _multiplierKRtarget = std::stoi(stratAparamMap[n]["multiplierKR"]);
            cout << "BOTH multiplierKRtarget is " << _multiplierKRtarget << endl;
            cout << "AND multiplierKRstop is " << _multiplierKRstop << endl;
        }
        if (stratAparamMap[n].count("multiplierKRstop") > 0) {
            _multiplierKRstop = std::stoi(stratAparamMap[n]["multiplierKRstop"]);
            cout << "multiplierKRstop is " << _multiplierKRstop << endl;
        }
        if (stratAparamMap[n].count("multiplierKRtarget") > 0) {
            _multiplierKRtarget = std::stoi(stratAparamMap[n]["multiplierKRtarget"]);
            cout << "multiplierKRtarget is " << _multiplierKRtarget << endl;
        }
        if (stratAparamMap[n].count("lookbackKR") > 0) {
            _lookbackKR = std::stoi(stratAparamMap[n]["lookbackKR"]);
            cout << "lookbackKR is " << _lookbackKR << endl;
        }
        if (stratAparamMap[n].count("trendMonikerKR") > 0) {
            _trendMonikerKR = std::stoi(stratAparamMap[n]["trendMonikerKR"]);
            cout << "trendMonikerKR is " << _trendMonikerKR << endl;
        }
        InitDate(name);
    } else {
        _paramsLoaded = false;
    }
    _ofsCandles << Candle::GetHeader() << endl;
    // below to debug quantum candles
    //_ofsCandles << Candle::GetHeader() << "," << Candle::GetHeaderEx() << endl;
    cout << "Chanel:  created strategy " << name << " " << FrequencyString();
    _logA << " Active signals: ";
    cout << " Active signals: ";
    for (int k = 0; k < EntryCriteria::EntryType::OFFSET; k++) {
        if (_supressedSignalsMap.count((EntryCriteria::EntryType) k) == 0) {
            _logA << EntryCriteria::getEntryType(k) << ",";
            cout << EntryCriteria::getEntryType(k) << ",";
            if (k != EntryCriteria::EntryType::CR0 && k != EntryCriteria::EntryType::KR &&
                k != EntryCriteria::EntryType::RKR) {
                cout << "This is turning point signal hence splines will be recorded and classified on each update"
                     << endl;
                _classifyTP = true;
                _computeTP = true;
            }
        }
    }
    if (_modeKR == 1 || _modeKR == 2) {
        cout << "Mode KR is " << _modeKR << " hence splines will be computed / recorded on each update" << endl;
        _computeTP = true;
    }
    _logA << endl;
    cout << endl;
}

bool replace(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void StrategyA::NotifyExposure(Position *p, int action, int filled) {
    if (_maxExposureStrategyMap.count(p->mEntryCriteria._type) == 0) {
        _maxExposureStrategyMap[p->mEntryCriteria._type] = 0;
    }
    if (_currentExposureStrategyMap.count(p->mEntryCriteria._type) == 0) {
        _currentExposureStrategyMap[p->mEntryCriteria._type] = 0;
    }
    if (_maxExposureStrategyMap.count(p->mEntryCriteria._type) > 0 &&
        _currentExposureStrategyMap.count(p->mEntryCriteria._type) > 0) {
        int exposure = _currentExposureStrategyMap[p->mEntryCriteria._type];
        if (action == 0) {

            exposure += filled;

        } else {

            exposure -= filled;
        }
        _currentExposureStrategyMap[p->mEntryCriteria._type] = exposure;
        if (abs(exposure) > abs(_maxExposureStrategyMap[p->mEntryCriteria._type]))
            _maxExposureStrategyMap[p->mEntryCriteria._type] = exposure;

    }
}

void StrategyA::SwitchDate(ptime newdate) {
    string oldDateStr = PosixTimeToStringFormat(_currentDate, "%Y%m%d");;
    string newDateStr = PosixTimeToStringFormat(newdate, "%Y%m%d");

    cout << "switching date from  " << oldDateStr << " to " << newDateStr << endl;

    string fn = _name;
    std::replace(fn.begin(), fn.end(), '/', '_');
    std::replace(fn.begin(), fn.end(), ' ', '_');

    if (replace(_currentDir, oldDateStr, newDateStr)) {
        OpenLogs();
        _currentDate = newdate;
    } else
        cerr << "error switching date from  " << oldDateStr << " to " << newDateStr << endl;
}

void StrategyA::InitDate(string name) {
    _currentDir = gOutputDirectory + "/";
    _currentDate = gAsofDate;
    string yyyymmdd = PosixTimeToStringFormat(_currentDate, "%Y%m%d");
    cout << "name:" << name << " date:" << yyyymmdd << endl;
    if (_name.find("/") != std::string::npos) {
        string first = name.substr(0, 3);
        string second = name.substr(4, 3);
        _currentDir += first + second + "/" + FrequencyString() + "/params-" + to_string(gParamNumber) + "/" + yyyymmdd;
    } else if (_name.find(" ") != std::string::npos)  // ICE symbol "DX  FMZ0020!"  for example
    {
        int loc = _name.find(" ");
        _currentDir +=
                name.substr(0, loc) + "/" + FrequencyString() + "/params-" + to_string(gParamNumber) + "/" + yyyymmdd;
    } else {
        _currentDir +=
                name.substr(0, 2) + "/" + FrequencyString() + "/params-" + to_string(gParamNumber) + "/" + yyyymmdd;
        _suffixFutures = name.substr(2, 2);
    }
    bool exists = true;
    struct stat info;
    if (stat(_currentDir.c_str(), &info) != 0) {
        printf("cannot access %s\n", _currentDir.c_str());
        exists = false;
    }
    if (exists == false) {
        if (gDebug)
            cout << "trying to create directories " << _currentDir << endl;
        if (boost::filesystem::create_directories(_currentDir.c_str())) {
            if (gDebug)
                cout << "Directory created " << _currentDir << endl;
        } else {
            cout << "Unable to create directory " << _currentDir << endl;
        }
    }

    OpenLogs();  //  all subsequen messages can goto  _logA

    string ydirpos;
    for (int k = gLookbackDays; k > 0; k--) {
        date yesterday = OffsetDate(_currentDate.date(), -k);
        string yyyymmddYesterday = PosixTimeToStringFormat(ptime(yesterday), "%Y%m%d");
        string ydir = replace_all_copy(_currentDir, yyyymmdd, yyyymmddYesterday);
        if (k == 1)
            ydirpos = ydir;
        string file = ydir + +"/candles" + _suffixFutures + ".txt";
        ReadCandles(file);
        if (k == gLookbackDays) {
            if (gBacktest && gDeleteCandles && gParamNumber != 0) {  // do not remove candles for paran# 0 for now
                if (boost::filesystem::remove(file))
                    _logA << "removed file: " << file << endl;
                else
                    _logA << "remove failed: " << file << endl;
            }
        }
    }
    ReadOpenPositions(ydirpos + +"/openpositions" + ".txt");
}

void StrategyA::RunCandles() {
    ReadCandles(_currentDir + +"/candles" + _suffixFutures + ".txt");
}

void StrategyA::LoopCandles() {
    string file1 = _currentDir + +"/candles" + _suffixFutures + ".txt";
    date yesterday = OffsetDate(_currentDate.date(), -1);
    string yyyymmddYesterday = PosixTimeToStringFormat(ptime(yesterday), "%Y%m%d");
    string yyyymmdd = PosixTimeToStringFormat(_currentDate, "%Y%m%d");
    string ydir = replace_all_copy(_currentDir, yyyymmdd, yyyymmddYesterday);
    string file0 = ydir + +"/candles" + _suffixFutures + ".txt";


    cout << "trying to read candles for " << file1 << endl;
    _logA << "trying to read candles for " << file1 << endl;
    int retval = 0;
    std::ifstream infile(file1);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            if (retval == 0) {
                retval++;
                continue; //skip first line
            }
            auto cdl = new Candle(line);
            try {
                HandleCandle(*cdl);
            }
            catch (const char *msg) {
                cerr << "EXCEPTION on: " << msg << endl;
            }
            retval++;
        }
    } else {
        cerr << "could not open file " << file1 << endl;
    }
    cout << "read candles # " << retval << endl;
    _logA << "read candles # " << retval << endl;
    return;
}

int StrategyA::ReadOpenPositions(string file) {
    cout << "trying to read openpositions for " << file << endl;
    _logA << "trying to read openpositions for " << file << endl;
    int retval = 0;
    vector<Position *> openPending;
    vector<Position *> closedPending;
    std::ifstream infile(file);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {

            auto pos = new Position(Position::nextId++, gAsofDate, Decimal(0.0), 0);
            try {
                pos->Read(line);
            }
            catch (const char *msg) {
                cerr << "EXCEPTION on: " << msg << endl;
            }
            if (pos->mState == Position::Open
                || (pos->mState == Position::OpenPending &&
                    !pos->FillTimeStamp.is_not_a_date_time())/*at least partially filled*/
                || (pos->mState == Position::ClosedPending &&
                    !pos->CloseFillTimeStamp.is_not_a_date_time())/*at least partially filled*/
                    ) {
                _positions.push_back(pos);
                _openpositions.push_back(pos);
                if (pos->mState == Position::OpenPending) {
                    // TODO: handle partial open and close
                    _logA << "read parially open position " << pos->ToString() << endl;
                } else if (pos->mState == Position::ClosedPending) {
                    // TODO: handle partial open and close
                    _logA << "read parially closed position " << pos->ToString() << endl;
                }
            } else if (pos->mState == Position::OpenPending) {
                openPending.push_back(pos);
                // if MISSEXPOSURE then due to maxexposure contraint otherwise unfilled
                _logA << "open pending skipped during read for reason " << Position::getExitCodeString(pos->mExitCode)
                      << " >> " << pos->ToString() << endl;
            } else if (pos->mState == Position::ClosedPending) {
                closedPending.push_back(pos);
                _logA << "close pending skipped during read for reason " << Position::getExitCodeString(pos->mExitCode)
                      << " >> " << pos->ToString() << endl;
            }
        }
    } else {
        cerr << "could not open file " << file << endl;
    }

    bool allfound = true;
    _logA << closedPending.size() << "  close pending positions found for offset " << endl;
    for (auto &cp:  closedPending) {
        Position *dup = 0;  // created in case offsetting position not found
        // find an offsetting position in openpositions
        auto it = _openpositions.begin();
        bool found = false;
        for (; it != _openpositions.end();) {
            if (cp->Size == -(*it)->Size) {
                _logA << "found offsetting position at read. closing... " << endl;
                (*it)->ClosePx = cp->AvgPx;
                (*it)->CloseFillPx = cp->FillPx;
                (*it)->mExitCode = Position::EXITEXPOSURE;
                (*it)->mState = Position::Closed;
                _logA << "erasing open position at read " << (*it)->ToString() << endl;
                it = _openpositions.erase(it);
                found = true;
                break;
            } else {
                ++it;
            }
        }
        if (!found) {
            _logA << "open positions: cannot find offsetting position at read for " << cp->ToString() << endl;
            Position *last = 0;
            if (_openpositions.size() > 0) {
                _logA << "replicating the last open position at read" << endl;
                last = _openpositions.back();
            } else {
                _logA << "no open position to replicate at read" << endl;
            }
            // duplicate the last position
            dup = new Position(0, gAsofDate, Decimal(0.0), 0);
            if (last != 0) {
                *dup = *last;
            } else  // case when no open positions exist to replicate from
            {
                if (closedPending.size() > 0) {
                    _logA << "using last closed pending to replicate " << closedPending.back()->ToString() << endl;
                    dup->Size = closedPending.back()->Size;
                    // dup->AvgPx = dup->Size>0?_pMarketable->_ask:_pMarketable->_bid; // TODO: make sure this is non zero
                    dup->AvgPx = closedPending.back()->AvgPx;
                    dup->FillPx = closedPending.back()->FillPx;  // used to compute  unrealized pnl etc
                    dup->FillTimeStamp = closedPending.back()->FillTimeStamp;
                    if (dup->AvgPx == Decimal(0)) {
                        cerr << "CRITICAL:  price not available?" << endl;
                    }
                    dup->mState = Position::Open;
                    date d = gAsofDate.date();

                    ptime expiry(d, hours(19) + minutes(0) + seconds(0));  // TOSD: reexamine

                    dup->mExitCriteria._expiry = expiry; // expire it today  TODO: probably with tight stops
                    cout << to_simple_string(expiry) << endl;
                    dup->Remaining = 0;
                    dup->CloseRemaining = abs(
                            dup->Size);   // this is usually filled when the position is opened / filled

                    dup->mEntryCriteria._description = "OPEN DUPLICATED ";
                    dup->mEntryCriteria._type = closedPending.back()->mEntryCriteria._type;


                    dup->mExitCriteria._stoplevel =
                            dup->AvgPx - (dup->Size > 0 ? _stopsize : -_stopsize);
                    dup->mExitCriteria._targetlevel =
                            dup->AvgPx + (dup->Size > 0 ? _targetsize : -_targetsize);
                    dup->mEntryCriteria._value = closedPending.back()->Id;

                } else {
                    cerr << "CRITICAL: makes no sense" << endl;
                    _logA << "CRITICAL: makes no sense" << endl;
                }

            }
            dup->Id = Position::nextId++;
            _openpositions.push_back(dup);
            _positions.push_back(dup);
            _logA << " duplicated and added to positions and open positions " << dup->ToString() << endl;
        }
    }
    int net = 0;
    if (true) {
        for (auto &op:  _openpositions) {
            pTrader->AddExposure(_pMarketable->Id, op->Size);
            retval++;
            net += op->Size;
        }
    }
    _logA << "net open position is " << net << endl;
    cout << "read open positions # " << retval << endl;
    _logA << "read open positions # " << retval << endl;
    return retval;
}


vector<Position *> StrategyA::ReduceOpenPositionsEx(int numop) {
    _logA << "will try to reduce open positions by " << numop << " for " << Name() << "_" << Moniker() << endl;
    vector<Position *> retval;
    int numfound=0;
    auto it = _openpositions.begin();
    for (; it != _openpositions.end();) { // go through open positions
        bool cond = ((*it)->mExitCode != Position::SUPRESSEDSIGNAL && (*it)->mExitCode != Position::MISSEXPOSURE);
        if (sgn<int>(numop) == sgn<int>((*it)->Size) && (*it)->mState == Position::Open &&
            cond) {  //  if -ive try to find open short positions to match long that is expiring but cannot be can not be closed
            _logA << "will erase from openpositions  " << (*it)->ToString() << endl;
            numfound+=(*it)->Size;
            retval.push_back(*it);
            it = _openpositions.erase(it);

        } else {
            ++it;
        }
        if(numop == numfound)
            break;
    }
    _logA << "reduced " << retval.size() << endl;
    return retval;
}

int StrategyA::CollapseExpired(vector<Position *>& collapsewith /* list that contains open positions */)
{
    int retval=0;
    for(auto it = collapsewith.begin();  it != collapsewith.end(); ++it) {

        for (auto &pos : _positions)  // find an expired position to collapse
        {
            if (pos->mExitCriteria._expiry.date() == gAsofDate.date() &&
                (*it)->Size == -pos->Size)  // match positions of same size/sign
            {
                if (pos->mExitCode == Position::CONTRA) {
                    _logA << "in collapse expired found a matching open position to collapse  " << (*it)->ToString() << endl;
                    _logA << "with the expiring position " << endl;
                    _logA << pos->ToString() << endl;
                    pos->ClosePx = (*it)->AvgPx;
                    pos->CloseFillPx = (*it)->FillPx;
                    pos->mExitCode = Position::EXITEXPOSURE;
                    retval++;
                    _logA << (pos->Size > 0 ? "LONG " : "SHORT ")
                          << " in reduce  Position could not be expired so collapsed with "
                          << ((*it)->Size > 0 ? "LONG " : "SHORT ") << endl;
                    break;
                }
            }
        }
    }
    return retval;
}

// this is done at expiry
// we will collapse certain  open positions with positions expired today
//  +ive numop means that a short positon cannot be closed so we will find an existing long open positions
// and collapse the 2  and since the constraint is going long  are plenty long positions to square with
int StrategyA::ReduceOpenPositions(int numop) {
    _logA << "will try to reduce open positions by " << numop << " for " << Name() << "_" << Moniker() << endl;
    int retval = 0;

    auto it = _openpositions.begin();
    for (; it != _openpositions.end();) { // go through open positions
        bool cond = ((*it)->mExitCode != Position::SUPRESSEDSIGNAL && (*it)->mExitCode != Position::MISSEXPOSURE);
        if (sgn<int>(numop) == sgn<int>((*it)->Size) && (*it)->mState == Position::Open &&
            cond) {  //  if -ive try to find open short positions to match long that is expiring but cannot be can not be closed
            _logA << "attempting to reduce " << (*it)->ToString() << endl;
            bool found = false;
            for (auto &pos : _positions)  // find an expired position to collapse
            {
                if (pos->mExitCriteria._expiry.date() == gAsofDate.date() &&
                    (*it)->Size == -pos->Size)  // match positions of same size/sign
                {
                    if (pos->mExitCode == Position::CONTRA) {
                        _logA << "in reduce  found a matching open position to collapse  " << (*it)->ToString() << endl;
                        _logA << "with the expiring position " << endl;
                        _logA << pos->ToString() << endl;
                        pos->ClosePx = (*it)->AvgPx;
                        pos->CloseFillPx = (*it)->FillPx;
                        pos->mExitCode = Position::EXITEXPOSURE;
                        retval += -pos->Size;
                        _logA << (pos->Size > 0 ? "LONG " : "SHORT ")
                              << " in reduce  Position could not be expired so collapsed with "
                              << ((*it)->Size > 0 ? "LONG " : "SHORT ") << endl;
                        found = true;
                        break;
                    }
                }
            }
            if (found) {  //  remove the position ONLY if a matching contra (expired)  was actually found and adjusted  (crossed)
                //  erase from positions FIRST
                auto it2 = _positions.begin();
                for (; it2 != _positions.end();) {
                    if ((*it)->Id == (*it2)->Id) {
                        int id = (*it2)->Id;
                        _logA << "ERASING from positions in reduce " << (*it2)->ToString() << endl;
                        it2 = _positions.erase(it2);
                        _logA << "removed position id in reduce " << id << endl;
                        break;
                    } else {
                        ++it2;
                    }
                }
                // then erase it from open positions!
                it = _openpositions.erase(it);
            } else {
                ++it;
            }
            if (retval == numop) {
                break;
            }
        } else {
            ++it;
        }
    }
    _logA << "reduced " << retval << endl;
    return retval;
}

int StrategyA::ReadCandles(string file) {
    cout << "trying to read candles for " << file << endl;
    _logA << "trying to read candles for " << file << endl;
    int retval = 0;
    std::ifstream infile(file);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            if (retval == 0) {
                retval++;
                continue; //skip first line
            }
            auto cdl = new Candle(line);
            try {
                Instrument *ins = static_cast<Instrument *>(_pMarketable);
                vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);
                cdls->push_back(cdl);
            }
            catch (const char *msg) {
                cerr << "EXCEPTION on: " << msg << endl;
            }
            retval++;
        }
    } else {
        cerr << "could not open file " << file << endl;
    }
    cout << "read candles # " << retval << endl;
    _logA << "read candles # " << retval << endl;
    infile.close();
    /*
    if (gBacktest && gDeleteCandles) {
        if (boost::filesystem::remove(file))
            _logA << "removed file: " << file << endl;
        else
            _logA << "remove failed: " << file << endl;
    }
    */
    return retval;
}

void StrategyA::OpenLogs() {
    string fn = _name;
    std::replace(fn.begin(), fn.end(), '/', '_');
    std::replace(fn.begin(), fn.end(), ' ', '_');
    /*
    int loc = name.find(" ");
    if(name ==  "DX  FMZ0020!")
    {
        cout << "expected"  << endl;
    }
    string fn = _name;
    if (loc != std::string::npos) {
        fn = fn.substr(0, loc);
    }
    */

    if (true || gDebug)
        cerr << "about to open " << fn << " @ " << to_simple_string(gAsofDate) << endl;


    fn = _currentDir + "/stratA" + "_" + fn + "_" + to_string(_moniker) + ".txt";
    if (gDebug)
        cout << "about to open " << fn << endl;
    if (_logA.is_open())
        _logA.close();
    _logA.open(fn, std::ofstream::out);
    fn = _currentDir + "/candles" + _suffixFutures + ".txt";
    if (_ofsCandles.is_open())
        _ofsCandles.close();
    _ofsCandles.open(fn, std::ofstream::out);
    fn = _currentDir + "/positions.txt";
    if (_ofsPositions.is_open())
        _ofsPositions.close();
    _ofsPositions.open(fn, gBacktest ? std::ofstream::out : (gWorkerThread ? std::ofstream::out : std::ofstream::app));
    fn = _currentDir + "/openpositions.txt";
    if (_ofsOpenPositions.is_open())
        _ofsOpenPositions.close();
    _ofsOpenPositions.open(fn,
                           gBacktest ? std::ofstream::out : (gWorkerThread ? std::ofstream::out : std::ofstream::app));
    fn = _currentDir + "/executions.txt";
    if (_ofsExecutions.is_open())
        _ofsExecutions.close();
    _ofsExecutions.open(fn, gBacktest ? std::ofstream::out : std::ofstream::app);
    _ofsExecutions << OrderDetails::GetHeader() << endl;

    fn = _currentDir + "/summary.txt";
    if (_ofsSummary.is_open())
        _ofsSummary.close();
    cerr << "about to open " << fn << "  @ " << to_simple_string(gAsofDate) << endl;
    _ofsSummary.open(fn, gBacktest ? std::ofstream::out : std::ofstream::app);

    fn = _currentDir + "/pnl.txt";
    if (_ofsPnL.is_open())
        _ofsPnL.close();
    cerr << "about to open " << fn << "  @ " << to_simple_string(gAsofDate) << endl;
    _ofsPnL.open(fn, gBacktest ? std::ofstream::out : std::ofstream::app);

    fn = _currentDir + "/spline.txt";
    if (_ofsSpline.is_open())
        _ofsSpline.close();
    cerr << "about to open " << fn << "  @ " << to_simple_string(gAsofDate) << endl;
    _ofsSpline.open(fn, gBacktest ? std::ofstream::out : std::ofstream::app);

}


int StrategyA::ExpireInterDay(Position *pos) {

    if (pos->mState == Position::OpenPending && pos->OrderIdEntry != 0) {
        //cancel outstanding order
        _logA << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
              << pos->Id << endl;
        cout << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
             << pos->Id << endl;
        pTrader->CancelOrder(pos->OrderIdEntry);
        _unfilled++;
    } else if (pos->mState == Position::ClosedPending && pos->OrderIdExit != 0) {
        //cancel outstanding order
        _logA << "canceling ClosedPending order before offset " << pos->OrderIdExit
              << "  for position "
              << pos->Id << endl;
        cout << "canceling ClosedPending order before offset " << pos->OrderIdExit << "  for position "
             << pos->Id << endl;
        pTrader->CancelOrder(pos->OrderIdExit);
        _unfilled++;
    }
    if (pos->mExitCode != Position::SUPRESSEDSIGNAL && pos->mExitCode != Position::MISSEXPOSURE) {
        if (pos->mState == Position::Closed)  // positions closed early intraday
            pos->PnlRealized = pos->PnL();
    }
    return 0;
}

//  return the amount of position about to expire at roll time
int StrategyA::NetExpiry() {
    int retval = 0;
    int numopen = 0;
    int closedpending = 0;
    for (auto &pos : _openpositions) {
        if (pos->mExitCriteria._expiry.is_not_a_date_time()) {
            cout << "CRITICAL expiry not set" << endl;
            continue;
        }

        bool cond = pos->mExitCriteria._expiry.date() <= gRoll.date();      // consistent expiry criteria (Sep 9. 2021)
        cond = cond && (pos->mExitCode != Position::SUPRESSEDSIGNAL && pos->mExitCode != Position::MISSEXPOSURE);
        cond = cond && pos->mState != Position::Closed;
        if (!cond) {
            if (!gBacktest) {
                cout << "Expiry " << to_simple_string(pos->mExitCriteria._expiry) << "  Roll "
                     << to_simple_string(gRoll)
                     << endl;
                cout << "not including in Net() count for purpose of close " << pos->ToString() << endl;
            }
            if (pos->mExitCode != Position::MISSEXPOSURE)
                numopen += pos->Size;  // these are not expiring today
            if (pos->mState == Position::ClosedPending)
                closedpending++;
        } else {
            _logA << "expiring " << pos->ToString() << endl;
        }
        if (!pos->mExitCriteria._expiry.is_not_a_date_time() && cond) {
            if (pos->mState == Position::OpenPending) {
                if (pos->Remaining == INT_MAX)
                    retval += 0; // no fills were received so position did not open
                else
                    retval += (pos->Size > 0) ? pos->Size - pos->Remaining : pos->Size + pos->Remaining;
            } else if (pos->mState ==
                       Position::Open) {   // includes case when order was not sent due to exposure contraint
                if (pos->CloseRemaining == INT_MAX)  // no fills received yet
                    retval += pos->Size;
                else
                    retval += pos->Size > 0 ? pos->CloseRemaining : -pos->CloseRemaining;  //partially closed
            } else if (pos->mState == Position::ClosedPending) {
                if (pos->mExitCode ==
                    Position::EXITEXPOSURE) {   // position was not closed earlier due to exit constraints i.e.  order was never sent
                    _logA << "position exited but order was not sent due to exposure constraints@ "
                          << to_simple_string(gAsofDate) << endl;
                    _logA << pos->ToString() << endl;
                } else {
                    _logA << "position exited but not filled" << to_simple_string(gAsofDate) << endl;
                    _logA << pos->ToString() << endl;
                }
                if (pos->CloseRemaining == INT_MAX)  // no fills received yet
                    retval += pos->Size;
                else
                    retval += pos->Size > 0 ? pos->CloseRemaining : -pos->CloseRemaining;  //partially closed
            } else if (pos->mState == Position::Closed) {
                // no net remaining
            } else {
                cerr << "unusual state for position id " << pos->Id << endl;
            }
        }
    }
    //  exclude missed exposure TODO
    _logA << Name() << "_" << Moniker()
          << " # open positions @close (need to adjust for closed pending does not include positions expiring today)"
          << numopen << ",closed pending " << closedpending << endl;

    return retval;
}

// returns the net position/exposure
int StrategyA::Net(bool mismatch) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    int retval = _netAdjust;
    if (mismatch) {
        _logA << "CRITICAL: mismatch so will print positions @ " << to_simple_string(gAsofDate) << endl;
    }

    int numopen = 0;
    for (auto &pos : _openpositions) {
        if (pos->mExitCriteria._expiry.is_not_a_date_time()) {
            cout << "CRITICAL expiry not set" << endl;
            continue;
        }
        bool cond = pos->mExitCriteria._expiry.date() >=
                    gRoll.date();      // bug:  early close date cannot be used  (Sep 9. 2021)

        if (!pos->mExitCriteria._expiry.is_not_a_date_time() && cond) {
            if (pos->mState == Position::OpenPending) {
                if (mismatch)
                    _logA << pos->ToString() << endl;
                if (pos->Remaining == INT_MAX)
                    retval += 0; // no fills were received so position did not open
                else
                    retval += (pos->Size > 0) ? pos->Size - pos->Remaining : pos->Size + pos->Remaining;
            } else if (pos->mState == Position::Open || pos->mState == Position::ClosedPending) {
                if (mismatch)
                    _logA << pos->ToString() << endl;
                if (pos->CloseRemaining == INT_MAX)  // no fills received yet
                    retval += pos->Size;
                else
                    retval += pos->Size > 0 ? pos->CloseRemaining : -pos->CloseRemaining;
            } else if (pos->mState == Position::Closed) {
                // not net remaining
            } else {
                cerr << "unusual state for position id " << pos->Id << endl;
            }
        }
    }
    return retval;
}

Position *StrategyA::ExpireEx() {

    Position *retval = 0;
    bool aggressor = true;
    cout << "=>starting expiry  process for strategy " << _pMarketable->Name() << "_" << FrequencyString() << endl;
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);
    int sz = cdls->size();

    _logA << "positions @ expiry (pre offset) " << _positions.size() << endl;
    if (true || !gReducedLogging) {
        for (auto &p: _positions) {
            _logA << p->ToString() << endl;
        }
    }
    _logA << "openpositions @ expiry (pre offset) " << _openpositions.size() << endl;
    if (!gReducedLogging) {
        for (auto &op: _openpositions) {
            _logA << op->ToString() << endl;
        }
    }

    if (sz > 0) {
        auto cdl = cdls->back();
        int net = NetExpiry();
        //Decimal closepx = net > 0 ? (aggressor ? cdl->LSTB : cdl->LSTA) : (aggressor ? cdl->LSTA : cdl->LSTB);  //bug  fixe March 25, 2021
        Decimal closepx = net < 0 ? (aggressor ? cdl->LSTB : cdl->LSTA) : (aggressor ? cdl->LSTA : cdl->LSTB);
        if (closepx == Decimal(0)) {
            cerr << "CRITICAL: close px is zero.  P&L  computation will be incorrect" << " @ "
                 << to_simple_string(gAsofDate) << endl;
            ofglobal << "CRITICAL: close px is zero.  P&L  computation will be incorrect" << endl;

            for (auto it = cdls->rbegin(); it < cdls->rend(); it++) {
                Decimal validpx =
                        net > 0 ? (aggressor ? (*it)->LSTB : (*it)->LSTA) : (aggressor ? (*it)->LSTA : (*it)->LSTB);
                if (validpx != Decimal(0)) {
                    closepx = validpx;
                    ofglobal << "CRITICAL: valid price found at " << to_simple_string((*it)->timeStamp) << endl;
                    cerr << "CRITICAL: valid price found at " << to_simple_string((*it)->timeStamp) << endl;
                    break;
                }
            }

        }

        if ((cdl->LSTA - cdl->LSTB).getAsDouble() > _toowide.getAsDouble()) {
            closepx = (_pMarketable->BestBid.Price + _pMarketable->BestAsk.Price) / Decimal(2.0);
            cout << "using LKGP to get mid for spread " << dec::toString((cdl->LSTA - cdl->LSTB)) << " and price ";
            cout << dec::toString(_pMarketable->BestBid.Price) << "/"
                 << dec::toString(_pMarketable->BestAsk.Price) << "@"
                 << to_simple_string(_pMarketable->BestAsk.Timestamp);
            cout << " for  " << Name() << "_" << Frequency() << endl;
        }
        Decimal mid = (cdl->LSTB + cdl->LSTA) / Decimal(2.0);
        vector<int> expiredPosId;
        for (size_t i = 0; i < _positions.size(); i++) {
            auto pos = _positions[i];
            if (pos->mExitCriteria._expiry.date() > gRoll.date()) {
                ExpireInterDay(pos);
                continue;
            }
            if (!pos->mExitCriteria._expiry.is_not_a_date_time() && pos->mExitCriteria._expiry.date() <= gRoll.date()) {
                if (pos->TimeStamp.date() <= gRoll.date()) {
                    if (pos->mState == Position::OpenPending && pos->OrderIdEntry != 0) {
                        //cancel outstanding order
                        _logA << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
                              << pos->Id << endl;
                        cout << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
                             << pos->Id << endl;
                        pTrader->CancelOrder(pos->OrderIdEntry);
                        _unfilled++;
                    } else if (pos->mState == Position::ClosedPending && pos->OrderIdExit != 0) {
                        //cancel outstanding order
                        _logA << "canceling ClosedPending order before offset " << pos->OrderIdExit
                              << "  for position "
                              << pos->Id << endl;
                        cout << "canceling ClosedPending order before offset " << pos->OrderIdExit << "  for position "
                             << pos->Id << endl;
                        pTrader->CancelOrder(pos->OrderIdExit);
                        _unfilled++;
                    }
                    if (pos->mExitCode != Position::SUPRESSEDSIGNAL && pos->mExitCode != Position::MISSEXPOSURE) {
                        if (pos->mExitCriteria._expiry.date() <=
                            gRoll.date()) {  // bug fix: do not offset positions that are not expiring today      // use this criteria as uniform criteria (Sep 9. 2021)
                            pos->Offset(closepx);
                            pos->PnlRealized = pos->PnL();
                            expiredPosId.push_back(pos->Id);
                        }
                    }
                }
            }
        }
        // remove the expired positions for open
        _logA << expiredPosId.size() << "  positions expiring today  @" << to_simple_string(gAsofDate) << endl;
        for (size_t i = 0; i < expiredPosId.size(); i++) {
            auto it = _openpositions.begin();
            for (; it != _openpositions.end();) {
                if ((*it)->Id == expiredPosId[i]) {
                    _logA << "erased from open positions " << (*it)->ToString() << endl;
                    it = _openpositions.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (net != 0) {
            // Special case of an offset position
            auto pos = new Position(Position::nextId, gAsofDate,
                                    closepx,
                                    -net);

            Position::nextId++;
            pos->mEntryCriteria._type = EntryCriteria::EntryType::OFFSET;
            pos->mExitCriteria._expiry = ptime(date(2030, Jan, 1),
                                               time_duration(hours(0)));  //  essentially manage manually
            //pos->Remaining= abs(pos->Size)  ;
            _positions.push_back(pos);
            _numPositions++;
            retval = pos;

            _logA << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
                  << pos->Size << endl;
            cout << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
                 << pos->Size << endl;
        } else {
            _logA << _pMarketable->Name() << "_" << FrequencyString()
                  << " net offset position (position expiring today) is nothing expires today" << endl;
        }
    }
    WritePositions();  // this will compute mark for open positions


    _logA << "openpositions @ expiry (POST offset) " << _openpositions.size() << endl;
    if (true || !gReducedLogging) {
        for (auto &op: _openpositions) {
            _logA << op->ToString() << endl;
        }
    }
    _scalepositionslmt.clear();
    _scalepositionsstp.clear();
    return retval;
}

//  TODo: merge with expireex
Position * StrategyA::Liquidate(int num) {

    bool aggressor = true;
    Position *retval=0;
    _logA << "=>starting liquidation  process for strategy " << _pMarketable->Name() << "_" << FrequencyString() << endl;

    _logA <<  "number to liquidate is "  << num  << endl;

    if(num== 0) {
        return retval;
    }
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);
    int sz = cdls->size();

    _logA << "positions @ liquidation (pre liquidation) " << _positions.size() << endl;
    if (true || !gReducedLogging) {
        for (auto &p: _positions) {
            _logA << p->ToString() << endl;
        }
    }
    _logA << "openpositions @ liquidation (pre liquidation) " << _openpositions.size() << endl;
    if (!gReducedLogging) {
        for (auto &op: _openpositions) {
            _logA << op->ToString() << endl;
        }
    }

    if (sz > 0) {
        auto cdl = cdls->back();
        int net = num;
        //Decimal closepx = net > 0 ? (aggressor ? cdl->LSTB : cdl->LSTA) : (aggressor ? cdl->LSTA : cdl->LSTB);  //bug  fixe March 25, 2021
        Decimal closepx = net < 0 ? (aggressor ? cdl->LSTB : cdl->LSTA) : (aggressor ? cdl->LSTA : cdl->LSTB);
        if (closepx == Decimal(0)) {
            cerr << "CRITICAL: close px is zero.  P&L  computation will be incorrect" << " @ "
                 << to_simple_string(gAsofDate) << endl;
            ofglobal << "CRITICAL: close px is zero.  P&L  computation will be incorrect" << endl;

            for (auto it = cdls->rbegin(); it < cdls->rend(); it++) {
                Decimal validpx =
                        net > 0 ? (aggressor ? (*it)->LSTB : (*it)->LSTA) : (aggressor ? (*it)->LSTA : (*it)->LSTB);
                if (validpx != Decimal(0)) {
                    closepx = validpx;
                    ofglobal << "CRITICAL: valid price found at " << to_simple_string((*it)->timeStamp) << endl;
                    cerr << "CRITICAL: valid price found at " << to_simple_string((*it)->timeStamp) << endl;
                    break;
                }
            }

        }

        if ((cdl->LSTA - cdl->LSTB).getAsDouble() > _toowide.getAsDouble()) {
            closepx = (_pMarketable->BestBid.Price + _pMarketable->BestAsk.Price) / Decimal(2.0);
            cout << "using LKGP to get mid for spread " << dec::toString((cdl->LSTA - cdl->LSTB)) << " and price ";
            cout << dec::toString(_pMarketable->BestBid.Price) << "/"
                 << dec::toString(_pMarketable->BestAsk.Price) << "@"
                 << to_simple_string(_pMarketable->BestAsk.Timestamp);
            cout << " for  " << Name() << "_" << Frequency() << endl;
        }
        Decimal mid = (cdl->LSTB + cdl->LSTA) / Decimal(2.0);

        int numclosed =0;
        for (size_t i = 0; i < _openpositions.size(); i++) {
            auto pos = _openpositions[i];

            if (pos->mState == Position::OpenPending && pos->OrderIdEntry != 0) {
                //cancel outstanding order
                _logA << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
                      << pos->Id << endl;
                cout << "canceling OpenPending order before offset " << pos->OrderIdEntry << "  for position "
                     << pos->Id << endl;
                pTrader->CancelOrder(pos->OrderIdEntry);
                _unfilled++;
            } else if (pos->mState == Position::ClosedPending && pos->OrderIdExit != 0) {
                //cancel outstanding order
                _logA << "canceling ClosedPending order before offset " << pos->OrderIdExit
                      << "  for position "
                      << pos->Id << endl;
                cout << "canceling ClosedPending order before offset " << pos->OrderIdExit << "  for position "
                     << pos->Id << endl;
                pTrader->CancelOrder(pos->OrderIdExit);
                _unfilled++;
            }
            if (pos->mExitCode != Position::SUPRESSEDSIGNAL && pos->mExitCode != Position::MISSEXPOSURE) {
                    pos->Offset(closepx);
                    pos->PnlRealized = pos->PnL();
                    numclosed+=pos->Size;
            }
            if(numclosed == num)
            {
                _logA  << "exiting after closing "  <<  numclosed  << endl;
                break;
            }
        }
        if (net != 0) {
            // Special case of an offset position
            auto pos = new Position(Position::nextId, gAsofDate,
                                    closepx,
                                    -net);

            Position::nextId++;
            pos->mEntryCriteria._type = EntryCriteria::EntryType::OFFSET;
            pos->mExitCriteria._expiry = ptime(date(2030, Jan, 1),
                                               time_duration(hours(0)));  //  essentially manage manually
            //pos->Remaining= abs(pos->Size)  ;
            _positions.push_back(pos);
            _numPositions++;
            retval = pos;

            _logA << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
                  << pos->Size << endl;
            cout << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
                 << pos->Size << endl;
        } else {
            _logA << _pMarketable->Name() << "_" << FrequencyString()
                  << " net offset position (position expiring today) is nothing expires today" << endl;
        }
    }

    WritePositions();  // this will compute mark for open positions
    WriteOpenPositions(gAsofDate);  // this will compute mark for open positions
    return retval;
}

Position *
StrategyA::TransmitOrder(int qty, double px, bool aggressor, vector<Position *> positions,
                         vector<int> expectedfills,
                         vector<bool> entryflags, string message) {

    Position *retval = 0;
    if (IsTradeable() == false)
        return retval;


    OrderDetails *od = new OrderDetails(_ofsExecutions);
    od->symbolId = _pMarketable->Id;
    od->name = _pMarketable->Name();
    od->aggressor = aggressor;
    od->requestType = qty > 0 ? 0 : 1;  //    BUY = 0, SELL = 1
    od->ptype = 2; // MARKET = 1,     LIMIT = 2
    od->quantity = abs(qty);
    od->payup = _payup;
    od->usemid = _usemid;
    od->tif = 0;
    od->spreadthreshold = _toowide.getAsDouble();
    od->price = px;
    for (size_t i = 0; i < expectedfills.size(); i++)
        od->fills.push_back(expectedfills[i]);
    for (size_t i = 0; i < positions.size(); i++)
        od->positions.push_back(positions[i]);
    for (size_t i = 0; i < entryflags.size(); i++)
        od->entryflags.push_back(entryflags[i]);


    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    int currentExposure = pTrader->GetCurrentExposure(_pMarketable->Id);
    _logA << "current exposure is  " << to_string(currentExposure) << " @ " << to_simple_string(gAsofDate) << endl;
    _logA << "max exposure is  " << to_string(_maxExposure) << endl;
    _logA << "qty is  " << to_string(qty) << endl;
    if (abs(currentExposure) >= _maxExposure && sgn(qty) == sgn(currentExposure)) {
        // do not send the order close the position indicating the same
        _logA << "Current exposure of " << currentExposure << " doesn't  allow " << od->ToString() << "@"
              << to_simple_string(gAsofDate) << endl;
        if (positions.size() > 0 && entryflags.size() > 0) {
            _logA << (entryflags[0] ? "ENTRY " : "EXIT ") << "order violates max exposure for " << positions[0]->Id
                  << endl;
            _logA << positions[0]->ToString() << endl;
            // close this position
            if (entryflags[0]) {
                _logA << "opening and closing without sending order ... ";
                positions[0]->mExitCode = Position::MISSEXPOSURE;
                positions[0]->mState = Position::Closed;
                positions[0]->Remaining = abs(positions[0]->Size);
            } else {
                positions[0]->mExitCode = Position::EXITEXPOSURE;
                //not closing because the exposure is still there and will be used at close to compute open positions for next day  July 11
                retval = positions[0];
            }
            return retval;
            /*
            else if (false) //  TODO: pending fix
            {
                _logA << "exiting without sending order ... ";
                positions[0]->mExitCode = Position::EXITEXPOSURE;
                positions[0]->mState = Position::Closed;
                if (px != 0)
                    positions[0]->CloseFillPx = px;
                else {
                    _logA << "using LKGP " << dec::toString(_pMarketable->BestBid.Price) << "/"
                          << dec::toString(_pMarketable->BestAsk.Price) << endl;
                    positions[0]->CloseFillPx = qty > 0 ? _pMarketable->BestBid.Price : _pMarketable->BestAsk.Price;
                }
                positions[0]->CloseFillTimeStamp = gAsofDate;
                int counter = 0;
                auto it = OpenPositions().begin();
                for (; it != OpenPositions().end();) {
                    if (positions[0]->Id == (*it)->Id) {
                        _logA << " and removing from openposition list ... " << endl;
                        it = _openpositions.erase(it);
                        counter++;
                    } else {
                        (++it);
                    }
                }
                _logA << "removed " << counter << " exposure, as computed by Net(), will be understated by " << qty
                      << endl;
                _netAdjust += qty;
                _logA << "net adjustment accumulated " << _netAdjust << endl;
            }

            */
        }
    }


    _logA << message << "  " << od->ToString() << endl;
    cout << message << "  " << od->ToString() << endl;
    cout << "@" << to_simple_string(gAsofDate) << endl;
    uint64_t oid = 0;
    if (gIOC && !gBacktest)
        oid = pTrader->SendOrderEx(od, this);
    else
        oid = pTrader->SendOrder(od, this);

    if (positions.size() > 0 && entryflags.size() > 0) {
        if (oid > 0) {
            _logA << (entryflags[0] ? "ENTRY " : "EXIT ") << oid << " sent successfully" << endl;
            cout << (entryflags[0] ? "ENTRY " : "EXIT ") << oid << " sent successfully" << endl;
            if (entryflags[0])
                positions[0]->OrderIdEntry = oid;
            else
                positions[0]->OrderIdExit = oid;
        } else {
            cerr << (entryflags[0] ? "ENTRY " : "EXIT ") << "problem sending order for " << positions[0]->Id
                 << endl;

            if (positions[0]->mEntryCriteria._type == EntryCriteria::EntryType::OFFSET)
                cerr << "POSSIBLY NEED TO SPLIT ORDER" << endl;
        }

    }
    return retval;
}

int StrategyA::GetLastLevel(bool mr) {
    //mr specifies  whether to pop a level or not
    int retval = 0;
    int lookback = (mr ? 2 : 1);
    if (_vbs.size() >= lookback) {
        retval = _vbs[_vbs.size() - lookback] ? 1 : -1;
        for (auto &pd: _previousDates)
            cout << "previous date " << to_simple_string(pd) << endl;
        cout << _previousSplines.size() << endl;
        if (_previousDates.size() >= lookback && _previousSplines.size() >= lookback) {
            GraphEx(_previousDates[_previousDates.size() - lookback],
                    _previousSplines[_previousSplines.size() - lookback]);
        } else
            cerr << "last level available but not last spline? odd..." << _previousDates.size() << ","
                 << _previousSplines.size() << endl;
    }
    return retval;
}

Decimal StrategyA::GetLevel(bool bs, int nth, bool passive, bool print) {
    int counter = 0;
    int ordinal = 0;
    Decimal retval;
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);


    for (auto rit = _vbs.rbegin(); rit != _vbs.rend(); ++rit) {
        if (bs == *rit) {
            int index = _changeIndex[_vbs.size() - counter - 1];
            if (ordinal == nth) {
                if (passive) {
                    retval = bs ? (*cdls)[index]->LSTB : (*cdls)[index]->LSTA;
                } else {
                    // note this is an active exit by default
                    retval = bs ? (*cdls)[index]->LSTA : (*cdls)[index]->LSTB;
                }
                if (print) {
                    _logA << (bs ? "BUY " : "SELL ") << ordinal << " Nth level at candle index " << index
                          << " candle timestamp " << to_simple_string((*cdls)[index]->timeStamp)
                          << "  price " << (*cdls)[index]->LSTB << "/" << (*cdls)[index]->LSTA << endl;
                }
                break;
            } else if (print) {
                _logA << (bs ? "BUY " : "SELL ") << ordinal << " level at candle index " << index
                      << " candle timestamp " << to_simple_string((*cdls)[index]->timeStamp)
                      << "  price " << (*cdls)[index]->LSTB << "/" << (*cdls)[index]->LSTA << endl;
            }
            ordinal++;
        }
        counter++;
    }
    return retval;
}

void StrategyA::PrintLevels(int start, vector<int> &changeIndex, vector<bool> &vbs, vector<Candle *> *cdls,
                            vector<double> &vs) {
    if (changeIndex.size() != vbs.size()) {
        cerr << "unexepected" << endl;
    }
    if (vbs.size() > 0 && cdls->size() > 0) {
        for (int k = 0; k < vbs.size(); k++) {
            int idx = changeIndex[k];
            cout << ">>level is " << k << ",";
            cout << "index is " << idx << ",";
            if (cdls->size() > idx) {
                cout << "price is " << dec::toString((*cdls)[changeIndex[k]]->LSTB) << "/"
                     << dec::toString((*cdls)[changeIndex[k]]->LSTA) << ",";
            } else {
                cerr << cdls->size() << " >  " << idx << endl;
            }
            int splineIndex = changeIndex[k] - start;
            cout << "spline point is " << vs[splineIndex] << endl;
            cout << ((vbs[k] == 1) ? "BUY" : "SELL") << " @ candle start time "
                 << to_simple_string(cdls->back()->timeStamp) << endl;
        }
    }

}

Decimal StrategyA::GetLevel(const vector<int> &changeIndex, const vector<bool> &vbs, bool bs, int nth, bool passive,
                            bool print) {
    int counter = 0;
    int ordinal = 0;
    Decimal retval;
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);


    for (auto rit = vbs.rbegin(); rit != vbs.rend(); ++rit) {
        if (bs == *rit) {
            int index = changeIndex[vbs.size() - counter - 1];
            if (ordinal == nth) {
                if (passive) {
                    retval = bs ? (*cdls)[index]->LSTB : (*cdls)[index]->LSTA;
                } else {
                    // note this is an active exit by default
                    retval = bs ? (*cdls)[index]->LSTA : (*cdls)[index]->LSTB;
                }
                if (print) {
                    ofglobal << (bs ? "BUY " : "SELL ") << ordinal << " Nth level at candle index " << index
                             << " candle timestamp " << to_simple_string((*cdls)[index]->timeStamp)
                             << "  price " << (*cdls)[index]->LSTB << "/" << (*cdls)[index]->LSTA << endl;
                }
                break;
            } else if (print) {
                ofglobal << (bs ? "BUY " : "SELL ") << ordinal << " level at candle index " << index
                         << " candle timestamp " << to_simple_string((*cdls)[index]->timeStamp)
                         << "  price " << (*cdls)[index]->LSTB << "/" << (*cdls)[index]->LSTA << endl;
            }
            ordinal++;
        }
        counter++;
    }
    return retval;
}

void StrategyA::Test() {
    // CHANEL_LUCE_BOFA_1, CHANEL_LUCE_BARCLAYS_1
    // BOFA,BARCLAYS
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);
    int sz = cdls->size();
    if (sz > 0 && _test) {
        _logA << "Sending TEST orders for " << _pMarketable->Name() << endl;
        OrderDetails *od1 = new OrderDetails(_ofsExecutions);
        OrderDetails *od2 = new OrderDetails(_ofsExecutions);
        od1->symbolId = od2->symbolId = _pMarketable->Id;
        od1->aggressor = _aggressorin;
        od1->ptype = 2; // MARKET = 1,     LIMIT = 2
        od1->tif = 0;  // 0 for DAY 3 for TIME_IN_FORCE::IOC
        od2->aggressor = _aggressorout;
        od1->requestType = _testBS;
        od2->requestType = !_testBS;
        od1->quantity = od2->quantity = _contracts;
        if (_testBS)  // buy order
        {
            cout << "first order is a SELL " << (_aggressorin ? "AGGRESSIVE" : "PASSIVE") << endl;
            od1->price = (_aggressorin == false ? cdls->back()->LSTA : cdls->back()->LSTB).getAsDouble();
            od2->price = (_aggressorout == false ? cdls->back()->LSTB : cdls->back()->LSTA).getAsDouble();
        } else //sell
        {
            cout << "first order is a BUY " << (_aggressorin ? "AGGRESSIVE" : "PASSIVE") << endl;
            od1->price = (_aggressorin == false ? cdls->back()->LSTB : cdls->back()->LSTA).getAsDouble();
            od2->price = (_aggressorout == false ? cdls->back()->LSTA : cdls->back()->LSTB).getAsDouble();
        }
        od1->ptype = od2->ptype = 2;
        cout << "First order " << od1->ToString() << endl;
        uint64_t orderid = pTrader->SendOrderEx(od1, this);
        return;


        std::this_thread::sleep_for(std::chrono::milliseconds(2));  // need to wait 2 millisecs

        cout << "Second order " << od2->ToString() << endl;
        // is filled
        int original;
        int filled;
        if (IsFilled(orderid, original, filled)) {
            cout << "all good  during test" << endl;
        }
        pTrader->SendOrder(od2, this);

        _test = 0;
        return;
    }

}

bool StrategyA::IsQualifiedTracker(Position *pos) {
    bool retval =
            (pos->mEntryCriteria._type == EntryCriteria::HH) &&
            (_supressedSignalsMap.count(EntryCriteria::RHH) == 0 ||
             _supressedSignalsMap[EntryCriteria::RHH] == false);

    retval = retval || ((pos->mEntryCriteria._type == EntryCriteria::LL) &&
                        (_supressedSignalsMap.count(EntryCriteria::RLL) == 0 ||
                         _supressedSignalsMap[EntryCriteria::RLL] == false));

    retval = retval || ((pos->mEntryCriteria._type == EntryCriteria::HL) &&
                        (_supressedSignalsMap.count(EntryCriteria::RHL) == 0 ||
                         _supressedSignalsMap[EntryCriteria::RHL] == false));

    retval = retval || ((pos->mEntryCriteria._type == EntryCriteria::LH) &&
                        (_supressedSignalsMap.count(EntryCriteria::RLH) == 0 ||
                         _supressedSignalsMap[EntryCriteria::RLH] == false));

    retval = retval || ((pos->mEntryCriteria._type == EntryCriteria::KR) &&
                        (_supressedSignalsMap.count(EntryCriteria::RKR) == 0 ||
                         _supressedSignalsMap[EntryCriteria::RKR] == false));
    return retval;
}

ptime StrategyA::ComputeExpiry() {
    date d = gRoll.date();
    d = OffsetDate(d, _holdingPeriod);
    // bug:  early close date cannot be used  (Sep 9. 2021)
    ptime expiry(d, time_duration(gRoll.time_of_day().hours(), gRoll.time_of_day().minutes(),
                                  gRoll.time_of_day().seconds()));
    return expiry;
}

void StrategyA::ClassifyTPs(ptime endTime, const vector<Candle *> *cdls, const vector<int> &currentChangeIndex,
                            const vector<bool> &vbs) {

    int current = cdls->size() - 1;


    int last = currentChangeIndex.back();
    int lag = current - last;
    bool buysignal = vbs.back();
    cout << (buysignal ? "BUY" : "SELL") << "@ : " << to_simple_string(gAsofDate) << endl;

    Decimal pxd;
    if (buysignal)  // buy order
    {
        pxd = (_aggressorin ? cdls->back()->LSTA : cdls->back()->LSTB);
    } else //sell
    {
        pxd = (_aggressorin ? cdls->back()->LSTB : cdls->back()->LSTA);
    }

    int multiplier = 1;

    Decimal tp = GetLevel(currentChangeIndex, vbs, buysignal, 0,
                          false);  // get the bid for the bottom and ask for the top
    int numtp = currentChangeIndex.size();
    cout << "# turning points " << _changeIndex.size() << " @ " << to_simple_string(gAsofDate) << endl;

    int net = Net();
    int lexp = NetExposure(true);
    int sexp = NetExposure(false);

    if (net != (lexp + sexp)) {
        cerr << "implementation issue?" << endl;
    }
    auto pos = new Position(Position::nextId, endTime,
                            pxd,
                            buysignal ? _contracts * multiplier : -_contracts * multiplier);
    pos->mEntryCriteria._description = "(" + dec::toString(tp) + ")";
    pos->mEntryCriteria._type = EntryCriteria::EntryType::A;
    if (_scale > 0) {  // 1:du  2:dd  3:du and dd
        if (buysignal) {
            Decimal tpprior = GetLevel(currentChangeIndex, vbs, !buysignal, 0, false);
            if (tp - tpprior > _ticksize) {
                if (gDebug) {
                    ofglobal << Name() << "_" << FrequencyString() << endl;
                    ofglobal << "BUY level 0 " << dec::toString(tp) << endl;
                    ofglobal << "corresponding prior SELL level 0 " << dec::toString(tpprior) << endl;
                    ofglobal << "unexpected buy higher than prior corresponding sell" << endl;
                    ofglobal << to_simple_string(endTime) << endl;
                }
                if (gDebug) {
                    GetLevel(currentChangeIndex, vbs, buysignal, 2, false, true);
                    GetLevel(currentChangeIndex, vbs, !buysignal, 2, false, true);
                }
            }

            for (int j = 1; j < numtp; j++) {
                Decimal tp1 = GetLevel(currentChangeIndex, vbs, buysignal, j, false);
                if (tp1 == Decimal(0)) {
                    if (gDebug) {
                        cout << "not enough levels available" << endl;
                    }
                    break;
                }
                if (gDebug) {
                    cout << "BUY level " << j << " is " << dec::toString(tp1) << endl;
                }
                pos->mEntryCriteria._description += dec::toString(tp1) + ")";
                if (j == 1) {
                    if (tp > tp1) {
                        if (_scale == 1 || _scale == 3) {
                            multiplier = _scaleMultiplier;
                            cout << "scaling buy " << multiplier << endl;
                        }
                        pos->mEntryCriteria._type = EntryCriteria::EntryType::HL;
                        _lastHL = tp1;
                    } else if (tp < tp1 && (_scale > 1)) {
                        pos->mEntryCriteria._type = EntryCriteria::EntryType::LL;
                        if (buysignal != true)
                            cerr << "CRITICAL: LL should be a buy signal" << endl;
                        Decimal refrate = (buysignal ? cdls->back()->LSTB : cdls->back()->LSTA);
                        if (tp1 > refrate)
                            pos->mExitCriteria._targetlevel = tp1;
                        else
                            pos->mExitCriteria._targetlevel = refrate + _targetsize;
                        pos->mExitCriteria._stoplevel =
                                refrate + (buysignal ? -_stopsize * Decimal(_h2l2stopx) : _stopsize *
                                                                                          Decimal(_h2l2stopx));  // set a really wide stop
                    }
                }
            }
        } else {
            Decimal tpprior = GetLevel(currentChangeIndex, vbs, !buysignal, 0, false);
            if ((tpprior - tp) > _ticksize) {
                if (gDebug) {
                    ofglobal << Name() << "_" << FrequencyString() << endl;
                    ofglobal << "SELL level 0 " << dec::toString(tp) << endl;
                    ofglobal << "corresponding prior BUY level 0 " << dec::toString(tpprior) << endl;
                    ofglobal << "unexpected sell lower than  prior corresponding buy" << endl;
                    ofglobal << to_simple_string(endTime) << endl;
                }
                if (gDebug) {
                    GetLevel(currentChangeIndex, vbs, buysignal, 2, false, true);
                    GetLevel(currentChangeIndex, vbs, !buysignal, 2, false, true);
                }
            }
            for (int j = 1; j < numtp; j++) {
                Decimal tp1 = GetLevel(currentChangeIndex, vbs, buysignal, j, false);
                if (tp1 == Decimal(0)) {
                    if (gDebug) {
                        cout << "not enough levels available for " << Name() << endl;
                    }
                    break;
                }
                if (gDebug) {
                    cout << "SELL level " << j << " is " << dec::toString(tp1) << endl;
                }
                pos->mEntryCriteria._description += dec::toString(tp1) + "(";
                if (j == 1) {
                    if (tp < tp1) {
                        if (_scale == 1 || _scale == 3) {
                            multiplier = _scaleMultiplier;
                            cout << "scaling sell " << multiplier << endl;
                        }
                        pos->mEntryCriteria._type = EntryCriteria::EntryType::LH;
                        _lastLH = tp1;
                    } else if (tp > tp1 && (_scale > 1)) {
                        pos->mEntryCriteria._type = EntryCriteria::EntryType::HH;
                        if (buysignal != false)
                            cerr << "CRITICAL: HH should be a sell signal" << endl;
                        Decimal refrate = (buysignal ? cdls->back()->LSTB : cdls->back()->LSTA);
                        if (tp1 < refrate)
                            pos->mExitCriteria._targetlevel = tp1;
                        else
                            pos->mExitCriteria._targetlevel = refrate - _targetsize;
                        pos->mExitCriteria._stoplevel =
                                refrate + (buysignal ? -_stopsize * Decimal(_h2l2stopx) : _stopsize *
                                                                                          Decimal(_h2l2stopx));  // set a really wide stop
                    }
                }
            }
        }
    }
    bool highestRank = true;

    if (pos->mEntryCriteria._type == EntryCriteria::EntryType::LH) {
        if (_lastLH < _lowestLH)
            _lowestLH = _lastLH;
        /*
        else if ((_supressedSignalsMap.count(EntryCriteria::EntryType::A) > 0 &&
                  _supressedSignalsMap[EntryCriteria::EntryType::A] == true))  // Signal A is suppressed
            highestRank = false;
        */
    } else if (pos->mEntryCriteria._type == EntryCriteria::EntryType::HL) {
        if (_lastHL > _highestHL)
            _highestHL = _lastHL;
        /*
        else if ((_supressedSignalsMap.count(EntryCriteria::EntryType::A) > 0 &&
                  _supressedSignalsMap[EntryCriteria::EntryType::A] == true))  // Signal A is suppressed
            highestRank = false;
        */
    }

    if (pos->mEntryCriteria._type == EntryCriteria::EntryType::LH ||
        pos->mEntryCriteria._type == EntryCriteria::EntryType::HL) {
        if (_useRank1) {
            // assume state of the strategy recorded in the state of the last position
            if ((_countHL > _countLH) && pos->mEntryCriteria._type == EntryCriteria::EntryType::LH) {
                highestRank = false;
                cout << pos->Id << "fails higest rank test for LH" << endl;
                cout << pos->ToString() << endl;
            }

            if ((_countLH > _countHL) && pos->mEntryCriteria._type == EntryCriteria::EntryType::HL) {
                highestRank = false;
                cout << pos->Id << "fails highest rank test for HL" << endl;
                cout << pos->ToString() << endl;
            }
        } else if (_useRank2) {   //case of switching HL and LH
            if (pos->mEntryCriteria._type == EntryCriteria::EntryType::LH) {
                if (_lastHL == Decimal(0) || _lastLH < _lastHL) {
                    cout << pos->Id << "PASSES highest rank test for LH: " << dec::toString(_lastLH)
                         << " < " << dec::toString(_lastHL) << endl;
                    cout << " OR " << _lastHL << " is zero" << endl;
                } else {
                    highestRank = false;
                    cout << pos->Id << "FAILS highest rank test for LH: " << dec::toString(_lastLH) << " > "
                         << dec::toString(_lastHL) << endl;
                    cout << pos->ToString() << endl;
                }
            } else if (pos->mEntryCriteria._type == EntryCriteria::EntryType::HL) {
                if (_lastLH == Decimal(0) || _lastHL > _lastLH) {
                    cout << pos->Id << "PASSES highest rank test for HL: " << dec::toString(_lastHL)
                         << " > " << dec::toString(_lastLH) << endl;
                } else {
                    highestRank = false;
                    cout << pos->Id << "FAILS highest rank test for HL: " << dec::toString(_lastHL) << " < "
                         << dec::toString(_lastLH) << endl;
                    cout << pos->ToString() << endl;
                }
            }
        }
    }
    pos->Size = buysignal ? _contracts * multiplier : -_contracts * multiplier;

    Position::nextId++;
    pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time

    cout << EntryCriteria::getEntryType(pos->mEntryCriteria._type) << "@ " << to_simple_string(gAsofDate)
         << endl;

    if ((_supressedSignalsMap.count(pos->mEntryCriteria._type) > 0 &&
         _supressedSignalsMap[pos->mEntryCriteria._type] == true) || highestRank == false) {
        std::string n = _pMarketable->Name() + "_" + to_string(_moniker);
        if (gDebug) {
            cout << "signal is suppressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type) << " for "
                 << n << endl;
            _logA << "signal is suppressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type) << " for "
                  << n << endl;
        }
        if (highestRank == false) {
            cout << "not the highest rank" << endl;
            _logA << "not the highest rank" << endl;
        }
        pos->mExitCode = Position::SUPRESSEDSIGNAL;
        pos->mState = Position::Closed;
        pos->Remaining = abs(pos->Size);
        _positions.push_back(pos);
        if (IsQualifiedTracker(pos)) {
            cout << "but add to  open positions because it is a IsQualifiedTracker" << endl;
            _openpositions.push_back(pos);
        }
    } else if (_skipmissedsignal && buysignal && tp > cdls->back()->LSTA) {  // it is offered below bottom
        cout << "too late to buy? signal at " << tp << "  px has moved to " << pxd << endl;
        pos->mExitCode = Position::MISSSIGNAL;
        pos->mState = Position::Closed;
        pos->Remaining = abs(pos->Size);
        _positions.push_back(pos);
    } else if (_skipmissedsignal && !buysignal && tp < cdls->back()->LSTB) {  // it is bid  above top
        cout << "too late to sell? signal at " << tp << "  px has moved to " << pxd << endl;
        pos->mExitCode = Position::MISSSIGNAL;
        pos->mState = Position::Closed;
        pos->Remaining = abs(pos->Size);
        _positions.push_back(pos);

    } else {
        bool skip = false;
        if (_useSR) {
            if (buysignal) // skip if px is below the previous sell
            {
                Decimal previousSellLevel = GetLevel(currentChangeIndex, vbs, false, 0, true);
                if (previousSellLevel != Decimal(0.0)) {
                    if (pxd < previousSellLevel) {
                        cout << "skip the BUY trade " << dec::toString(pxd) << " below "
                             << dec::toString(previousSellLevel) << endl;
                        skip = true;
                        auto posBuy = new Position(Position::nextId, gAsofDate,
                                                   previousSellLevel,
                                                   buysignal ? _contracts * multiplier : -_contracts *
                                                                                         multiplier);
                        posBuy->mEntryCriteria._description = "BREAK  RESISTANCE " + to_string(pos->Id);
                        posBuy->mEntryCriteria._type = pos->mEntryCriteria._type;
                        Position::nextId++;
                        posBuy->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
                        //Decimal stopsize = _stopsize - stopoffset;
                        posBuy->mExitCriteria._stoplevel =
                                previousSellLevel - (buysignal ? _stopsize : -_stopsize);
                        posBuy->mExitCriteria._targetlevel =
                                previousSellLevel + (buysignal ? _targetsize : -_targetsize);
                        posBuy->mEntryCriteria._value = 100; // broke resistance
                        posBuy->Remaining = abs(pos->Size);
                        cout << "break resistance " << posBuy->ToString() << endl;
                        _scalepositionsstp.push_back(posBuy);
                    }
                }
            } else  // px is bellow the previous buy
            {
                Decimal previousBuyLevel = GetLevel(currentChangeIndex, vbs, true, 0, true);
                if (previousBuyLevel != Decimal(0.0)) {
                    if (pxd > previousBuyLevel) {
                        cout << "skip the SELL trade " << dec::toString(pxd) << " above "
                             << dec::toString(previousBuyLevel) << endl;
                        skip = true;
                        auto posSell = new Position(Position::nextId, gAsofDate,
                                                    previousBuyLevel,
                                                    buysignal ? _contracts * multiplier : -_contracts *
                                                                                          multiplier);
                        posSell->mEntryCriteria._description = "BREAK  SUPPORT " + to_string(pos->Id);
                        posSell->mEntryCriteria._type = pos->mEntryCriteria._type;
                        Position::nextId++;
                        posSell->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
                        //Decimal stopsize = _stopsize - stopoffset;
                        posSell->mExitCriteria._stoplevel =
                                previousBuyLevel - (buysignal ? _stopsize : -_stopsize);
                        posSell->mExitCriteria._targetlevel =
                                previousBuyLevel + (buysignal ? _targetsize : -_targetsize);
                        posSell->mEntryCriteria._value = 100; // broke resistance
                        posSell->Remaining = abs(pos->Size);
                        cout << "break support " << posSell->ToString() << endl;
                        _scalepositionsstp.push_back(posSell);
                    }
                }
            }
        }
        if (!skip) {
            Decimal refrate = (buysignal ? cdls->back()->LSTB : cdls->back()->LSTA);
            if (pos->mEntryCriteria._type != EntryCriteria::HH &&
                pos->mEntryCriteria._type != EntryCriteria::LL) // here the target / stops is set earlier
            {
                pos->mExitCriteria._stoplevel = refrate + (buysignal ? -_stopsize : _stopsize);
                pos->mExitCriteria._targetlevel = refrate + (buysignal ? _targetsize : -_targetsize);
            }
            pos->mEntryCriteria._value = lag; // record the lag
            ofglobal << "lag is " << lag << endl;
            pos->mEntryCriteria._value = _throttleCount; // record the throttle count
            pos->Remaining = abs(pos->Size);
            _positions.push_back(pos);
            _openpositions.push_back(pos);
            vector<Position *> positions;
            positions.push_back(pos);
            if (_scalePositions != 0)
                ScalePositions(pos, _scalefactor);

            if (!gBacktest) {  // temp please reverse
                _logA << pos->ToString() << endl;
                cout << pos->ToString() << endl;
            }
            /*
            positions.insert(positions.end(), positionsDesiredANDClosed.begin(),
                             positionsDesiredANDClosed.end());
            */
            vector<int> expectedfills;
            expectedfills.push_back(pos->Size);

            vector<bool> entryflags;
            entryflags.push_back(true);  //since this is an exit

            int qty = pos->Size;
            TransmitOrder(qty, pxd.getAsDouble(), _aggressorin, positions, expectedfills,
                          entryflags,
                          "Enter New Position " + to_string(pos->Id) + " @" + to_simple_string(gAsofDate));
        }
    }


}

void StrategyA::GenerateChangePoints(int start, ptime endTime, vector<Candle *> *cdls, int boundaryAdjustment,
                                     vector<bool> &vbs, vector<int> &currentChangeIndex, vector<double> &vs,
                                     vector<double> &vds, vector<double> &vd2s) {
    X.clear();
    Y.clear();

    //cout << "in STRATEGYA, processing candle # " << ins->GetCandles(_freq).size() - 1 << to_simple_string(endTime) << endl;
    // _logA << _name << ", processing candle # " << _candles.size() - 1 << endl;
    int ctr = 0;

    for (size_t i = start; i < (cdls->size() - boundaryAdjustment); i++) {
        X.push_back(i);
        Candle *c = (*cdls)[i];
        double mid = ((c->LSTA + c->LSTB) / Decimal(2.0)).getAsDouble();
        Y.push_back(mid);
        //cout << "last 100 candles " << endl;
        if (gDebug) {
            if (endTime == time_from_string("2021-Jun-17 13:30:01.363000")) {
                ctr++;
                cout << to_simple_string(c->timeStamp) << "  " << mid;
                if (ctr % 25 == 0)
                    cout << endl;
                else
                    cout << ";";
            }
        }
        // cout << "=> END last 100 candles " << endl;
    }


    alglib::real_1d_array AX, AY;
    AX.setcontent(X.size(), &(X[0]));
    AY.setcontent(Y.size(), &(Y[0]));

    auto t1 = std::chrono::high_resolution_clock::now();

    GetZerosEx(endTime, AX, AY, start, currentChangeIndex, vbs, vs, vds, vd2s);
    if (gDebug) {
        if (_freq == 5) {
            cout << "last candle =>" << endl;
            cout << to_simple_string(endTime) << endl;
            int ctr2 = 0;
            for (auto it = vs.begin(); it != vs.end(); it++) {
                ctr2++;
                cout << (*it);
                if (ctr2 % 25 == 0)
                    cout << endl;
                else
                    cout << ",";

            }
            cout << endl;
            //2021-Jun-17 13:30:01.363000
        }
        cout << "pushing back " << to_simple_string(endTime) << endl;
        if (endTime == time_from_string("2021-Jun-17 13:30:01.363000")) {
            cout << cdls->back()->ToString() << endl;
        }
    }
    _previousDates.push_back(endTime);
    _previousSplines.push_back(vs);
    if (_previousDates.size() > 2) {
        _previousDates.erase(_previousDates.begin());
    }
    if (_previousSplines.size() > 2) {
        _previousSplines.erase(_previousSplines.begin());
    }

    if (gDebug) {
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        std::cout << "time in microseconds " << duration << " for computation "
                  << to_simple_string(cdls->back()->timeStamp)
                  << endl;
        cout << cdls->back()->ToString() << endl;
        Candle *c = (*cdls)[start];
        cout << "first lookback candle timestamp " << to_simple_string(c->timeStamp) << "@ index "
             << to_string(start) << endl;
        cout << c->ToString() << endl;
    }

}


void StrategyA::HandleCandle(const Candle &candle) {
    //   const std::lock_guard<std::mutex> lock(gPositionsMutex);

    if (IsTradingHalted()) {
        return;
    }
    if (candle.LSTB == Decimal(0) || candle.LSTA == Decimal(0)) {
        cerr << "CRITICAL:  BAD CANDLE" << endl;
        cerr << candle.ToString() << endl;
        return;
    }
    Strategy::HandleCandle(candle);
    ptime endTime;
    endTime = _candleQuantum > 0 ? candle.lastUpdate : candle.timeStamp +
                                                       candle.interval; // assuming constant time candles

    if (_pMarketable == 0) {
        throw "why is marketable zero?";
    }

    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);

    if (cdls->back()->timeStamp != candle.timeStamp) {
        cout << to_simple_string(cdls->back()->timeStamp) << ",";
        cout << to_simple_string(candle.timeStamp) << endl;
    }
    int previousIntervalVolume = 0;

    if (_candleQuantum > 0) {  //candleQuantum  will ususally be equal to  _intv  when both are specified
        if (cdls->size() > 1 + _throttleCount) {
            int i = cdls->size() - 2 - _throttleCount;
            auto last = (*cdls)[i];
            time_duration td = candle.lastUpdate - last->lastUpdate;
            // this ensures that the # of candles within a candleFrequency (in mins) interval are recorded.
            if (td < minutes(_freq)) {
                _throttleCount++;
                _currentIntervalVolume += candle.INTV;
                if (_throttleCount > 0 && gDebug) {
                    cerr << "throttle is " << to_string(_throttleCount) << endl;
                    cerr << "current candle lastupdate " << to_simple_string(candle.lastUpdate) << " timestamp "
                         << to_simple_string(candle.timeStamp) << " LSTP " << dec::toString(candle.LSTP) << endl;
                    cerr << "last candle lastupdate " << to_simple_string(last->lastUpdate) << " timestamp "
                         << to_simple_string(last->timeStamp) << " LSTP last " << dec::toString(last->LSTP) << endl;
                }

            } else if (_throttleCount != 0) {
                if (gDebug) {
                    cerr << "resetting throttle at endtime " << to_simple_string(endTime) << "  "
                         << to_string(_throttleCount) << endl;
                    cerr << "timestamp " << to_simple_string(candle.timeStamp) << " LSTP "
                         << dec::toString(candle.LSTP)
                         << endl;
                    cerr << "previous volume is " << _currentIntervalVolume << endl;
                }
                _throttleCount = 0;
                previousIntervalVolume = _currentIntervalVolume;
                _currentIntervalVolume = candle.INTV;
            } else {  // case when candle arrives later than _freq
                previousIntervalVolume = _currentIntervalVolume;
                _currentIntervalVolume = candle.INTV;
                //cerr << to_simple_string(td)  << endl;
            }

        }
    } else  // regular candles (no Q)
    {
        _currentIntervalVolume = candle.INTV;
    }

    if (gDebug) {
        if (_positions.size() > 0)  // no more than 1 position per minute for now  (need to ignore RHH etc..)
        {
            for (auto rit = _positions.rbegin(); rit != _positions.rend(); rit++) {
                auto pos = *rit;
                if (pos->mEntryCriteria._type != EntryCriteria::RHL &&
                    pos->mEntryCriteria._type != EntryCriteria::RLH &&
                    pos->mEntryCriteria._type != EntryCriteria::RHH &&
                    pos->mEntryCriteria._type != EntryCriteria::RLL) {
                    time_duration td = endTime - pos->TimeStamp;
                    if (td < minutes(1)) {
                        cerr << Name() << "-" << FrequencyString()
                             << " position taken less than 1 minute ago potentially skipping " << pos->Id
                             << " timestamp "
                             << to_simple_string(pos->TimeStamp) << endl;
                        cerr << "last position taken @ " << to_simple_string(pos->TimeStamp) << "  "
                             << EntryCriteria::getEntryType(pos->mEntryCriteria._type) << " throttle count "
                             << _throttleCount << endl;
                        cerr << "candle ends at " << to_simple_string(endTime) << endl;
                        break;
                    }
                }
            }
        }
    }

    if (gDebug && _pMarketable != 0) {
        cout << "handle single candles from strategy : " << _pMarketable->Name() << "  @endtime  "
             << to_simple_string(endTime) << endl;
        cout << FrequencyString() << endl;
    }

    if (_supressedSignalsMap.count(EntryCriteria::EntryType::CR0) == 0) {
        ExecuteCircadian(candle, cdls, endTime);
        if (_circadianRun) {
            if (gWriteCandles) {
                _ofsCandles << candle.ToString() << endl;
            }
            return;
        }
    }


    bool isKR = (_candleQuantum > 0) ? (previousIntervalVolume > (_intv * _intvMultiplier)) : (
            _currentIntervalVolume >
            (_intv *
             _intvMultiplier));
    if (_modeKR == 4 && isKR) {
        _krCandles.push_back((*cdls)[cdls->size() - 1]);
        _logA << "pushed KR candle:  " << (*cdls)[cdls->size() - 1]->ToString() << endl;
    }


    if (isKR) {
        _logA << "previous volume is " << previousIntervalVolume << " for " << _pMarketable->Name() << " @"
              << to_simple_string(gAsofDate) << endl;
        _logA << "current volume is " << _currentIntervalVolume << " for " << _pMarketable->Name() << " @"
              << to_simple_string(gAsofDate) << endl;
    }
    int boundaryAdjustment = 0;
    int sz = cdls->size();

    if (sz >= (_minlen + boundaryAdjustment) && InSession(endTime)) {

        vector<bool> vbs;
        vector<int> currentChangeIndex;
        vector<double> vs, vds, vd2s;
        int start = cdls->size() - (_minlen + boundaryAdjustment);
        bool updated;

        if (_computeTP) {
            GenerateChangePoints(start, endTime, cdls, boundaryAdjustment, vbs, currentChangeIndex, vs, vds, vd2s);


            updated = UpdateChangePointsV7(vbs, currentChangeIndex);
            /*
            bool notempty = !currentChangeIndex.empty() && !vbs.empty();
            if (notempty) {
                if (_optimizeFit == 1 && _mr) {
                    currentChangeIndex.pop_back();
                    vbs.pop_back();
                }
                if (gDebug) {
                    PrintLevels(start, currentChangeIndex, vbs, cdls, vs);
                }
            }
            */
            updated = updated && (vbs.size() > 0);
        }
        if ((_modeKR > 0 && _modeKR < 4 && isKR) ||
            (/*!isKR && _modeKR == 4 &&
             (_krCandles.size() > 0)*/ false))  // vbs is needed to know the trend hence inside loop
        {
            int volume = (_candleQuantum > 0) ? previousIntervalVolume : _currentIntervalVolume;
            ExecuteKR(vbs, candle, cdls, endTime, _modeKR, volume);
        } else if (isKR && _modeKR == 4 &&
                   (_krCandles.size() > 0)) {
            int volume = (_candleQuantum > 0) ? previousIntervalVolume : _currentIntervalVolume;
            ExecuteKR2(vbs, candle, cdls, endTime, _modeKR, volume);
        }


        if (updated && _classifyTP && gNewSignal && !_disableSignal) {
            ClassifyTPs(endTime, cdls, currentChangeIndex, vbs);
        }

        if ((updated /*&& _classifyTP*/) || _graphSpline || (_modeKR > 0 && isKR)/* spline at KR point */) {
            try {
                // GraphEx(endtime, x, y, _rho, x.length() * 3, vs, vds, vd2s);
                if (vs.size() > 0 && !gReducedLogging)
                    GraphEx(endTime, vs);
            }
            catch (exception ex) {
                _logA << "exception in graphex" << endl;
            }
        }
    }


    if (gWriteCandles) {
        _ofsCandles << candle.ToString() << endl;
        // below to debug quantum candles
        //_ofsCandles << candle.ToString() << "," << candle.ToStringEx() << endl;
        //_ofsCandles << candle.ToString() << "," << candle.ToStringEx() << endl;
    }
}

void StrategyA::ScalePositions(Position *mainpos, int scalefactor) {

    if (_clearscaling) {
        _scalepositionslmt.clear();
        _scalepositionsstp.clear();
        /*
        for(auto it1 = _scalepositionslmt.begin(); it1 !=  _scalepositionslmt.end();) {
            if((*it1)->mEntryCriteria._type == mainpos->mEntryCriteria._type)
            {
                it1=_scalepositionslmt.erase(it1);
            }
            else
            {
                ++it1;
            }

        }
        for(auto it1 = _scalepositionsstp.begin(); it1 !=  _scalepositionsstp.end();) {
            if((*it1)->mEntryCriteria._type == mainpos->mEntryCriteria._type)
            {
                it1=_scalepositionsstp.erase(it1);
            }
            else
            {
                ++it1;
            }
        }
         */
    }

    bool buy = mainpos->Size > 0;
    /*
    Decimal stopoffset =
            (mainpos->AvgPx - mainpos->mExitCriteria._stoplevel) * sgn<int>(mainpos->Size) / Decimal(scalefactor);
    */
    Decimal stopoffset = _stopsize / Decimal(scalefactor);

    if (stopoffset > _stopsize) {
        cerr << "stop offset too large " << stopoffset << endl;
        return;
    }
    for (int i = 1; i < scalefactor; i++) {
        Decimal pxd = mainpos->AvgPx - Decimal(i) * stopoffset * sgn<int>(mainpos->Size);
        int multiplier = 1;
        // 1:fade  2:add  3:both
        if (_scalePositions == 1 || _scalePositions == 3) {
            auto pos = new Position(Position::nextId, gAsofDate,
                                    pxd,
                                    buy ? _contracts * multiplier : -_contracts * multiplier);
            pos->mEntryCriteria._description = "STOP OFFSET for " + to_string(mainpos->Id) + " # " + to_string(i);
            pos->mEntryCriteria._type = mainpos->mEntryCriteria._type;
            Position::nextId++;
            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
            pos->mExitCriteria._stoplevel = pxd - Decimal(scalefactor - i) * stopoffset * sgn<int>(mainpos->Size);
            pos->mExitCriteria._targetlevel = pxd + (buy ? _targetsize : -_targetsize);
            pos->mEntryCriteria._value = Decimal(i) * stopoffset * sgn<int>(mainpos->Size);
            pos->Remaining = abs(pos->Size);
            _scalepositionslmt.push_back(pos);
        }

        if (_scalePositions >= 2) {
            /*
            Decimal targetoffset =
                    (mainpos->AvgPx - mainpos->mExitCriteria._targetlevel) * sgn<int>(mainpos->Size) / Decimal(scalefactor);
            */
            Decimal targetoffset = _targetsize / Decimal(scalefactor);
            if (targetoffset > _targetsize) {
                cerr << "target offset too large " << targetoffset << endl;
                return;
            }

            pxd = mainpos->AvgPx + Decimal(i) * targetoffset * sgn<int>(mainpos->Size);
            auto pos = new Position(Position::nextId, gAsofDate,
                                    pxd,
                                    buy ? _contracts * multiplier : -_contracts * multiplier);
            pos->mEntryCriteria._description = "TARGET OFFSET for " + to_string(mainpos->Id) + " # " + to_string(i);
            pos->mEntryCriteria._type = mainpos->mEntryCriteria._type;
            Position::nextId++;
            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
            //Decimal stopsize = _stopsize - stopoffset;
            pos->mExitCriteria._stoplevel = pxd - (buy ? _stopsize : -_stopsize);
            pos->mExitCriteria._targetlevel = pxd + (buy ? _targetsize : -_targetsize);
            pos->mEntryCriteria._value = targetoffset;
            pos->Remaining = abs(pos->Size);
            _scalepositionsstp.push_back(pos);
        }
    }
}

void StrategyA::ExecuteScalePositions(Decimal bid, Decimal ask) {
    if (IsTradeable() == false)
        return;

    auto it = _scalepositionslmt.begin();
    for (; it != _scalepositionslmt.end();) {
        if (((*it)->Size > 0 && ask <= (*it)->AvgPx) /* buy limit*/
            || ((*it)->Size < 0 && bid >= (*it)->AvgPx)) // sell limit
        {
            vector<Position *> positions;
            positions.push_back((*it));
            vector<int> expectedfills;
            expectedfills.push_back((*it)->Size);
            vector<bool> entryflags;
            entryflags.push_back(true);  //since this is an exit
            int qty = (*it)->Size;
            TransmitOrder(qty, ask.getAsDouble(), _aggressorfade, positions, expectedfills,
                          entryflags,
                          "Scale Limit " + to_string((*it)->Id) + " @" +
                          to_simple_string(gAsofDate));
            cout << "adding open position limit " << (*it)->Id << "@ " << to_simple_string(gAsofDate) << endl;
            (*it)->TimeStamp = gAsofDate;
            _openpositions.push_back(*it);
            _positions.push_back(*it);
            it = _scalepositionslmt.erase(it);
        } else {
            (++it);
        }
    }

    it = _scalepositionsstp.begin();
    for (; it != _scalepositionsstp.end();) {
        if (((*it)->Size > 0 && bid >= (*it)->AvgPx) /* bid higher*/
            || ((*it)->Size < 0 && ask <= (*it)->AvgPx)) // offered lower
        {
            vector<Position *> positions;
            positions.push_back((*it));
            vector<int> expectedfills;
            expectedfills.push_back((*it)->Size);
            vector<bool> entryflags;
            entryflags.push_back(true);  //since this is an exit
            int qty = (*it)->Size;
            TransmitOrder(qty, ask.getAsDouble(), _aggressoradd, positions, expectedfills,
                          entryflags,
                          "Scale Stop " + to_string((*it)->Id) + " @" +
                          to_simple_string(gAsofDate));
            cout << "adding open position stop " << (*it)->Id << "@ " << to_simple_string(gAsofDate) << endl;
            (*it)->TimeStamp = gAsofDate;
            _openpositions.push_back(*it);
            _positions.push_back(*it);
            it = _scalepositionsstp.erase(it);
        } else {
            (++it);
        }
    }
}


void StrategyA::ExecuteCircadian(const Candle &candle, const vector<Candle *> *cdls, ptime endtime) {
    if (!_circadianStart && !_circadianEnd) // check for circadian start
    {
        if (endtime >= _sessionStart) {
            _circadianStart = true;
            cout << "circadian start reached @ " << to_simple_string(endtime) << endl;
            Candle *opening = 0;
            bool found = false;
            for (auto rit = cdls->rbegin(); rit != cdls->rend(); ++rit) {
                if ((*rit)->timeStamp.date() == endtime.date()) {
                    opening = (*rit);
                    continue;
                } else {
                    found = true;
                    cout << "opening candle @ " << to_simple_string(opening->timeStamp) << endl;
                    break;
                }
            }

            if (found == false) {
                cout << "using opening candle @ " << to_simple_string(opening->timeStamp) << endl;
                cout << " previous data missing?" << endl;
            }
            if (opening != 0) {
                bool buy = sgn<Decimal>(opening->LSTB - candle.LSTB) > 0;
                Decimal pxd;
                if (buy)  // buy order
                {
                    pxd = (_aggressorin ? cdls->back()->LSTA : cdls->back()->LSTB);
                } else //sell
                {
                    pxd = (_aggressorin ? cdls->back()->LSTB : cdls->back()->LSTA);
                }

                int multiplier = 1;

                auto pos = new Position(Position::nextId, endtime,
                                        pxd,
                                        buy ? _contracts * multiplier : -_contracts * multiplier);
                pos->mEntryCriteria._description = "(" + dec::toString(opening->LSTB) + ")";
                pos->mEntryCriteria._type = EntryCriteria::EntryType::CR0;

                Position::nextId++;
                pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time

                Decimal refrate = (buy ? cdls->back()->LSTB : cdls->back()->LSTA);
                pos->mExitCriteria._stoplevel = refrate - (buy ? _stopsize : -_stopsize);
                pos->mExitCriteria._targetlevel = refrate + (buy ? _targetsize : -_targetsize);
                pos->mEntryCriteria._value = opening->LSTB;
                pos->Remaining = abs(pos->Size);
                _positions.push_back(pos);
                _openpositions.push_back(pos);
                if ((_supressedSignalsMap.count(pos->mEntryCriteria._type) > 0 &&
                     _supressedSignalsMap[pos->mEntryCriteria._type] == true)) {
                    std::string n = _pMarketable->Name() + "_" + to_string(_moniker);
                    if (gDebug) {
                        cout << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                             << " for "
                             << n << endl;
                        _logA << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                              << " for "
                              << n << endl;
                    }
                    pos->mExitCode = Position::SUPRESSEDSIGNAL;
                    pos->mState = Position::Closed;
                    pos->Remaining = abs(pos->Size);
                    _positions.push_back(pos);
                    if (IsQualifiedTracker(pos)) {
                        _openpositions.push_back(pos);
                    }
                } else  // actually send the order
                {
                    vector<Position *> positions;
                    positions.push_back(pos);
                    vector<int> expectedfills;
                    expectedfills.push_back(pos->Size);
                    vector<bool> entryflags;
                    entryflags.push_back(true);  //since this is an exit
                    int qty = pos->Size;
                    TransmitOrder(qty, pxd.getAsDouble(), _aggressorin, positions, expectedfills,
                                  entryflags,
                                  "Enter New Position (CR0) " + to_string(pos->Id) + " @" +
                                  to_simple_string(gAsofDate));
                }

            }
        }
    } else if (_circadianStart && !_circadianEnd)  // check for circadian end
    {
        _circadianEnd = true;
        cout << "circadian end reached @ " << to_simple_string(endtime) << endl;
    } else if (!_circadianStart && _circadianEnd)  // c
    {
        cerr << "not in a circadian session" << endl;
    }
}


int StrategyA::GetTrendMoniker(TrendMonikerType monikerType) {
    int retval = 5;
    switch (monikerType) {
        case OTHER:
            retval = _trendMonikerKR;
            break;
        case CURRENT:
            retval = _moniker;
            break;
        case HIGHER:
            switch (_freq) {
                case 1:
                    retval = 5;
                    break;
                case 5:
                    retval = 15;
                    break;
                case 15:
                    retval = 60;
                    break;
                default:
                    break;
            }
            break;
    }
    return retval;
}

void StrategyA::StopKR(bool krsignal, const vector<Candle *> *cdls) {
    Decimal pxd;
    if (!krsignal)  // buy order
    {
        pxd = (_aggressorin ? cdls->back()->LSTA : cdls->back()->LSTB);
    } else //sell
    {
        pxd = (_aggressorin ? cdls->back()->LSTB : cdls->back()->LSTA);
    }

    for (auto &op : _openpositions) {
        if ((op->Size > 0) != krsignal) {
// stop out all positions that go against the krsignal
            bool canclose =
                    op->mState == Position::Open ||
                    (op->mState == Position::OpenPending && abs(op->ComputeNet()) > 0);
            if (canclose) {
                op->Close(Position::ExitCode::EXIT, pxd);
                try {

                    int offsettoclose = -op->ComputeNet();
                    if (offsettoclose != -op->Size) {
// case of a partial fill
                        if (op->OrderIdEntry != 0) {
                            _logA << "Stopping out a Partially filled position at KR id:" << op->Id
                                  << endl;
                            _logA << "Cancelling entry order # " << op->OrderIdEntry << endl;
                            pTrader->CancelOrder(op->OrderIdEntry);
                        } else {
                            cerr << "missing entry order id" << endl;
                        }
                    }
                    vector<Position *> positions3;
                    positions3.push_back(op);
                    vector<int> expectedfills;
                    expectedfills.push_back(offsettoclose);
                    vector<bool> entryflags;
                    entryflags.push_back(false);  //since this is an exit

// always stop out aggressively
                    TransmitOrder(offsettoclose, pxd.getAsDouble(), /*_aggressorout*/ true, positions3,
                                  expectedfills,
                                  entryflags,
                                  "Stop Position on KR:" + to_string(op->Id));
                }
                catch (const char *message) {
                    cout << "EXCEPTION: " << message << endl;
                }
            }
        }
    }
}

void
StrategyA::SendOrderKR(bool krsignal, Decimal tp, const Candle &candle, const vector<Candle *> *cdls, ptime endtime,
                       int mode, int volume) {
    Decimal pxd;
    bool upcandle = candle.INTF < candle.LSTB;
    if (krsignal)  // buy order
    {
        pxd = (_aggressorin ? cdls->back()->LSTA : cdls->back()->LSTB);
    } else //sell
    {
        pxd = (_aggressorin ? cdls->back()->LSTB : cdls->back()->LSTA);
    }

    int multiplier = 1;

    auto pos = new Position(Position::nextId, endtime,
                            pxd,
                            krsignal ? _contracts * multiplier : -_contracts * multiplier);
    pos->mEntryCriteria._description = "(" + dec::toString(tp) + ")";

    if (krsignal == upcandle) {
        _logA << "Key Reversal in sync, candle direction is " << (upcandle ? "BUY" : "SELL") << endl;
        pos->mEntryCriteria._description += "I" + to_string(mode);
    } else {
        _logA << "Key Reversal in OUT of sync, candle direction is " << (upcandle ? "BUY" : "SELL") << endl;
        pos->mEntryCriteria._description += "O" + to_string(mode);
    }


    pos->mEntryCriteria._type = EntryCriteria::EntryType::KR;

    Position::nextId++;
    pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time

    Decimal refrate = (krsignal ? cdls->back()->LSTB : cdls->back()->LSTA);

    // set the stop to the candle H/L for now
    if (krsignal) // buying  set stop to candle low
    {
        pos->mExitCriteria._stoplevel = refrate - _stopsize * _multiplierKRstop;
        if (candle.LBID > Decimal(0.0) && candle.LBID < pos->mExitCriteria._stoplevel) {
            pos->mExitCriteria._stoplevel = candle.LBID;
            _logA << "key reversal stop set to LBID" << endl;
        }
        _logA << "BUYING kr stop level is " << dec::toString(pos->mExitCriteria._stoplevel) << endl;
    } else {
        pos->mExitCriteria._stoplevel = refrate + _stopsize * _multiplierKRstop;
        if (candle.HASK > pos->mExitCriteria._stoplevel) {
            pos->mExitCriteria._stoplevel = candle.HASK;
            _logA << "key reversal stop set to HASK" << endl;
        }
        _logA << "SELLING kr stop level is " << dec::toString(pos->mExitCriteria._stoplevel) << endl;
    }

    // pos->mExitCriteria._stoplevel = refrate + (krsignal ? -_stopsize : _stopsize);
    pos->mExitCriteria._targetlevel =
            refrate + (krsignal ? _targetsize * _multiplierKRtarget : -_targetsize * _multiplierKRtarget);

    pos->mEntryCriteria._value = volume;
    pos->Remaining = abs(pos->Size);
    _positions.push_back(pos);
    _openpositions.push_back(pos);
    vector<Position *> positions;
    positions.push_back(pos);
    if (!gBacktest) {  // temp please reverse
        _logA << pos->ToString() << endl;
        cout << pos->ToString() << endl;
    }

    if ((_supressedSignalsMap.count(pos->mEntryCriteria._type) > 0 &&
         _supressedSignalsMap[pos->mEntryCriteria._type] == true)) {
        std::string n = _pMarketable->Name() + "_" + to_string(_moniker);
        if (gDebug) {
            cout << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                 << " for "
                 << n << endl;
            _logA << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                  << " for "
                  << n << " Position id " << pos->Id << endl;
        }
        pos->mExitCode = Position::SUPRESSEDSIGNAL;
        pos->mState = Position::Closed;
        pos->Remaining = abs(pos->Size);
        _positions.push_back(pos);
        if (IsQualifiedTracker(pos)) {
            _openpositions.push_back(pos);
        }
    } else  // actually send the order
    {
        vector<int> expectedfills;
        expectedfills.push_back(pos->Size);

        vector<bool> entryflags;
        entryflags.push_back(true);  //since this is an exit

        int qty = pos->Size;
        TransmitOrder(qty, pxd.getAsDouble(), _aggressorin, positions, expectedfills,
                      entryflags,
                      "Enter New Position (KR) " + to_string(pos->Id) + " @" +
                      to_simple_string(gAsofDate));
    }

}


void
StrategyA::ExecuteKR2(const vector<bool> &vbs, const Candle &candle, const vector<Candle *> *cdls, ptime endtime,
                      int mode, int volume) {
    if (gNewSignal && !_disableSignal && mode == 4)  //  both turning point AND Key Reversal can happen
    {
        bool krsignal;
        Decimal low = Decimal(INT16_MAX), high = Decimal(0);
        for (auto &cdl : _krCandles) {
            Decimal clow, chigh;
            clow = candle.LBID;
            chigh = candle.HASK;
            if (clow < low)
                low = clow;
            if (chigh > high)
                high = chigh;
        }
        auto recentcandle = cdls->back();
        Position *p0 = 0, *p1 = 0;
        for (auto it = _positions.rbegin(); it != _positions.rend(); it++) {
            if ((*it)->mEntryCriteria._type == EntryCriteria::KR) {
                p0 = (*it);
                break;
            }
        }
        for (auto it = _openpositions.rbegin(); it != _openpositions.rend(); it++) {
            if ((*it)->mEntryCriteria._type == EntryCriteria::KR) {
                p1 = (*it);
                if (p0 != 0)
                    p0 = p0->TimeStamp > p1->TimeStamp ? p0 : p1;
                else
                    p0 = p1;
                break;

            }
        }
        if (p0 != 0) {
            time_duration td = endtime - p0->TimeStamp;
            cout << to_simple_string(td) << endl;
            if (p0 != 0 &&  /*15 min passed */ td >= minutes(15)) {
                if (p0->Size > 0) {
                    if (candle.LSTA < p0->AvgPx) {
                        cout << "this negates the BUY " << volume << " , previous volume"
                             << p0->mEntryCriteria._value << endl;
                        krsignal = false;
                        if (_stopKR) {
                            StopKR(krsignal, cdls);
                        }
                    }

                } else {
                    if (candle.LSTB > p0->AvgPx) {
                        cout << "this negates the SELL " << volume << " , previous volume"
                             << p0->mEntryCriteria._value << endl;
                        krsignal = true;
                        if (_stopKR) {
                            StopKR(krsignal, cdls);
                        }
                    }
                }
            }
        }
        SendOrderKR(krsignal, Decimal(0), candle, cdls, endtime,
                    mode, volume);
    }
}

void
StrategyA::ExecuteKR(const vector<bool> &vbs, const Candle &candle, const vector<Candle *> *cdls, ptime endtime,
                     int mode, int volume) {

    if (gNewSignal && !_disableSignal)  //  both turning point AND Key Reversal can happen
    {
        {
            bool buysignal;
            if (mode == 1) {
                if (vbs.size() > 0) {
                    //  determine  trend
                    buysignal = vbs.back();
                    _logA << ">>last tp was a " << (buysignal ? "LOW hence SELL" : "HIGH hence BUY") << " @ "
                          << to_simple_string(gAsofDate) << endl;
                    if (_mr) {
                        buysignal = !buysignal;
                        _logA << "spline trend reversed is " << (buysignal ? "BUY" : "SELL") << endl;
                    }

                } else {
                    _logA << " not enough levels @ " << to_simple_string(gAsofDate) << endl;
                    return;
                }
            } else if (mode == 2) {
                if (pTrader != 0) {
                    int h = GetTrendMoniker(_trendMonikerKR == 0 ? CURRENT : OTHER);
                    _logA << "trend moniker is " << h << endl;
                    auto shigher = pTrader->GetStrategy(_pMarketable->Id, h);
                    if (shigher != 0) {
                        int bs = shigher->GetLastLevel(_mr);
                        if (bs != 0) {
                            _logA << "trend monikerlast signal  " << (bs == 1 ? "BUY" : "SELL") << endl;
                        } else {
                            _logA << "trend moniker level not available.  not executing KR." << endl;
                            return;
                        }
                        buysignal = bs == 1 ? true : false;
                    } else {
                        _logA << "trend moniker not enabled in config? " << h
                              << " not executing KR" << endl;
                        return;
                    }
                }
            } else if (mode == 3)//naive trend
            {
                if (cdls->size() >= _lookbackKR) {
                    Decimal current = cdls->back()->LSTB;
                    Decimal prev = (*cdls)[cdls->size() - _lookbackKR]->LSTB;
                    _logA << "naive trend is " << ((current - prev) > Decimal(0.0) ? "BUY" : "SELL") << endl;
                    buysignal = (current - prev) > Decimal(0.0);
                    if (_mr) {
                        buysignal = !buysignal;
                        _logA << "naive trend reversed is " << (buysignal ? "BUY" : "SELL") << endl;
                    }
                } else {
                    return;
                }
            } else if (mode == 4) {
                Decimal low = Decimal(INT16_MAX), high = Decimal(0);
                for (auto &cdl : _krCandles) {
                    Decimal clow, chigh;
                    clow = candle.LBID;
                    chigh = candle.HASK;
                    if (clow < low)
                        low = clow;
                    if (chigh > high)
                        high = chigh;
                }
                auto recentcandle = cdls->back();

                if (recentcandle->LSTA < low) {
                    buysignal = true;  // indicates break of support?
                    _logA << "KR: break of support" << endl;
                } else if (recentcandle->LSTB > high) {
                    buysignal = false;   // indicates break of resistance
                    _logA << "KR: break of resistance" << endl;
                } else {
                    // assume the naive trend ,  then reverse later ...
                    if (cdls->size() >= _lookbackKR) {
                        Decimal current = cdls->back()->LSTB;
                        Decimal prev = (*cdls)[cdls->size() - _lookbackKR]->LSTB;
                        _logA << "KR: naive trend is " << ((current - prev) > Decimal(0.0) ? "BUY" : "SELL")
                              << endl;
                        buysignal = (current - prev) > Decimal(0.0);
                    } else {
                        return;
                    }
                }

                cout << "KR candles analyzed " << _krCandles.size() << endl;
                _krCandles.clear();
            } else {
                cerr << "CRITICAL: invalid KR mode" << endl;
            }
            bool upcandle = candle.INTF < candle.LSTB;
            Decimal tp = GetLevel(buysignal, 0, false);  // get the bid for the bottom and ask for the top

            bool krsignal = !buysignal;

            if (mode == 4) {  // following condition will dominate
                // get the volume of the last KR position
                Position *p0 = 0, *p1 = 0;
                for (auto it = _positions.rbegin(); it != _positions.rend(); it++) {
                    if ((*it)->mEntryCriteria._type == EntryCriteria::KR) {
                        p0 = (*it);
                        break;
                    }
                }
                for (auto it = _openpositions.rbegin(); it != _openpositions.rend(); it++) {
                    if ((*it)->mEntryCriteria._type == EntryCriteria::KR) {
                        p1 = (*it);
                        if (p0 != 0)
                            p0 = p0->TimeStamp > p1->TimeStamp ? p0 : p1;
                        else
                            p0 = p1;
                        break;

                    }
                }


                if (p0 != 0) {
                    if (p0->Size > 0) {
                        // if (p0->mEntryCriteria._value)
                        if (candle.LSTA < p0->AvgPx) {
                            cout << "this negates the BUY " << volume << " , previous volume"
                                 << p0->mEntryCriteria._value << endl;
                            krsignal = false;
                        }

                    } else {
                        if (candle.LSTB > p0->AvgPx) {
                            cout << "this negates the SELL " << volume << " , previous volume"
                                 << p0->mEntryCriteria._value << endl;
                            krsignal = true;
                        }
                    }
                }
            }


            Decimal pxd;
            if (krsignal)  // buy order
            {
                pxd = (_aggressorin ? cdls->back()->LSTA : cdls->back()->LSTB);
            } else //sell
            {
                pxd = (_aggressorin ? cdls->back()->LSTB : cdls->back()->LSTA);
            }

            int multiplier = 1;

            auto pos = new Position(Position::nextId, endtime,
                                    pxd,
                                    krsignal ? _contracts * multiplier : -_contracts * multiplier);
            pos->mEntryCriteria._description = "(" + dec::toString(tp) + ")";

            if (krsignal == upcandle) {
                _logA << "Key Reversal in sync, candle direction is " << (upcandle ? "BUY" : "SELL") << endl;
                pos->mEntryCriteria._description += "I" + to_string(mode);
            } else {
                _logA << "Key Reversal in OUT of sync, candle direction is " << (upcandle ? "BUY" : "SELL") << endl;
                pos->mEntryCriteria._description += "O" + to_string(mode);
            }


            pos->mEntryCriteria._type = EntryCriteria::EntryType::KR;

            Position::nextId++;
            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time

            Decimal refrate = (krsignal ? cdls->back()->LSTB : cdls->back()->LSTA);

            // set the stop to the candle H/L for now
            if (krsignal) // buying  set stop to candle low
            {
                pos->mExitCriteria._stoplevel = refrate - _stopsize * _multiplierKRstop;
                if (candle.LBID > Decimal(0.0) && candle.LBID < pos->mExitCriteria._stoplevel) {
                    pos->mExitCriteria._stoplevel = candle.LBID;
                    _logA << "key reversal stop set to LBID" << endl;
                }
                _logA << "BUYING kr stop level is " << dec::toString(pos->mExitCriteria._stoplevel) << endl;
            } else {
                pos->mExitCriteria._stoplevel = refrate + _stopsize * _multiplierKRstop;
                if (candle.HASK > pos->mExitCriteria._stoplevel) {
                    pos->mExitCriteria._stoplevel = candle.HASK;
                    _logA << "key reversal stop set to HASK" << endl;
                }
                _logA << "SELLING kr stop level is " << dec::toString(pos->mExitCriteria._stoplevel) << endl;
            }

            // pos->mExitCriteria._stoplevel = refrate + (krsignal ? -_stopsize : _stopsize);
            pos->mExitCriteria._targetlevel =
                    refrate + (krsignal ? _targetsize * _multiplierKRtarget : -_targetsize * _multiplierKRtarget);

            pos->mEntryCriteria._value = volume;
            pos->Remaining = abs(pos->Size);
            _positions.push_back(pos);
            _openpositions.push_back(pos);
            vector<Position *> positions;
            positions.push_back(pos);
            if (!gBacktest) {  // temp please reverse
                _logA << pos->ToString() << endl;
                cout << pos->ToString() << endl;
            }

            if ((_supressedSignalsMap.count(pos->mEntryCriteria._type) > 0 &&
                 _supressedSignalsMap[pos->mEntryCriteria._type] == true)) {
                std::string n = _pMarketable->Name() + "_" + to_string(_moniker);
                if (gDebug) {
                    cout << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                         << " for "
                         << n << endl;
                    _logA << "signal is supressed " << EntryCriteria::getEntryType(pos->mEntryCriteria._type)
                          << " for "
                          << n << " Position id " << pos->Id << endl;
                }
                pos->mExitCode = Position::SUPRESSEDSIGNAL;
                pos->mState = Position::Closed;
                pos->Remaining = abs(pos->Size);
                _positions.push_back(pos);
                if (IsQualifiedTracker(pos)) {
                    _openpositions.push_back(pos);
                }
            } else  // actually send the order
            {
                vector<int> expectedfills;
                expectedfills.push_back(pos->Size);

                vector<bool> entryflags;
                entryflags.push_back(true);  //since this is an exit

                int qty = pos->Size;
                TransmitOrder(qty, pxd.getAsDouble(), _aggressorin, positions, expectedfills,
                              entryflags,
                              "Enter New Position (KR) " + to_string(pos->Id) + " @" +
                              to_simple_string(gAsofDate));
            }

            if (_stopKR) {
                for (auto &op : _openpositions) {
                    if ((op->Size > 0) != krsignal) {
                        // stop out all positions that go against the krsignal
                        bool canclose =
                                op->mState == Position::Open ||
                                (op->mState == Position::OpenPending && abs(op->ComputeNet()) > 0);
                        if (canclose) {
                            op->Close(Position::ExitCode::EXIT, pxd);
                            try {

                                int offsettoclose = -op->ComputeNet();
                                if (offsettoclose != -op->Size) {
                                    // case of a partial fill
                                    if (op->OrderIdEntry != 0) {
                                        _logA << "Stopping out a Partially filled position at KR id:" << op->Id
                                              << endl;
                                        _logA << "Cancelling entry order # " << op->OrderIdEntry << endl;
                                        pTrader->CancelOrder(op->OrderIdEntry);
                                    } else {
                                        cerr << "missing entry order id" << endl;
                                    }
                                }
                                vector<Position *> positions3;
                                positions3.push_back(op);
                                vector<int> expectedfills;
                                expectedfills.push_back(offsettoclose);
                                vector<bool> entryflags;
                                entryflags.push_back(false);  //since this is an exit

                                // always stop out aggressively
                                TransmitOrder(offsettoclose, pxd.getAsDouble(), /*_aggressorout*/ true, positions3,
                                              expectedfills,
                                              entryflags,
                                              "Stop Position on KR:" + to_string(op->Id));
                            }
                            catch (const char *message) {
                                cout << "EXCEPTION: " << message << endl;
                            }
                        }
                    }
                }
            }
        }

    }
}

bool StrategyA::IsTradeable() {
    return _tradeable;
}


bool StrategyA::IsTradingHalted() {

    if (_tradingHalt && gAsofDate >= sHalt1 && gAsofDate <= sResume1) {
        return true;
    }
    return false;
}

bool StrategyA::IsSupressed(EntryCriteria::EntryType value) {
    return (_supressedSignalsMap.count(value) > 0 &&
            _supressedSignalsMap[value] == true);
}

int lastDataLogCounter = 0;


int StrategyA::CloseOpenPositions() {
    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);
    Decimal bid = cdls->back()->LSTB;
    Decimal ask = cdls->back()->LSTA;
    _pMarketable->BestBid.Price = bid;
    _pMarketable->BestAsk.Price = ask;
    _pMarketable->BestBid.Timestamp = gAsofDate;
    _pMarketable->BestAsk.Timestamp = gAsofDate;
    Decimal mid = (bid + ask) / Decimal(2.0);
    int net = Net();
    for (auto it = _openpositions.begin(); it != _openpositions.end(); it++) {
        (*it)->Close(Position::ExitCode::EXIT, mid);
        int offsettoclose = -(*it)->ComputeNet();
        if (offsettoclose != -(*it)->Size) {
            // case of a partial fill
            if ((*it)->OrderIdEntry != 0) {
                _logA << "Expiring out a Partially filled position id:" << (*it)->Id << endl;
                _logA << "Cancelling entry order # " << (*it)->OrderIdEntry << endl;
                pTrader->CancelOrder((*it)->OrderIdEntry);
            } else {
                cerr << "misisng entry order id" << endl;
            }
        }

    }

    if (net != 0) {
        // Special case of an offset position
        auto pos = new Position(Position::nextId, gAsofDate,
                                net > 0 ? bid : ask,
                                -net);

        Position::nextId++;
        pos->mEntryCriteria._type = EntryCriteria::EntryType::OFFSET;
        pos->mExitCriteria._expiry = ptime(date(2030, Jan, 1),
                                           time_duration(hours(0)));  //  essentially manage manually
        //pos->Remaining= abs(pos->Size)  ;
        _positions.push_back(pos);
        _numPositions++;

        _logA << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
              << pos->Size << endl;
        cout << _pMarketable->Name() << FrequencyString() << " net offset position (position expiring today) is "
             << pos->Size << endl;
    } else {
        _logA << _pMarketable->Name() << "_" << FrequencyString()
              << " net offset position (position expiring today) is nothing expires today" << endl;
    }
    return net;
}


int StrategyA::ExecuteExits(Decimal bid, Decimal ask) {

    if ((ask - bid > _toowide || ask < bid) && gMarketHalted == false) {

        if (crossCountMap.count(gAsofDate.date()) > 0) {
            int num = crossCountMap[gAsofDate.date()].numTooWide;
            crossCountMap[gAsofDate.date()].numTooWide = ++num;
            crossCountMap[gAsofDate.date()].lastWide = gAsofDate;

        } else {
            crossCountMap[gAsofDate.date()].numTooWide = 1;
            crossCountMap[gAsofDate.date()].firstWide = gAsofDate;
            crossCountMap[gAsofDate.date()].lastWide = gAsofDate;
        }
        if (_lastDataLog.is_not_a_date_time()) {
            _lastDataLog = gAsofDate;
        }

        if ((gAsofDate - _lastDataLog) > time_duration(0, 0, 30)) {
            if (lastDataLogCounter == 0) {
                cerr << "unusual price  at " << to_simple_string(gAsofDate) << "  for " << Name() << "_"
                     << FrequencyString() << endl;
                cerr << dec::toString(bid) << "/" << dec::toString(ask)
                     << endl;
                cerr << "LKGP was received at " << to_simple_string(_pMarketable->BestBid.Timestamp) << endl;
                cerr << dec::toString(_pMarketable->BestBid.Price) << "/"
                     << dec::toString(_pMarketable->BestAsk.Price)
                     << endl;
                lastDataLogCounter++;
                if (lastDataLogCounter > 100) {
                    lastDataLogCounter = 0;
                }
            }
            _lastDataLog = gAsofDate;
        }

        return -1;
    }
    // set the last known decent market
    _pMarketable->BestBid.Price = bid;
    _pMarketable->BestAsk.Price = ask;
    _pMarketable->BestBid.Timestamp = gAsofDate;
    _pMarketable->BestAsk.Timestamp = gAsofDate;


    if (IsTradingHalted()) {
        return 0;
    }

    int state = 0;
    if (_positions.size() > 0) {
        if (_positions.back()->mEntryCriteria._type == EntryCriteria::LH) {
            state = 1;
        } else if (_positions.back()->mEntryCriteria._type == EntryCriteria::HL) {
            state = 2;
        }
    }

    vector<Position *> newReversals;
    vector<Position *> constrainedPositions;  // ones thaat could not be closed due to max exposure constraint

    for (auto it = OpenPositions().begin(); it != OpenPositions().end();) {

        Decimal stopOutPx(0);
        Decimal targetOutPx(0);

        std::function<bool()> targetHit;
        std::function<bool()> stopHit;
        std::function<bool()> hardStopHit;
        std::function<bool()> winner;

        Position *p = (*it);

        if (_aggressorout) {
            //cout << "target passive" << endl;
            targetOutPx = stopOutPx = p->Size > 0 ? ask : bid;
        } else {
            //cout << "target active" << endl;
            targetOutPx = stopOutPx = p->Size > 0 ? bid : ask;
        }

        if (p->Size > 0) {
            targetHit = [&]() {
                return (targetOutPx >= p->Target());
            };
            stopHit = [&]() {
                return (stopOutPx <= p->Stop());
            };
            hardStopHit = [&]() {
                return false;  //TBD
            };
        } else {
            targetHit = [&]() {
                return (targetOutPx <= p->Target());
            };
            stopHit = [&]() {
                return (stopOutPx >= p->Stop());
            };
            hardStopHit = [&]() {
                return false;
            };
        }

        bool qtonly = IsQualifiedTracker(p) && IsSupressed(p->mEntryCriteria._type);
        bool didreverse = false;
        bool canclose =
                p->mState == Position::Open || (p->mState == Position::OpenPending && abs(p->ComputeNet()) > 0);

        if (_signalExpiry > 0 && gAsofDate - p->TimeStamp > time_duration(0, _signalExpiry, 0) &&
            canclose) //TODO:  what about addon positions
        {
            Instrument *ins = static_cast<Instrument *>(_pMarketable);
            vector<Candle *> *cdls = ins->GetCandles(_candleQuantum > 0 ? _candleQuantum : _freq);

            Decimal mid = (bid + ask) / Decimal(2.0);
            bool expired = p->IsSignalExpired(cdls, _signalExpiry, mid);
            if (expired) {
                p->Close(Position::ExitCode::EXPIRE, mid);
                int offsettoclose = -p->ComputeNet();
                if (offsettoclose != -p->Size) {
                    // case of a partial fill
                    if (p->OrderIdEntry != 0) {
                        _logA << "Expiring out a Partially filled position id:" << p->Id << endl;
                        _logA << "Cancelling entry order # " << p->OrderIdEntry << endl;
                        pTrader->CancelOrder(p->OrderIdEntry);
                    } else {
                        cerr << "misisng entry order id" << endl;
                    }
                }
                vector<Position *> positions;
                positions.push_back(p);
                vector<int> expectedfills;
                expectedfills.push_back(offsettoclose);
                vector<bool> entryflags;
                entryflags.push_back(false);  //since this is an exit

                // always stop out aggressively
                TransmitOrder(offsettoclose, mid.getAsDouble(), /*_aggressorout*/ true, positions,
                              expectedfills,
                              entryflags,
                              "Expire Position:" + to_string(p->Id));
            }
        }


        canclose =
                p->mState == Position::Open || (p->mState == Position::OpenPending && abs(p->ComputeNet()) > 0) ||
                qtonly;


        if (canclose) {
            if (p->mExitCriteria._stoplevel > Decimal(0) && stopHit()) {
                try {
                    if (!qtonly) {  // its NOT just a marker
                        p->Close(Position::ExitCode::STOP, p->Stop());
                        int offsettoclose = -p->ComputeNet();
                        if (offsettoclose != -p->Size) {
                            // case of a partial fill
                            if (p->OrderIdEntry != 0) {
                                _logA << "Stopping out a Partially filled position id:" << p->Id << endl;
                                _logA << "Cancelling entry order # " << p->OrderIdEntry << endl;
                                pTrader->CancelOrder(p->OrderIdEntry);
                            } else {
                                cerr << "misisng entry order id" << endl;
                            }
                        }
                        vector<Position *> positions;
                        positions.push_back(p);
                        vector<int> expectedfills;
                        expectedfills.push_back(offsettoclose);
                        vector<bool> entryflags;
                        entryflags.push_back(false);  //since this is an exit


                        auto temp = TransmitOrder(offsettoclose, p->Stop().getAsDouble(), _aggressorout, positions,
                                                  expectedfills,
                                                  entryflags,
                                                  "Stop Position:" + to_string(p->Id));
                        if (temp != 0)
                            constrainedPositions.push_back(temp);

                    }
                    if (gNewSignal && (_scale == 2 || _scale == 3))     // 1:du  2:dd  3:du and dd
                    {
                        if (p->mEntryCriteria._type == EntryCriteria::HL &&
                            /*signal is not supressed*/ !IsSupressed(EntryCriteria::RHL))
                            // we have been buying higher lows
                        {
                            auto pos = new Position(Position::nextId, gAsofDate,
                                                    ((-p->Size > 0) == _aggressorin) ? ask : bid,
                                                    -p->Size);

                            Position::nextId++;
                            pos->mEntryCriteria._type = EntryCriteria::EntryType::RHL;
                            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time

                            Decimal refrate = (pos->Size > 0 ? bid : ask);
                            pos->mExitCriteria._stoplevel = refrate + (pos->Size > 0 ? -_stopsize : _stopsize);
                            pos->mExitCriteria._targetlevel =
                                    refrate + (pos->Size > 0 ? _targetsize : -_targetsize);
                            //pos->Remaining= abs(pos->Size)  ;
                            _positions.push_back(pos);
                            newReversals.push_back(pos);
                            _numPositions++;

                            _logA << _pMarketable->Name() << "RHL position is " << pos->ToString() << endl;
                            cout << _pMarketable->Name() << "RHL position is " << pos->ToString() << endl;
                            if (IsTradeable()) {
                                if (_scalePositions != 0)
                                    ScalePositions(pos, _scalefactor);
                                vector<Position *> positions2;
                                positions2.push_back(pos);
                                vector<int> expectedfills2;
                                expectedfills2.push_back(pos->Size);
                                vector<bool> entryflags2;
                                entryflags2.push_back(true);  //since this is an entry
                                double px = (pos->Size > 0 ? bid
                                                           : ask).getAsDouble();
                                TransmitOrder(pos->Size, px, _aggressorin, positions2, expectedfills2,
                                              entryflags2,
                                              "RHL Position:" + to_string(pos->Id));
                            }
                            didreverse = true;

                        } else if (p->mEntryCriteria._type == EntryCriteria::LH &&
                                   !IsSupressed(EntryCriteria::RLH))
                            // we have been selling lower highs
                        {
                            auto pos = new Position(Position::nextId, gAsofDate,
                                                    ((-p->Size > 0) == _aggressorin) ? ask : bid,
                                                    -p->Size);

                            Position::nextId++;
                            pos->mEntryCriteria._type = EntryCriteria::EntryType::RLH;
                            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
                            Decimal refrate = (pos->Size > 0 ? bid : ask);
                            pos->mExitCriteria._stoplevel = refrate + (pos->Size > 0 ? -_stopsize : _stopsize);
                            pos->mExitCriteria._targetlevel =
                                    refrate + (pos->Size > 0 ? _targetsize : -_targetsize);
                            //pos->Remaining= abs(pos->Size)  ;
                            _positions.push_back(pos);
                            newReversals.push_back(pos);
                            _numPositions++;

                            _logA << _pMarketable->Name() << "RLH position is " << pos->ToString() << endl;
                            cout << _pMarketable->Name() << "RLH position is " << pos->ToString() << endl;
                            if (IsTradeable()) {
                                if (_scalePositions != 0)
                                    ScalePositions(pos, _scalefactor);
                                vector<Position *> positions3;
                                positions3.push_back(pos);
                                vector<int> expectedfills3;
                                expectedfills3.push_back(pos->Size);
                                vector<bool> entryflags3;
                                entryflags3.push_back(true);  //since this is an entry
                                double px = (pos->Size > 0 ? bid
                                                           : ask).getAsDouble();
                                TransmitOrder(pos->Size, px, _aggressorin, positions3, expectedfills3,
                                              entryflags3,
                                              "RLH Position:" + to_string(pos->Id));
                            }
                            didreverse = true;
                        } else if (p->mEntryCriteria._type == EntryCriteria::KR &&
                                   !IsSupressed(EntryCriteria::RKR)) {
                            auto pos = new Position(Position::nextId, gAsofDate,
                                                    _aggressorin ? ask : bid,
                                                    -p->Size);

                            Position::nextId++;
                            pos->mEntryCriteria._type = EntryCriteria::EntryType::RKR;
                            pos->mExitCriteria._expiry = ComputeExpiry(); // expire at roll time
                            Decimal refrate = (pos->Size > 0 ? bid : ask);
                            pos->mExitCriteria._stoplevel = refrate + (pos->Size > 0 ? -_stopsize : _stopsize);
                            pos->mExitCriteria._targetlevel =
                                    refrate + (pos->Size > 0 ? _targetsize : -_targetsize);
                            //pos->Remaining= abs(pos->Size)  ;
                            _positions.push_back(pos);
                            newReversals.push_back(pos);
                            _numPositions++;

                            _logA << _pMarketable->Name() << "RKR position is " << pos->ToString() << endl;
                            cout << _pMarketable->Name() << "RKR position is " << pos->ToString() << endl;
                            if (IsTradeable()) {
                                vector<Position *> positions3;
                                positions3.push_back(pos);
                                vector<int> expectedfills3;
                                expectedfills3.push_back(pos->Size);
                                vector<bool> entryflags3;
                                entryflags3.push_back(true);  //since this is an entry
                                double px = (pos->Size > 0 ? bid
                                                           : ask).getAsDouble();
                                TransmitOrder(pos->Size, px, _aggressorin, positions3, expectedfills3,
                                              entryflags3,
                                              "RKR Position:" + to_string(pos->Id));
                            }
                            didreverse = true;
                        }
                    }
                }
                catch (exception e) {
                    cout << "EXITING  on EXCEPTION " << e.what() << endl;
                }

            } else if (p->mExitCriteria._targetlevel > Decimal(0) && targetHit()) {
                try {
                    if (!qtonly) {
                        p->Close(Position::ExitCode::TARGET, p->Target());
                        int offsettoclose = -p->ComputeNet();
                        if (offsettoclose != -p->Size) {
                            // case of a partial fill
                            if (p->OrderIdEntry != 0) {
                                _logA << "Target out a Partially filled position id:" << p->Id << " @ "
                                      << to_simple_string(gAsofDate) << endl;
                                _logA << "Cancelling entry order # " << p->OrderIdEntry << endl;
                                pTrader->CancelOrder(p->OrderIdEntry);
                            } else {
                                cerr << "missing entry order id" << endl;
                            }
                        }
                        vector<Position *> positions;
                        positions.push_back(p);
                        vector<int> expectedfills;
                        expectedfills.push_back(offsettoclose);
                        vector<bool> entryflags;
                        entryflags.push_back(false);  //since this is an exit


                        auto temp = TransmitOrder(offsettoclose, p->Target().getAsDouble(), _aggressorout,
                                                  positions,
                                                  expectedfills,
                                                  entryflags,
                                                  "Target Position:" + to_string(p->Id));
                        if (temp != 0)
                            constrainedPositions.push_back(temp);
                    }
                    if (gNewSignal && (_scale == 2 || _scale == 3)) {
                        if (p->mEntryCriteria._type == EntryCriteria::HH &&
                            !IsSupressed(EntryCriteria::RHH))
                            // we have been selling higher highs
                        {

                            auto pos = new Position(Position::nextId, gAsofDate,
                                                    ((p->Size > 0) == _aggressorin) ? ask : bid,
                                                    p->Size);

                            Position::nextId++;
                            pos->mEntryCriteria._type = EntryCriteria::EntryType::RHH;
                            pos->mExitCriteria._expiry = gRoll;
                            Decimal refrate = (pos->Size > 0 ? bid : ask);
                            pos->mExitCriteria._stoplevel = refrate +
                                                            (pos->Size > 0 ? -_stopsize * Decimal(_h2l2stopx) :
                                                             _stopsize * Decimal(_h2l2stopx));
                            pos->mExitCriteria._targetlevel =
                                    refrate + (pos->Size > 0 ? _targetsize : -_targetsize);
                            //pos->Remaining= abs(pos->Size)  ;
                            _positions.push_back(pos);
                            newReversals.push_back(pos);
                            _numPositions++;

                            _logA << _pMarketable->Name() << "RHH position is " << pos->ToString() << endl;
                            cout << _pMarketable->Name() << "RHH position is " << pos->ToString() << endl;
                            if (IsTradeable()) {
                                if (_scalePositions != 0)
                                    ScalePositions(pos, _scalefactor);
                                vector<Position *> positions3;
                                positions3.push_back(pos);
                                vector<int> expectedfills3;
                                expectedfills3.push_back(pos->Size);
                                vector<bool> entryflags3;
                                entryflags3.push_back(true);  //since this is an entry
                                double px = (pos->Size > 0 ? bid
                                                           : ask).getAsDouble();
                                TransmitOrder(pos->Size, px, _aggressorin, positions3, expectedfills3,
                                              entryflags3,
                                              "RHH Position:" + to_string(pos->Id));
                            }
                            didreverse = true;
                        } else if (p->mEntryCriteria._type == EntryCriteria::LL &&
                                   !IsSupressed(EntryCriteria::RLL))
                            // we have been selling higher highs
                        {
                            auto pos = new Position(Position::nextId, gAsofDate,
                                                    ((p->Size > 0) == _aggressorin) ? ask : bid,
                                                    p->Size);

                            Position::nextId++;
                            pos->mEntryCriteria._type = EntryCriteria::EntryType::RLL;
                            pos->mExitCriteria._expiry = gRoll;
                            Decimal refrate = (pos->Size > 0 ? bid : ask);
                            pos->mExitCriteria._stoplevel = refrate +
                                                            (pos->Size > 0 ? -_stopsize * Decimal(_h2l2stopx) :
                                                             _stopsize * Decimal(_h2l2stopx));
                            pos->mExitCriteria._targetlevel =
                                    refrate + (pos->Size > 0 ? _targetsize : -_targetsize);
                            //pos->Remaining= abs(pos->Size)  ;
                            _positions.push_back(pos);
                            newReversals.push_back(pos);
                            _numPositions++;

                            _logA << _pMarketable->Name() << "RLL position is " << pos->ToString() << endl;
                            cout << _pMarketable->Name() << "RLL position is " << pos->ToString() << endl;
                            if (IsTradeable()) {
                                if (_scalePositions != 0)
                                    ScalePositions(pos, _scalefactor);
                                vector<Position *> positions3;
                                positions3.push_back(pos);
                                vector<int> expectedfills3;
                                expectedfills3.push_back(pos->Size);
                                vector<bool> entryflags3;
                                entryflags3.push_back(true);  //since this is an entry
                                double px = (pos->Size > 0 ? bid
                                                           : ask).getAsDouble();
                                TransmitOrder(pos->Size, px, _aggressorin, positions3, expectedfills3,
                                              entryflags3,
                                              "RLL Position:" + to_string(pos->Id));
                            }
                            didreverse = true;
                        }
                    }

                }
                catch (exception e) {
                    cout << "EXITING  on EXCEPTION " << e.what() << endl;
                }

            }
        }

        if (qtonly && didreverse) {
            cout << "ERASING " << (*it)->ToString() << endl;
            _logA << "ERASING " << (*it)->ToString() << endl;
            it = OpenPositions().erase(it);
        } else
            (++it);
    }


    if (constrainedPositions.size() > 0)  //  this position could not be closed due to exposure constraint
    {
        for (auto &cp: constrainedPositions) {
            _logA << "Constrained Position " << cp->ToString() << endl;
            bool found = false;

            for (auto it = _openpositions.begin(); it != _openpositions.end();) {
                if ((*it)->mState != Position::OpenPending)  // some closed positions may not have been removed
                {
                    ++it;
                    continue;
                }
                if ((*it)->FillPx == Decimal(0)) {
                    if ((*it)->mExitCode != Position::MISSEXPOSURE && (*it)->mState != Position::OpenPending) {
                        cerr << "another case when FillPX maybe zero " << (*it)->ToString()
                             << endl;  // TODO: check this
                    }
                }
                if (cp->Size == -(*it)->Size && (*it)->FillPx != Decimal(0)) {
                    cp->CloseFillPx = (*it)->FillPx;
                    cp->ClosePx = (*it)->FillPx;
                    cp->CloseRemaining = 0;
                    cp->CloseFillTimeStamp = gAsofDate;
                    cp->mState = Position::Closed;
                    _logA << "erasing op " << (*it)->ToString() << endl;
                    it = _openpositions.erase(it);
                    found = true;
                    break;
                } else {
                    ++it;
                }
            }
            if (found) {
                _logA << "Constrained position closed on match " << cp->ToString() << endl;
            } else {
                _logA << "CRITICAL:  no corresponding offset found for   " << cp->ToString() << endl;
            }
        }
    }


    if (newReversals.size() > 0)
        _openpositions.insert(_openpositions.end(), newReversals.begin(),
                              newReversals.end());  // add the new reversals here since they cannot be added in the iterator major loop

    map<int, bool> noduplicates;
    for (auto &i : _openpositions) {
        if (noduplicates.count(i->Id) == 0)
            noduplicates[i->Id] = true;
        else
            cout << "duplicate id " << i->Id << endl;
    }
    ExecuteScalePositions(bid, ask);
    return 0;
}


int
StrategyA::findChangePointsEx(vector<double> v, vector<double> vd, int start, vector<int> &vindex,
                              vector<bool> &vbs) {
    int retval = -1;
    if (!v.empty()) {
        int sign = sgn<double>(v[0]);
        for (size_t i = 0; i <= v.size() - 1; i++) {
            if (sgn<double>(v[i]) != sign) {
                vindex.push_back(start + i);

                bool bs;
                if (sign < sgn<double>(v[i]))
                    bs = true;
                else
                    bs = false;

                if (bs != (sgn<double>(vd[i]) > 0) ? true : false) {
                    cout << "lag/swift change in direction, probably @" << to_simple_string(gAsofDate) << endl;
                    /*
                    cout << (bs ? "BUY" : "SELL") << endl;
                    cout << "sign of deriv: " << sgn<double>(v[i]) << endl;
                    cout << v[i] << endl;
                    PrintVector(v, i, cout);
                    cout << "sign of second deriv: " << sgn<double>(vd[i]) << endl;
                    cout << vd[i] << endl;
                    PrintVector(vd, i, cout);
                     */
                }
                vbs.push_back(bs);
                retval = i;
                sign = sgn<double>(v[i]);
            }
        }
    }
    return retval;
}

int StrategyA::GetZerosEx(ptime cdlts, real_1d_array x, real_1d_array y, int start, vector<int> &vindex,
                          vector<bool> &vbs, vector<double> &vs, vector<double> &vds, vector<double> &vd2s) {
    int retval = 0;

    try {
        Derivatives(x, y, _rho, x.length() * 3, vs, vds, vd2s);
    }
    catch (exception ex) {
        _logA << "exception in derivatives" << endl;
    }
    retval = findChangePointsEx(vds, vd2s, start, vindex, vbs);
    return retval;
}

int StrategyA::Derivatives(real_1d_array x, real_1d_array y, double rho, ae_int_t m, vector<double> &vs,
                           vector<double> &vds, vector<double> &vd2s) {
    ae_int_t info;
    spline1dinterpolant c;
    spline1dfitreport rep;
    (void) rho;
    // number of basis functions

    /*
    clock_t t;
    t = clock();
    */
    switch (_optimizeFit) {
        case 0:
            //  3.12 obsolete
            spline1dfitpenalized(x, y, m, _rho, info, c, rep);
            break;
        case 1:
            // a rho of 6 in 3.12 is equivalent to rho of 0.001 in 3.17
            spline1dfit(x, y, m, _rho, c, rep);
            break;
        case 2:
            spline1dbuildcubic(x, y, c);
            break;

    }


    /*
    t = clock() - t;
    printf("It took me %d clicks (%f seconds).\n", t, ((float)t) / CLOCKS_PER_SEC);
    */
    // printf("fit result %d\n", int(info)); // EXPECTED: 1

    for (int i = 0; i < x.length(); i++) {
        // auto r = alglib::spline1dcalc(c, x[i]);
        double s, ds, d2s;
        spline1ddiff(c, x[i], s, ds, d2s);
        vs.push_back(s);
        vds.push_back(ds);
        vd2s.push_back(d2s);
    }

    return 0;
}


bool StrategyA::UpdateChangePointsV7(const vector<bool> vbs, const vector<int> currentChangeIndex) {
    bool retval = false;
    if (currentChangeIndex.size() == 0)
        return retval;


    if (_changeIndex.size() == 0) {
        _changeIndex = currentChangeIndex;
        _vbs = vbs;
        retval = true;
    } else {
        // bool buy = vbs.back();
        // is there a new turning point
        if (_changeIndex.back() != currentChangeIndex.back()) {
            _changeIndex = currentChangeIndex;
            if (_vbs.back() != vbs.back()) {
                _vbs = vbs; // bug fix: changed on Oct 31
                retval = true;  // bug in WQ code?
            }
        }
    }
    return retval;
}


void StrategyA::GraphEx(ptime date, const vector<double> &vs) {

    if (_ofsSpline.is_open()) {
        //string d = Utility::posixTimeToString(date,"%Y%m%d %H:%M");
        string d = to_simple_string(date);

        _ofsSpline << d << endl;
        //cout << vs.size() << endl;
        for (size_t i = 0; i < vs.size(); i++) {
            _ofsSpline << vs[i] << ",";
        }
        _ofsSpline << std::endl;
    } else {
        _logA << "cannot open spline file " << endl;
    }
}

/*
void StrategyA::GraphEx(ptime date, alglib::real_1d_array AX, alglib::real_1d_array AY, double rho, ae_int_t m,
                        const vector<double> &vs, const vector<double> &vds, const vector<double> &vd2s,
                        bool printLS) {

    if (_ofsSpline.is_open()) {
        //string d = Utility::posixTimeToString(date,"%Y%m%d %H:%M");
        string d = to_simple_string(date);

        _ofsSpline << d << endl;
        //cout << vs.size() << endl;
        for (size_t i = 0; i < vs.size(); i++) {
            _ofsSpline << vs[i] << ",";
        }
        _ofsSpline << std::endl;
    } else {
        _logA << "cannot open spline file "  << endl;
    }
}
*/