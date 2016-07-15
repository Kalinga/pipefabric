#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>
#include <boost/filesystem.hpp>

#include <zmq.hpp>

#include "core/Tuple.hpp"
#include "qop/ZMQSource.hpp"
#include "qop/ConsoleWriter.hpp"
#include "qop/TupleExtractor.hpp"
#include "qop/TupleDeserializer.hpp"


using namespace pfabric;

typedef Tuple< int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the ZMQSource operator.
 */
TEST_CASE("Receiving a ascii tuple stream via ZMQSource", "[ZMQSource]") {
	typedef ZMQSource< TStringPtr > TestZMQSource;
	typedef ConsoleWriter< MyTuplePtr > TestWriter;

	zmq::context_t context (1);
	zmq::socket_t publisher (context, ZMQ_PUB);
	publisher.bind("tcp://*:5678");

	auto src = std::make_shared< TestZMQSource > ("tcp://localhost:5678");
	auto extractor = std::make_shared<TupleExtractor<MyTuplePtr> >('|');
	CREATE_DATA_LINK(src, extractor);

	auto formatter = [&](std::ostream& os, const MyTuplePtr& tp ) {
		os << fmt::format("{0},{1}", tp->getAttribute<0>(), tp->getAttribute<1>()) << std::endl;
	};
	std::stringstream strm;
	auto writer = std::make_shared< TestWriter >(strm, formatter);
	CREATE_DATA_LINK(extractor, writer);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto handle = std::async(std::launch::async, [&publisher](){
		std::vector<std::string> input = {
			"0|10", "1|11", "2|12", "3|13", "4|14", "5|15"
		};
		for(const std::string &s : input) {
			zmq::message_t request (4);
			memcpy (request.data (), s.c_str(), 4);
			publisher.send (request);
		}
	});

  handle.get();
  std::this_thread::sleep_for(2s);
  src->stop();
	std::string expected = "0,10\n1,11\n2,12\n3,13\n4,14\n5,15\n";
	REQUIRE(strm.str() == expected);
  publisher.close();
}

/**
 * A second test of the ZMQSource operator.
 */
TEST_CASE("Receiving a binary tuple stream via ZMQSource", "[ZMQSource]") {
  typedef ZMQSource< TBufPtr > TestZMQSource;
  typedef ConsoleWriter< MyTuplePtr > TestWriter;
  
  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");
  
  auto src = std::make_shared< TestZMQSource > ("tcp://localhost:5678");
  auto extractor = std::make_shared<TupleDeserializer<MyTuplePtr> >();
  CREATE_DATA_LINK(src, extractor);
  
  auto formatter = [&](std::ostream& os, const MyTuplePtr& tp ) {
    os << fmt::format("{0},{1}", tp->getAttribute<0>(), tp->getAttribute<1>()) << std::endl;
  };
  std::stringstream strm;
  auto writer = std::make_shared< TestWriter >(strm, formatter);
  CREATE_DATA_LINK(extractor, writer);
  
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  
  auto handle = std::async(std::launch::async, [&publisher](){
    std::vector<MyTuplePtr> input = {
      makeTuplePtr(0, 10),
      makeTuplePtr(1, 11),
      makeTuplePtr(2, 12),
      makeTuplePtr(3, 13),
      makeTuplePtr(4, 14),
      makeTuplePtr(5, 15)
    };
    for(MyTuplePtr &tp : input) {
      StreamType res;
      tp->serializeToStream(res);
      zmq::message_t request (res.size());
      memcpy (request.data (), res.data(), res.size());
      publisher.send (request);
    }
  });
  
  handle.get();
  std::this_thread::sleep_for(2s);
  src->stop();
  std::string expected = "0,10\n1,11\n2,12\n3,13\n4,14\n5,15\n";
  REQUIRE(strm.str() == expected);
  publisher.close();
}