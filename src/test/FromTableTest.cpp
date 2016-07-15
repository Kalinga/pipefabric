#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include "core/Tuple.hpp"
#include "qop/ToTable.hpp"
#include "qop/FromTable.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"

#include "StreamMockup.hpp"

using namespace pfabric;

typedef Tuple< int, int, int > MyTuple;
typedef TuplePtr< MyTuple > MyTuplePtr;

/**
 * A simple test of the projection operator.
 */
TEST_CASE("Producing a data stream from inserts into a table", "[FromTable]") {
  typedef Table<MyTuplePtr, int> MyTable;
  auto testTable = std::make_shared<MyTable>();

  for (int i = 0; i < 10; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    testTable->insert(i, tp);
  }

  auto op = std::make_shared<FromTable<MyTuplePtr, int> >(testTable);

	std::vector<MyTuplePtr> expected;

  for (int i = 20; i < 30; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    expected.push_back(tp);
  }

	auto mockup = std::make_shared< StreamMockup<MyTuplePtr, MyTuplePtr> >(expected, expected);

	CREATE_DATA_LINK(op, mockup);

  for (int i = 20; i < 30; i++) {
    auto tp = makeTuplePtr(i, i + 10, i + 100);
    testTable->insert(i, tp);
  }

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
  REQUIRE(mockup->numTuplesProcessed() == 10);
}