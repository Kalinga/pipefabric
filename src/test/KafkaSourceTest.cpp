/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <cppkafka/producer.h>
#include "cppkafka/configuration.h"
#include <string>
#include <iostream>

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"
#include "dsl/PFabricContext.hpp"

using namespace pfabric;

TEST_CASE("Producing and receiving tuples via Apache Kafka protocol", "[Kafka]") {

  //prepare the consuming - necessary to subscribe to the topic before we start
  //producing tuples (to set the "pointer" on the current topic data correctly)
  typedef TuplePtr<int, double> InTuplePtr;
  int resCntr = 0;
  std::string grp = "TestGroup"+std::to_string(rand()%100000);

  PFabricContext ctx;
  auto t = ctx.createTopology();

  auto s = t->newStreamFromKafka("127.0.0.1:9092", "PipeFabric", grp)
    .extract<InTuplePtr>(',')
    .print()
    .notify([&resCntr](auto tp, bool outdated) { resCntr++; })
    ;

  //start the producing
  std::cout<<"Producing 100 tuples..."<<std::endl;

  cppkafka::Configuration config = {
    { "metadata.broker.list", "127.0.0.1:9092" }
  };

  cppkafka::Producer producer(config);
  cppkafka::MessageBuilder builder("PipeFabric");

  std::string msg;

  for(auto i=0; i<100; i++) { //100 tuples
    msg = "";
    msg.append(std::to_string(i));
    msg.append(",1.5");
    builder.payload(msg);
    producer.produce(builder);
  }

  //start the consuming
  t->start(false);

  REQUIRE(resCntr == 100);
}