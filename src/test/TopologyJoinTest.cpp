#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/filesystem.hpp>

#include "core/Tuple.hpp"

#include "TestDataGenerator.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

using namespace pfabric;
using namespace ns_types;

TEST_CASE("Building and running a topology with joins", "[Unpartitioned Join]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;

  TestDataGenerator tgen1("file1.csv");
  tgen1.writeData(5);

  TestDataGenerator tgen2("file2.csv");
  tgen2.writeData(8);

  std::stringstream strm;
  std::string expected = "0,This is a string field,0.5,0,This is a string field,0.5\n\
1,This is a string field,100.5,1,This is a string field,100.5\n\
2,This is a string field,200.5,2,This is a string field,200.5\n\
3,This is a string field,300.5,3,This is a string field,300.5\n\
4,This is a string field,400.5,4,This is a string field,400.5\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file2.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return get<0>(tp); });

  auto s2 = t.newStreamFromFile("file1.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return get<0>(tp); })
    .join<int>(s1, [](auto tp1, auto tp2) { return true; })
    .print(strm);

  t.start(false);

  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with joins on different tuple formats",
          "[Join different tuples]") {
  typedef TuplePtr<Tuple<int, std::string, double> > T1;
  typedef TuplePtr<Tuple<int, double>> T2;

  TestDataGenerator tgen1("file1.csv");
  tgen1.writeData(5);

  TestDataGenerator tgen2("file2.csv");
  tgen2.writeData(8);

  std::stringstream strm;
  std::string expected = "0,This is a string field,0.5,0,0.5\n\
1,This is a string field,100.5,1,100.5\n\
2,This is a string field,200.5,2,200.5\n\
3,This is a string field,300.5,3,300.5\n\
4,This is a string field,400.5,4,400.5\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file2.csv")
    .extract<T1>(',')
    .map<T2>([](auto tp, bool outdated) -> T2 {
        return makeTuplePtr(get<0>(tp), get<2>(tp));
    })
    .keyBy<int>([](auto tp) { return get<0>(tp); });

  auto s2 = t.newStreamFromFile("file1.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return get<0>(tp); })
    .join<int>(s1, [](auto tp1, auto tp2) { return true; })
    .print(strm);

  t.start(false);

  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with a join on one partitioned stream", 
          "[Partitioned and unpartitioned Join]") {
  typedef TuplePtr<Tuple<unsigned long, unsigned long>> MyTuplePtr;

  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, n % 100); 
  });
  unsigned long num = 1000;

  std::vector<std::vector<unsigned long>> results;

  Topology t;
  auto s2 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>();

  auto s1 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .partitionBy([](auto tp) { return get<1>(tp) % 5; }, 5)
    .join(s2, [](auto tp1, auto tp2) { return get<1>(tp1) == get<1>(tp2); })
    .merge()
    .notify([&](auto tp, bool outdated) {
        std::vector<unsigned long> tmp_vec;
        tmp_vec.push_back(get<0>(tp));
        tmp_vec.push_back(get<1>(tp));
        tmp_vec.push_back(get<2>(tp));
        tmp_vec.push_back(get<3>(tp));

        results.push_back(tmp_vec);
    });
    //.print();

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);

  for (auto i=0u; i<num; i++) {
    REQUIRE(results[i][0] == results[i][2]);
    REQUIRE(results[i][1] == results[i][3]);
  }
}

/*TEST_CASE("Building and running a topology with a join on two partitioned streams", 
          "[Partitioned Join]") {
  typedef TuplePtr<Tuple<unsigned long, unsigned long>> MyTuplePtr;

  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, n % 100); 
  });
  unsigned long num = 1000;

  std::vector<std::vector<unsigned long>> results;

  Topology t;
  auto s2 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .partitionBy([](auto tp) { return get<1>(tp) % 5; }, 5)
    .keyBy<0>();

  auto s1 = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .partitionBy([](auto tp) { return get<1>(tp) % 5; }, 5)
    .join(s2, [](auto tp1, auto tp2) { return get<1>(tp1) == get<1>(tp2); })
    .merge()
    .notify([&](auto tp, bool outdated) {
        std::vector<unsigned long> tmp_vec;
        tmp_vec.push_back(get<0>(tp));
        tmp_vec.push_back(get<1>(tp));
        tmp_vec.push_back(get<2>(tp));
        tmp_vec.push_back(get<3>(tp));

        results.push_back(tmp_vec);
    });
    //.print();

  t.start(false);

  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == num);

  for (auto i=0u; i<num; i++) {
    REQUIRE(results[i][0] == results[i][2]);
    REQUIRE(results[i][1] == results[i][3]);
  }
}*/