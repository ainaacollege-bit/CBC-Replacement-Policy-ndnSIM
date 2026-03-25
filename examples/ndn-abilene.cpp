// ndn-abilene.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/node.h"
#include "ns3/object-vector.h"
#include "ns3/pointer.h"
#include "ns3/object.h"
#include "ns3/node-list.h"
#include "daemon/mgmt/fib-manager.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-fileconsumer-log-tracer.hpp"

namespace ns3 {

/**
 * This scenario simulates an abilene network topology:
 *
 */

int
main(int argc, char* argv[])
{
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes by reading topology from topo-abilene.txt
  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-abilene.txt");
  topologyReader.Read();

  const NodeContainer allNodes {topologyReader.GetNodes()};

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;

  ndnHelper.setPolicy("nfd::cs::cbc");
  ndnHelper.setCsSize(500);

  ndnHelper.SetDefaultRoutes(false);
  ndnHelper.InstallAll();

  topologyReader.ApplyOspfMetric();

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second
  consumerHelper.SetAttribute("NumberOfContents", StringValue("100")); // 1100 different contents

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(15.0)));

  for (const auto& n : allNodes) {

    std::string nodeName = Names::FindName(n);
    Ptr<Node> nNode = Names::Find<Node>(nodeName);

    // Set prefix
    std::string prefix = "/" + nodeName + "/file";

    // Add prefix origin
    ndnGlobalRoutingHelper.AddOrigins(prefix, nNode);

    // Forwarding strategy
    ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/best-route");

    // Producer
    producerHelper.SetPrefix(prefix);
    producerHelper.Install(nNode);

    // Consumer
    consumerHelper.SetPrefix(prefix);
    consumerHelper.Install(allNodes);

    std::cout << "Node " << nNode << "\tName: " << nodeName << "\tPrefix:" << prefix << "\n";
  }

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  
  // Tracer
  ndn::AppDelayTracer::InstallAll("Abilene_resultrun/pfr2/pfr2500-consumer-trace.txt");
  ndn::CsTracer::InstallAll("Abilene_resultrun/pfr2/pfr2500-cache-trace.txt", Seconds(5.0));
  ndn::L3RateTracer::InstallAll("Abilene_resultrun/pfr2/pfr2500-producer-trace.txt", Seconds(5.0));

  Simulator::Stop(Seconds(30.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}











