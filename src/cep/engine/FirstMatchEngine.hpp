/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
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

#ifndef FirstMatchEngine_hpp_
#define FirstMatchEngine_hpp_
#include "../Instance.hpp"
#include "../CEPEngine.hpp"
#include "../MatchProducer.hpp"
/**
 * Engine to process events according to first match approach
 */
namespace pfabric {

template<class Tin, class Tout, class Tdep>
class FirstMatchEngine: public CEPEngine<Tin, Tout, Tdep> {
	typedef boost::intrusive_ptr<Tin> TinPtr;
private:
	/**
	 * process the current event for a particular structure by checking whether this event can fit
	 * in this structure or not according to first match approach
	 * @param event the current event
	 * @param str the structure
	 */
	void engineProcess(const TinPtr& event, const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str);
public:
	/**
	 * the main function, run the main engine to process the event
	 * @param event the current event
	 */
	void runEngine(const TinPtr & event);
	/**
	 * constructor to receive the CEP manager to publish new match
	 * @param manager
	 */
	FirstMatchEngine(Matcher<Tin, Tout, Tdep>* manager) :	CEPEngine<Tin, Tout, Tdep>(manager){}
	/**
	 * destructor: nothing to do because of using boost library
	 */
	~FirstMatchEngine() {}

	/**
	 * print number of matches
	 * @param os the output stream object
	 */
	void printNumMatches(std::ostream& os);
};
}

namespace pfabric {


template<class Tin, class Tout, class Tdep>
void FirstMatchEngine<Tin, Tout, Tdep>::engineProcess(const TinPtr& event, const typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr& str) {

	int result = -1;
	bool wind = true ;
	typename NFAState<Tin>::StateType type = NFAState<Tin>::Normal;
	result = this->checkPredicate(event, str, type); // check predicate
	if (result != -1) { // the predicate if ok.
		if (this->hasWindow())
			wind = this->checkWindowTime(event, str); // the time window is ok
		if (wind != false) { // predicate and time window are ok
			//instance_ptr inst(new instance(event));
			str->addEvent(event,
					((NormalState<Tin, Tout, Tdep>*)(str->getCurrentState()))->getForwardEdgeByIndex(result));

			if (str->isComplete()) { //final state
				this->manager->publishResulMatches(str);
				this->counter++;
				this->pool->clear();

			} else {
			}
		} else {
			this->pool->clear();
		}
	}
	else if(result==-1 && type == NFAState<Tin>::Negation) {
		this->pool->clear();
	}
 }
template<class Tin, class Tout, class Tdep>
void FirstMatchEngine<Tin, Tout, Tdep>::runEngine(const TinPtr& event) {
	//std::cout << event << std::endl;
	if (this->pool->size() == 0)
		this->createStartStructure(event);
	else {
		typename ValueIDMultimap<typename NFAStructure<Tin, Tout, Tdep>::NFAStructurePtr, TinPtr>::MultimapConstIterator it =this->pool->beginConstIterator();
		engineProcess(event, it->second);
	}
}
template<class Tin, class Tout, class Tdep>
void FirstMatchEngine<Tin, Tout, Tdep>::printNumMatches(std::ostream& os) {
	CEPEngine<Tin, Tout, Tdep>::printNumMatches(os);
	os << "number of matches using 'first match' approach = " << this->counter
			<< std::endl;
}

}

#endif /* FirstMatchEngine_hpp_ */