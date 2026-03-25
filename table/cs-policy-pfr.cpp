

#include "cs-policy-pfr.hpp"
#include "cs.hpp"
#include "common/global.hpp"

namespace nfd {
namespace cs {
namespace pfr {

const std::string PfrPolicy::POLICY_NAME = "pfr";
NFD_REGISTER_CS_POLICY(PfrPolicy);

PfrPolicy::PfrPolicy()
  : Policy(POLICY_NAME)
{
}

PfrPolicy::~PfrPolicy()
{
  for (auto entryInfoMapPair : m_entryInfoMap) {
    delete entryInfoMapPair.second;
  }
}

void
PfrPolicy::doAfterInsert(EntryRef i)
{
  this->attachQueue(i);
  this->evictEntries();
}

void
PfrPolicy::doAfterRefresh(EntryRef i)
{
  this->detachQueue(i);
  this->attachQueue(i);
}

void
PfrPolicy::doBeforeErase(EntryRef i)
{
  this->detachQueue(i);
}

void
PfrPolicy::doBeforeUse(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());
  
  EntryInfo* entryInfo = m_entryInfoMap[i];
  
  entryInfo->p = entryInfo->p + 1;
  entryInfo->trec = ns3::Simulator::Now().GetSeconds();
  
  double age = ns3::Simulator::Now().GetSeconds() - entryInfo->tc;   
  double remainingLife = static_cast<double>(i->getData().getFreshnessPeriod().count()) - age;
  entryInfo->r = remainingLife;
}

void
PfrPolicy::evictEntries()
{
  BOOST_ASSERT(this->getCs() != nullptr);

  while (this->getCs()->size() > this->getLimit()) {
    this->evictOne();
  }
}

void
PfrPolicy::evictOne()
{
  BOOST_ASSERT(!m_queues[QUEUE_STALE].empty() ||
               !m_queues[QUEUE_PFR].empty());

  EntryRef i;
  if (!m_queues[QUEUE_STALE].empty()) {
    i = m_queues[QUEUE_STALE].front();
  }
  else if (!m_queues[QUEUE_PFR].empty()) {
    double temp = 0.0;
    double Epmax = 0.0;
    i = m_queues[QUEUE_PFR].front();
    temp = m_entryInfoMap[i]->Ep;
    
      for(auto it = m_queues[QUEUE_PFR].begin(); it != m_queues[QUEUE_PFR].end(); ++it) {
        if (*it == m_queues[QUEUE_PFR].front())
            continue;
        else {
            Epmax = m_entryInfoMap[*it]->Ep;
            if (temp > Epmax) {
                temp = Epmax;
                i = *it;              
            }
        }
    }
  }

  this->detachQueue(i);
  this->emitSignal(beforeEvict, i);
}

void
PfrPolicy::attachQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) == m_entryInfoMap.end());

  EntryInfo* entryInfo = new EntryInfo();
  
  // initialize value
  entryInfo->p = 1; // content popularity 
  entryInfo->r = static_cast<double>(i->getData().getFreshnessPeriod().count()); // content freshness
  entryInfo->tc = ns3::Simulator::Now().GetSeconds(); // content caching time
  entryInfo->trec = ns3::Simulator::Now().GetSeconds(); // content recently access time
  entryInfo->Ep = 1.0; // probability of content eviction
  
  if (!i->isFresh()) {
    entryInfo->queueType = QUEUE_STALE;
  }
  else {
    entryInfo->queueType = QUEUE_PFR;
    entryInfo->moveStaleEventId = getScheduler().schedule(i->getData().getFreshnessPeriod(),
                                                          [=] { moveToStaleQueue(i); });
  }

  Queue& queue = m_queues[entryInfo->queueType];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
  
  
}

void
PfrPolicy::detachQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  if (entryInfo->queueType == QUEUE_PFR) {
    entryInfo->moveStaleEventId.cancel();
  }

  m_queues[entryInfo->queueType].erase(entryInfo->queueIt);
  m_entryInfoMap.erase(i);
  delete entryInfo;
}

void
PfrPolicy::moveToStaleQueue(EntryRef i)
{
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  BOOST_ASSERT(entryInfo->queueType == QUEUE_PFR);

  m_queues[QUEUE_PFR].erase(entryInfo->queueIt);

  entryInfo->queueType = QUEUE_STALE;
  Queue& queue = m_queues[QUEUE_STALE];
  entryInfo->queueIt = queue.insert(queue.end(), i);
  m_entryInfoMap[i] = entryInfo;
}

void PfrPolicy::calculateEvictionProbability(EntryRef i) {
  BOOST_ASSERT(m_entryInfoMap.find(i) != m_entryInfoMap.end());

  EntryInfo* entryInfo = m_entryInfoMap[i];
  
  double x1 = entryInfo->p / getMaxPopularity(); // popularity
  double x2 = entryInfo->r / getMaxRemainingLife();// remainingLife
  double x3 = (ns3::Simulator::Now().GetSeconds() - entryInfo->trec) / getMaxUnusedTime(); // unusedTime
  
  entryInfo->Ep = 1 - ((x1 + x2 + x3)/3); 
}

// Utility functions to get max values for normalization
double PfrPolicy::getMaxPopularity() {   
  EntryRef candidate;
  double temp = 0.0;
  double maxPopularity = 0.0;
  
  if (!m_queues[QUEUE_PFR].empty()) {
    candidate = m_queues[QUEUE_PFR].front();
    temp = m_entryInfoMap[candidate]->p;
    
      for(auto it = m_queues[QUEUE_PFR].begin(); it != m_queues[QUEUE_PFR].end(); ++it) {
        if (*it == m_queues[QUEUE_PFR].front())
            continue;
        else {
            maxPopularity = m_entryInfoMap[*it]->p;
            if (temp > maxPopularity) {
                temp = maxPopularity;
                candidate = *it;              
            }
        }
    }
  }
      return maxPopularity;
}

double PfrPolicy::getMaxRemainingLife() {    
  EntryRef candidate;
  double temp = 0.0;
  double maxRemainingLife = 0.0;
  
  if (!m_queues[QUEUE_PFR].empty()) {
    candidate = m_queues[QUEUE_PFR].front();
    temp = m_entryInfoMap[candidate]->r;
    
      for(auto it = m_queues[QUEUE_PFR].begin(); it != m_queues[QUEUE_PFR].end(); ++it) {
        if (*it == m_queues[QUEUE_PFR].front())
            continue;
        else {
            maxRemainingLife = m_entryInfoMap[*it]->r;
            if (temp > maxRemainingLife) {
                temp = maxRemainingLife;
                candidate = *it;              
            }
        }
    }
  }
      return maxRemainingLife;
}

double PfrPolicy::getMaxUnusedTime() {
  EntryRef candidate;
  double temp = 0.0;
  double maxUnusedTime = 0.0;
  
  if (!m_queues[QUEUE_PFR].empty()) {
    candidate = m_queues[QUEUE_PFR].front();
    temp = ns3::Simulator::Now().GetSeconds() - m_entryInfoMap[candidate]->trec;
    
      for(auto it = m_queues[QUEUE_PFR].begin(); it != m_queues[QUEUE_PFR].end(); ++it) {
        if (*it == m_queues[QUEUE_PFR].front())
            continue;
        else {
            maxUnusedTime = ns3::Simulator::Now().GetSeconds() - m_entryInfoMap[*it]->trec;
            if (temp > maxUnusedTime) {
                temp = maxUnusedTime;
                candidate = *it;              
            }
        }
    }
  }
      return maxUnusedTime;
}

} // namespace pfr
} // namespace cs
} // namespace nfd
