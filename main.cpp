#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace std;

bool eof(vector<ifstream*> vec)
{
    bool x = false;
    for (int i = 0; i < vec.size(); i++) {
        //x = x || !(*vec[i]).eof();
        bool status = (*vec[i]).eof();
        x = x || !status;
        if (status == true)
            cout << "EOF reached for " << i << endl;

    }

    return x;

}
int main() {
    std::cout << "Hello, World!" << std::endl;
    string quotefile1 =  "ConfigParams.Instruments.Instrument1.File";
    string quotefile2 = "ConfigParams.Instruments.Instrument2.File";
    std::ifstream isq(quotefile1, ios::in );
    std::ifstream isq2(quotefile2, ios::in);

    vector<ifstream*> streamVector;
    streamVector.push_back(&isq);
    streamVector.push_back(&isq2);

    int vectorSize = streamVector.size();
    vector<bool> quoteProcessed;
    vector<time_t> timeStamp(vectorSize);
    vector<int64_t> msgType(vectorSize); //exist
    vector<int64_t> tickType(vectorSize);
    vector<string> value(vectorSize);//exist

    bool firstProcess = true;
    for (int i = 0; i < streamVector.size(); i++) {
        quoteProcessed.push_back(true);
        string line;
        //Skip first header line
        getline(*streamVector[i], line);
    }
    while (eof(streamVector))
    {
        for (int i = 0; i < streamVector.size(); i++) {
            if (quoteProcessed[i] == true && !streamVector[i]->eof()) {

                string line;
                getline(*streamVector[i], line);
                if (line == "")
                    continue;
                vector<string> fields;

                string ts = "";

                ts = fields[0];

                msgType[i] = stoi(fields[1]);
                tickType[i] = stoi(fields[2]);
                value[i] = fields[3] ;

                ptime periodStart = time_from_string(ts);
                time_t myT = boost::posix_time::to_time_t(periodStart);
                timeStamp[i] = myT;



                //std::cout << "ticks:" << ticksQ << ", sideQ:" << sideQ << " ,rateQ:" << rateQ << " ,size:" << sizeQ << endl;
                //	}
                quoteProcessed[i] = false;
            }
        }
        firstProcess = false;

        //  identify earliest timestamp  index
        std::vector<int64_t>::iterator result = std::min_element(std::begin(timeStamp), std::end(timeStamp));
        int t = std::distance(std::begin(timeStamp), result);


        quoteProcessed[t] = true;

        int symbolId;
        if (t == 0)
            symbolId = 1000;
        else
            symbolId = 1001;
        /*
        if (msgType[t] == TICK_PRICE) {
            if (tickType[t] == BID)
                m_pTrader->HandleQuotePx(symbolId, 0x0, Decimal(value[t]));
            else if (tickType[t] == ASK)
                m_pTrader->HandleQuotePx(symbolId, 0x1, Decimal(value[t]));
            else
                m_pTrader->HandleTradePx(symbolId, Decimal(value[t]));
        }
        else if (msgType[t] == TICK_SIZE) {
            if (tickType[t] == BID_SIZE)
                m_pTrader->HandleQuoteSz(symbolId, 0x0, stoi(value[t]));
            else if (tickType[t] == ASK_SIZE)
                m_pTrader->HandleQuoteSz(symbolId, 0x1, stoi(value[t]));
            else
                m_pTrader->HandleTradeSz(symbolId, stoi(value[t]));
        }
        else {
            cout << "ERROR:Wrong Message Type in Simulator" << endl;
        }
        */



        if (streamVector[t]->eof())
        {

            timeStamp[t] = (std::numeric_limits<int64_t>::max)();// std::numeric_limits<int>::max();
        }

    }

    for (int i = 0; i < streamVector.size(); i++)
        streamVector[i]->close();



    return 0;
}

#ifdef A
void Simulator::ProcessMultipleTick() {
    try {
        int counter = 0;
        string quotefile1 =  m_configMap["ConfigParams.Instruments.Instrument1.File"];
        string quotefile2 = m_configMap["ConfigParams.Instruments.Instrument2.File"];
        std::ifstream isq(quotefile1, ios::in );
        std::ifstream isq2(quotefile2, ios::in);

        // check if files were actually opened?
        if (!isq.is_open()) {
            cout << "Problem opening input file: " << quotefile1 << endl;
            //  doesnot wrok with CENTOS 7.2
            // throw exception("Problem opening quote input file 1");

        }

        if (m_configMap["ConfigParams.Mode"] == "1" && !isq2.is_open()) {
            cout << "Problem opening input file: " << quotefile2 << endl;
            // throw exception("Problem opening input file 2");

        }


        vector<ifstream*> streamVector;

        int64_t ticksQ;
        unsigned char sideQ;
        int64_t rateQ;
        int64_t sizeQ;


        streamVector.push_back(&isq); //0
        if (m_configMap["ConfigParams.Mode"] == "1") {
            streamVector.push_back(&isq2);// 2

        }

        int vectorSize = streamVector.size();
        vector<bool> quoteProcessed;
        vector<time_t> timeStamp(vectorSize);
        vector<int64_t> msgType(vectorSize); //exist
        vector<int64_t> tickType(vectorSize);
        vector<string> value(vectorSize);//exist


        /*for (int i = 0; i < streamVector.size(); i++) {
            quoteProcessed.push_back(true);
        }*/

        //For skipping header row
        bool firstProcess = true;
        for (int i = 0; i < streamVector.size(); i++) {
            quoteProcessed.push_back(true);
            string line;
            //Skip first header line
            getline(*streamVector[i], line);
        }
        while (eof(streamVector))
        {
            counter++;
            /*if (counter == 63339)
            cout << "there";*/
            Quote quote;
            Quote trade;
            for (int i = 0; i < streamVector.size(); i++) {
                if (quoteProcessed[i] == true && !streamVector[i]->eof()) {

                    string line;
                    getline(*streamVector[i], line);
                    if (line == "")
                        continue;
                    vector<string> fields;
                    fields = split(line, ',');

                    string ts = "";

                    ts = fields[0];

                    msgType[i] = stoi(fields[1]);
                    tickType[i] = stoi(fields[2]);
                    value[i] = fields[3] ;

                    ptime periodStart = time_from_string(ts);
                    time_t myT = boost::posix_time::to_time_t(periodStart);
                    timeStamp[i] = myT;



                    //std::cout << "ticks:" << ticksQ << ", sideQ:" << sideQ << " ,rateQ:" << rateQ << " ,size:" << sizeQ << endl;
                    //	}
                    quoteProcessed[i] = false;
                }
            }
            firstProcess = false;

            //  identify earliest timestamp  index
            std::vector<int64_t>::iterator result = std::min_element(std::begin(timeStamp), std::end(timeStamp));
            int t = std::distance(std::begin(timeStamp), result);


            quoteProcessed[t] = true;

            int symbolId;
            if (t == 0)
                symbolId = 1000;
            else
                symbolId = 1001;

            if (msgType[t] == TICK_PRICE) {
                if (tickType[t] == BID)
                    m_pTrader->HandleQuotePx(symbolId, 0x0, Decimal(value[t]));
                else if (tickType[t] == ASK)
                    m_pTrader->HandleQuotePx(symbolId, 0x1, Decimal(value[t]));
                else
                    m_pTrader->HandleTradePx(symbolId, Decimal(value[t]));
            }
            else if (msgType[t] == TICK_SIZE) {
                if (tickType[t] == BID_SIZE)
                    m_pTrader->HandleQuoteSz(symbolId, 0x0, stoi(value[t]));
                else if (tickType[t] == ASK_SIZE)
                    m_pTrader->HandleQuoteSz(symbolId, 0x1, stoi(value[t]));
                else
                    m_pTrader->HandleTradeSz(symbolId, stoi(value[t]));
            }
            else {
                cout << "ERROR:Wrong Message Type in Simulator" << endl;
            }




            if (streamVector[t]->eof())
            {

                timeStamp[t] = (std::numeric_limits<int64_t>::max)();// std::numeric_limits<int>::max();
            }

        }

        for (int i = 0; i < streamVector.size(); i++)
            streamVector[i]->close();
        //string candleDir = GetOutDir();
        //_candleMaker.WriteCandlesToFile(candleDir + "CandleMakerOut.csv", false);
    }
    catch (exception & e) {

        cout << "Simulator Exception -ProcessMultipleTick" << endl;
        throw e;
    }
}
#endif