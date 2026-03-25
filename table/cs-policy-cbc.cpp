//
// CBC version 3
//

#include "cs-policy-cbc.hpp"
#include "cs.hpp"
#include "common/global.hpp"
#include "common/logger.hpp"
#include "fib.hpp"

NFD_LOG_INIT(CbcPolicy);

using namespace std;

namespace nfd {
namespace cs {
namespace cbc {

const std::string CbcPolicy::POLICY_NAME = "cbc";
NFD_REGISTER_CS_POLICY(CbcPolicy);

CbcPolicy::CbcPolicy()
  : Policy(POLICY_NAME)
{
}

CbcPolicy::~CbcPolicy()
{
  for (auto entryInfoMapPair : m_entryInfoMap) {
    delete entryInfoMapPair.second;
  }
}

void
CbcPolicy::doAfterInsert(EntryRef i) // the policy decides whether to accept the new entry
{
  this->attachQueue(i); // if accepted
  this->evictEntries(); // if CS size exceeds the capacity limit
  
}

void
CbcPolicy::doAfterRefresh(EntryRef i) // the policy may update its cleanup index to note that the same Data has arrived again.
{
  this->detachQueue(i); 
  this->attachQueue(i);
}

void
CbcPolicy::doBeforeErase(EntryRef i) // the policy should delete the corresponding item in its cleanup index.
{ 
  this->detachQueue(i);
}

void
CbcPolicy::doBeforeUse(EntryRef i) 
// the policy may update its cleanup index to note that the indicated entry is accessed to satisfy an incoming Interest.
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());
  
  //NFD_LOG_TRACE("CACHEHITS: " << i->getName());
  
  EntryInfo* entryInfo = m_entryInfoMap[i];
  entryInfo->frequency = entryInfo->frequency + 1;
  
  if (entryInfo->queueType == PRIMARY_QUEUE) {
    //NFD_LOG_TRACE("CACHEHITS_PRIMARY: " << i->getName());
    updateCostValue(i);
  }
  else if (entryInfo->queueType == SECONDARY_QUEUE) {
    //NFD_LOG_TRACE("CACHEHITS_SECONDARY: " << i->getName());
    double currentTime = ns3::Simulator::Now().GetSeconds();
    double lastAccess = entryInfo->last_access;
    entryInfo->recency = currentTime - lastAccess;
    entryInfo->last_access = currentTime;
    moveToPrimaryQueue(i);
  }

}

void
CbcPolicy::evictEntries() // the policy should evict enough entries so that the CS does not exceed capacity limit.
{
  BOOST_ASSERT(this->getCs() != nullptr);

  while (this->getCs()->size() > this->getLimit()) {
    this->evictOne();
  }
}

void
CbcPolicy::evictOne()
{
  BOOST_ASSERT(!m_queues[PRIMARY_QUEUE].empty() ||
               !m_queues[SECONDARY_QUEUE].empty());
  
  EntryRef i;
  if (!m_queues[SECONDARY_QUEUE].empty()) {
    i = m_queues[SECONDARY_QUEUE].front();
    //NFD_LOG_TRACE("EVICT_SECONDARY_QUEUE: " << i->getName());
  }
  else if (m_queues[SECONDARY_QUEUE].empty()) {
    updateQueue();
    i = m_queues[SECONDARY_QUEUE].front();
    //NFD_LOG_TRACE("EVICT_SECONDARY_QUEUE(UPDATE): " << i->getName());
  }
  else if (!m_queues[PRIMARY_QUEUE].empty()) {
    i = m_queues[PRIMARY_QUEUE].front();
    //NFD_LOG_TRACE("EVICT_PRIMARY_QUEUE: " << i->getName());
  }
  
  //writeInfo(i, "evicted");

  this->detachQueue(i);
  this->emitSignal(beforeEvict, i);
  totalEviction++;
  m_cs->writeInfo(i->getName(), "evict");
}

void
CbcPolicy::attachQueue(EntryRef i)
{ 
  BOOST_ASSERT(m_entryInfoMap.find(i) == m_entryInfoMap.end());

  EntryInfo* entryInfo = new EntryInfo();
  
  // initialize the value
  entryInfo->prefix = i->getName();
  entryInfo->link_cost = getSourceDistance(i);
  entryInfo->last_access = ns3::Simulator::Now().GetSeconds();
  entryInfo->recency = 0;
  entryInfo->frequency = 1;
  entryInfo->cost_value = 1.0;
  
  if (!i->isFresh()) {
    entryInfo->queueType = SECONDARY_QUEUE;
  }
  else {
    entryInfo->queueType = PRIMARY_QUEUE;
    entryInfo->moveToSecondaryQueue = getScheduler().schedule(i->getData().getFreshnessPeriod(),
                                                          [=] { moveToSecondaryQueue(i); });
  }

  Queue& queue = m_queues[entryInfo->queueType];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

void
CbcPolicy::detachQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  if (entryInfo->queueType == PRIMARY_QUEUE) {
    entryInfo->moveToSecondaryQueue.cancel();
  }

  m_queues[entryInfo->queueType].erase(entryInfo->queueIt);
  m_entryInfoMap.erase(i);
  delete entryInfo;
}

void
CbcPolicy::moveToSecondaryQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());
  
  EntryInfo* entryInfo = m_entryInfoMap[i];
  
  if (entryInfo->queueType == PRIMARY_QUEUE) {
     entryInfo->moveToSecondaryQueue.cancel();   
  }

  m_queues[PRIMARY_QUEUE].erase(entryInfo->queueIt);

  entryInfo->queueType = SECONDARY_QUEUE;
  Queue& queue = m_queues[SECONDARY_QUEUE];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

void
CbcPolicy::moveToPrimaryQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());
  
  EntryInfo* entryInfo = m_entryInfoMap[i];

  m_queues[SECONDARY_QUEUE].erase(entryInfo->queueIt);

  entryInfo->queueType = PRIMARY_QUEUE;
  entryInfo->moveToSecondaryQueue = getScheduler().schedule(i->getData().getFreshnessPeriod(),
                                                          [=] { moveToSecondaryQueue(i); });
  Queue& queue = m_queues[PRIMARY_QUEUE];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

void
CbcPolicy::updateCostValue(EntryRef i) {
  BOOST_ASSERT(!m_queues[PRIMARY_QUEUE].empty() ||
               !m_queues[SECONDARY_QUEUE].empty());
  
  // Extract features
  double freq = m_entryInfoMap[i]->frequency;
  double distance = m_entryInfoMap[i]->link_cost;
  double recency = ns3::Simulator::Now().GetSeconds() - m_entryInfoMap[i]->last_access;
  m_entryInfoMap[i]->last_access = ns3::Simulator::Now().GetSeconds();
  
  // Implement EWMA for recency
  double alpha = 0.8;
  double prevR = m_entryInfoMap[i]->recency;
  double currentR = recency;
  double ewma = alpha * currentR + (1 - alpha) * prevR;
  m_entryInfoMap[i]->recency = ewma;


  // Updated cost function
  double k1 = 0.1, k2 = 0.3, k3 = 0.6; // Adjust these weights as needed
  m_entryInfoMap[i]->cost_value = (k1*distance) + (k2*ewma) + (k3*freq);
  
  if (!m_queues[PRIMARY_QUEUE].empty())
    updateQueue();
  
  dataLogging(i);
}

void
CbcPolicy::updateQueue() {
  BOOST_ASSERT(!m_queues[PRIMARY_QUEUE].empty() ||
               !m_queues[SECONDARY_QUEUE].empty());
  
  EntryRef candidate;
  double temp, cost;
  
  if (!m_queues[PRIMARY_QUEUE].empty()) {
    candidate = m_queues[PRIMARY_QUEUE].front();
    temp = m_entryInfoMap[candidate]->cost_value;
    
      for(auto it = m_queues[PRIMARY_QUEUE].begin(); it != m_queues[PRIMARY_QUEUE].end(); ++it) {
        if (*it == m_queues[PRIMARY_QUEUE].front())
            continue;
        else {
            cost = m_entryInfoMap[*it]->cost_value;
            if (temp > cost) {
                temp = cost;
                candidate = *it;              
            }
        }
    }
    moveToSecondaryQueue(candidate); 
    
  }
}

void
CbcPolicy::dataLogging(EntryRef i) {
  BOOST_ASSERT(!m_queues[PRIMARY_QUEUE].empty() ||
               !m_queues[SECONDARY_QUEUE].empty());
  
  std::ofstream outData("cbc-training-data-timestamp.csv", std::ios::app);
  
  outData << ns3::Simulator::Now().GetSeconds() << ","
  		<< m_entryInfoMap[i]->prefix << ","
        	<< m_entryInfoMap[i]->frequency << ","
        	<< m_entryInfoMap[i]->recency << "," 
    	    	<< m_entryInfoMap[i]->link_cost << "," 
    	    	<< i->getData().getFreshnessPeriod() << "," 
    	    	<< m_entryInfoMap[i]->queueType << "," 
    	    	<< this->getCs()->size() << "," 
    	    	<< m_entryInfoMap[i]->cost_value << "\n";
  outData.close();
}//dataLogging(EntryRef i)

} // namespace cbc
} // namespace cs
} // namespace nfd

