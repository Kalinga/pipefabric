#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <vector>

#include "core/Tuple.hpp"
#include "table/Table.hpp"
#include "format/format.hpp"

using namespace pfabric;

typedef TuplePtr<Tuple<unsigned long, int, std::string, double>> MyTuplePtr;

TEST_CASE("Creating a table with a given schema, inserting and deleting data", "[Table]") {
  auto testTable = std::make_shared<Table<MyTuplePtr>> ();
  for (int i = 0; i < 10000; i++) {
    auto tp = makeTuplePtr((unsigned long) i, i + 100, fmt::format("String#{}", i), i / 100.0);
    testTable->insert(i, tp);
  }

  SECTION("checking inserts of data") {
    REQUIRE(testTable->size() == 10000);
    for (int i = 0; i < 10000; i++) {
      auto tp = testTable->getByKey(i);
      REQUIRE(tp->getAttribute<0>() == i);
      REQUIRE(tp->getAttribute<1>() == i + 100);
      REQUIRE(tp->getAttribute<2>() == fmt::format("String#{}", i));
      REQUIRE(tp->getAttribute<3>() == i / 100.0);
    }
  }

  SECTION("deleting data by key") {
    REQUIRE(testTable->size() == 10000);
    for (int i = 0; i < 10000; i += 100)
      testTable->deleteByKey(i);

    REQUIRE(testTable->size() == 9900);
    // check if the records were really deleted
    for (int i = 0; i < 10000; i += 100) {
      try {
        auto tp = testTable->getByKey(i);
        REQUIRE(false);
      }
      catch (TableException& exc) {
        // if the key wasn't found then an exception is raised
        // which we can ignore here
      }
    }
  }

  SECTION("deleting data using a predicate") {
    REQUIRE(testTable->size() == 10000);
    auto num = testTable->deleteWhere([&](const MyTuplePtr& tp) -> bool {
      return tp->getAttribute<0>() % 100 == 0;
    });
    REQUIRE(num == 100);
    REQUIRE(testTable->size() == 9900);
    for (int i = 0; i < 10000; i += 100) {
      try {
        auto tp = testTable->getByKey(i);
        REQUIRE(false);
      }
      catch (TableException& exc) {
        // if the key wasn't found then an exception is raised
        // which we can ignore here
      }
    }
  }

  SECTION("updating some data by key") {
    // TODO
  }

  SECTION("updating some data by predicate") {
    // TODO
  }
}