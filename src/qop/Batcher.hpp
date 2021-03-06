/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef Batcher_hpp_
#define Batcher_hpp_

#include <vector>

#include "qop/UnaryTransform.hpp"
#include "qop/OperatorMacros.hpp"

namespace pfabric {

template <typename InputStreamElement>
using BatchPtr = TuplePtr<std::vector<std::pair<InputStreamElement, bool>>>;

template <typename InputStreamElement>
class Batcher : public UnaryTransform< InputStreamElement, BatchPtr<InputStreamElement> > // use default unary transform
{
typedef BatchPtr<InputStreamElement> OutputStreamElement;

private:
PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement)

public:
  Batcher(std::size_t batchSize = SIZE_MAX) : mBatchSize(batchSize), mPos(0), 
    mBuf(batchSize != SIZE_MAX ? batchSize : 0) {}

  /**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Batcher, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Batcher, processPunctuation );

	const std::string opName() const override { return std::string("Batcher"); }

private:

  /**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
    if (mBatchSize == SIZE_MAX)
      publishBatch();
		this->getOutputPunctuationChannel().publish(punctuation);
	}

	/**
	 * This method is invoked when a data stream element arrives.
	 *
	 * It ... and forwards the resulting element to its subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const InputStreamElement& data, const bool outdated ) {
    // ensure capacity
    if (mBatchSize == SIZE_MAX)
      mBuf.resize(mPos + 1);
		mBuf[mPos++] = std::make_pair(data, outdated);
    if (mPos == mBatchSize) {
      publishBatch();
      /*
      auto tup = makeTuplePtr(std::move(mBuf));
      this->getOutputDataChannel().publish(tup, false);
      mPos = 0;
      mBuf.resize(mBatchSize);
      */
    }
	}

  void publishBatch() {
      auto tup = makeTuplePtr(std::move(mBuf));
      this->getOutputDataChannel().publish(tup, false);
      mPos = 0;
      mBuf.resize(mBatchSize);
  }

  std::size_t mBatchSize, mPos;
  std::vector<std::pair<InputStreamElement, bool>> mBuf;
};

template <typename InputStreamElement>
class UnBatcher : public UnaryTransform< BatchPtr<InputStreamElement>, InputStreamElement> // use default unary transform
{
private:
typedef BatchPtr<InputStreamElement> InputStreamBatch;

PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamBatch, InputStreamElement)

public:
  /**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, UnBatcher, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, UnBatcher, processPunctuation );

	const std::string opName() const override { return std::string("UnBatcher"); }

private:

  /**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		this->getOutputPunctuationChannel().publish(punctuation);
	}

	/**
	 * This method is invoked when a data stream element arrives.
	 *
	 * It ... and forwards the resulting element to its subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const InputStreamBatch& data, const bool outdated ) {
    // data = TuplePtr<std::vector<std::pair<InputStreamElement, bool>>>
    auto& vec = get<0>(data);
    for (auto& elem : vec) {
      this->getOutputDataChannel().publish(elem.first, outdated);
    }
	}

};

}

#endif
