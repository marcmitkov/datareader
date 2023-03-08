#include "Utilities.h"
#include <cmath>


string CurrentThreadId(const char* f) {
    thread::id tid = this_thread::get_id();
    ostringstream o;
     o << "Thread id for "  << f <<  " is "<< tid << "@" <<  to_simple_string(gAsofDate) << endl;
    return o.str();
}

string GetCurrentWorkingDir(void) {

	boost::filesystem::path full_path(boost::filesystem::current_path());
	std::cout << "Current path is : " << full_path << std::endl;
	/*
	char buff[FILENAME_MAX];
	//GetCurrentDir(buff, FILENAME_MAX);
	string current_working_dir(buff);
	*/
	return full_path.string();
}

void CreateDirectoryNew(string p)
{
    boost::filesystem::path dir(p);
	if (boost::filesystem::create_directory(dir)  && gDebug)
	{
		std::cout << "Success creating " <<  p << "\n";
	}
    /*
	struct stat info;

	bool exists = true;
	if (stat(dir.c_str(), &info) != 0)
	{
		printf("cannot access %s\n", dir.c_str());
		exists = false;
	}
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
	{
		printf("%s is a directory\n", dir.c_str());
	}
	else
	{
		printf("%s is no directory\n", dir.c_str());
		exists = false;
	}

	if (exists == false)
	{
		if (boost::filesystem::create_directories(dir.c_str()))
		{
			std::cout << "Directory created" << endl;
		}
		else
		{
			std::cout << "Unable to create directory" << endl;
		}
	}
     */
}
 

string FormatTime(ptime now)
{
	std::stringstream stream;
	// Use a facet to display time in a custom format (only hour and minutes).
	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
	facet->format("%Y%m%d %H:%M:%S ");
	stream.imbue(std::locale(std::locale::classic(), facet));
	stream << now;
	return stream.str();
}

string PrintThreadID(string message) {
	thread::id tid = this_thread::get_id();
	ostringstream o;
	o << "Thread Id: " <<  tid << "  " << message  << "@" <<  to_simple_string(gAsofDate) << endl;
	// BOOST_LOG_TRIVIAL(info) << "Thread Id: " << tid << "  " << message;
	return o.str();
}
/*
void ReadConfig(string configPath, string configFile) {
	boost::property_tree::ptree pt;
	try {
		if (configPath == "") {
			configPath = GetCurrentWorkingDir();
			cout << "reading config params from " << configPath << "\\" + configFile << endl;
		}
		else
			cout << "reading config params from " << configPath << configFile << endl;
		read_xml(configPath + "\\" + configFile, pt);

		map<string, string>::iterator itr;

		for (auto& p : pt.get_child("Config")) {
			
			auto key = p.first;
			auto value = p.second.data();
			//cout << "Key: " << p.first << " Value: " << p.second.data() << endl;

			if (!gConfigMap.insert({ key, value }).second)
				throw std::runtime_error("Duplicate key: " + key);
		}

		for (itr = gConfigMap.begin(); itr != gConfigMap.end(); ++itr)
		{
			cout << itr->first << ":" << itr->second << endl;
		}
		//cout << gConfigMap["BaseDataDir"] << endl;
		
	}
	catch (exception e)
	{
		cout << "exception when reading config: " << e.what() << endl;
	}
}
*/
bool eof(vector<ifstream*> vec)
{
	bool x = false;
	for (size_t i = 0; i < vec.size(); i++) {
		//x = x || !(*vec[i]).eof();
		bool status = (*vec[i]).eof();
		x = x || !status;
		if (status == true)
			cout << "EOF reached for " << i << endl;

	}

	return x;

}

vector<string> Split(const string &s, char delim) {
	stringstream ss(s);
	string item;
	vector<string> tokens;
	while (getline(ss, item, delim)) {
		// tokens.push_back(item);
	}
	return tokens;
}

bool To_bool(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	std::istringstream is(str);
	bool b;
	is >> std::boolalpha >> b;
	return b;
}


string PosixTimeToStringFormat(ptime time, const char* timeformat) {
	std::stringstream stream;
	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
	//facet->format("%H:%M");
	facet->format(timeformat);
	stream.imbue(std::locale(std::locale::classic(), facet));
	stream << time;
	return stream.str();
}


vector<string> split(const string &s, char delim) {
	stringstream ss(s);
	string item;
	vector<string> tokens;
	while (getline(ss, item, delim)) {
//		tokens.push_back(item);
	}
	return tokens;
}

string GetDateDir(bool today, bool input) {
    (void) today;
    (void) input;
    string dir;
    string yyyymmdd = PosixTimeToStringFormat(gAsofDate, "%Y%m%d");

     return dir;
}

ptime AdvanceTime(date base, int hour,  int min, int sec)
{
    return ptime(base, hours(hour)  +  minutes(min)  + seconds(sec));
}

void ConvertFloat(float timedecimal, float *hours, float *mins)
{

    *mins = modf(timedecimal, hours)*60;

}
#include <boost/date_time.hpp>
boost::posix_time::ptime parse_time_object(const std::string &time, const std::string &format) {
    std::stringstream ss;
    ss << time.c_str();
    ss.imbue(std::locale( std::locale::classic(), new boost::local_time::local_time_input_facet(format.c_str())));
    boost::posix_time::ptime time_object;
    ss >> time_object;
    return time_object;
}


// global variable
ptime gAsofDate;
ptime gTomorrow;
ptime gRoll;
ptime gEOD;
int gMode=0;
map<string, string> gConfigMap;
bool gWriteTicks = true;
string gOutputDirectory = "./";
bool gDebug = false;
bool gSendOrder = false;
bool gBacktest;
bool gNewSignal=true;

int gRollHours;
int gRollMins;
int gEODHours;
int gEODMins;

std::mutex gOrderMapTradeMutex;
std::mutex gOrderMapStrategyMutex;
std::mutex gPositionsMutex;

bool gWriteCandles=true;
bool  gCancelKludge=false; //  cancelled orders
int gParamNumber=0;
ofstream ofglobal("cleanup.txt");

bool gEnableMultipleBooks=false;

bool IsDST(ptime now)
{
    // 2020  March 8 - Nov 1
    // 2021  March 14 - Nov 7
    date dstStart20(2020, Mar, 8);
    date dstEnd20(2020, Nov, 1);
    date dstStart21(2021, Mar, 14);
    date dstEnd21(2021, Nov, 7);

    bool retval = dstStart20 < now.date() && now.date() < dstEnd20;
    retval = retval || (dstStart21 < now.date() && now.date() < dstEnd21);
    return retval;
}

std::string int64_to_string( int64_t value ) {
    std::ostringstream os;
    os << value;
    return os.str();
}

string formatDouble(double d,int prec)
{
    std::string s;
    std::stringstream sstream;
    sstream.setf(std::ios::fixed);
    sstream.precision(prec);
    sstream << d;
    return sstream.str();
}

struct Holiday{
    static const map<date, bool> listofholidays;

    static map<date, bool> createHolidays()
    {
        map<date, bool> m;
        date d(2021,Apr,2);
        m[d]=true;
        //using namespace boost::gregorian;
        return m;
    }
};

const map<date, bool> Holiday::listofholidays = Holiday::createHolidays();

bool IsHoliday(date p) {

    if (p.day_of_week() == 0 || p.day_of_week() == 6 || Holiday::listofholidays.count(p) > 0 )   //  enum weekdays {Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday};
    {
        return true;
    }
    return false;
}

date OffsetDate(date d, int offset)
{
    date retval = d;
    int sign = sgn<int>(offset);
    int abs = fabs(offset);

    for (int i = 0; i < abs; i++)
    {
        do
        {
            retval = retval + boost::gregorian::days(sign);
        } while (IsHoliday(retval));
    }
    return retval;

}
map<date, CrossStats> crossCountMap;
int gMarketType=0;
bool gMarketHalted=false;