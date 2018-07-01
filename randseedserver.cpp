#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <map>
#include <boost/random.hpp>
#include <boost/process.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <crypto++/sha.h>
#include <crypto++/filters.h>
#include <crypto++/hex.h>

#define SEEDNUM 3

using namespace std;
using namespace boost;
using namespace boost::process;
using namespace boost::posix_time;
using namespace CryptoPP;

int main()
{
	vector<mt19937> rngs;
	for(int i = 0 ; i < SEEDNUM ; i++)rngs.push_back(mt19937(7*i+87938));
	vector<variate_generator<mt19937&,uniform_int<> > > intgen;
	for(auto & rng : rngs)
		intgen.push_back(variate_generator<mt19937&,uniform_int<> >(rng,uniform_int<>(0,numeric_limits<unsigned short>::max())));
	//setsize
	boost::shared_ptr<child> handler;
	do {
		vector<string> arguments;
		arguments.push_back("--wallet-url");
		arguments.push_back("http://localhost:8899");
		arguments.push_back("--url");
		arguments.push_back("http://localhost:8788");
		arguments.push_back("push");
		arguments.push_back("action");
		arguments.push_back("eosdacrandom");
		arguments.push_back("setsize");
		arguments.push_back("{\"size\":" + lexical_cast<string>(SEEDNUM) + "}");
		arguments.push_back("-p");
		arguments.push_back("eosdacrandom");
		handler = boost::shared_ptr<child>(new child(search_path("cleos"),arguments));
		handler->wait();
		if(EXIT_SUCCESS != handler->exit_code()) cout<<"setsize failed! will redo it."<<endl;
	} while(EXIT_SUCCESS != handler->exit_code());
	boost::this_thread::sleep(seconds(1));
	while(1) {
		//send hash and seed
		map<int,int> randnum;
		//send hash
		for(int i = 0 ; i < intgen.size() ; i++) {
			boost::shared_ptr<child> handler;
			int r = intgen[i]();
			randnum[i] = r;
			string input = lexical_cast<string>(r);
			SHA256 hash;
			string sha256;
			StringSource foo(input,true,
				new HashFilter(hash,
					new HexEncoder(
						new StringSink(sha256),false
					)
				)
			);
			vector<string> arguments;
			arguments.push_back("--wallet-url");
			arguments.push_back("http://localhost:8899");
			arguments.push_back("--url");
			arguments.push_back("http://localhost:8788");
			arguments.push_back("push");
			arguments.push_back("action");
			arguments.push_back("eosdacrandom");
			arguments.push_back("sendhash");
			arguments.push_back("{\"owner\":\"seed" + lexical_cast<string>(i + 1) + "\",\"hash\":\"" + sha256 + "\"}");
			arguments.push_back("-p");
			arguments.push_back("seed" + lexical_cast<string>(i + 1));
			handler = boost::shared_ptr<child>(new child(search_path("cleos"),arguments));
			handler->wait();
			if(EXIT_SUCCESS != handler->exit_code()) cout<<"sendhash failed!"<<endl;
		}
		boost::this_thread::sleep(seconds(1));
		assert(randnum.size() == intgen.size());
		//send seed
		for(int i = 0 ; i < intgen.size() ; i++) {
			boost::shared_ptr<child> handler;
			int r = randnum[i];
			vector<string> arguments;
			arguments.push_back("--wallet-url");
			arguments.push_back("http://localhost:8899");
			arguments.push_back("--url");
			arguments.push_back("http://localhost:8788");
			arguments.push_back("push");
			arguments.push_back("action");
			arguments.push_back("eosdacrandom");
			arguments.push_back("sendseed");
			arguments.push_back("{\"owner\":\"seed" + lexical_cast<string>(i + 1) + "\",\"seed\":" + lexical_cast<string>(r) + "}");
			arguments.push_back("-p");
			arguments.push_back("seed" + lexical_cast<string>(i + 1));
			handler = boost::shared_ptr<child>(new child(search_path("cleos"),arguments));
			handler->wait();
			if(EXIT_SUCCESS != handler->exit_code()) cout<<"sendseed failed! will redo it."<<endl;
		}
		boost::this_thread::sleep(seconds(1));
	}//end while true
	return EXIT_SUCCESS;
}

