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

#ifndef Aggregation_hpp_
#define Aggregation_hpp_

#include "core/Tuple.hpp"
#include "core/Punctuation.hpp"
#include "qop/AggregateFunctions.hpp"
#include "qop/OperatorMacros.hpp"
#include "qop/UnaryTransform.hpp"
#include "qop/TriggerNotifier.hpp"

#include <boost/thread/thread.hpp>

namespace pfabric {

  class BaseAggregateState {
  public:
    BaseAggregateState() {}
    virtual ~BaseAggregateState() {}

    virtual void init() = 0;
    virtual AggregateStatePtr clone() const = 0;
  };


  /**
   * @brief An aggregation operator for streams of tuples.
   *
   * This operator implements the aggregation of data streams. For each incoming tuple the
   * aggregates are computed incrementally using the IterateFunc function. The final aggregation
   * results calculated by an FinalFunc function are either produced periodically or at the end
   * of the stream. This class implements the non-realtime variant of aggregation, i.e., the slen
   * interval is derived from the timestamps of the incoming tuples instead of the realtime clock.
   *
   * @tparam InputStreamElement
   *    the data stream element type consumed by the projection
   * @tparam OutputStreamElement
   *    the data stream element type produced by the projection
   */
  template<
  typename InputStreamElement,
  typename OutputStreamElement
  >
  class Aggregation : public UnaryTransform< InputStreamElement, OutputStreamElement > {
  protected:
    PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement);

  public:
    typedef std::function<Timestamp(const InputStreamElement&)> TimestampExtractorFunc;

    /**
     * @brief The aggregation function which produces the final (or periodic) aggregation result.
     *
     * This function gets a pointer to the aggregate state as well as the timestamp for the result elment.
     */
    typedef std::function< OutputStreamElement(AggregateStatePtr) > FinalFunc;

    /**
     * @brief The function which is invoked for each incoming stream element to calculate the incremental aggregates.
     *
     * This function gets the incoming stream element, the aggregate state, and the boolean flag for outdated elements.
     */
    typedef std::function< void(const InputStreamElement&, AggregateStatePtr, const bool) > IterateFunc;


    /**
     * Creates a new aggregation operator which receives an input stream and applies the given aggregate function.
     * If a slide_len > 0 is specified, the aggregate values are produced periodically (every slide_len seconds),
     * with slide_len = 0 each input element triggers the publishing of an output element for the updated aggregate,
     * and in case of slide_len = UINT_MAX the aggregates are produced as response to punctuation tuples.
     *
     * @param aggrs the aggregate state, i.e. a pointer to a class with members for all aggregates
     * @param final_fun a function pointer to the aggregation function
     * @param it_fun a function pointer to an iteration function called for each incoming tuple
     * @param slen the time interval in seconds to produce aggregation tuples
     */
    Aggregation( AggregateStatePtr aggrs, FinalFunc final_fun, IterateFunc it_fun,
                AggregationTriggerType tType = TriggerAll, const unsigned int tInterval = 0) :
    mAggrState( aggrs->clone() ), mIterateFunc( it_fun ), mFinalFunc( final_fun ),
    mTriggerType(tType), mTriggerInterval( tInterval ),
    mNotifier(tInterval > 0 && tType == TriggerByTime ?
             new TriggerNotifier(std::bind(&Aggregation::notificationCallback, this), tInterval) : nullptr),
    mLastTriggerTime(0), mCounter(0) {
    }

    Aggregation( AggregateStatePtr aggrs, FinalFunc final_fun, IterateFunc it_fun,
                TimestampExtractorFunc func, const unsigned int tInterval) :
    mAggrState( aggrs->clone() ), mIterateFunc( it_fun ), mFinalFunc( final_fun ),
    mTimestampExtractor(func),
    mTriggerType(TriggerByTimestamp), mTriggerInterval( tInterval ),
    mNotifier(nullptr),
    mLastTriggerTime(0), mCounter(0) {
    }

    /**
     * @brief Bind the callback for the data channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Aggregation, processDataElement );

    /**
     * @brief Bind the callback for the punctuation channel.
     */
    BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Aggregation, processPunctuation );


  private:

    /**
     * This method is invoked when a data stream element arrives.
     *
     * TODO doc
     *
     * @param[in] data
     *    the incoming stream element
     * @param[in] outdated
     *    flag indicating whether the tuple is new or invalidated now
     */
    void processDataElement( const InputStreamElement& data, const bool outdated ) {
      boost::lock_guard<boost::mutex> guard(aggrMtx);

      // the actual aggregation is outsourced to a user-defined expression
      mIterateFunc(data, mAggrState, outdated);

      switch (mTriggerType) {
        case  TriggerAll:
        {
          // produce an aggregate tuple
          auto tn = mFinalFunc(mAggrState);
          this->getOutputDataChannel().publish( tn, outdated );
          break;
        }
        case TriggerByCount:
        {
          if (++mCounter == mTriggerInterval) {
            notificationCallback();
            mCounter = 0;
          }
          break;
        }
        case TriggerByTimestamp:
        {
          auto ts = mTimestampExtractor(data);
          if (ts - mLastTriggerTime >= mTriggerInterval) {
            notificationCallback();
            mLastTriggerTime = ts;
          }
          break;
        }
        default:
          break;
      }
    }

    /**
     * @brief This method is invoked when a punctuation arrives.
     *
     * Punctuation tuples can trigger aggregation results if specified for the operator
     * via the punctuation mask.
     *
     * @param[in] punctuation
     *    the incoming punctuation tuple
     */
    void processPunctuation( const PunctuationPtr& punctuation ) {
      // if we receive a punctuation on expired slides we produce aggregates
      // (but only if we don't have our own slide notifier) ...
      if( punctuation->ptype() == Punctuation::EndOfStream
         || punctuation->ptype() == Punctuation::WindowExpired
         || punctuation->ptype() == Punctuation::SlideExpired ) {
        produceAggregates();
      }
      this->getOutputPunctuationChannel().publish(punctuation);
    }


  protected:

    /**
     * @brief TODO doc
     */
    void produceAggregates() {
      boost::lock_guard<boost::mutex> guard(aggrMtx);

      auto aggregationResult = mFinalFunc( mAggrState );
      this->getOutputDataChannel().publish( aggregationResult, false );
    }

    /**
     * A function called by the slide_notifier thread.
     */
    void notificationCallback() {
      this->produceAggregates();
      PunctuationPtr punctuation = std::make_shared< Punctuation >( Punctuation::SlideExpired );
      this->getOutputPunctuationChannel().publish(punctuation);
    }

    TimestampExtractorFunc mTimestampExtractor; //!< a pointer to the function for extracting the timestamp from the tuple
    AggregateStatePtr mAggrState;               //!< a pointer to the object representing the aggregation state
    mutable boost::mutex aggrMtx;               //!< a mutex for synchronizing access between the trigger notifier thread
                                                //!< and aggregation operator
    IterateFunc mIterateFunc;                   //!< a pointer to the iteration function called for each tuple
    FinalFunc mFinalFunc;                       //!< a  pointer to a function computing the final (or periodical) aggregates
   // unsigned int mSlideLen;        //< TODO doc
    std::unique_ptr<TriggerNotifier> mNotifier; //!< the notifier object which triggers the computation of aggregates periodically
    Timestamp mLastTriggerTime;                 //!< the timestamp of the last aggregate publishing
    AggregationTriggerType mTriggerType;        //!< the type of trigger activating the publishing of an aggregate value
    unsigned int mTriggerInterval;              //!< the interval (time in seconds, number of tuples) for publishing aggregates
    unsigned int mCounter;                      //!< the number of tuples processed since the last aggregate publishing
  };

}

#endif