#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do
// this in one cpp file


#include "catch.hpp"
#include <unistd.h>
#include "fmt/format.h"

#include "nvm/BDCCInfo.hpp"
#include "nvm/persistent_table.hpp"
#include "table/TableInfo.hpp"
#include "core/Tuple.hpp"
#include "core/serialize.hpp"
#include "pfabric.hpp"

#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/pool.hpp"


using namespace pfabric::nvm;

using nvml::obj::make_persistent;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::pool;
using nvml::obj::transaction;

typedef pfabric::Tuple<int, int, string, double> MyTuple;
typedef persistent_table<MyTuple, uint64_t> pTable_type;

struct root {
    persistent_ptr<pTable_type> pTable;
};

TEST_CASE("Testing storing tuples in persistent_table", "[persistent_table]") {
  std::chrono::high_resolution_clock::time_point start, end;
  std::vector<typename std::chrono::duration<int64_t,micro>::rep> measures;

  pool<root> pop;
  const std::string path = "/tmp/testdb.db";

  std::remove(path.c_str());

  if (access(path.c_str(), F_OK) != 0) {
    pop = pool<root>::create(path, LAYOUT);
  } else {
    pop = pool<root>::open(path, LAYOUT);
  }

  auto q = pop.get_root();
  if (!q->pTable) {
    transaction::exec_tx(pop, [&] {
      auto tInfo = make_persistent<pfabric::TableInfo, std::string, std::initializer_list<ColumnInfo>>("MyTable", {
            ColumnInfo("a", ColumnInfo::Int_Type),
            ColumnInfo("b", ColumnInfo::Int_Type),
            ColumnInfo("c", ColumnInfo::String_Type),
            ColumnInfo("d", ColumnInfo::Double_Type)
          });
      auto dimInfo = BDCCInfo::ColumnBitsMap( {
            { ColumnInfo("a", ColumnInfo::Int_Type), 4},
            { ColumnInfo("d", ColumnInfo::Double_Type), 6}
          });
      q->pTable = make_persistent<pTable_type>(*tInfo, dimInfo);
      //TODO: Rewrite constuctor
    });
  } else {
    std::cerr << "WARNING: Table already exists" << std::endl;
  }


  for (unsigned int i = 0; i < 500; i++) {
    auto tup = MyTuple(i + 1,
                       (i + 1) * 100,
                       fmt::format("String #{0}", i),
                       i * 12.345);
    start = std::chrono::high_resolution_clock::now();
    q->pTable->insert(i+1, tup);
    end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    measures.push_back(diff);
  }

  auto avg = std::accumulate(measures.begin(), measures.end(), 0) / measures.size();
  auto minmax = std::minmax_element(std::begin(measures), std::end(measures));
  std::cout << "\nInsert Statistics in µs: "
            << "\n\tAverage: " << avg
            << "\n\tMin: "     << *minmax.first
            << "\n\tMax: "     << *minmax.second << '\n';

  auto ptp = q->pTable->getByKey(5);
  std::cout << "Tuple 5: " << *ptp << '\n';
  q->pTable->print(false);


  /* Clean up */
  transaction::exec_tx(pop, [&] {delete_persistent<pTable_type>(q->pTable);});
  pop.close();
}
