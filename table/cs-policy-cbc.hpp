//
// CBC version 3
//

#ifndef NFD_DAEMON_TABLE_CS_POLICY_CBC_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_CBC_HPP

#include "cs-policy.hpp"

#include <list>
#include <ns3/nstime.h>
#include <ns3/node.h>
#include <ns3/node-list.h>
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/pointer.h"
#include "fw/forwarder.hpp"
#include "math.h"
#include "ns3/simulator.h"
#include <algorithm>

namespace nfd {
namespace cs {
namespace cbc {

using Queue = std::list<Policy::EntryRef>;

enum QueueType {
  PRIMARY_QUEUE,
  SECONDARY_QUEUE,
  CBC_QUEUE
};

struct EntryInfo
{
  QueueType queueType;
  Queue::iterator queueIt;
  scheduler::EventId moveToSecondaryQueue;
  Name prefix;
  uint64_t link_cost;
  double last_access;
  double recency;
  double frequency;
  double cost_value;
  double popularity;
  
};

class CbcPolicy final : public Policy
{
public:
  CbcPolicy();

  ~CbcPolicy() final;

public:
  Cs* m_cs;
  static const std::string POLICY_NAME;
  
  // Returns the cache hit rate for the given content name.
  double GetCacheRecency(const Name& content_name);

  // Sets the cache hit rate for the given content name.
  void SetCacheRecency(const Name& content_name, double recency);

private:
  void
  doAfterInsert(EntryRef i) final;

  void
  doAfterRefresh(EntryRef i) final;

  void
  doBeforeErase(EntryRef i) final;

  void
  doBeforeUse(EntryRef i) final;

  void
  evictEntries() final;

private:
  /** \brief evicts one entry
   *  \pre CS is not empty
   */
  void
  evictOne();

  /** \brief attaches the entry to an appropriate queue
   *  \pre the entry is not in any queue
   */
  void
  attachQueue(EntryRef i);
  
  /** \brief detaches the entry from its current queue
   *  \post the entry is not in any queue
   */
  void
  detachQueue(EntryRef i);

  /** \brief moves an entry from SECONDARY_QUEUE to PRIMARY_QUEUE
   */
  void
  moveToSecondaryQueue(EntryRef i);
  
  /** \brief moves an entry from SECONDARY_QUEUE to PRIMARY_QUEUE
   */
  void
  moveToPrimaryQueue(EntryRef i);
  
  void
  updateCostValue(EntryRef i);
  
  void
  updateQueue();
  
  void
  dataLogging(EntryRef i);
  
  /** \brief get the link_cost of the data source
   */
  uint64_t
  getSourceDistance(EntryRef i) {
  	uint64_t linkCost = 0;
  
  	if (ns3::Simulator::GetContext() < 1000) { // this check prevents 4535459 value of Context
    	ns3::Ptr<ns3::Node> node = ns3::NodeList::GetNode(ns3::Simulator::GetContext()); // Get Current node from simulator context
    	ns3::Ptr<ns3::ndn::L3Protocol> l3Object = node->GetObject<ns3::ndn::L3Protocol>(); // Getting l3 Object
    	shared_ptr<nfd::Forwarder> m_forwarder = l3Object->getForwarder(); // Get the forwarder instance
         
    	const nfd::Fib& fib = m_forwarder->getFib(); // Get the fib entry
    	const nfd::fib::Entry& fibEntry = fib.findLongestPrefixMatch(i->getName());
    
    	for (const auto& nh : fibEntry.getNextHops()) {
        	linkCost = nh.getCost();
    	}
 	 }
    
 	 return linkCost;
  };

  private:
  Queue m_queues[CBC_QUEUE];
  std::map<EntryRef, EntryInfo*> m_entryInfoMap;
  
  int totalEviction;

};

} // namespace cbc

using cbc::CbcPolicy;

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_POLICY_CBC_HPP
