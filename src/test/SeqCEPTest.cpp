#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <boost/assign/std/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <list>
#include <map>
#include <fstream>

#include "cep/Matcher.hpp"
#include "cep/NFAController.hpp"
#include "cep/dsl/CEPState.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"

#include "StreamMockup.hpp"

using namespace std;
using namespace boost::assign;
using namespace pfabric;

typedef Tuple<int, int, int> InTuple;
typedef TuplePtr<InTuple> InTuplePtr;
typedef Tuple<int, int, int> OutTuple;
typedef TuplePtr<OutTuple> OutTuplePtr;

TEST_CASE("Verifying the correct behavior of the CEP operator", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >("cep_test.in", "cep_test.res");

	auto stateAFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 1; };
	auto stateBFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 2; };
	auto stateCFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 3; };

	auto matcher = std::make_shared<Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>(
			Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);

	auto nfa = matcher->getNFAController();

	auto edgeAState = nfa->createForwardEdge(stateAFilter);
	auto edgeBState = nfa->createForwardEdge(stateBFilter);
	auto edgeCState = nfa->createForwardEdge(stateCFilter);

	auto startStateA = nfa->createStartState("A");
	auto stateB = nfa->createNormalState("B");
	auto stateC = nfa->createNormalState("C");
	auto stateD = nfa->createFinalState("D");

	nfa->createForwardTransition(startStateA, edgeAState, stateB);
	nfa->createForwardTransition(stateB, edgeBState, stateC);
	nfa->createForwardTransition(stateC, edgeCState, stateD);

	CREATE_DATA_LINK(mockup, matcher);
	CREATE_DATA_LINK(matcher, mockup);

	mockup->start();
}

TEST_CASE("Verifying the correct behavior of the CEP operator with related values", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr1;
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr2;
	typedef TuplePtr<Tuple<RelatedTuplePtr1, RelatedTuplePtr2>> RelatedTuplePtr;

	auto mockup = std::make_shared< StreamMockup<InTuplePtr, OutTuplePtr> >("cep_test.in", "cep_test.res");

	auto matcher = std::make_shared<Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>(
				Matcher<InTuplePtr, OutTuplePtr, RelatedTuplePtr>::FirstMatch);
	auto nfa = matcher->getNFAController();
	auto initState = nfa->createStartState("A");
	auto stateA = nfa->createNormalState("B");
	auto stateB = nfa->createNormalState("C");
	auto stateC = nfa->createFinalState("D");

	auto t4 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == 1);
	});
	t4->setID(4);
	nfa->createForwardTransition(initState, t4, stateA);

	auto t5 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == getAttribute<0>(*rt)->getValue() + 1);
	});
	t5->setID(5);
	nfa->createForwardTransition(stateA, t5, stateB);

	auto t6 = nfa->createForwardEdge([&](const InTuplePtr& tp, const RelatedTuplePtr& rt) {
		return (getAttribute<0>(*tp) == getAttribute<1>(*rt)->getValue() + 1);
	});
	t6->setID(6);

	nfa->createForwardTransition(stateB, t6, stateC);
	auto init = [&]() {
		RelatedTuplePtr1 related1 = new RelatedStateValuePrevious<InTuplePtr, int, int, 0 >();
		RelatedTuplePtr2 related2 = new RelatedStateValuePrevious<InTuplePtr, int, int, 0 >();
		auto tp = makeTuplePtr(related1, related2);
		return tp;
	};

	auto update = [&](const RelatedTuplePtr& tp, int id, const InTuplePtr& event) {
		if (id == 4) {
			auto rel0 = getAttribute<0>(*tp);
			rel0->updateValue(event);
		}
		else if (id == 5) {
			auto rel1 = getAttribute<1>(*tp);
			rel1->updateValue(event);
		}
	};

	nfa->setDependency(init, update);

	CREATE_DATA_LINK(mockup, matcher);
	CREATE_DATA_LINK(matcher, mockup);

	mockup->start();
}

TEST_CASE("Verifying the correct behavior of the CEP operator using Topology", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;

	std::string expected = "1,71,421\n2,76,390\n3,97,467\n1,71,52\n2,76,942\n3,97,639\n1,71,242\n2,76,901\n3,97,868\n";
	auto nfa = std::make_shared<NFAController<InTuplePtr, OutTuplePtr, RelatedTuplePtr>>();

	auto stateAFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 1 && getAttribute<1>(*tp) == 71; };
	auto stateBFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 2 && getAttribute<1>(*tp) == 76; };
	auto stateCFilter = [&](const InTuplePtr& tp, const RelatedTuplePtr& rt ) -> bool {
		return getAttribute<0>(*tp) == 3 && getAttribute<1>(*tp) == 97; };

	auto edgeAState = nfa->createForwardEdge(stateAFilter);
	auto edgeBState = nfa->createForwardEdge(stateBFilter);
	auto edgeCState = nfa->createForwardEdge(stateCFilter);

	auto startStateA = nfa->createStartState("A");
	auto stateB = nfa->createNormalState("B");
	auto stateC = nfa->createNormalState("C");
	auto stateD = nfa->createFinalState("D");

	nfa->createForwardTransition(startStateA, edgeAState, stateB);
	nfa->createForwardTransition(stateB, edgeBState, stateC);
	nfa->createForwardTransition(stateC, edgeCState, stateD);

	std::stringstream strm;

	auto inputFile = std::string(TEST_DATA_DIRECTORY) + "cep_test.in";
	Topology t;
	auto s = t.newStreamFromFile(inputFile)
	    		.extract<InTuplePtr>(',')
				.matchByNFA<OutTuplePtr, RelatedTuplePtr>(nfa)
				.print(strm);

	t.start(false);
	REQUIRE(strm.str() == expected);
}

TEST_CASE("Verifying the correct behavior of the CEP operator using Topology & DSL", "[CEP]") {
	typedef typename RelatedStateValue<InTuplePtr, int, int, 0>::RelatedStateValuePtr RelatedTuplePtr;
	typedef CEPState<InTuplePtr, RelatedTuplePtr> MyCEPState;

	std::string expected = "1,71,421\n2,76,390\n3,97,467\n1,71,52\n2,76,942\n3,97,639\n1,71,242\n2,76,901\n3,97,868\n";
	std::stringstream strm;

  MyCEPState a;
  MyCEPState b([](auto tp, auto rt) { return get<0>(*tp) == 1 && get<1>(*tp) == 71; });
  MyCEPState c([](auto tp, auto rt) { return get<0>(*tp) == 2 && get<1>(*tp) == 76; });
  MyCEPState d([](auto tp, auto rt) { return get<0>(*tp) == 3 && get<1>(*tp) == 97; }, MyCEPState::Stopp);

	auto inputFile = std::string(TEST_DATA_DIRECTORY) + "cep_test.in";
	Topology t;
	auto s = t.newStreamFromFile(inputFile)
	    		.extract<InTuplePtr>(',')
				  .matcher<OutTuplePtr, RelatedTuplePtr>(a >> b >> c >> d)
				  .print(strm);

	t.start(false);
	REQUIRE(strm.str() == expected);
}
