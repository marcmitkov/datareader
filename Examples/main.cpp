/**
 * @file    main.cpp
 * @short   Program entry point.
 */

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <iostream>



IAdapterOut* pTrader=0;


#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

//#include <iostream>
int
test()
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    date d(2002,Feb,1); //an arbitrary date
    //construct a time by adding up some durations durations
    ptime t1(d, hours(5)+minutes(4)+seconds(2)+millisec(1));
    //construct a new time by subtracting some times
    ptime t2 = t1 - hours(5)- minutes(4)- seconds(2)- millisec(1);
    //construct a duration by taking the difference between times
    time_duration td = t2 - t1;

    std::cout << to_simple_string(t2) << " - "
              << to_simple_string(t1) << " = "
              << to_simple_string(td) << std::endl;

    std::string s = to_simple_string(t1);
    std::string s1 = to_string(1);
    std::string s2 = dec::toString(Decimal(1.0));
    s+=s1 +s2;
    return 0;
}
map<string, string> _strategyMap;
map<string,map<string, string> >stratAparamMap;
map<string,vector<string> >stratAFreqMap;
extern bool gDebug;
void GetParams(VelioSessionManager &sessionManager)
{
    boost::char_separator<char> sep{ ",", "", boost::keep_empty_tokens };

    const char * StratAProperty = "StratA";

    const char * StratAStr = sessionManager.getManifestParser()->getProperty( StratAProperty ) ;

    //if (StratAStr !== "")
    if (StratAStr != 0)
    {
        string StratAStr2 = StratAStr;
        if(gDebug)
            cout<<"GetParams -- StratA :"<<StratAStr2<<endl;

        boost::tokenizer<boost::char_separator<char>> tok(StratAStr2, sep);

        for (const auto& t : tok) {
            if (gDebug) {
                cout << "====================" << endl;
                cout << "StratA item:" << t << endl;
            }

            string StratAFreqProperty = t + "_Freq";
            const char * StratAFreqStr = sessionManager.getManifestParser()->getProperty(StratAFreqProperty .c_str()) ;

            string StratAFreqStr2=StratAFreqStr;
            if (StratAFreqStr && StratAFreqStr[0]) {
                if (gDebug) {
                    cout << "Freq  List for " << t << ": " << StratAFreqStr2 << endl;
                }
            }
            else{
                if (gDebug) {
                    cout << "Skip. No Frequency in config for " << StratAFreqProperty << endl;
                }
                continue;
            }

            boost::tokenizer<boost::char_separator<char>> tokFreq(StratAFreqStr2, sep);
            for (const auto& freq : tokFreq) {

                string StratAParamsProperty = t + "_" + freq + "_Params";
                const char *StratAParamsStr = sessionManager.getManifestParser()->getProperty(
                        StratAParamsProperty.c_str());
                if (StratAParamsStr  && StratAParamsStr[0]){
                    if (gDebug) {
                        cout << "Params List for  " << StratAParamsProperty << ": " << StratAParamsStr << endl;
                    }
                }
                else{
                    if (gDebug) {
                        cout << "Skip. No Frequency in config for " << StratAParamsProperty << endl;
                    }
                    continue;
                }

                string StratAParamsStr2 = StratAParamsStr;
                boost::tokenizer<boost::char_separator<char>> tok2(StratAParamsStr2, sep);
                map<string, string> paramsMap;
                for (const auto& param : tok2) {
                    if (gDebug) {
                        cout << "param: " << param << endl;
                    }
                    string param2 = param;
                    boost::tokenizer<boost::char_separator<char>> tokKeyVal(param2, sep);
                    vector<string> result;
                    boost::split(result,param,boost::is_any_of(":"));
                    if (gDebug) {
                        cout << "Inserting into params map - Key:" << result[0] << " Value: " << result[1] << endl;
                    }
                    paramsMap[result[0]] = result[1];

                }

                cout<<"Chanel: Inserting params map into stratAparamMap, key: "<< t +  "_" + freq <<endl;

                stratAparamMap[t +  "_" + freq] = paramsMap;

            }
        }
    }
    else
    {
        cout <<  "Chanel: no StratA defined"  << endl;
        return;
    }

    if (gDebug) {
        map<string, map<string, string> >::iterator itr;
        cout << "printing param map" << endl;
        for (itr = stratAparamMap.begin(); itr != stratAparamMap.end(); ++itr) {

            cout << itr->first << endl;
            cout << "minlen => " << stratAparamMap[itr->first]["minlen"] << endl;
            cout << "rho => " << stratAparamMap[itr->first]["rho"] << endl;
        }
        cout << "end  printing param map" << endl;
    }
}
extern string gOutputDirectory;
int main(int argc, char *argv[])
{
    test();
    const char * futuresQueuePositionProperty = "trade_model.futures_queue_position" ;
    const char * logTypeProperty = "trade_model.log_type" ;
    const char * logNameProperty = "trade_model.log_name" ;
    const char * logLevelProperty = "trade_model.log_level" ;
    const char * allowProdProperty = "trade_model.allow_prod" ;
    const char * prodHostPrefixProperty = "trade_model.prod_host_prefix" ;

    ArgumentParser::parseArguments(argc, argv);
    VelioSessionManager sessionManager;     // Session manager maintains registered models and starts data processing.
    sessionManager.initialize();            // Initialize session manager.

    // Create an additional logger for the model that is separate from the main model lib log.
    // This is not necessary, but can be used to separate the messages from this part of the program
    // from the main logging of the model lib if desired.  All logging is done on maintenance thread.
    // NOTE: Never log price updates to system log, this can flood loghost and cause network congestion.

    const char * logTypeStr = sessionManager.getManifestParser()->getProperty( logTypeProperty ) ;
    const char * logNameStr = sessionManager.getManifestParser()->getProperty( logNameProperty ) ;
    const char * logLevelStr = sessionManager.getManifestParser()->getProperty( logLevelProperty ) ;
    HC::Logger::LOGGER_TYPE logType = HC::Logger::CONSOLE_LOG ;
    if ( logNameStr && logNameStr[0] ) {
        if ( 0 == strncmp ( logTypeStr, "SYS_LOG", 7 /* compare length */ ) ) {
            logType = HC::Logger::SYS_LOG ;
        } else if ( 0 == strncmp ( logTypeStr, "FILE_LOG", 8 /* compare length */ ) ) {
            logType = HC::Logger::FILE_LOG ;
        }
    }

    // Creates a logger that is separate from the application logger.
    uint32_t logLevel = ( logLevelStr ? atoi( logLevelStr ) : 1 /* INFO */ ) ;
    if ( 2 /* ERROR */ < logLevel ) {
        logLevel = 2 ; /* ERROR */
    }
    HC::Logger logger( logNameStr, logType );
    logger.setLogLevel( logLevel ) ;
    logger.start();

    if ( logType == HC::Logger::CONSOLE_LOG ) {
        LOG_INFO_CUSTOM((&logger), "Warning: Using CONSOLE_LOG type is not supported / permitted in production." ) ;
    } else {
        LOG_INFO_CUSTOM((&logger), "Started Model Log named %s", logNameStr ) ;
    }

    ConfigKeeper config( logger, sessionManager ) ;
    config.load( ) ;

    // This program will only run on UAT, NextGen, or backtest machines.
    // Invalid host will cause the program to exit.  Prevent running on production by mistake.

    const char * allowProdStr = sessionManager.getManifestParser()->getProperty( allowProdProperty ) ;
    const char * prodHostPrefixStr = sessionManager.getManifestParser()->getProperty( prodHostPrefixProperty ) ;

    if ( !VerifyHost::verifyHost( logger, prodHostPrefixStr, ( 0 == strcmp( allowProdStr, "true" ) ) ) ) {
        LOG_CRITICAL_AND_EXIT("Invalid hostname.");
    }

    // Model and books will be instantiated via factory callback.  Get parameters to initialize the factory.
    // Market type FX = use TopFxModel from topfxmodel.cpp
    // Market type FUTURES = use TopFtModel from topftmodel.cpp
    // TopFxModel = FX trading + Futures market data (no futures execution)
    // TopFtModel = Futures trading

    gMarketType = config.getMarketType( ) ;
    MARKET_TYPE market = (MARKET_TYPE)gMarketType;
    const char * futuresQueuePositionStr = sessionManager.getManifestParser()->getProperty( futuresQueuePositionProperty ) ;
    bool useFuturesQueuePosition = false ;
    if ( futuresQueuePositionStr && 0 == strcasecmp( futuresQueuePositionStr, "true" ) ) {
        useFuturesQueuePosition = true ; // Setting for ICE / CME / LIFFE only. EUREX always has it.
    }
    TopModelAndBookFactory factory( logger, config, market, useFuturesQueuePosition ) ;
    sessionManager.registerModelFactory( &factory );
    sessionManager.registerBookFactory( &factory );
 //MR
    const char * VAR1Property = "VAR1" ;
    const char * StratAProperty = "StratA";


    string VAR1Str = sessionManager.getManifestParser()->getProperty( VAR1Property ) ;
    string pv = sessionManager.getManifestParser()->getProperty("OutputDirectory");
    if (!pv.empty())
     gOutputDirectory = pv;
    pv =  sessionManager.getManifestParser()->getProperty("Chanel_Debug");
    if (!pv.empty()) {
        gDebug = pv == "true";
    }
    cout << "Chanel_Debug is "   << (gDebug?"ON":"OFF") << endl;

    pv =  sessionManager.getManifestParser()->getProperty("Chanel_SendOrder");
    if (!pv.empty()) {
        gSendOrder = pv == "true";
    }
    cout << "Chanel_SendOrder is "   << (gSendOrder?"ON":"OFF") << endl;

    pv =  sessionManager.getManifestParser()->getProperty("Chanel_WriteCandles");
    if (!pv.empty()) {
        gWriteCandles = pv == "true";
    }
    cout << "Chanel_WriteCandles is "   << (gWriteCandles?"ON":"OFF") << endl;

    pv =  sessionManager.getManifestParser()->getProperty("Chanel_ParamNumber");
    if (!pv.empty()) {
        gParamNumber = std::atoi(pv.c_str());
    }
    cout << "Chanel_ParamNumber is "   << gParamNumber << endl;

    pv =  sessionManager.getManifestParser()->getProperty("Chanel_Roll");
    extern void ConvertFloat(float timedecimal, float *hours, float *mins);
    if (!pv.empty()) {
        //gRoll = ConvertFloat(std::atof(pv.c_str()), date(2020, Dec, 31));
        // ptime(basedt, hours(hour) + minutes(min * 60) + seconds(0) + millisec(0));
        float h, m;
        ConvertFloat(std::atof(pv.c_str()), &h, &m);
        gRollHours = h;
        gRollMins = m;
    }
    cout << "Chanel_Roll is "   <<  "Hours:" << gRollHours << ", Mins:"  << gRollMins  << endl;

    pv =  sessionManager.getManifestParser()->getProperty("Chanel_EOD");
    if (!pv.empty()) {
        float h, m;
        ConvertFloat(std::atof(pv.c_str()), &h, &m);
        gEODHours = h;
        gEODMins = m;
    }
    cout << "Chanel_EOD is "   <<  "Hours:" << gEODHours << ", Mins:"  << gEODMins  <<endl;

    if(VAR1Str == "" )
        cout<< VAR1Property << "is missing from property file" << endl;
    else
        _strategyMap["VAR1"]=VAR1Str;

    cout << CurrentThreadId("main")  << endl;

    string StratASymbols = sessionManager.getManifestParser()->getProperty( StratAProperty ) ;

    vector <string> symbols;
    // StratA => "6EZ0,ESZ0,EUR/USD"
    boost::char_separator<char> sep{",", "", boost::keep_empty_tokens};
    boost::tokenizer<boost::char_separator<char>> tok(StratASymbols, sep);
    symbols.assign(tok.begin(), tok.end());
    for(auto& s : symbols)
    {
        string symbol_Freq =  s + string("_Freq");  // EUR/USD_Freq = > 1, 5
        string freqs = sessionManager.getManifestParser()->getProperty(symbol_Freq.c_str());
        _strategyMap[s] = freqs;
    }

    GetParams(sessionManager);

    try {
        pTrader = new Trader(new AdapterIn(&factory));
    }
    catch(exception e)
    {
        cout << "exception creating trader " << endl;
    }

//    auto om = factory.getModel()->getOrderManager();
    cout << "Starting model lib for all models." << endl;
    LOG_INFO_CUSTOM((&logger), "Starting model lib for all models." ) ;
    sessionManager.run();
    return 0;
}


