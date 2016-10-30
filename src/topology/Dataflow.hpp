/*
 * Copyright (c) 2014-16 The PipeFabric team,
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
#ifndef Dataflow_hpp_
#define Dataflow_hpp_

#include <string>
#include <list>

#include "qop/DataSink.hpp"
#include "qop/DataSource.hpp"

namespace pfabric {

  class Dataflow {
  public:
    Dataflow() {}
    /**
     * Typedef for pointer to BaseOp (any PipeFabric operator).
     */
    typedef std::shared_ptr<BaseOp> BaseOpPtr;
    typedef std::list<BaseOpPtr> BaseOpList;
    typedef BaseOpList::iterator BaseOpIterator;

    BaseOpIterator addPublisher(BaseOpPtr op);

    BaseOpIterator addPublisherList(const BaseOpList& lst);

    BaseOpIterator publisherEnd() { return publishers.end(); }
    BaseOpIterator publisherBegin() { return publishers.begin(); }

    void addSink(BaseOpPtr op);

    /**
     * @brief Returns the operator at the end of the publisher list.
     *
     * Returns the operator which acts as the publisher for the next
     * added operator.
     *
     * @return
     *    the last operator in the publisher list
     */
    BaseOpPtr getPublisher();

    BaseOpIterator getPublishers(unsigned int num);

    std::size_t size() const;

private:
  BaseOpList publishers; //< the list of all operators acting as publisher (source)
  BaseOpList sinks;     //< the list of sink operators (which are not publishers)
};

typedef std::shared_ptr<Dataflow> DataflowPtr;
}

#endif
