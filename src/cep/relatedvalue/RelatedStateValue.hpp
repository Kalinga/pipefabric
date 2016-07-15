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

#ifndef RelatedStateValue_hpp_
#define RelatedStateValue_hpp_

#include "core/Tuple.hpp"

namespace pfabric {
template <class Tin, class StorageType, class Type, int Index>
class RelatedStateValue {
protected:

	mutable boost::atomic<short> refCount;
public:
	typedef boost::intrusive_ptr<RelatedStateValue<Tin, StorageType, Type, Index>> RelatedStateValuePtr;
	typedef boost::intrusive_ptr<Tin> TinPtr;
	/**
	 * gets the current related value
	 * @return the current value
	 */
	virtual Type getValue() = 0;

	/**
	 * updates the related value according to incoming event (instance)
	 * @param e the new selected instance
	 */
	virtual void updateValue(const TinPtr& e) = 0;

	/**
	 * get the attribute index
	 * @return the attribute name
	 */
	int getAttributeIdx() { return Index; }

	/**
	 * initializes the value by a new incoming event (instance)
	 * @param e incoming event to init the value accordingly
	 */
	virtual void initValue(const TinPtr& e) = 0;
	/**
	 * constructor: nothing to do
	 */
	RelatedStateValue() {}
	/**
	 * destructor, nothing to do
	 */
	virtual ~RelatedStateValue() {}


	friend void intrusive_ptr_add_ref(const RelatedStateValue *x) {
		x->refCount.fetch_add(1, boost::memory_order_relaxed);
	}
	friend void intrusive_ptr_release(const RelatedStateValue *x) {
		if (x->refCount.fetch_sub(1, boost::memory_order_release) == 1) {
			boost::atomic_thread_fence(boost::memory_order_acquire);
			delete x;
		}
	}

};
}
#endif /* RelatedStateValue_hpp_ */