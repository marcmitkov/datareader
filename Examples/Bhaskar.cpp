#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/thread.hpp>
#include <math.h>
#include <unordered_map>
#ifndef _GNU_SOURCE
#include "../WinBase/wingetopts.h"
#endif
#include <string>
#include <fstream>
#ifdef __GNUC__
#include <getopt.h>
#endif
#include "../Base/Statics.h"
#include "../Base/Candle.h"
typedef std::unordered_map<int, int> IntHash;
//------------------------------------------------------------------------------
using namespace std;
//------------------------------------------------------------------------------
extern void TestComms();
extern void StartComms();

extern void readeFifo(string namedPipe, string msg);
extern void writeFifo(string namedPipe, string msg);

map<string, WQSymbol> WQSymbolMap;

const vector<Anon::Candle>	GetCandles(string symbol, int freq)
{
    return *(new vector<Anon::Candle>());
}



void ProcessOrder(string m)
{

}
void ProcessRefresh(string symbol, string freq, string m)
{

}

int main(int argc, char *argv[]) {

    cout << "Entering ...." << endl;
    char *region = (char *) "us";
    char *opts = (char *) "r:atc";

    int option = -1;
    bool test = false;
    bool comms = false;
    while ((option = getopt(argc, argv, opts)) != -1) {
        switch (option) {
            case 'r':
                cout << optarg << endl;
                break;
            case 'a':
                region = (char *) "asia";
                break;
            case 't':
                test = true;
                break;
            case 'c':
                comms = true;
                break;
            default:
                cout << "Invalid Arg" << endl;
                break;
        }

    }




    vector<string>  names;



    if (region == "asia")
    {

    }


    map<string, int>  ecnMap;
    string fn = (region == "asia") ? "namesAsia.dat" : "names.dat";
    string nameEcnId;
    fstream ifile(fn);
    if (ifile.is_open())
    {
        while (std::getline(ifile, nameEcnId))
        {
            boost::char_separator<char> sep{ ";" };
            boost::tokenizer<boost::char_separator<char>> tok(nameEcnId, sep);
            vector<string>  vec;
            vec.assign(tok.begin(), tok.end());
            if (vec.size() == 2)
            {

                names.push_back(vec[0]);
                ecnMap[vec[0]] = atoi(vec[1].c_str());
            }
            else
            {
                cout << "missing ecnid ?" << endl;
            }
        }
    }
    else
    {
        cout << "unable to open file" << endl;
    }
    if(test)
    {
        TestComms();
    } else
    {
        StartComms();
    }
}

extern void reader(string);
extern void writer(string);
extern void readFifo(string);

void TestComms()
{

	boost::thread threadWriter( writer, "/tmp/myfifo" );
    //boost::this_thread::sleep_for(boost::chrono::seconds(1));
    boost::thread threadWriter2( writer, "/tmp/myfifo" );
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
	boost::thread threadReader(reader, "/tmp/myfifo" );
    threadWriter.join();
    threadReader.join();
    threadWriter2.join();
    std::cout << "Exiting StartComm" << std::endl;
}

extern void mainClient();
extern void mainServer();

void StartComms(){
	boost::thread threadFIFO {readFifo,"/tmp/togui"};
	boost::thread threadServer(mainServer);
	boost::thread threadClient{ mainClient };
	threadServer.join();
	threadClient.join();
	threadFIFO.join();
	std::cout << "Exiting StartComm" << std::endl;
}
