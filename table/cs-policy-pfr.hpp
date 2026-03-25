/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_TABLE_CS_POLICY_PFR_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_PFR_HPP

#include "cs-policy.hpp"

#include <list>

namespace nfd {
namespace cs {
namespace pfr {

using Queue = std::list<Policy::EntryRef>;

enum QueueType {
  QUEUE_STALE,
  QUEUE_PFR,
  QUEUE_MAX
};

struct EntryInfo
{
  QueueType queueType;
  Queue::iterator queueIt;
  scheduler::EventId moveStaleEventId;
  double p; // content popularity 
  double r; // content remaining life
  double tc; // content caching time
  double trec; // content recently access time
  double Ep; // probability of content eviction
};

class PfrPolicy final : public Policy
{
public:
  PfrPolicy();

  ~PfrPolicy() final;

public:
  static const std::string POLICY_NAME;

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

  /** \brief moves an entry from FIFO queue to STALE queue
   */
  void
  moveToStaleQueue(EntryRef i);

  // Functions to Calculate Maximum Values for Normalization
  double getMaxPopularity();
  double getMaxRemainingLife();
  double getMaxUnusedTime();
  
  void calculateEvictionProbability(EntryRef i);

private:
  Queue m_queues[QUEUE_MAX];
  std::map<EntryRef, EntryInfo*> m_entryInfoMap;
};

} // namespace pfr

using pfr::PfrPolicy;

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_POLICY_PFR_HPP
